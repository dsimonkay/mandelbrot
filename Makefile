# =====================================================================
#  Mandelbrot — minimal Makefile (CPU + CUDA binaries)
#
#  Usage:
#    make             -> build both binaries (mandelbrot, mandelbrot_gpu)
#    make mandelbrot  -> CPU binary only (works on machines without nvcc)
#    make test        -> render with both binaries and byte-compare (cmp)
#    make clean       -> remove objects, binaries and test images
#
#  Anatomy reminder (for a first Makefile):
#      target: prerequisites
#      <TAB> recipe
#  Recipe lines start with a TAB character, NOT spaces — this is make's
#  most famous trap (the "missing separator" error).
#  make only rebuilds a target if it is older than any of its
#  prerequisites — that is why the headers are listed too.
# =====================================================================

CXX       := g++
NVCC      := nvcc
CXXFLAGS  := -std=c++20 -O3 -march=native -Wall -Wextra
NVCCFLAGS := -std=c++20 -O3 -arch=native

.PHONY: all test clean

all: mandelbrot mandelbrot_gpu

# ---- object files ----------------------------------------------------
# The shared part is compiled ONCE (with g++) and linked into both
# binaries. CLI11.hpp (the lion's share of the compile time) is thus
# compiled once per build instead of twice.
common.o: common.cpp common.h CLI11.hpp
	$(CXX) $(CXXFLAGS) -c common.cpp -o common.o

mandelbrot.o: mandelbrot.cpp common.h
	$(CXX) $(CXXFLAGS) -c mandelbrot.cpp -o mandelbrot.o

mandelbrot_gpu.o: mandelbrot.cu common.h
	$(NVCC) $(NVCCFLAGS) -c mandelbrot.cu -o mandelbrot_gpu.o

# ---- binaries --------------------------------------------------------
# The GPU binary is linked by nvcc, because nvcc knows how to pull in
# the CUDA runtime; the CPU binary is fine with plain g++.
mandelbrot: mandelbrot.o common.o
	$(CXX) mandelbrot.o common.o -o mandelbrot

mandelbrot_gpu: mandelbrot_gpu.o common.o
	$(NVCC) mandelbrot_gpu.o common.o -o mandelbrot_gpu

# ---- the crown of correctness, institutionalized as a ritual ---------
# The two binaries already name their outputs differently on their own:
#   CPU: <output_file>.ppm      GPU: <output_file>_gpu.ppm
test: mandelbrot mandelbrot_gpu
	./mandelbrot     --output_file render_test
	./mandelbrot_gpu --output_file render_test
	cmp render_test.ppm render_test_gpu.ppm
	@echo "OK: the two images are byte-identical."

clean:
	rm -f mandelbrot mandelbrot_gpu *.o render_test.ppm render_test_gpu.ppm
