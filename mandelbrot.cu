#include "CLI11.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <format>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#define CUDA_CHECK(call)                                               \
    do                                                                 \
    {                                                                  \
        const cudaError_t err = (call);                                \
        if (err != cudaSuccess)                                        \
        {                                                              \
            std::cerr << "CUDA error: " << cudaGetErrorString(err)     \
                      << " (" << __FILE__ << ":" << __LINE__ << ")\n"; \
            std::exit(1);                                              \
        }                                                              \
    } while (0)

__global__ void mandelbrot_kernel(int *escape,
                                  const int width, const int height,
                                  const double real_lower, const double imaginary_upper,
                                  const double step_r, const double step_i,
                                  const int iteration_count)
{
    // "who am I?"
    const int col = blockIdx.x * blockDim.x + threadIdx.x;
    const int row = blockIdx.y * blockDim.y + threadIdx.y;
    if ((col >= width) || (row >= height)) // sanity check
    {
        return;
    }

    const double c_r = real_lower + (col * step_r);
    const double c_i = imaginary_upper - (row * step_i);

    double z_r = 0.0;
    double z_i = 0.0;

    int e = iteration_count;
    for (int i = 0; i < iteration_count; ++i)
    {
        const double saved_z_r = z_r;
        z_r = (z_r * z_r) - (z_i * z_i) + c_r;
        z_i = (2 * saved_z_r * z_i) + c_i;
        if (((z_r * z_r) + (z_i * z_i)) > 4.0)
        {
            e = i;
            break;
        }
    }

    escape[row * width + col] = e;
}

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
    std::string output_file{"./mandelbrot_cuda.ppm"};
};

/// @brief Generate a palette for the fractal generator.
///
/// @note This is an overflowing palette.
///
/// @param config The current configuration of the program
/// @return the pregenerated palette
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

int main(int argc, char **argv)
{
    const auto wall_start = std::chrono::steady_clock::now();

    Config config{};

    CLI::App app{"Simple Mandelbrot fractal generator"};
    app.add_option("--real_lower", config.real_lower, "Range start on the real axis");
    app.add_option("--real_upper", config.real_upper, "Range end on the real axis");
    app.add_option("--imaginary_lower", config.imaginary_lower, "Range start on the imaginary axis");
    app.add_option("--imaginary_upper", config.imaginary_upper, "Range end on the imaginary axis");
    app.add_option("--image_width", config.image_width, "Image width");
    app.add_option("--image_height", config.image_height, "Image height");
    app.add_option("--iteration_count", config.iteration_count, "Iteration_count");
    app.add_option("--palette_size", config.palette_size, "Number of colors in the palette");
    app.add_option("-o, --output_file", config.output_file, "Output image file name");
    CLI11_PARSE(app, argc, argv);

    auto image_file = std::ofstream{config.output_file, std::ios::binary};
    if (!image_file.is_open())
    {
        std::cerr << " Error '" << config.output_file << "' could not be opened; exiting.\n";
        return 1;
    }

    // Create binary image file
    image_file << "P6\n"
               << config.image_width << "\n"
               << config.image_height << "\n"
               << static_cast<std::uint16_t>(255) << "\n";

    const double step_r = (config.real_upper - config.real_lower) / config.image_width;
    const double step_i = (config.imaginary_upper - config.imaginary_lower) / config.image_height;

    // -------------------- Let the GPU compute the escape numbers
    const auto pixel_count = std::size_t{config.image_width} * config.image_height;
    int *d_escape = nullptr;
    CUDA_CHECK(cudaMalloc(&d_escape, pixel_count * sizeof(int)));

    const dim3 block(16, 16); // 256 threads / block
    const dim3 grid((config.image_width + block.x - 1) / block.x,
                    (config.image_height + block.y - 1) / block.y);
    mandelbrot_kernel<<<grid, block>>>(d_escape, config.image_width, config.image_height,
                                       config.real_lower, config.imaginary_upper,
                                       step_r, step_i, config.iteration_count);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    std::vector<int> escape(pixel_count);
    CUDA_CHECK(cudaMemcpy(escape.data(), d_escape, pixel_count * sizeof(int),
                          cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaFree(d_escape));
    // ------------------------------------------------------

    auto row_buffer = std::vector<std::array<std::uint8_t, 3U>>(config.image_width);
    const auto palette = generate_palette(config);

    const auto start = std::chrono::steady_clock::now();
    for (int row = 0; row < config.image_height; ++row)
    {
        for (int col = 0; col < config.image_width; ++col)
        {
            // new: instead of the Z-iteration
            const int e = escape[static_cast<std::size_t>(row) * config.image_width + col];
            row_buffer[col] = (e >= config.iteration_count)
                                  ? std::array<std::uint8_t, 3U>{}
                                  : palette[e % config.palette_size];
        }

        image_file.write(reinterpret_cast<const char *>(row_buffer.data()), static_cast<std::streamsize>(3 * config.image_width));
    }
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::chrono::duration<double> wall_elapsed = end - wall_start;
    std::cout << "       Wall time: " << std::fixed << std::setprecision(9) << wall_elapsed.count() << " seconds   \n"
              << "Coloring + write: " << elapsed.count() << " seconds\n";

    image_file.close();

    return 0;
}
