#!/usr/bin/env bash

set -e

OUTPUT="results/cpu_backend_results.csv"
REPEATS=5

mkdir -p results

echo "backend,threads,run,time" > "$OUTPUT"

run_benchmark() {
    backend="$1"
    threads="$2"

    for run in $(seq 1 "$REPEATS"); do
        output=$(
            ./mandelbrot \
                --preset seahorse \
                --backend "$backend" \
                --threads "$threads" \
                --chunk-size 1 \
                --no-output
        )

        time=$(echo "$output" |
            awk '/Compute time:/ {print $3}')

        echo "$backend,$threads,$run,$time" >> "$OUTPUT"

        echo "$backend threads=$threads run=$run time=$time"
    done
}

echo "Benchmarking serial..."
run_benchmark serial 1

echo "Benchmarking AVX2..."
run_benchmark avx2 1

for threads in 2 4 8 16; do
    echo "Benchmarking pthread, $threads threads..."
    run_benchmark pthread "$threads"
done

for threads in 2 4 8 16; do
    echo "Benchmarking pthread-avx2, $threads threads..."
    run_benchmark pthread-avx2 "$threads"
done

echo
echo "Results written to $OUTPUT"
