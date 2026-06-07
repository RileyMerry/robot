#include "WebServer.hpp"

#include <arpa/inet.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>

WebServer::WebServer(Robot& robot, int port)
    : robot(robot), port(port) {}

std::string WebServer::buildHttpResponse(const std::string& body, const std::string& contentType) {
    std::ostringstream response;

    response << "HTTP/1.1 200 OK\r\n";
    response << "Content-Type: " << contentType << "\r\n";
    response << "Content-Length: " << body.size() << "\r\n";
    response << "Connection: close\r\n";
    response << "\r\n";
    response << body;

    return response.str();
}

std::string WebServer::loadIndexPage() {
std::ifstream file("/home/rmerry/robot_ws/projects/ugv_v2/web/index.html");
    if (!file) {
        return "<html><body><h1>Robot Controller</h1><p>index.html not found</p></body></html>";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string WebServer::getRequestPath(const std::string& request) {
    std::istringstream stream(request);
    std::string method;
    std::string path;
    std::string version;

    stream >> method >> path >> version;

    return path;
}

void WebServer::handleCommand(const std::string& path) {
    if (path == "/forward") {
        robot.forward();
    } else if (path == "/backward") {
        robot.backward();
    } else if (path == "/left") {
        robot.left();
    } else if (path == "/right") {
        robot.right();
    } else if (path == "/stop") {
        robot.stop();
    }
}

bool WebServer::start() {
    int serverFd = socket(AF_INET, SOCK_STREAM, 0);

    if (serverFd < 0) {
        std::cerr << "Failed to create socket\n";
        return false;
    }

    int opt = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(serverFd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        std::cerr << "Failed to bind to port " << port << "\n";
        close(serverFd);
        return false;
    }

    if (listen(serverFd, 10) < 0) {
        std::cerr << "Failed to listen\n";
        close(serverFd);
        return false;
    }

    std::cout << "Robot web controller running on port " << port << "\n";
    std::cout << "Open this on your phone: http://Riley.local:" << port << "\n";

    while (true) {
        sockaddr_in clientAddress{};
        socklen_t clientLength = sizeof(clientAddress);

        int clientFd = accept(serverFd, reinterpret_cast<sockaddr*>(&clientAddress), &clientLength);

        if (clientFd < 0) {
            std::cerr << "Failed to accept client\n";
            continue;
        }

        char buffer[4096] = {0};
        read(clientFd, buffer, sizeof(buffer) - 1);

        std::string request(buffer);
        std::string path = getRequestPath(request);

        std::string body;
        std::string contentType;

        if (path == "/" || path == "/index.html") {
            body = loadIndexPage();
            contentType = "text/html";
        } else {
            handleCommand(path);
            body = "OK";
            contentType = "text/plain";
        }

        std::string response = buildHttpResponse(body, contentType);
        send(clientFd, response.c_str(), response.size(), 0);

        close(clientFd);
    }

    close(serverFd);
    return true;
}

