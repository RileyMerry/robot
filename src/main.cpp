#include "Robot.hpp"
#include "WebServer.hpp"

#include <iostream>

int main() {
    Robot robot;

    if (!robot.initialize()) {
        std::cerr << "Robot failed to initialize\n";
        return 1;
    }

    WebServer server(robot, 8080);

    if (!server.start()) {
        std::cerr << "Web server failed to start\n";
        return 1;
    }

    return 0;
}
