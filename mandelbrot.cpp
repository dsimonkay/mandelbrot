#include "CLI11.hpp"

#include <chrono>
#include <cstdint>
#include <format>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <string>
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
};

int main(int argc, char **argv)
{
    Config config{};

    CLI::App app{"Simple Mandelbrot fractal generator"};
    app.add_option("--real_lower", config.real_lower, "Range start on the real axis");
    app.add_option("--real_upper", config.real_upper, "Range end on the real axis");
    app.add_option("--imaginary_lower", config.imaginary_lower, "Range start on the imaginary axis");
    app.add_option("--imaginary_upper", config.imaginary_upper, "Range end on the imaginary axis");
    app.add_option("--image_width", config.image_width, "Image width");
    app.add_option("--image_height", config.image_height, "Image height");
    app.add_option("--iteration_count", config.iteration_count, "Iteration_count");

    CLI11_PARSE(app, argc, argv);

    const auto file_name{"./mandelbrot.ppm"};
    auto image_file = std::ofstream{file_name, std::ios::binary};
    if (!image_file.is_open())
    {
        std::cerr << " Error " << file_name << " could not be opened; exiting.\n";
        return 1;
    }

    // Create binary image file
    image_file << "P6\n"
               << config.image_width << "\n"
               << config.image_height << "\n"
               << static_cast<std::uint16_t>(255) << "\n";

    const double step_r = (config.real_upper - config.real_lower) / config.image_width;
    const double step_i = (config.imaginary_upper - config.imaginary_lower) / config.image_height;

    const std::size_t row_buffer_length = 3 * config.image_width;
    auto row_buffer = std::vector<std::uint8_t>(row_buffer_length);

    const auto start = std::chrono::steady_clock::now();

    for (int row{0}; row < config.image_height; ++row)
    {
        const double c_i = config.imaginary_upper - (row * step_i);
        for (int col{0}; col < config.image_width; ++col)
        {
            const double c_r = config.real_lower + (col * step_r);

            // pixel colors
            std::uint8_t pixel_r = 0;
            std::uint8_t pixel_g = 0;
            std::uint8_t pixel_b = 0;

            double z_r = 0.0;
            double z_i = 0.0;

            for (int i{0}; i < config.iteration_count; ++i)
            {
                // z = z^2 + c
                const double saved_z_r = z_r;
                z_r = (z_r * z_r) - (z_i * z_i) + c_r;
                z_i = (2 * saved_z_r * z_i) + c_i;

                // exit condition check: |z| > 2.0?
                if (((z_r * z_r) + (z_i * z_i)) > 4.0)
                {
                    const auto d_ic = static_cast<double>(config.iteration_count);
                    pixel_r = static_cast<std::uint8_t>(255.0 * (d_ic - i) / d_ic);
                    pixel_g = static_cast<std::uint8_t>(255.0 * (d_ic - i) / d_ic);
                    pixel_b = static_cast<std::uint8_t>(255.0 * (d_ic - i) / d_ic);

                    break;
                }
            }

            row_buffer[3 * col + 0] = pixel_r;
            row_buffer[3 * col + 1] = pixel_g;
            row_buffer[3 * col + 2] = pixel_b;
        }

        image_file.write(reinterpret_cast<const char *>(row_buffer.data()), static_cast<std::streamsize>(row_buffer_length));
    }
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Running took " << std::fixed << std::setprecision(9) << elapsed.count() << " seconds\n";

    image_file.close();

    return 0;
}