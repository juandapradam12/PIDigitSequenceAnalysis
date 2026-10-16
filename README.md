# Digit Sequence Analysis in $\pi$

A computational case study: count ascending digit sequences in the first $\sim 2.5 \times 10^6$ digits of $\pi$, compare observations against a uniform i.i.d. digit model, and measure how sequential file partitioning affects wall-clock cost.

![Observed vs expected sequence counts](figures/observed_vs_expected.png)

---

## Use case

Given a long digit expansion of $\pi$, the project answers three concrete questions:

1. **Frequency** — How often do the patterns $1$, $12$, $123$, $1234$, and $12345$ appear?
2. **Model fit** — Are those counts consistent with $\mathbb{E}[C_k]=(N-k+1)10^{-k}$, and are marginal digit frequencies consistent with uniformity?
3. **Throughput under chunking** — How does wall-clock time behave when the same digit stream is processed as $P$ sequential partitions?

Under a uniform independent-digit model, a length-$k$ pattern has match probability $10^{-k}$ at each alignment, so

$$
\mathbb{E}[C_k] = (N - k + 1)\,10^{-k},
\qquad
z_k \approx \frac{C_k - \mathbb{E}[C_k]}{\sqrt{\mathbb{E}[C_k]\,(1-10^{-k})}}.
$$

| Pattern | Observed | Expected | $z$ | Rel. error |
| --- | ---: | ---: | ---: | ---: |
| `1` | 252478 | 253132.7 | −1.37 | 0.26% |
| `12` | 25069 | 25313.3 | −1.54 | 0.96% |
| `123` | 2527 | 2531.3 | −0.09 | 0.17% |
| `1234` | 237 | 253.1 | −1.01 | 6.4% |
| `12345` | 22 | 25.3 | −0.66 | 13.1% |

---

## Architecture

```
├── include/sequence_counter.h   # Public API
├── src/sequence_counter.c       # Digit load + sliding-window counting
├── src/main.c                   # CLI (patterns, CSV export)
├── tests/                       # C unit tests
├── scripts/benchmark.sh         # Multi-trial partition wall-clock study
├── scripts/analyze.py           # z-scores, digit χ², figures
├── data/pi_digits.txt           # Input digit stream
├── results/                     # CSV outputs
├── figures/                     # Generated plots
└── docs/methodology.md          # Model, metrics, and design notes
```

| Layer | Responsibility |
| --- | --- |
| **C library** | Load digits, count overlapping substring hits, time each pattern |
| **CLI** | Thin interface over the library; optional CSV export |
| **Benchmark script** | Split into $P$ chunks; report mean ± std over repeated trials |
| **Python analysis** | $z$-scores, digit-frequency $\chi^2$, plots |
| **CI** | Build, `make test`, full pipeline on every push/PR |

---

## Build & run

```bash
make                    # build bin/pi_counter
make test               # unit tests
make run                # → results/pattern_counts.csv
make benchmark          # → results/partition_benchmark.csv  (TRIALS=5 by default)
pip install -r requirements.txt
make analyze            # comparison CSVs + figures/
```

```bash
./bin/pi_counter data/pi_digits.txt --csv results/pattern_counts.csv
./bin/pi_counter data/pi_digits.txt --patterns 7,77,314
TRIALS=10 make benchmark
```

---

## Results at a glance

![Digit frequency histogram](figures/digit_frequency.png)

![Partition timing with error bars](figures/partition_timing.png)

- **Sequence counts** track the i.i.d. baseline closely for $k \le 3$; larger $k$ show bigger relative deviations as $\mathbb{E}[C_k]$ shrinks.
- **Digit marginals** pass a uniformity check ($\chi^2_9 \approx 11.4$, $p \approx 0.25$; see `results/digit_frequency.csv`).
- **Partition curve** rises with $P$: total digit work stays $\Theta(N)$, while process/I/O overhead accumulates. Naive byte splits also drop patterns that straddle chunk boundaries — the timing study measures overhead, not full recall across seams.

---

## Design choices

- **Overlapping matches** are counted, matching the alignment model behind $\mathbb{E}[C_k]$.
- **Non-digit characters** are stripped on load.
- **Multi-trial benchmarks** report mean ± sample std instead of a single noisy wall-clock sample.
- **CSV-first outputs** keep the analysis layer replaceable without touching the C core.

Details: [docs/methodology.md](docs/methodology.md).

---

## Requirements

- GCC (C11) or compatible compiler
- GNU `split`, `bash`, `make`
- Python 3.10+ with `numpy`, `matplotlib`

License: [MIT](LICENSE).
