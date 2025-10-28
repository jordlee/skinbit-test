// Standalone GPIO Trigger Test - No SDK Dependencies
// This script tests pure GPIO trigger performance without camera SDK overhead

#include <iostream>
#include <chrono>
#include <thread>
#include <fstream>
#include <iomanip>
#include <sstream>

#if defined(__linux__)
#include <gpiod.h>
#endif

// GPIO Configuration
constexpr int GPIO_TRIGGER_PIN = 12;  // Physical pin 32, GPIO 12 on gpiochip4
constexpr const char* GPIO_CHIP = "gpiochip4";

class GPIOTrigger {
private:
    void* m_gpio_chip = nullptr;
    void* m_gpio_line = nullptr;
    bool m_gpio_initialized = false;

public:
    GPIOTrigger() = default;
    ~GPIOTrigger() {
        cleanup();
    }

    bool init() {
#if defined(__linux__)
        if (m_gpio_initialized) {
            std::cout << "GPIO already initialized.\n";
            return true;
        }

        std::cout << "Initializing GPIO " << GPIO_CHIP << " pin " << GPIO_TRIGGER_PIN
                  << " (physical pin 32) using libgpiod...\n";

        // Open GPIO chip
        struct gpiod_chip* chip = gpiod_chip_open_by_name(GPIO_CHIP);
        if (!chip) {
            std::cerr << "ERROR: Failed to open GPIO chip " << GPIO_CHIP << "\n";
            std::cerr << "Make sure libgpiod is installed: sudo apt-get install libgpiod-dev\n";
            return false;
        }
        m_gpio_chip = chip;

        // Get GPIO line
        struct gpiod_line* line = gpiod_chip_get_line(chip, GPIO_TRIGGER_PIN);
        if (!line) {
            std::cerr << "ERROR: Failed to get GPIO line " << GPIO_TRIGGER_PIN << "\n";
            gpiod_chip_close(chip);
            m_gpio_chip = nullptr;
            return false;
        }
        m_gpio_line = line;

        // Request line as output, initially LOW
        int ret = gpiod_line_request_output(line, "GPIO_Test", 0);
        if (ret < 0) {
            std::cerr << "ERROR: Failed to request GPIO line as output\n";
            gpiod_chip_close(chip);
            m_gpio_chip = nullptr;
            m_gpio_line = nullptr;
            return false;
        }

        m_gpio_initialized = true;
        std::cout << "GPIO pin " << GPIO_TRIGGER_PIN << " initialized successfully.\n";
        return true;
#else
        std::cerr << "ERROR: GPIO control only supported on Linux.\n";
        return false;
#endif
    }

    void cleanup() {
#if defined(__linux__)
        if (!m_gpio_initialized) {
            return;
        }

        std::cout << "Cleaning up GPIO pin " << GPIO_TRIGGER_PIN << "...\n";

        // Set pin to LOW before cleanup
        if (m_gpio_line) {
            struct gpiod_line* line = static_cast<struct gpiod_line*>(m_gpio_line);
            gpiod_line_set_value(line, 0);
            gpiod_line_release(line);
            m_gpio_line = nullptr;
        }

        if (m_gpio_chip) {
            struct gpiod_chip* chip = static_cast<struct gpiod_chip*>(m_gpio_chip);
            gpiod_chip_close(chip);
            m_gpio_chip = nullptr;
        }

        m_gpio_initialized = false;
        std::cout << "GPIO cleanup complete.\n";
#endif
    }

    void press() {
#if defined(__linux__)
        if (m_gpio_line) {
            struct gpiod_line* line = static_cast<struct gpiod_line*>(m_gpio_line);
            gpiod_line_set_value(line, 1);
        }
#endif
    }

    void release() {
#if defined(__linux__)
        if (m_gpio_line) {
            struct gpiod_line* line = static_cast<struct gpiod_line*>(m_gpio_line);
            gpiod_line_set_value(line, 0);
        }
#endif
    }
};

int main() {
    using namespace std::chrono;

    std::cout << "\n";
    std::cout << std::string(80, '=') << "\n";
    std::cout << "Standalone GPIO Trigger Test (No SDK)\n";
    std::cout << std::string(80, '=') << "\n\n";

    GPIOTrigger gpio;

    if (!gpio.init()) {
        std::cerr << "ERROR: Failed to initialize GPIO. Exiting.\n";
        return 1;
    }

    // Create timestamped log file
    auto now = system_clock::now();
    auto time_t_now = system_clock::to_time_t(now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", localtime(&time_t_now));

    std::string log_filename = std::string("gpio_standalone_test_") + timestamp + ".log";
    std::ofstream log_file(log_filename);

    // Lambda to write to both console and file
    auto log = [&](const std::string& msg) {
        std::cout << msg;
        if (log_file.is_open()) {
            log_file << msg;
            log_file.flush();
        }
    };

    log("\n");
    log(std::string(80, '=') + "\n");
    log("Standalone GPIO Trigger Test (No SDK Dependencies)\n");
    log("GPIO Chip: " + std::string(GPIO_CHIP) + "\n");
    log("GPIO Pin: " + std::to_string(GPIO_TRIGGER_PIN) + " (Physical Pin 32)\n");

    auto start_time_t = system_clock::to_time_t(now);
    char start_str[32];
    strftime(start_str, sizeof(start_str), "%Y-%m-%d %H:%M:%S", localtime(&start_time_t));
    log(std::string("Started: ") + start_str + "\n");
    log(std::string(80, '=') + "\n\n");

    log("Configuration:\n");
    log("  - Press duration: 20ms\n");
    log("  - Cycle delay: 300ms (for camera processing + SD card save)\n");
    log("  - Total triggers: 30\n\n");

    std::this_thread::sleep_for(milliseconds(500));

    const int TOTAL_TRIGGERS = 30;
    const int PRESS_DURATION_MS = 50;
    const int CYCLE_DELAY_MS = 200;

    auto test_start = high_resolution_clock::now();

    log("Starting GPIO trigger sequence...\n\n");

    for (int i = 1; i <= TOTAL_TRIGGERS; i++) {
        std::ostringstream progress;
        progress << "Trigger " << i << "/" << TOTAL_TRIGGERS << "\r";
        std::cout << progress.str() << std::flush;

        // GPIO trigger press
        gpio.press();
        std::this_thread::sleep_for(milliseconds(PRESS_DURATION_MS));
        gpio.release();

        // Wait for camera to complete capture and save to SD card
        std::this_thread::sleep_for(milliseconds(CYCLE_DELAY_MS));
    }

    std::cout << "\n";

    auto test_end = high_resolution_clock::now();
    auto total_time = duration_cast<milliseconds>(test_end - test_start).count();
    double total_seconds = total_time / 1000.0;
    double avg_fps = TOTAL_TRIGGERS / total_seconds;

    std::ostringstream summary;
    summary << "\n=== Test Complete ===\n";
    summary << "Total triggers: " << TOTAL_TRIGGERS << "\n";
    summary << "Total time: " << std::fixed << std::setprecision(2) << total_seconds << " seconds\n";
    summary << "Average speed: " << std::fixed << std::setprecision(2) << avg_fps << " fps\n";
    summary << "Average cycle time: " << std::fixed << std::setprecision(0)
            << (total_time / (double)TOTAL_TRIGGERS) << " ms\n";
    summary << "\nLog saved to: " << log_filename << "\n";
    summary << std::string(80, '=') << "\n\n";
    log(summary.str());

    log_file.close();
    gpio.cleanup();

    return 0;
}
