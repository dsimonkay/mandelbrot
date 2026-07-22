#include "common.h"
#include "CLI11.hpp"

#include <algorithm>
#include <iomanip>

std::vector<std::array<std::uint8_t, 3U>> generate_palette(const Config &config)
{
    std::vector<std::array<std::uint8_t, 3U>> palette(config.palette_size);

    const float r_start = config.start_color[0];
    const float g_start = config.start_color[1];
    const float b_start = config.start_color[2];

    const float r_diff = config.end_color[0] - r_start;
    const float g_diff = config.end_color[1] - g_start;
    const float b_diff = config.end_color[2] - b_start;

    for (std::size_t i{0U}; i < config.palette_size; ++i)
    {
        const float ratio = static_cast<float>(i) / static_cast<float>(config.palette_size - 1);
        const float palindrom_ratio = 1 - std::abs(1 - (2 * ratio)); // creates a palindromic effect for the palette
        palette[i] = {
            static_cast<std::uint8_t>(std::clamp(r_start + (r_diff * palindrom_ratio), 0.0f, 255.0f)),
            static_cast<std::uint8_t>(std::clamp(g_start + (g_diff * palindrom_ratio), 0.0f, 255.0f)),
            static_cast<std::uint8_t>(std::clamp(b_start + (b_diff * palindrom_ratio), 0.0f, 255.0f)),
        };
    }

    return palette;
}

Config get_config(int argc, char **argv)
{
    Config config{};

    CLI::App app{"Simple Mandelbrot fractal generator"};
    app.add_option("--real_center", config.real_center, "Viewport center on the real axis");
    app.add_option("--imaginary_center", config.imaginary_center, "Viewport center on the imaginary axis");
    app.add_option("--viewport_width", config.viewport_width, "Viewport width on the real axis");
    app.add_option("--viewport_height", config.viewport_height, "Viewport height on the real axis");
    app.add_option("--image_width", config.image_width, "Image width (height will be calculated automatically)");
    app.add_option("--iteration_count", config.iteration_count, "Iteration_count");
    app.add_option("--palette_size", config.palette_size, "Number of colors in the palette");
    app.add_option("--output_path", config.output_path, "Path for output images");
    app.add_option("--output_file", config.output_file, "Output image file name (without extension)");

    try
    {
        app.parse(argc, argv);
    }
    catch (const CLI::ParseError &e)
    {
        std::exit(app.exit(e));
    }

    return config;
}

void set_viewport_details(Config &config)
{
    config.viewport_left = config.real_center - (config.viewport_width / 2.0);
    config.viewport_top = config.imaginary_center + (config.viewport_height / 2.0);

    config.step = config.viewport_width / config.image_width;
    config.image_height = static_cast<decltype(config.image_height)>(config.viewport_height / config.step);
}

void TimingReport::print(const double wall_seconds) const
{
    double measured_sum = 0.0;
    std::cout << "\n---- timing report ---------------------------------------------\n";
    for (const auto &[label, seconds] : entries_)
    {
        print_line(label, seconds, wall_seconds);
        measured_sum += seconds;
    }
    print_line("(everything else: CLI parse, file open, ...)",
               wall_seconds - measured_sum, wall_seconds);
    std::cout << "----------------------------------------------------------------\n";
    print_line("TOTAL wall time", wall_seconds, wall_seconds);
}

void TimingReport::print_line(const std::string &label, const double seconds, const double wall_seconds)
{
    std::cout << std::left << std::setw(46) << label
              << std::right << std::fixed
              << std::setw(11) << std::setprecision(6) << seconds << " s"
              << std::setw(7) << std::setprecision(1)
              << (100.0 * seconds / wall_seconds) << " %\n";
}
