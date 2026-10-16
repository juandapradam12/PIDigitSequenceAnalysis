#!/usr/bin/env python3
"""Generate N decimal digits of pi via Machin's formula (stdlib only)."""

from __future__ import annotations

import argparse
from decimal import Decimal, getcontext
from pathlib import Path


def arctan_inv(x: int, terms_guard: int = 10) -> Decimal:
    """Compute arctan(1/x) with the current Decimal precision."""
    getcontext().prec += terms_guard
    x_dec = Decimal(x)
    total = Decimal(0)
    term = Decimal(1) / x_dec
    n = 0
    while True:
        total += term / (2 * n + 1)
        term *= -Decimal(1) / (x_dec * x_dec)
        n += 1
        if abs(term) < Decimal(10) ** (-getcontext().prec):
            break
    getcontext().prec -= terms_guard
    return +total


def pi_digits(n_digits: int) -> str:
    """
    Return the first n_digits decimal digits of pi (no decimal point).

    Uses Machin's formula: pi/4 = 4*arctan(1/5) - arctan(1/239).
    """
    if n_digits < 1:
        raise ValueError("n_digits must be >= 1")

    getcontext().prec = n_digits + 10
    pi = 4 * (4 * arctan_inv(5) - arctan_inv(239))
    # Digits only: integer part + fractional digits, no radix point.
    scaled = (pi * (Decimal(10) ** (n_digits - 1))).quantize(Decimal(1))
    text = format(scaled, "f")
    if len(text) < n_digits:
        text = text.zfill(n_digits)
    return text[:n_digits]


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--digits", type=int, default=10_000_000, help="Number of digits")
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("data/pi_digits_large.txt"),
        help="Output path",
    )
    args = parser.parse_args()

    print(f"Generating {args.digits} digits of pi ...")
    digits = pi_digits(args.digits)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(digits + "\n")
    print(f"wrote {args.output} ({len(digits)} digits)")


if __name__ == "__main__":
    main()
