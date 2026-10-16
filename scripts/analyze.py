#!/usr/bin/env python3
"""Analyze pattern counts and partition timings; emit figures and a summary CSV."""

from __future__ import annotations

import csv
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np

ROOT = Path(__file__).resolve().parents[1]
RESULTS = ROOT / "results"
FIGURES = ROOT / "figures"


def expected_count(n_digits: int, pattern_len: int) -> float:
    """E[C] = (N - k + 1) * 10^{-k} under a uniform i.i.d. digit model."""
    if n_digits < pattern_len:
        return 0.0
    return (n_digits - pattern_len + 1) * (10.0**-pattern_len)


def load_pattern_counts(path: Path) -> list[dict]:
    with path.open(newline="") as fh:
        return list(csv.DictReader(fh))


def load_benchmark(path: Path) -> tuple[np.ndarray, np.ndarray]:
    partitions: list[int] = []
    times: list[float] = []
    with path.open(newline="") as fh:
        for row in csv.DictReader(fh):
            partitions.append(int(row["partitions"]))
            times.append(float(row["total_elapsed_seconds"]))
    return np.asarray(partitions), np.asarray(times)


def write_comparison(counts: list[dict], n_digits: int, out_path: Path) -> None:
    with out_path.open("w", newline="") as fh:
        writer = csv.writer(fh)
        writer.writerow(
            [
                "pattern",
                "length",
                "observed",
                "expected",
                "relative_error",
            ]
        )
        for row in counts:
            k = int(row["length"])
            observed = int(row["count"])
            exp = expected_count(n_digits, k)
            rel_err = abs(observed - exp) / exp if exp > 0 else float("nan")
            writer.writerow(
                [
                    row["pattern"],
                    k,
                    observed,
                    f"{exp:.6f}",
                    f"{rel_err:.6f}",
                ]
            )


def plot_observed_vs_expected(counts: list[dict], n_digits: int, out_path: Path) -> None:
    lengths = [int(r["length"]) for r in counts]
    observed = [int(r["count"]) for r in counts]
    expected = [expected_count(n_digits, k) for k in lengths]

    fig, ax = plt.subplots(figsize=(7.5, 4.8))
    x = np.arange(len(lengths))
    width = 0.36

    ax.bar(x - width / 2, observed, width, label="Observed", color="#1f4e5f")
    ax.bar(x + width / 2, expected, width, label="Expected (i.i.d.)", color="#e07a3d")

    ax.set_xticks(x)
    ax.set_xticklabels([r["pattern"] for r in counts])
    ax.set_yscale("log")
    ax.set_xlabel("Digit sequence")
    ax.set_ylabel("Count (log scale)")
    ax.set_title("Sequence counts in π digits vs uniform model")
    ax.legend(frameon=False)
    ax.spines["top"].set_visible(False)
    ax.spines["right"].set_visible(False)
    fig.tight_layout()
    fig.savefig(out_path, dpi=160)
    plt.close(fig)


def plot_partition_timing(partitions: np.ndarray, times: np.ndarray, out_path: Path) -> None:
    fig, ax = plt.subplots(figsize=(7.5, 4.8))
    ax.plot(partitions, times, marker="o", color="#1f4e5f", linewidth=2)

    ax.set_xlabel("Number of partitions")
    ax.set_ylabel("Wall-clock time (s)")
    ax.set_title("Sequential chunked processing time vs partitions")
    ax.set_xscale("log")
    ax.spines["top"].set_visible(False)
    ax.spines["right"].set_visible(False)
    fig.tight_layout()
    fig.savefig(out_path, dpi=160)
    plt.close(fig)


def main() -> None:
    FIGURES.mkdir(parents=True, exist_ok=True)
    RESULTS.mkdir(parents=True, exist_ok=True)

    counts_path = RESULTS / "pattern_counts.csv"
    bench_path = RESULTS / "partition_benchmark.csv"

    if not counts_path.exists():
        raise SystemExit(f"Missing {counts_path}. Run: make run")
    if not bench_path.exists():
        raise SystemExit(f"Missing {bench_path}. Run: make benchmark")

    counts = load_pattern_counts(counts_path)
    # Digit length is recovered from the benchmark's last full scan or file size.
    digits_file = ROOT / "data" / "pi_digits.txt"
    n_digits = sum(1 for ch in digits_file.read_text() if ch.isdigit())

    write_comparison(counts, n_digits, RESULTS / "observed_vs_expected.csv")
    plot_observed_vs_expected(counts, n_digits, FIGURES / "observed_vs_expected.png")

    partitions, times = load_benchmark(bench_path)
    plot_partition_timing(partitions, times, FIGURES / "partition_timing.png")

    print(f"digits_analyzed={n_digits}")
    print(f"wrote {RESULTS / 'observed_vs_expected.csv'}")
    print(f"wrote {FIGURES / 'observed_vs_expected.png'}")
    print(f"wrote {FIGURES / 'partition_timing.png'}")


if __name__ == "__main__":
    main()
