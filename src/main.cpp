#include <gpiod.h>
#include <iostream>
#include <unistd.h>
#include <termios.h>

// Enable raw mode (instant key press)
void enableRawMode() {
    struct termios t;
    tcgetattr(STDIN_FILENO, &t);
    t.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

// Restore terminal
void disableRawMode() {
    struct termios t;
    tcgetattr(STDIN_FILENO, &t);
    t.c_lflag |= (ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

int main() {
    const char *chip_path = "/dev/gpiochip0";

    unsigned int ain1 = 17;
    unsigned int ain2 = 27;
    unsigned int stby = 22;

    gpiod_chip *chip = gpiod_chip_open(chip_path);

    gpiod_line_settings *settings = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);
    gpiod_line_settings_set_output_value(settings, GPIOD_LINE_VALUE_INACTIVE);

    gpiod_line_config *config = gpiod_line_config_new();
    unsigned int lines[] = {ain1, ain2, stby};
    gpiod_line_config_add_line_settings(config, lines, 3, settings);

    gpiod_request_config *req = gpiod_request_config_new();
    gpiod_request_config_set_consumer(req, "keyboard-control");

    gpiod_line_request *request = gpiod_chip_request_lines(chip, req, config);

    // Wake up driver
    gpiod_line_request_set_value(request, stby, GPIOD_LINE_VALUE_ACTIVE);

    enableRawMode();

    std::cout << "Controls: f=forward, b=backward, s=stop, q=quit\n";

    while (true) {
        char c = getchar();

        if (c == 'f') {
            std::cout << "Forward\n";
            gpiod_line_request_set_value(request, ain1, GPIOD_LINE_VALUE_ACTIVE);
            gpiod_line_request_set_value(request, ain2, GPIOD_LINE_VALUE_INACTIVE);
        }
        else if (c == 'b') {
            std::cout << "Reverse\n";
            gpiod_line_request_set_value(request, ain1, GPIOD_LINE_VALUE_INACTIVE);
            gpiod_line_request_set_value(request, ain2, GPIOD_LINE_VALUE_ACTIVE);
        }
        else if (c == 's') {
            std::cout << "Stop\n";
            gpiod_line_request_set_value(request, ain1, GPIOD_LINE_VALUE_INACTIVE);
            gpiod_line_request_set_value(request, ain2, GPIOD_LINE_VALUE_INACTIVE);
        }
        else if (c == 'q') {
            break;
        }
    }

    disableRawMode();

    gpiod_line_request_release(request);
    gpiod_request_config_free(req);
    gpiod_line_config_free(config);
    gpiod_line_settings_free(settings);
    gpiod_chip_close(chip);

    return 0;
}
