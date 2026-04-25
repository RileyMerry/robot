#ifndef WEB_SERVER_HPP
#define WEB_SERVER_HPP

#include "Robot.hpp"
#include <string>

class WebServer {
public:
    WebServer(Robot& robot, int port = 8080);

    bool start();

private:
    Robot& robot;
    int port;

    std::string buildHttpResponse(const std::string& body, const std::string& contentType);
    std::string loadIndexPage();
    std::string getRequestPath(const std::string& request);
    void handleCommand(const std::string& path);
};

#endif
