#!/usr/bin/env python3

import pandas as pd
import matplotlib.pyplot as plt

INPUT_FILE = "results/benchmark_results.csv"

df = pd.read_csv(INPUT_FILE)

serial = df[df["mode"] == "serial"]
parallel = df[df["mode"] == "pthreads"]

serial_mean = serial["time_seconds"].mean()

summary = (
    parallel
    .groupby(["threads", "chunk_size"])["time_seconds"]
    .agg(["mean", "std"])
    .reset_index()
)

summary["speedup"] = serial_mean / summary["mean"]
summary["efficiency"] = (
    summary["speedup"] / summary["threads"]
) * 100.0

print(f"Serial mean: {serial_mean:.6f} seconds")
print()
print(summary.to_string(index=False))

best = (
    summary
    .sort_values("mean")
    .groupby("threads", as_index=False)
    .first()
    .sort_values("threads")
)

serial_row = pd.DataFrame({
    "threads": [1],
    "chunk_size": [0],
    "mean": [serial_mean],
    "std": [serial["time_seconds"].std()],
    "speedup": [1.0],
    "efficiency": [100.0],
})

scaling = pd.concat(
    [serial_row, best],
    ignore_index=True
)

print()
print("Best configuration per thread count:")
print(scaling.to_string(index=False))

summary.to_csv(
    "results/benchmark_summary.csv",
    index=False
)

scaling.to_csv(
    "results/benchmark_scaling.csv",
    index=False
)

plt.figure()

plt.plot(
    scaling["threads"],
    scaling["speedup"],
    marker="o",
    label="Measured speedup"
)

plt.plot(
    scaling["threads"],
    scaling["threads"],
    linestyle="--",
    label="Ideal speedup"
)

plt.xlabel("Threads")
plt.ylabel("Speedup")
plt.title("Mandelbrot Parallel Speedup")
plt.xticks(scaling["threads"])
plt.grid(True)
plt.legend()
plt.tight_layout()

plt.savefig("results/benchmark_speedup.png", dpi=200)
plt.close()

plt.figure()

plt.plot(
    scaling["threads"],
    scaling["efficiency"],
    marker="o"
)

plt.xlabel("Threads")
plt.ylabel("Parallel Efficiency (%)")
plt.title("Mandelbrot Parallel Efficiency")
plt.xticks(scaling["threads"])
plt.ylim(0, 105)
plt.grid(True)
plt.tight_layout()

plt.savefig("results/benchmark_efficiency.png", dpi=200)
plt.close()

plt.figure()

for threads in sorted(summary["threads"].unique()):
    subset = summary[summary["threads"] == threads]

    plt.plot(
        subset["chunk_size"],
        subset["mean"],
        marker="o",
        label=f"{threads} threads"
    )

plt.xlabel("Chunk Size (rows)")
plt.ylabel("Mean Compute Time (seconds)")
plt.title("Effect of Dynamic Scheduling Chunk Size")
plt.xscale("log", base=2)
plt.xticks(
    [1, 2, 4, 8, 16, 32],
    [1, 2, 4, 8, 16, 32]
)
plt.grid(True)
plt.legend()
plt.tight_layout()

plt.savefig("results/benchmark_chunk_size.png", dpi=200)
plt.close()

print()
print("Generated:")
print("  results/benchmark_summary.csv")
print("  results/benchmark_scaling.csv")
print("  results/benchmark_speedup.png")
print("  results/benchmark_efficiency.png")
print("  results/benchmark_chunk_size.png")
