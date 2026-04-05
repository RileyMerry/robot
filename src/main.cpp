#include <gpiod.h>
#include <iostream>
#include <unistd.h>

int main() {
    const char *chip_path = "/dev/gpiochip0";
    unsigned int in1 = 17; // physical pin 11
    unsigned int in2 = 27; // physical pin 13

    gpiod_chip *chip = gpiod_chip_open(chip_path);
    if (!chip) {
        std::cerr << "Failed to open chip\n";
        return 1;
    }

    gpiod_line_settings *settings = gpiod_line_settings_new();
    if (!settings) {
        std::cerr << "Failed to create line settings\n";
        gpiod_chip_close(chip);
        return 1;
    }

    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);
    gpiod_line_settings_set_output_value(settings, GPIOD_LINE_VALUE_INACTIVE);

    gpiod_line_config *config = gpiod_line_config_new();
    if (!config) {
        std::cerr << "Failed to create line config\n";
        gpiod_line_settings_free(settings);
        gpiod_chip_close(chip);
        return 1;
    }

    unsigned int lines[] = {in1, in2};
    if (gpiod_line_config_add_line_settings(config, lines, 2, settings) < 0) {
        std::cerr << "Failed to add line settings\n";
        gpiod_line_config_free(config);
        gpiod_line_settings_free(settings);
        gpiod_chip_close(chip);
        return 1;
    }

    gpiod_request_config *req = gpiod_request_config_new();
    if (!req) {
        std::cerr << "Failed to create request config\n";
        gpiod_line_config_free(config);
        gpiod_line_settings_free(settings);
        gpiod_chip_close(chip);
        return 1;
    }

    gpiod_request_config_set_consumer(req, "motor-test");

    gpiod_line_request *request = gpiod_chip_request_lines(chip, req, config);
    if (!request) {
        std::cerr << "Failed to request lines\n";
        gpiod_request_config_free(req);
        gpiod_line_config_free(config);
        gpiod_line_settings_free(settings);
        gpiod_chip_close(chip);
        return 1;
    }

    std::cout << "Forward\n";
    gpiod_line_request_set_value(request, in1, GPIOD_LINE_VALUE_ACTIVE);
    gpiod_line_request_set_value(request, in2, GPIOD_LINE_VALUE_INACTIVE);
    sleep(3);

    std::cout << "Reverse\n";
    gpiod_line_request_set_value(request, in1, GPIOD_LINE_VALUE_INACTIVE);
    gpiod_line_request_set_value(request, in2, GPIOD_LINE_VALUE_ACTIVE);
    sleep(3);

    std::cout << "Stop\n";
    gpiod_line_request_set_value(request, in1, GPIOD_LINE_VALUE_INACTIVE);
    gpiod_line_request_set_value(request, in2, GPIOD_LINE_VALUE_INACTIVE);
    sleep(3);

    gpiod_line_request_release(request);
    gpiod_request_config_free(req);
    gpiod_line_config_free(config);
    gpiod_line_settings_free(settings);
    gpiod_chip_close(chip);

    return 0;
}
