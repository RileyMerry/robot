#include "MotorDriver.hpp"
#include <iostream>

bool MotorDriver::initialize() {
    chip = gpiod_chip_open(chipPath);
    if (!chip) {
        std::cerr << "Failed to open GPIO chip\n";
        return false;
    }

    gpiod_line_settings* settings = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);
    gpiod_line_settings_set_output_value(settings, GPIOD_LINE_VALUE_INACTIVE);

    gpiod_line_config* config = gpiod_line_config_new();

    unsigned int lines[] = {ain1, ain2, bin1, bin2, stby};
    gpiod_line_config_add_line_settings(config, lines, 5, settings);

    gpiod_request_config* req = gpiod_request_config_new();
    gpiod_request_config_set_consumer(req, "robot-car");

    request = gpiod_chip_request_lines(chip, req, config);

    if (!request) {
        std::cerr << "Failed to request GPIO lines\n";
        return false;
    }

    // Wake up motor driver
    gpiod_line_request_set_value(request, stby, GPIOD_LINE_VALUE_ACTIVE);

    stop();
    return true;
}

void MotorDriver::setMotorA(bool in1, bool in2) {
    gpiod_line_request_set_value(
        request,
        ain1,
        in1 ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE
    );

    gpiod_line_request_set_value(
        request,
        ain2,
        in2 ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE
    );
}

void MotorDriver::setMotorB(bool in1, bool in2) {
    gpiod_line_request_set_value(
        request,
        bin1,
        in1 ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE
    );

    gpiod_line_request_set_value(
        request,
        bin2,
        in2 ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE
    );
}

void MotorDriver::forward() {
    std::cout << "Forward\n";
    setMotorA(true, false);
    setMotorB(true, false);
}

void MotorDriver::backward() {
    std::cout << "Backward\n";
    setMotorA(false, true);
    setMotorB(false, true);
}

void MotorDriver::left() {
    std::cout << "Left\n";
    setMotorA(false, false);
    setMotorB(true, false);
}

void MotorDriver::right() {
    std::cout << "Right\n";
    setMotorA(true, false);
    setMotorB(false, false);
}

void MotorDriver::stop() {
    std::cout << "Stop\n";
    setMotorA(false, false);
    setMotorB(false, false);
}
MotorDriver::~MotorDriver() {
    if (request) {
        stop();
        gpiod_line_request_release(request);
        request = nullptr;
    }

    if (chip) {
        gpiod_chip_close(chip);
        chip = nullptr;
    }
}
