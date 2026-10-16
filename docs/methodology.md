# Methodology

## Problem statement

Let $D = d_1 d_2 \ldots d_N$ be the first $N$ decimal digits of $\pi$ (non-digit characters removed). For a pattern $p$ of length $k$,

$$
C_k(p) = \sum_{i=1}^{N-k+1} \mathbf{1}\{d_i d_{i+1}\cdots d_{i+k-1} = p\}.
$$

Default patterns:

$$
p \in \{1,\, 12,\, 123,\, 1234,\, 12345\}.
$$

---

## Statistical baseline

Assume digits are i.i.d. uniform on $\{0,1,\ldots,9\}$. Then

$$
\mathbb{P}(\text{match}) = 10^{-k},
\qquad
\mathbb{E}[C_k(p)] = (N - k + 1)\,10^{-k}.
$$

Approximate standardized residual (Bernoulli-trials variance, ignoring window dependence):

$$
z_k \approx \frac{C_k - \mathbb{E}[C_k]}{\sqrt{\mathbb{E}[C_k]\,(1-10^{-k})}}.
$$

Relative discrepancy:

$$
\varepsilon_k = \frac{\bigl|C_k - \mathbb{E}[C_k]\bigr|}{\mathbb{E}[C_k]}.
$$

### Marginal digit frequencies

Let $n_d$ be the count of digit $d \in \{0,\ldots,9\}$ and $N=\sum_d n_d$. Under uniformity,

$$
\chi^2 = \sum_{d=0}^{9} \frac{(n_d - N/10)^2}{N/10},
$$

compared to a $\chi^2_9$ reference (Wilson–Hilferty $p$-value approximation in `analyze.py`).

---

## Counting algorithm

Sliding-window scan with `memcmp` per alignment:

- Cost per pattern: $O\bigl((N-k+1)\,k\bigr)$.
- Overlaps counted (every starting index).
- Digits loaded once, filtered to `'0'..'9'`, then scanned in memory.

---

## Partition benchmark

For

$$
P \in \{1, 2, 5, 10, 20, 50, 100\},
$$

the file is split into $P$ roughly equal **byte** chunks and each chunk is processed sequentially. Each $P$ is timed for $T$ independent trials (default $T=5$); the CSV stores sample mean and sample standard deviation of wall-clock time.

Interpretation:

- Aggregate digit work remains $\Theta(N)$.
- $T(P)$ typically **increases** with $P$ because startup and short-file overhead accumulate.
- This is not a parallel speedup study.
- Byte splits can cut patterns across chunk boundaries; summed per-chunk counts are therefore a lower bound on the full-file $C_k$. Frequency results always use the single full-file pass.

---

## Outputs

| Artifact | Content |
| --- | --- |
| `results/pattern_counts.csv` | $p$, $k$, $C_k$, per-pattern CPU seconds |
| `results/observed_vs_expected.csv` | $\mathbb{E}[C_k]$, $\sigma$, $z_k$, $\varepsilon_k$ |
| `results/digit_frequency.csv` | $n_d$, shares, $\chi^2$, $p$ |
| `results/partition_benchmark.csv` | $P$, mean/std seconds, trials |
| `figures/*.png` | Sequence counts, digit histogram, timing curve |

---

## Software structure

```
sequence_counter  →  pure counting + digit I/O
main              →  CLI
tests/            →  unit coverage for overlaps, filtering, errors
benchmark.sh      →  multi-trial partition experiment
analyze.py        →  z-scores, χ², plots
.github/workflows →  build + test + pipeline CI
```
