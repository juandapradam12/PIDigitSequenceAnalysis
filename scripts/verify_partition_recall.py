#!/usr/bin/env python3
"""Compare naive byte-chunk counts vs boundary-stitched owned-range counts."""

from __future__ import annotations

import csv
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DIGITS_PATH = ROOT / "data" / "pi_digits.txt"
OUT_CSV = ROOT / "results" / "partition_recall.csv"
PATTERNS = ["1", "12", "123", "1234", "12345"]
# Include denser partitions so seam collisions become visible in the CSV/figure.
PARTITIONS = [1, 2, 5, 10, 20, 50, 64, 100, 128, 256, 500, 1000, 2000, 5000]


def load_digits(path: Path) -> str:
    return "".join(ch for ch in path.read_text() if ch.isdigit())


def count_full(digits: str, pattern: str) -> int:
    k = len(pattern)
    return sum(1 for i in range(len(digits) - k + 1) if digits[i : i + k] == pattern)


def count_naive(digits: str, pattern: str, n_partitions: int) -> int:
    """Sum counts on disjoint slices with no look-ahead (misses boundary hits)."""
    n = len(digits)
    k = len(pattern)
    total = 0
    for part in range(n_partitions):
        begin = (part * n) // n_partitions
        end = ((part + 1) * n) // n_partitions
        chunk = digits[begin:end]
        total += sum(1 for i in range(len(chunk) - k + 1) if chunk[i : i + k] == pattern)
    return total


def count_stitched(digits: str, pattern: str, n_partitions: int) -> int:
    """Owned-range counting with full-buffer look-ahead (matches full scan)."""
    n = len(digits)
    k = len(pattern)
    total = 0
    last_start = n - k + 1
    for part in range(n_partitions):
        owned_begin = (part * n) // n_partitions
        owned_end = ((part + 1) * n) // n_partitions
        end = min(owned_end, last_start)
        for i in range(owned_begin, end):
            if digits[i : i + k] == pattern:
                total += 1
    return total


def main() -> None:
    digits = load_digits(DIGITS_PATH)
    OUT_CSV.parent.mkdir(parents=True, exist_ok=True)

    full = {p: count_full(digits, p) for p in PATTERNS}

    with OUT_CSV.open("w", newline="") as fh:
        writer = csv.writer(fh)
        writer.writerow(
            [
                "partitions",
                "pattern",
                "full_count",
                "naive_count",
                "stitched_count",
                "naive_missed",
                "stitched_ok",
            ]
        )
        for n_partitions in PARTITIONS:
            for pattern in PATTERNS:
                naive = count_naive(digits, pattern, n_partitions)
                stitched = count_stitched(digits, pattern, n_partitions)
                writer.writerow(
                    [
                        n_partitions,
                        pattern,
                        full[pattern],
                        naive,
                        stitched,
                        full[pattern] - naive,
                        int(stitched == full[pattern]),
                    ]
                )

    # Pattern "12" is frequent enough that seam collisions show up clearly.
    pattern = "12"
    print(f"digits={len(digits)} pattern={pattern} full={full[pattern]}")
    for n_partitions in PARTITIONS:
        naive = count_naive(digits, pattern, n_partitions)
        stitched = count_stitched(digits, pattern, n_partitions)
        print(
            f"P={n_partitions:5d}  naive={naive:5d}  missed={full[pattern]-naive:4d}  "
            f"stitched={stitched:5d}  ok={stitched == full[pattern]}"
        )
    print(f"wrote {OUT_CSV}")


if __name__ == "__main__":
    main()
