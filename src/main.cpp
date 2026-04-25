#include "Robot.hpp"
#include <iostream>
#include <termios.h>
#include <unistd.h>

void enableRawMode() {
    termios t;
    tcgetattr(STDIN_FILENO, &t);
    t.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

void disableRawMode() {
    termios t;
    tcgetattr(STDIN_FILENO, &t);
    t.c_lflag |= (ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

int main() {
    Robot robot;

    if (!robot.initialize()) {
        std::cerr << "Init failed\n";
        return 1;
    }

    enableRawMode();

    std::cout << "Controls: w s a d space q\n";

    bool running = true;

    while (running) {
        char c = getchar();

        if (c == 'w') robot.forward();
        else if (c == 's') robot.backward();
        else if (c == 'a') robot.left();
        else if (c == 'd') robot.right();
        else if (c == ' ') robot.stop();
        else if (c == 'q') running = false;
    }

    robot.stop();
    disableRawMode();
}
