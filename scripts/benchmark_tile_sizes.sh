#!/usr/bin/env bash

set -e

OUTPUT="results/tile_size_results.csv"
REPEATS=5

TILE_SIZES=(4 8 16 32 64 128 256 512)
BACKENDS=("pthread-avx2")

mkdir -p results

echo "backend,threads,tile_size,run,time_seconds" > "$OUTPUT"

for backend in "${BACKENDS[@]}"; do
    for tile in "${TILE_SIZES[@]}"; do
        for run in $(seq 1 "$REPEATS"); do

            echo "Backend=$backend Tile=$tile Run=$run"

            output=$(
                ./mandelbrot \
                    --preset seahorse \
                    --backend "$backend" \
                    --threads 16 \
                    --tile-size "$tile" \
                    --no-output
            )

            time_seconds=$(
                echo "$output" |
                awk '/Compute time:/ {print $3}'
            )

            echo \
                "$backend,16,$tile,$run,$time_seconds" \
                >> "$OUTPUT"
        done
    done
done

echo
echo "Results written to $OUTPUT"
