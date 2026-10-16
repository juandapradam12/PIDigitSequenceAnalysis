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

## Boundary-aware partitioning

Split the index domain into $P$ **owned** ranges

$$
I_p = \Bigl[\Big\lfloor \tfrac{p N}{P}\Big\rfloor,\, \Big\lfloor \tfrac{(p+1)N}{P}\Big\rfloor\Bigr),
\qquad p = 0,\ldots,P-1.
$$

Partition $p$ counts only starts $i \in I_p$, but may read $d_i,\ldots,d_{i+k-1}$ beyond the owned end. Summing over $p$ recovers $C_k$ exactly.

**Naive byte chunks** (no look-ahead) miss any pattern whose start lies near the end of a chunk and whose body crosses the cut. `scripts/verify_partition_recall.py` measures those misses; `count_patterns_partitioned` implements the stitched rule.

---

## OpenMP parallel scan

`bin/pi_counter_omp` parallelizes the loop over owned ranges with OpenMP. Threads reduce per-pattern counts without double-counting because ownership of $i$ is unique. This is a contrast to the **I/O chunking overhead** study in `benchmark.sh`, which intentionally processes separate files sequentially.

---

## I/O chunking benchmark

For

$$
P \in \{1, 2, 5, 10, 20, 50, 100\},
$$

the file is split into $P$ roughly equal **byte** chunks and each chunk is processed sequentially. Each $P$ is timed for $T$ trials (default $T=5$); the CSV stores sample mean and sample standard deviation.

Interpretation: aggregate digit work stays $\Theta(N)$, while startup/I/O overhead grows with $P$. That experiment measures overhead, not parallel speedup and not full recall across seams.

---

## Larger digit corpora

`scripts/generate_pi_digits.py` builds $N$ digits with Machin's formula

$$
\frac{\pi}{4} = 4\arctan\frac{1}{5} - \arctan\frac{1}{239}
$$

using Python's `decimal` (stdlib only). Large outputs are gitignored; the committed default remains $\sim 2.5\times 10^6$ digits.

```bash
N=10000000 make generate-digits   # → data/pi_digits_large.txt
./bin/pi_counter data/pi_digits_large.txt --csv results/pattern_counts_large.csv
```

---

## Outputs

| Artifact | Content |
| --- | --- |
| `results/pattern_counts.csv` | $p$, $k$, $C_k$, per-pattern CPU seconds |
| `results/observed_vs_expected.csv` | $\mathbb{E}[C_k]$, $\sigma$, $z_k$, $\varepsilon_k$ |
| `results/digit_frequency.csv` | $n_d$, shares, $\chi^2$, $p$ |
| `results/partition_benchmark.csv` | I/O chunking mean/std seconds |
| `results/partition_recall.csv` | Naive misses vs stitched recall |
| `results/parallel_benchmark.csv` | Serial vs OpenMP stitched timings |
| `figures/*.png` | Plots for the tables above |

---

## Software structure

```
sequence_counter     → counting, owned-range stitch, optional OpenMP
main                 → CLI (--partitions, --parallel, --threads)
tests/               → overlaps, stitch correctness, load filtering
benchmark.sh         → sequential file-chunk overhead
verify_partition_recall.py → naive vs stitched counts
parallel_bench.sh    → serial full / stitched serial / OpenMP
generate_pi_digits.py → optional larger corpora
analyze.py           → z-scores, χ², figures
```
