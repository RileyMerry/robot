#include "Robot.hpp"

bool Robot::initialize() {
    return driver.initialize();
}

void Robot::forward() {
    driver.forward();
}

void Robot::backward() {
    driver.backward();
}

void Robot::left() {
    driver.left();
}

void Robot::right() {
    driver.right();
}

void Robot::stop() {
    driver.stop();
}
