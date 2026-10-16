#!/usr/bin/env python3
"""Analyze pattern counts and partition timings; emit figures and summary CSVs."""

from __future__ import annotations

import csv
from collections import Counter
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


def count_zscore(observed: int, n_digits: int, pattern_len: int) -> tuple[float, float, float]:
    """
    Approximate z-score treating alignments as Bernoulli(p) trials.

    Dependence across overlapping windows is ignored; the approximation is
    standard for large N and is most reliable for rare (large-k) patterns.
    """
    mu = expected_count(n_digits, pattern_len)
    p = 10.0**-pattern_len
    sigma = float(np.sqrt(max(mu * (1.0 - p), 0.0)))
    z = (observed - mu) / sigma if sigma > 0 else float("nan")
    return mu, sigma, z


def load_pattern_counts(path: Path) -> list[dict]:
    with path.open(newline="") as fh:
        return list(csv.DictReader(fh))


def load_benchmark(path: Path) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
    partitions: list[int] = []
    means: list[float] = []
    stds: list[float] = []
    with path.open(newline="") as fh:
        for row in csv.DictReader(fh):
            partitions.append(int(row["partitions"]))
            if "mean_seconds" in row:
                means.append(float(row["mean_seconds"]))
                stds.append(float(row.get("std_seconds", 0.0) or 0.0))
            else:
                # Backward-compatible with single-trial CSV.
                means.append(float(row["total_elapsed_seconds"]))
                stds.append(0.0)
    return np.asarray(partitions), np.asarray(means), np.asarray(stds)


def digit_frequencies(digits_path: Path) -> tuple[np.ndarray, int]:
    text = digits_path.read_text()
    counts = Counter(ch for ch in text if ch.isdigit())
    freq = np.array([counts.get(str(d), 0) for d in range(10)], dtype=float)
    return freq, int(freq.sum())


def chi_square_uniform(freq: np.ndarray) -> tuple[float, float]:
    """Pearson chi-square against Digits ~ Uniform{0..9}; returns (stat, p_approx)."""
    n = float(freq.sum())
    expected = np.full(10, n / 10.0)
    stat = float(np.sum((freq - expected) ** 2 / expected))
    # Survival function for chi-square(df=9) via Wilson–Hilferty approximation.
    df = 9.0
    z = ((stat / df) ** (1.0 / 3.0) - (1.0 - 2.0 / (9.0 * df))) / np.sqrt(2.0 / (9.0 * df))
    # One-sided upper-tail approx using erfc.
    from math import erfc, sqrt

    p = 0.5 * erfc(z / sqrt(2.0))
    return stat, float(p)


def write_comparison(counts: list[dict], n_digits: int, out_path: Path) -> None:
    with out_path.open("w", newline="") as fh:
        writer = csv.writer(fh)
        writer.writerow(
            [
                "pattern",
                "length",
                "observed",
                "expected",
                "std_approx",
                "z_score",
                "relative_error",
            ]
        )
        for row in counts:
            k = int(row["length"])
            observed = int(row["count"])
            mu, sigma, z = count_zscore(observed, n_digits, k)
            rel_err = abs(observed - mu) / mu if mu > 0 else float("nan")
            writer.writerow(
                [
                    row["pattern"],
                    k,
                    observed,
                    f"{mu:.6f}",
                    f"{sigma:.6f}",
                    f"{z:.6f}",
                    f"{rel_err:.6f}",
                ]
            )


def write_digit_frequency(freq: np.ndarray, out_path: Path) -> tuple[float, float]:
    n = float(freq.sum())
    expected = n / 10.0
    chi2, p = chi_square_uniform(freq)
    with out_path.open("w", newline="") as fh:
        writer = csv.writer(fh)
        writer.writerow(["digit", "count", "expected", "share"])
        for d, c in enumerate(freq):
            writer.writerow([d, int(c), f"{expected:.6f}", f"{c / n:.6f}"])
        writer.writerow([])
        writer.writerow(["chi_square", f"{chi2:.6f}"])
        writer.writerow(["df", 9])
        writer.writerow(["p_approx", f"{p:.6g}"])
    return chi2, p


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


def plot_partition_timing(
    partitions: np.ndarray,
    means: np.ndarray,
    stds: np.ndarray,
    out_path: Path,
) -> None:
    fig, ax = plt.subplots(figsize=(7.5, 4.8))
    ax.errorbar(
        partitions,
        means,
        yerr=stds,
        marker="o",
        color="#1f4e5f",
        linewidth=2,
        capsize=3,
        label="mean ± std",
    )

    ax.set_xlabel("Number of partitions")
    ax.set_ylabel("Wall-clock time (s)")
    ax.set_title("Sequential chunked processing time vs partitions")
    ax.set_xscale("log")
    ax.legend(frameon=False)
    ax.spines["top"].set_visible(False)
    ax.spines["right"].set_visible(False)
    fig.tight_layout()
    fig.savefig(out_path, dpi=160)
    plt.close(fig)


def plot_digit_frequency(freq: np.ndarray, out_path: Path) -> None:
    fig, ax = plt.subplots(figsize=(7.5, 4.8))
    digits = np.arange(10)
    expected = np.full(10, freq.sum() / 10.0)

    ax.bar(digits - 0.2, freq, width=0.4, label="Observed", color="#1f4e5f")
    ax.bar(digits + 0.2, expected, width=0.4, label="Uniform expected", color="#e07a3d")
    ax.set_xticks(digits)
    ax.set_xlabel("Digit")
    ax.set_ylabel("Count")
    ax.set_title("Marginal digit frequencies in π")
    ax.legend(frameon=False)
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
    digits_file = ROOT / "data" / "pi_digits.txt"

    if not counts_path.exists():
        raise SystemExit(f"Missing {counts_path}. Run: make run")
    if not bench_path.exists():
        raise SystemExit(f"Missing {bench_path}. Run: make benchmark")

    counts = load_pattern_counts(counts_path)
    freq, n_digits = digit_frequencies(digits_file)

    write_comparison(counts, n_digits, RESULTS / "observed_vs_expected.csv")
    chi2, p = write_digit_frequency(freq, RESULTS / "digit_frequency.csv")
    plot_observed_vs_expected(counts, n_digits, FIGURES / "observed_vs_expected.png")
    plot_digit_frequency(freq, FIGURES / "digit_frequency.png")

    partitions, means, stds = load_benchmark(bench_path)
    plot_partition_timing(partitions, means, stds, FIGURES / "partition_timing.png")

    print(f"digits_analyzed={n_digits}")
    print(f"digit_chi_square={chi2:.4f} p_approx={p:.4g}")
    print(f"wrote {RESULTS / 'observed_vs_expected.csv'}")
    print(f"wrote {RESULTS / 'digit_frequency.csv'}")
    print(f"wrote {FIGURES / 'observed_vs_expected.png'}")
    print(f"wrote {FIGURES / 'digit_frequency.png'}")
    print(f"wrote {FIGURES / 'partition_timing.png'}")


if __name__ == "__main__":
    main()
