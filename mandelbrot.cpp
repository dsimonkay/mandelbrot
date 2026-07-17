#include "CLI11.hpp"

#include <chrono>
#include <cstdint>
#include <format>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <string>

/// @brief Configuration object for rendering a single Mandelbrot frame
struct Config
{
    double start_real{-2.5};       ///< Range start on the real axis
    double end_real{1.0};          ///< Range end on the real axis
    double start_imaginary{-1.75}; ///< Range start on the imaginary axis
    double end_imaginary{1.75};    ///< Range end on the imaginary axis
    std::uint16_t image_width{1600};
    std::uint16_t image_height{1600};
    std::uint16_t iteration_count{1000};
};

int main(int argc, char **argv)
{
    Config config{};

    CLI::App app{"Simple Mandelbrot fractal generator"};
    app.add_option("--start_real", config.start_real, "Range start on the real axis");
    app.add_option("--end_real", config.end_real, "Range end on the real axis");
    app.add_option("--start_imaginary", config.start_imaginary, "Range start on the imaginary axis");
    app.add_option("--end_imaginary", config.end_imaginary, "Range end on the imaginary axis");
    app.add_option("--image_width", config.image_width, "Image width");
    app.add_option("--image_height", config.image_height, "Image height");
    app.add_option("--iteration_count", config.iteration_count, "Iteration_count");

    CLI11_PARSE(app, argc, argv);

    const auto file_name{"./mandelbrot.ppm"};
    auto image_file = std::ofstream{file_name};
    if (!image_file.is_open())
    {
        std::cerr << " Error " << file_name << " could not be opened; exiting.\n";
        return 1;
    }

    image_file << "P3\n"
               << std::to_string(config.image_width) << " " << std::to_string(config.image_height) << "\n255\n";

    const double step_r = (config.end_real - config.start_real) / config.image_width;
    const double step_i = (config.end_imaginary - config.start_imaginary) / config.image_height;

    double c_r = config.start_real;
    double c_i = config.end_imaginary; // because we're gonna go top down!!

    const auto start = std::chrono::steady_clock::now();

    for (int row{0}; row < config.image_height; ++row)
    {
        for (int col{0}; col < config.image_width; ++col)
        {
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

                // const auto x = ((z_r * z_r) + (z_i * z_i));
                // std::cout << "[" << c_r << ", " << c_i << "] step #" << i << ": z = [" << z_r << ", " << z_i << "] --> |z| = " << x << "\n";
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

            image_file << std::format("{} {} {}  ", pixel_r, pixel_g, pixel_b);
            c_r += step_r;
        }

        image_file << "\n";
        c_r = config.start_real;
        c_i -= step_i; // progressing "downwards"
    }
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Running took " << std::fixed << std::setprecision(9) << elapsed.count() << " seconds\n";

    image_file.close();

    return 0;
}