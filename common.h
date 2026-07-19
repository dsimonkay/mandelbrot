#ifndef MANDELBROT_COMMON_H
#define MANDELBROT_COMMON_H

#include <array>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

/// @brief Configuration object for rendering a single Mandelbrot frame
struct Config
{
    double real_lower{-2.5};       ///< Range start on the real axis
    double real_upper{1.0};        ///< Range end on the real axis
    double imaginary_lower{-1.75}; ///< Range start on the imaginary axis
    double imaginary_upper{1.75};  ///< Range end on the imaginary axis
    std::uint16_t image_width{1600};
    std::uint16_t image_height{1600};
    std::uint16_t iteration_count{1000};
    std::uint16_t palette_size{256};
    std::array<std::uint8_t, 3> start_color{0x1B, 0x2A, 0x6B}; ///< Color of a pixel escaping at the 1st iteration (R, G, B)
    std::array<std::uint8_t, 3> end_color{0xF5, 0xA6, 0x23};   ///< Color of a pixel escaping at the <iteration_count>th iteration (R, G, B)
    std::string output_path{"."};
    std::string output_file{"mandelbrot"};
};

/// @brief Assemble the runtime configuration for the application
/// @param argc number of arguments
/// @param argv array of null-terminated argument strings
/// @return the collected configuration struct
Config get_config(int argc, char **argv);

/// @brief Generate a palette for the fractal generator.
/// @note This is an overflowing palette.
/// @param config The current configuration of the program
/// @return the pregenerated palette
std::vector<std::array<std::uint8_t, 3U>> generate_palette(const Config &config);

/// @brief Named stopwatch collection: measures labelled code blocks and
///        prints a summary report (seconds + percentage of total wall time).
class TimingReport
{
public:
    using clock_type = std::chrono::steady_clock;

    /// @brief Run the given callable and record its runtime under @p label.
    template <typename F>
    void measure(const std::string &label, F &&callable)
    {
        const auto t0 = clock_type::now();
        std::forward<F>(callable)();
        const auto t1 = clock_type::now();
        entries_.emplace_back(label, std::chrono::duration<double>(t1 - t0).count());
    }

    /// @brief Print all measured blocks, the unmeasured remainder and the total.
    void print(const double wall_seconds) const;

private:
    static void print_line(const std::string &label, const double seconds, const double wall_seconds);

    std::vector<std::pair<std::string, double>> entries_;
};

#endif // MANDELBROT_COMMON_H
