# Mandelbrot Renderer

A high-performance Mandelbrot set renderer written in C, featuring serial,
multithreaded, and AVX2-vectorised CPU backends, smooth histogram-based
colouring, direct PNG output, dynamic tile scheduling, progressive rendering,
and an interactive SDL3 viewer.

The project is designed as both a fractal renderer and an experimental platform
for exploring numerical computation, CPU parallelism, SIMD vectorisation,
dynamic scheduling, rendering optimisation, and interactive visualisation.

## Features

### Rendering

- Mandelbrot escape-time rendering
- Smooth escape-time values
- Histogram-based colour distribution
- Smooth CDF interpolation
- Gamma correction
- RGB palette mapping
- Direct PNG output using `libpng`
- Row-streamed PNG writing
- Configurable image resolution
- Configurable maximum iteration count
- Manual complex-plane bounds
- Centre/zoom navigation
- Named render presets
- High-resolution rendering tested up to 15360 x 8640

### CPU Backends

The renderer supports multiple computation backends:

```text
serial
pthread
avx2
pthread-avx2
```

These allow direct comparison between scalar, multithreaded, SIMD-vectorised,
and combined multithreaded + SIMD implementations.

### CPU Optimisations

- POSIX threads (`pthreads`)
- AVX2 SIMD vectorisation
- Combined pthread + AVX2 rendering
- Dynamic tile-based work scheduling
- Configurable tile size
- Thread-local histograms
- Histogram reduction after worker completion
- Analytical main-cardioid rejection
- Analytical period-2 bulb rejection
- Optional periodicity checking
- Reduced synchronisation during pixel computation
- Compute-only benchmarking with `--no-output`

### Progressive Rendering

The tile renderer supports progressive completion callbacks.

Completed tiles can be passed from worker threads through a thread-safe tile
queue, allowing another thread, such as the SDL viewer, to display rendering
progress while computation continues.

The progressive renderer also supports cancellation, allowing an active render
to be stopped when the user changes the view.

### Interactive SDL3 Viewer

An SDL3-based viewer provides interactive Mandelbrot exploration.

Current viewer features include:

- Progressive tile display
- Background rendering thread
- Thread-safe completed-tile queue
- Mouse-wheel zoom
- Cursor-centred zooming
- Render cancellation when the view changes
- Automatic restart using the new complex-plane bounds
- Final histogram/CDF colouring after the render completes
- Resizable SDL window
- Left-click drag panning

The viewer therefore remains responsive while the Mandelbrot image is being
computed.

## Project Structure

```text
mandelbrot/
├── include/
│   ├── mandelbrot.h
│   └── mandelbrot_queue.h
│
├── plot/
│   └── *.png
│
├── results/
│   ├── benchmark_results.csv
│   ├── benchmark_summary.csv
│   ├── benchmark_scaling.csv
│   └── *.png
│
├── scripts/
│   ├── benchmark.sh
│   └── analyse_benchmark.py
│
├── src/
│   ├── main.c
│   ├── viewer.c
│   ├── mandelbrot_core.c
│   ├── mandelbrot_avx2.c
│   ├── mandelbrot_colour.c
│   ├── mandelbrot_output.c
│   └── mandelbrot_queue.c
│
├── tests/
│   ├── test_avx2.c
│   ├── test_tiles.c
│   ├── test_progressive.c
│   ├── test_queue.c
│   └── test_progressive_queue.c
│   
├── makefile
└── README.md
```

## Dependencies

The command-line renderer requires:

- GCC or another compatible C compiler
- POSIX threads
- `libm`
- `libpng`
- An AVX2-capable x86-64 CPU for the AVX2 backends

The interactive viewer additionally requires:

- SDL3

On Debian/Ubuntu:

```bash
sudo apt install build-essential libpng-dev libsdl3-dev
```

The benchmark analysis scripts additionally use Python with:

- `pandas`
- `matplotlib`

For example:

```bash
python3 -m pip install pandas matplotlib
```

## Building

Build the command-line renderer:

```bash
make
```

Build the interactive viewer:

```bash
make viewer
```

Clean compiled objects and executables:

```bash
make clean
```

## Running the Renderer

A basic render can be generated with:

```bash
./mandelbrot
```

By default, the image is written directly as a PNG.

No intermediate text image or Gnuplot processing stage is required.

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
    --backend pthread-avx2 \
    --threads 16 \
    --tile-size 32 \
    --output plot/seahorse.png
```

### High-Resolution Example

```bash
./mandelbrot \
    --preset seahorse \
    --width 15360 \
    --height 8640 \
    --iterations 20000 \
    --backend pthread-avx2 \
    --threads 16 \
    --tile-size 32 \
    --output plot/seahorse_16k.png
```

## Interactive Viewer

Build and launch the SDL3 viewer with:

```bash
make viewer
```

The viewer opens an interactive Mandelbrot window and progressively displays
tiles as they are completed.

### Controls

```text
Mouse wheel up      Zoom in
Mouse wheel down    Zoom out
Left-click + drag   Pan around the fractal
Close window        Exit viewer
```

Zooming is centred on the current mouse position. The complex coordinate under
the cursor therefore remains approximately fixed on screen while zooming.

Left-click dragging pans the current complex-plane view. When the drag ends,
the active render is cancelled and restarted using the updated bounds.

When the view changes, the current render is cancelled and a new progressive
render begins immediately.

## Render Presets

Named presets are available for commonly used Mandelbrot views:

```text
full
seahorse
deep-zoom
```

For example:

```bash
./mandelbrot \
    --preset seahorse \
    --backend pthread-avx2 \
    --threads 16
```

Preset values can be overridden by options appearing later on the command
line.

For example:

```bash
./mandelbrot \
    --preset seahorse \
    --width 7680 \
    --height 4320 \
    --iterations 12000 \
    --backend pthread-avx2 \
    --threads 16 \
    --output plot/seahorse_8k.png
```

## Command-Line Options

Display the current built-in help with:

```bash
./mandelbrot --help
```

The renderer supports configuration of:

- image width and height
- maximum iteration count
- gamma correction
- PNG output filename
- manual complex-plane bounds
- centre and zoom
- render presets
- computation backend
- pthread worker count
- tile size
- periodicity checking
- compute-only benchmarking

Use `./mandelbrot --help` for the authoritative list of options supported by
the current build.

## Rendering Architecture

The primary rendering pipeline is:

```text
complex-plane coordinates
          |
          v
analytical interior tests
          |
          v
Mandelbrot escape computation
          |
          +-------------------------+
          |                         |
          v                         v
iteration counts             smooth escape values
          |                         |
          +------------+------------+
                       |
                       v
              per-thread histograms
                       |
                       v
               histogram reduction
                       |
                       v
        cumulative distribution function
                       |
                       v
       smooth CDF interpolation + gamma
                       |
                       v
                RGB palette mapping
                       |
                       v
             row-streamed PNG output
```

For interactive rendering, tile completion events provide an additional path:

```text
worker threads
      |
      v
dynamic tile scheduler
      |
      v
completed tile
      |
      v
progress callback
      |
      v
thread-safe tile queue
      |
      v
SDL viewer thread
      |
      v
texture update
```

This allows computation and display to proceed concurrently.

## Tile-Based Parallel Rendering

Mandelbrot rendering has a highly irregular workload.

Points far outside the set may escape after only a few iterations, while points
near the boundary may require thousands of iterations. Static work division can
therefore leave some worker threads idle while others continue processing
expensive regions.

The pthread renderer instead divides the image into tiles.

Worker threads dynamically claim tiles until the image is complete:

```text
Image
  |
  v
+----+----+----+----+
| T0 | T1 | T2 | T3 |
+----+----+----+----+
| T4 | T5 | T6 | T7 |
+----+----+----+----+
| T8 | T9 |... |    |
+----+----+----+----+
        |
        v
   shared scheduler
        |
   +----+----+----+
   |    |    |    |
   v    v    v    v
  W0   W1   W2   W3
```

This provides fine-grained load balancing across irregular regions of the
fractal.

The tile size can be adjusted to trade scheduler overhead against load
balancing.

## AVX2 Vectorisation

The AVX2 backend evaluates multiple Mandelbrot points simultaneously using
256-bit SIMD instructions.

Four double-precision complex-plane points can be processed together within a
vectorised iteration loop.

The combined:

```text
pthread-avx2
```

backend uses both forms of CPU parallelism:

```text
image
  |
  v
dynamic tiles
  |
  +--------+--------+--------+
  |        |        |        |
  v        v        v        v
thread   thread   thread   thread
  |        |        |        |
  v        v        v        v
 AVX2     AVX2     AVX2     AVX2
 lanes    lanes    lanes    lanes
```

This combines thread-level parallelism across CPU cores with SIMD parallelism
within each worker.

## Analytical Interior Rejection

Before performing the iterative escape calculation, points can be tested
against regions known to lie inside the Mandelbrot set.

The renderer includes analytical rejection for:

- the main cardioid
- the period-2 bulb

Points identified by these tests can immediately be classified as interior
without running the full iteration loop.

This is particularly useful for views containing large areas of the set.

## Periodicity Checking

The renderer also supports optional periodicity checking.

Points whose orbit begins repeating can be identified as likely interior
without continuing all the way to the maximum iteration count.

This optimisation is configurable because its benefit depends on the rendered
region and workload.

## Progressive Rendering

The progressive renderer reports completed tiles through a callback.

Worker threads do not directly perform SDL operations. Instead, completed tile
metadata is pushed into a thread-safe queue.

The main viewer thread consumes that queue and updates the SDL texture.

This architecture keeps rendering computation separate from display logic and
avoids performing SDL rendering operations from worker threads.

When a view change occurs:

```text
user input
    |
    v
request cancellation
    |
    v
stop current render
    |
    v
update complex-plane bounds
    |
    v
clear render state
    |
    v
start new render
```

This forms the basis for interactive fractal navigation.

## Colouring

During computation, the renderer records both integer iteration counts and
smooth escape values.

Escaped pixels contribute to histograms which are reduced after worker
completion.

A cumulative distribution function is then constructed and used with the
smooth escape values to generate the final colour distribution.

During progressive rendering, tiles can initially be displayed using preview
colouring. Once all tiles have completed, the final global histogram and CDF
are available and the complete image is recoloured using the final colour
mapping.

## Testing

The project includes validation tests for the major optimised rendering paths.

### AVX2 Validation

```bash
make test-avx2
```

This compares the AVX2 computation against the scalar implementation.

The implementations should produce:

```text
Iteration mismatches:   0
Smooth mismatches:      0
Result:                 PASS
```

### Tile Renderer Validation

```bash
make test-tiles
```

This validates both:

```text
pthread
pthread-avx2
```

tile renderers against the reference implementation.

### Progressive Rendering

```bash
make test-progressive
```

This validates progressive tile rendering against the corresponding complete
render.

### Tile Queue Validation

```bash
make test-queue
```

This validates the thread-safe producer/consumer queue independently of the
renderer.

### Progressive Queue

```bash
make test-progressive-queue
```

This validates the completed-tile queue and checks for:

- duplicate tiles
- invalid tiles
- missing tiles

A successful run should report:

```text
Duplicate tiles:        0
Invalid tiles:          0
Missing tiles:          0
Renderer result:        PASS
Overall result:         PASS
```

### Full Validation

A useful development check is:

```bash
make clean && make
make test-avx2
make test-tiles
make test-progressive
make test_queue
make test-progressive-queue
make viewer
```

## Performance

Performance depends heavily on:

- image resolution
- maximum iteration count
- rendered region
- tile size
- thread count
- backend
- analytical rejection
- periodicity checking
- CPU architecture

The current benchmark workload uses:

```text
Resolution: 1500 x 1500
Maximum iterations: 5000
Centre: (-0.743643887, 0.131825904)
Zoom: 100
Tile size: 32 x 32
Repeats per configuration: 10
```

All measurements use --no-output so PNG encoding and final colour output do
not affect compute timing.

| Backend | Threads | Mean Time (s) | Best Time (s) | Speedup vs Serial |
|---|---:|---:|---:|---:|
| Serial | 1 | 1.2920 | 1.2757 | 1.00x |
| Pthreads | 16 | 0.1051 | 0.1024 | 12.29x |
| AVX2 | 1 | 0.6475 | 0.6443 | 2.00x |
| Pthreads + AVX2 | 16 | 0.0572 | 0.0553 | 22.59x |

The fastest current backend is pthread-avx2, with a mean compute time of
approximately 57 ms and a best measured time of approximately 55 ms
for this workload.
Compared with the current scalar implementation, the combined pthread + AVX2
backend provides approximately 22.6x speedup.
The AVX2-only backend is approximately 2x faster than the scalar renderer,
while the 16-thread pthread backend provides approximately 12.3x speedup.

## Benchmarking

Compute-only mode can be used to measure the renderer without PNG encoding and
colour output affecting the results.

For example:

```bash
./mandelbrot \
    --preset seahorse \
    --backend pthread-avx2 \
    --threads 16 \
    --no-output
```

Benchmark scripts are provided under:

```text
scripts/
```

and benchmark results are stored under:

```text
results/
```

The analysis scripts can be used to calculate and visualise performance metrics
such as:

- mean render time
- speedup
- parallel efficiency
- backend comparisons
- scheduler/tile-size behaviour

## Current Optimisations

The renderer currently includes:

- Dynamic pthread scheduling
- Tile-based work distribution
- Configurable tile sizes
- Thread-local histograms
- Histogram reduction
- AVX2 SIMD vectorisation
- Combined pthread + AVX2 execution
- Main-cardioid analytical rejection
- Period-2 bulb analytical rejection
- Optional periodicity checking
- Progressive tile callbacks
- Thread-safe completed-tile queue
- Cancellable progressive rendering
- Compute-only benchmarking
- Row-streamed PNG generation
- Reduced final-colour memory requirements

## Future Work

Potential extensions include:

- Live texture preview while dragging/panning
- Improved viewer controls and status information
- Runtime AVX2 capability detection and backend selection
- Additional colour palettes
- Interactive palette and iteration controls
- Additional deep-zoom presets
- Further profiling and CPU-level optimisation
- Adaptive tile scheduling
- Higher-precision floating-point rendering
- Arbitrary-precision arithmetic for extreme deep zooms
- GPU acceleration using CUDA, OpenCL, Vulkan compute, or similar APIs
- Expanded benchmark suites across CPUs, resolutions, and fractal regions

## Development Direction

The project has progressed through several optimisation stages:

```text
scalar renderer
      |
      v
pthread parallelism
      |
      v
dynamic scheduling
      |
      v
analytical interior rejection
      |
      v
tile-based rendering
      |
      v
AVX2 SIMD
      |
      v
pthread + AVX2
      |
      v
progressive rendering
      |
      v
interactive SDL viewer
      |
      v
interactive navigation
      |
      v
higher-precision / GPU rendering
```

The current focus is interactive navigation while retaining the optimised
parallel CPU renderer underneath the viewer.

The longer-term goal is to use the project as a platform for experimenting
with increasingly advanced CPU, GPU, numerical, and real-time rendering
techniques.
