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
    double real_center{-0.75};    ///< Viewport center on the real axis
    double imaginary_center{0.0}; ///< Viewport center on the imaginary axis
    double viewport_width{3.5};   ///< Viewport width on the real axis
    double viewport_height{3.5};  ///< Viewport height on the imaginary axis
    double viewport_left{};       ///< Real coordinate of the left edge of viewport window (will be calculated)
    double viewport_top{};        ///< Imaginary coordinate of the top edge of viewport window (will be calculated)
    double step{};                ///< Uniform step size on each axis

    std::uint16_t image_width{1600};
    std::uint16_t image_height{}; // Will be calculated; cannot be set directly

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

/// @brief Set the details of the viewport to be rendered in the provided configuration:
///          - viewport left
///          - viewport top
///          - step
///          - image height
/// @param config The currently active configuration
void set_viewport_details(Config &config);

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
