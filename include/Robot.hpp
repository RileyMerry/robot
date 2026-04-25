#ifndef ROBOT_HPP
#define ROBOT_HPP

#include "MotorDriver.hpp"

class Robot {
public:
    bool initialize();

    void forward();
    void backward();
    void left();
    void right();
    void stop();

private:
    MotorDriver driver;
};

#endif
