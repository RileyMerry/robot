#include <gpiod.h>
#include <iostream>
#include <unistd.h>

int main() {
    const char *chip_path = "/dev/gpiochip0";

    // GPIO pins
    unsigned int ain1 = 17;  // GPIO17 → AIN1
    unsigned int ain2 = 27;  // GPIO27 → AIN2
    unsigned int stby = 22;  // GPIO22 → STBY

    // Open GPIO chip
    gpiod_chip *chip = gpiod_chip_open(chip_path);
    if (!chip) {
        std::cerr << "Failed to open chip\n";
        return 1;
    }

    // Create line settings
    gpiod_line_settings *settings = gpiod_line_settings_new();
    if (!settings) {
        std::cerr << "Failed to create settings\n";
        gpiod_chip_close(chip);
        return 1;
    }

    // Set as outputs, default LOW
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);
    gpiod_line_settings_set_output_value(settings, GPIOD_LINE_VALUE_INACTIVE);

    // Create config
    gpiod_line_config *config = gpiod_line_config_new();
    if (!config) {
        std::cerr << "Failed to create config\n";
        gpiod_line_settings_free(settings);
        gpiod_chip_close(chip);
        return 1;
    }

    unsigned int lines[] = {ain1, ain2, stby};

    if (gpiod_line_config_add_line_settings(config, lines, 3, settings) < 0) {
        std::cerr << "Failed to add line settings\n";
        gpiod_line_config_free(config);
        gpiod_line_settings_free(settings);
        gpiod_chip_close(chip);
        return 1;
    }

    // Request control
    gpiod_request_config *req = gpiod_request_config_new();
    gpiod_request_config_set_consumer(req, "tb6612-test");

    gpiod_line_request *request = gpiod_chip_request_lines(chip, req, config);
    if (!request) {
        std::cerr << "Failed to request lines\n";
        gpiod_request_config_free(req);
        gpiod_line_config_free(config);
        gpiod_line_settings_free(settings);
        gpiod_chip_close(chip);
        return 1;
    }

    // 🔥 IMPORTANT: wake up the driver
    gpiod_line_request_set_value(request, stby, GPIOD_LINE_VALUE_ACTIVE);

    // FORWARD
    std::cout << "Forward\n";
    gpiod_line_request_set_value(request, ain1, GPIOD_LINE_VALUE_ACTIVE);
    gpiod_line_request_set_value(request, ain2, GPIOD_LINE_VALUE_INACTIVE);
    sleep(3);

    // REVERSE
    std::cout << "Reverse\n";
    gpiod_line_request_set_value(request, ain1, GPIOD_LINE_VALUE_INACTIVE);
    gpiod_line_request_set_value(request, ain2, GPIOD_LINE_VALUE_ACTIVE);
    sleep(3);

    // STOP
    std::cout << "Stop\n";
    gpiod_line_request_set_value(request, ain1, GPIOD_LINE_VALUE_INACTIVE);
    gpiod_line_request_set_value(request, ain2, GPIOD_LINE_VALUE_INACTIVE);
    sleep(3);

    // Cleanup
    gpiod_line_request_release(request);
    gpiod_request_config_free(req);
    gpiod_line_config_free(config);
    gpiod_line_settings_free(settings);
    gpiod_chip_close(chip);

    return 0;
}
