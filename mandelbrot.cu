#include "common.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
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

int main(int argc, char **argv)
{
    const auto wall_start = TimingReport::clock_type::now();
    TimingReport timing{};

    const auto config = get_config(argc, argv);

    // The very first CUDA call wakes up the GPU and builds the CUDA context.
    // This is a one-time "entry ticket"; we pay it here explicitly, on its own
    // stopwatch, so every later measurement shows pure work.
    timing.measure("CUDA init (context + wake-up)", []
                   { CUDA_CHECK(cudaFree(nullptr)); });

    // Create binary image file
    const auto filename = config.output_path + "/" + config.output_file + "_gpu.ppm";
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

    const double step_r = (config.real_upper - config.real_lower) / config.image_width;
    const double step_i = (config.imaginary_upper - config.imaginary_lower) / config.image_height;

    // -------------------- Let the GPU compute the escape numbers
    const auto pixel_count = std::size_t{config.image_width} * config.image_height;
    int *d_escape = nullptr;
    timing.measure("GPU alloc (cudaMalloc)", [&]
                   { CUDA_CHECK(cudaMalloc(&d_escape, pixel_count * sizeof(int))); });

    const dim3 block(16, 16); // 256 threads / block
    const dim3 grid((config.image_width + block.x - 1) / block.x,
                    (config.image_height + block.y - 1) / block.y);

    timing.measure("GPU compute (kernel)", [&]
                   {
                       mandelbrot_kernel<<<grid, block>>>(d_escape, config.image_width, config.image_height,
                                                          config.real_lower, config.imaginary_upper,
                                                          step_r, step_i, config.iteration_count);
                       CUDA_CHECK(cudaGetLastError());
                       CUDA_CHECK(cudaDeviceSynchronize()); });

    std::vector<int> escape(pixel_count);
    timing.measure("copy device -> host (cudaMemcpy)", [&]
                   { CUDA_CHECK(cudaMemcpy(escape.data(), d_escape, pixel_count * sizeof(int),
                                           cudaMemcpyDeviceToHost)); });
    CUDA_CHECK(cudaFree(d_escape));
    // ------------------------------------------------------

    auto row_buffer = std::vector<std::array<std::uint8_t, 3U>>(config.image_width);
    const auto palette = generate_palette(config);

    timing.measure("coloring + PPM write", [&]
                   {
                       for (int row = 0; row < config.image_height; ++row)
                       {
                           for (int col = 0; col < config.image_width; ++col)
                           {
                               // instead of the Z-iteration: read the precomputed escape count
                               const int e = escape[static_cast<std::size_t>(row) * config.image_width + col];
                               row_buffer[col] = (e >= config.iteration_count)
                                                     ? std::array<std::uint8_t, 3U>{}
                                                     : palette[e % config.palette_size];
                           }

                           image_file.write(reinterpret_cast<const char *>(row_buffer.data()),
                                            static_cast<std::streamsize>(3 * config.image_width));
                       } });

    image_file.close();
    const double wall_seconds =
        std::chrono::duration<double>(TimingReport::clock_type::now() - wall_start).count();
    timing.print(wall_seconds);

    return 0;
}
