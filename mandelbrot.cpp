#include "common.h"

#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

int main(int argc, char **argv)
{
    const auto wall_start = TimingReport::clock_type::now();
    TimingReport timing{};

    auto config = get_config(argc, argv);
    set_viewport_details(config);

    // Create binary image file
    const auto filename = config.output_path + "/" + config.output_file + ".ppm";
    auto image_file = std::ofstream{filename, std::ios::binary};
    if (!image_file.is_open())
    {
        std::cerr << " Error '" << filename << "' could not be opened; exiting.\n";
        return 1;
    }
    image_file << "P6\n"
               << config.image_width << "\n"
               << config.image_height << "\n"
               << static_cast<std::uint16_t>(255) << "\n";

    auto row_buffer = std::vector<std::array<std::uint8_t, 3U>>(config.image_width);
    const auto palette = generate_palette(config);

    timing.measure("CPU compute + PPM write", [&]
                   {    for (int row{0}; row < config.image_height; ++row)
    {
        const double c_i = config.viewport_top - (row * config.step);
        for (int col{0}; col < config.image_width; ++col)
        {
            const double c_r = config.viewport_left + (col * config.step);

            // pixel colors
            std::array<std::uint8_t, 3U> pixel_color{};

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
                    pixel_color = palette[i % config.palette_size];
                    break;
                }
            }

            row_buffer[col] = pixel_color;
        }

        image_file.write(reinterpret_cast<const char *>(row_buffer.data()), static_cast<std::streamsize>(3 * config.image_width));
    } });

    image_file.close();
    const double wall_seconds =
        std::chrono::duration<double>(TimingReport::clock_type::now() - wall_start).count();
    timing.print(wall_seconds);

    return 0;
}