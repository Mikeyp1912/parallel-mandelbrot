# Mandelbrot Renderer

A configurable Mandelbrot set renderer written in C, featuring smooth
histogram-based colouring, direct PNG output, and a multithreaded POSIX
threads implementation.

The project explores fractal rendering, numerical computation, parallel CPU
performance, dynamic work scheduling, high-resolution image generation, and
benchmarking across different thread counts and scheduler chunk sizes.

## Features

- Serial Mandelbrot renderer
- POSIX threads (`pthreads`) implementation
- Dynamic row scheduling for improved load balancing
- Configurable scheduler chunk size
- Per-thread histograms with reduction
- Smooth escape-time values
- Histogram-based colour distribution
- Smooth CDF interpolation
- Gamma correction
- RGB palette mapping
- Direct PNG output using `libpng`
- Row-streamed PNG writing
- Configurable PNG output filename
- Configurable image resolution
- Configurable maximum iteration count
- Manual complex-plane bounds
- Centre/zoom navigation
- Named render presets
- Robust command-line validation and error reporting
- Wall-clock performance measurement
- `--no-output` mode for compute-only benchmarking
- Automated benchmarking
- Speedup and parallel-efficiency analysis
- Reduced render memory usage by avoiding a full final-colour pixel buffer
- High-resolution rendering tested up to 15360 x 8640

## Project Structure

```text
mandelbrot/
├── include/
│   └── mandelbrot.h
├── plot/
│   └── *.png                  # generated renders (ignored by Git)
├── results/
│   ├── benchmark_results.csv
│   ├── benchmark_summary.csv
│   ├── benchmark_scaling.csv
│   ├── benchmark_speedup.png
│   ├── benchmark_efficiency.png
│   └── benchmark_chunk_size.png
├── scripts/
│   ├── benchmark.sh
│   └── analyse_benchmark.py
├── src/
│   ├── main.c
│   ├── mandelbrot_core.c
│   ├── mandelbrot_colour.c
│   └── mandelbrot_output.c
├── makefile
└── README.md
```

## Dependencies

The renderer requires:

- A C compiler such as GCC
- POSIX threads
- `libm`
- `libpng`

On Debian/Ubuntu, the PNG development library can be installed with:

```bash
sudo apt install libpng-dev
```

The benchmark analysis script additionally uses Python with `pandas` and
`matplotlib`.

## Building

Build the renderer with:

```bash
make
```

To clean the compiled objects and executable:

```bash
make clean
```

To remove the default generated render:

```bash
make clean-render
```

To clean both:

```bash
make clean-all
```

## Rendering

A basic render can be generated directly as a PNG:

```bash
./mandelbrot
```

By default, the image is written to:

```text
plot/mandel.png
```

No intermediate text image file or Gnuplot processing step is required.

### Seahorse Example

```bash
./mandelbrot \
    --width 1920 \
    --height 1080 \
    --iterations 5000 \
    --gamma 3.5 \
    --center-x -0.743643887 \
    --center-y 0.131825904 \
    --zoom 100 \
    --threads 16 \
    --chunk-size 1 \
    --output plot/seahorse.png
```

### High-Resolution Example

The renderer can also generate very large images directly:

```bash
./mandelbrot \
    --preset seahorse \
    --width 15360 \
    --height 8640 \
    --iterations 20000 \
    --threads 16 \
    --chunk-size 1 \
    --output plot/seahorse_16k.png
```

## Render Presets

Three named presets are currently available:

```text
full
seahorse
deep-zoom
```

For example:

```bash
./mandelbrot --preset seahorse --threads 16
```

Preset values can be overridden by options that appear later on the command
line:

```bash
./mandelbrot \
    --preset seahorse \
    --width 7680 \
    --height 4320 \
    --iterations 12000 \
    --threads 16 \
    --output plot/seahorse_8k.png
```

## Command-Line Options

Display the built-in help with:

```bash
./mandelbrot --help
```

General options:

```text
-h, --help             Show the help message
--width <pixels>       Image width
--height <pixels>      Image height
--iterations <count>   Maximum iterations
--gamma <value>        Colour gamma correction
--no-output            Skip colouring and output file generation
--output <file>        PNG output filename
```

Manual bounds:

```text
--xmin <value>         Minimum real coordinate
--xmax <value>         Maximum real coordinate
--ymin <value>         Minimum imaginary coordinate
--ymax <value>         Maximum imaginary coordinate
```

Centre/zoom navigation:

```text
--center-x <value>     Centre real coordinate
--center-y <value>     Centre imaginary coordinate
--zoom <value>         Zoom factor (1.0 = full view)
```

Parallelisation:

```text
--threads <count>      Number of worker threads
--chunk-size <rows>    Rows assigned per scheduler request
```

Presets:

```text
--preset <name>        Use a named render preset
                       Available: full, seahorse, deep-zoom
```

Manual bounds and centre/zoom-based views cannot be used simultaneously.
Presets use the centre/zoom view mode and therefore cannot be combined with
manual bounds.

## Rendering Pipeline

The current rendering pipeline is:

```text
complex-plane coordinates
          |
          v
Mandelbrot escape computation
          |
          +--> iteration counts
          |
          +--> smooth escape values
          |
          v
per-thread escape histograms
          |
          v
histogram reduction
          |
          v
cumulative distribution function (CDF)
          |
          v
smooth CDF interpolation + gamma correction
          |
          v
RGB palette mapping
          |
          v
row-streamed libpng output
          |
          v
PNG image
```

The renderer does not allocate a separate full-resolution final-colour buffer.
Instead, colour values are calculated while each PNG row is generated. The
main full-image storage therefore consists of the integer iteration counts and
double-precision smooth escape values, reducing the principal per-pixel buffer
requirement from approximately 20 bytes to 12 bytes.

## Parallel Implementation

Mandelbrot rendering has an irregular computational workload. Points that
escape quickly require relatively little computation, while points near or
inside the Mandelbrot set may require many iterations.

A simple static division of image rows can therefore produce load imbalance
between threads.

The pthread renderer uses a shared dynamic scheduler. Worker threads repeatedly
request chunks of rows until the entire image has been processed. The
`--chunk-size` option controls how many rows are claimed in each scheduler
request.

Each worker maintains a private histogram while rendering. These histograms are
reduced into the final global histogram after all worker threads complete,
avoiding fine-grained histogram locking during pixel computation.

## Performance

The benchmark workload used:

```text
Resolution: 1500 x 1500
Maximum iterations: 5000
Centre: (-0.743643887, 0.131825904)
Zoom: 100
Repeats per configuration: 5
```

The benchmark uses compute-only mode so image colouring and PNG output do not
distort the renderer scaling measurements.

| Threads | Best Chunk Size | Mean Time (s) | Speedup | Efficiency |
|--------:|----------------:|--------------:|--------:|-----------:|
| 1 | - | 7.098 | 1.00x | 100.0% |
| 2 | 16 | 3.558 | 1.99x | 99.7% |
| 4 | 1 | 1.925 | 3.69x | 92.2% |
| 8 | 4 | 0.978 | 7.26x | 90.7% |
| 16 | 1 | 0.490 | 14.49x | 90.6% |

The 16-thread implementation reduced mean compute time from approximately
**7.10 seconds to 0.49 seconds**, corresponding to approximately
**14.49x speedup** while retaining around **90.6% parallel efficiency** on
16 logical CPUs.

### Parallel Scaling

![Parallel speedup](results/benchmark_speedup.png)

The pthread implementation scales from a mean serial compute time of
7.098 seconds to 0.490 seconds using 16 threads.

![Parallel efficiency](results/benchmark_efficiency.png)

Parallel efficiency remains around 90% at the best measured 8-thread and
16-thread configurations.

### Dynamic Scheduling

![Dynamic scheduling chunk size](results/benchmark_chunk_size.png)

Smaller scheduler chunks generally perform better at higher thread counts for
this workload. Although larger chunks reduce synchronization frequency, they
also reduce the scheduler's ability to balance the irregular Mandelbrot
workload.

## Benchmarking

Run the benchmark suite with:

```bash
./scripts/benchmark.sh
```

Raw measurements are written to:

```text
results/benchmark_results.csv
```

Analyse the results with:

```bash
python3 scripts/analyse_benchmark.py
```

This generates the benchmark summaries and performance figures in `results/`.

## Current Optimisations

The renderer currently includes several performance and memory-oriented design
choices:

- Dynamic pthread work scheduling rather than fixed row partitions
- Configurable scheduler chunk sizes
- Thread-local histograms to avoid locking for every escaped pixel
- Histogram reduction after worker completion
- Compute-only benchmarking through `--no-output`
- Row-streamed PNG generation
- No full-resolution final-colour buffer
- Smooth escape values retained separately from integer iteration counts

## Future Work

Potential extensions include:

- Analytical interior tests for the main cardioid and period-2 bulb
- Additional colour palettes and palette selection through the CLI
- Additional render presets and deep-zoom locations
- Tile-based rendering for lower memory usage on extremely large images
- SIMD/vectorised Mandelbrot computation using AVX2 or similar instruction sets
- Further profiling and CPU-level optimisation
- Higher-precision or arbitrary-precision arithmetic for deep zooms
- GPU implementation using CUDA, OpenCL, or another compute API
- Interactive fractal exploration and progressive rendering
- Expanded benchmarking across optimisation strategies, resolutions, and
  iteration counts

## Development Direction

The project is intended to continue as both a fractal renderer and a platform
for experimenting with numerical and parallel-computing techniques. A likely
development path is:

```text
analytical interior rejection
        |
        v
tile-based rendering
        |
        v
SIMD CPU rendering
        |
        v
arbitrary-precision deep zoom
        |
        v
GPU rendering
        |
        v
interactive exploration
```
