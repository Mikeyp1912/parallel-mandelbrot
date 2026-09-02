#!/usr/bin/env bash

set -e

OUTPUT="results/benchmark_results.csv"

THREADS=(2 4 8 16)
CHUNKS=(1 2 4 8 16 32)
REPEATS=5

WIDTH=1500
HEIGHT=1500
ITERATIONS=5000
CENTER_X=-0.743643887
CENTER_Y=0.131825904
ZOOM=100

echo "mode,threads,chunk_size,run,time_seconds" > "$OUTPUT"

echo "Running serial baseline..."

for run in $(seq 1 "$REPEATS"); do
    output=$(
        ./mandelbrot \
            --width "$WIDTH" \
            --height "$HEIGHT" \
            --iterations "$ITERATIONS" \
            --center-x "$CENTER_X" \
            --center-y "$CENTER_Y" \
            --zoom "$ZOOM" \
            --threads 1 \
            --no-output
    )

    time=$(echo "$output" | awk '/Compute time:/ {print $3}')

    echo "serial,1,0,$run,$time" >> "$OUTPUT"

    echo "  serial run $run: $time s"
done

for threads in "${THREADS[@]}"; do
    for chunk in "${CHUNKS[@]}"; do

        echo "Running threads=$threads chunk=$chunk..."

        for run in $(seq 1 "$REPEATS"); do
            output=$(
                ./mandelbrot \
                    --width "$WIDTH" \
                    --height "$HEIGHT" \
                    --iterations "$ITERATIONS" \
                    --center-x "$CENTER_X" \
                    --center-y "$CENTER_Y" \
                    --zoom "$ZOOM" \
                    --threads "$threads" \
                    --chunk-size "$chunk" \
                    --no-output
            )

            time=$(echo "$output" | awk '/Compute time:/ {print $3}')

            echo "pthreads,$threads,$chunk,$run,$time" >> "$OUTPUT"

            echo "  run $run: $time s"
        done
    done
done

echo
echo "Benchmark complete."
echo "Results written to $OUTPUT"
