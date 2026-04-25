
#ifndef MOTOR_DRIVER_HPP
#define MOTOR_DRIVER_HPP

#include <gpiod.h>

class MotorDriver {
public:
    bool initialize();
   ~MotorDriver();
    void forward();
    void backward();
    void left();
    void right();
    void stop();

private:
    const char* chipPath = "/dev/gpiochip0";

    unsigned int ain1 = 17;
    unsigned int ain2 = 27;
    unsigned int bin1 = 23;
    unsigned int bin2 = 24;
    unsigned int stby = 22;

    gpiod_chip* chip = nullptr;
    gpiod_line_request* request = nullptr;

    void setMotorA(bool in1, bool in2);
    void setMotorB(bool in1, bool in2);
};

#endif
