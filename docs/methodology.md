# Methodology

## Problem statement

Let $D = d_1 d_2 \ldots d_N$ be the first $N$ decimal digits of $\pi$ (integer part and fractional digits, with non-digit characters removed). For a pattern $p$ of length $k$, define the occurrence count

$$
C_k(p) = \sum_{i=1}^{N-k+1} \mathbf{1}\{d_i d_{i+1}\cdots d_{i+k-1} = p\}.
$$

This project evaluates $C_k(p)$ for

$$
p \in \{1,\, 12,\, 123,\, 1234,\, 12345\}
$$

and records CPU time per pattern plus wall-clock time under sequential partitioning.

---

## Statistical baseline

Assume digits are i.i.d. uniform on $\{0,1,\ldots,9\}$. Then each alignment matches $p$ with probability

$$
\mathbb{P}(\text{match}) = 10^{-k},
$$

and linearity of expectation yields

$$
\mathbb{E}[C_k(p)] = (N - k + 1)\,10^{-k},
$$

independent of the specific digit content of $p$ (as long as $|p| = k$).

Relative discrepancy used in the comparison CSV:

$$
\varepsilon_k = \frac{\bigl|C_k(p) - \mathbb{E}[C_k(p)]\bigr|}{\mathbb{E}[C_k(p)]}.
$$

For large $N$ and small $k$, $\varepsilon_k$ is typically small if $\pi$'s digits behave like the uniform model at these scales. Longer patterns ($k = 4,5$) have large relative variance because $\mathbb{E}[C_k]$ is $O(N/10^k)$.

---

## Counting algorithm

The core scan is a sliding-window comparison:

- For each pattern $p$ of length $k$, walk $i = 0, \ldots, N-k$.
- Compare $D[i..i+k)$ to $p$ with `memcmp` ($O(k)$ per alignment).
- Total cost per pattern: $O\bigl((N-k+1)\,k\bigr)$.

Overlaps are intentional: the indicator in $C_k(p)$ fires at every starting index, so the empirical count is directly comparable to $\mathbb{E}[C_k]$.

Digits are loaded once, filtered to `'0'..'9'`, then scanned in memory — I/O is amortized outside the per-pattern timers.

---

## Partition benchmark

For partition counts

$$
P \in \{1, 2, 5, 10, 20, 50, 100\},
$$

the digit file is split into $P$ roughly equal byte chunks. Each chunk is fed to the counter **sequentially**. The reported metric is wall-clock time for the full loop:

$$
T(P) = t_{\text{end}} - t_{\text{start}}.
$$

Interpretation:

- Work on digits remains $\Theta(N)$ in aggregate.
- As $P$ grows, process startup, open/close, and short-file overhead accumulate, so $T(P)$ is expected to **increase** rather than decrease.
- This isolates chunking overhead; it is not a parallel speedup study.

---

## Outputs

| Artifact | Content |
| --- | --- |
| `results/pattern_counts.csv` | $p$, $k$, $C_k$, per-pattern CPU seconds |
| `results/observed_vs_expected.csv` | $C_k$ vs $\mathbb{E}[C_k]$ and $\varepsilon_k$ |
| `results/partition_benchmark.csv` | $P$, $T(P)$, digits scanned |
| `figures/observed_vs_expected.png` | Bar chart of $C_k$ vs $\mathbb{E}[C_k]$ |
| `figures/partition_timing.png` | $T(P)$ vs $P$ |

---

## Software structure

```
sequence_counter  →  pure counting + digit I/O
main              →  argument parsing, human + CSV reporting
benchmark.sh      →  partition experiment orchestration
analyze.py        →  expectation formulas, plots, summary tables
```

The C API (`count_patterns`, `load_digits`, `write_report_csv`) is intentional: the same library can back alternative drivers (e.g. different pattern sets or online streams) without changing the analysis layer.
