cmake_minimum_required(VERSION 3.16)

project(robot)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_executable(robot
    src/main.cpp
    src/MotorDriver.cpp
    src/Robot.cpp
    src/WebServer.cpp
)

target_link_libraries(robot gpiod)
