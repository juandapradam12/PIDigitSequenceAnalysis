# GitHub About (copy into the repo page)

GitHub **Settings → General** (or the gear next to **About** on the repo home).  
Paste the description, then add the topics. Website can stay empty.

---

## Description

```
Computational case study on π digits: sequence frequencies vs a uniform i.i.d. model, boundary-aware partitioned scans, and serial vs OpenMP throughput.
```

*(152 characters — fits GitHub’s 350-character About field.)*

### Shorter alternative

```
π digit sequences: i.i.d. frequency tests, seam-aware partitioning, and OpenMP parallel scans in C.
```

---

## Topics

Add these (all lowercase, as GitHub requires):

```
pi
mathematics
statistics
data-science
scientific-computing
computational-science
numerical-analysis
c
python
openmp
parallel-computing
pattern-matching
benchmarking
hpc
portfolio
```

### Optional extras (if you want a stronger DS signal)

```
goodness-of-fit
chi-square-test
time-series
```

*(Prefer the first block only if you want a tight topic list.)*

---

## CLI equivalent (run locally with your own token)

```bash
gh repo edit juandapradam12/PIDigitSequenceAnalysis \
  --description "Computational case study on π digits: sequence frequencies vs a uniform i.i.d. model, boundary-aware partitioned scans, and serial vs OpenMP throughput." \
  --add-topic pi \
  --add-topic mathematics \
  --add-topic statistics \
  --add-topic data-science \
  --add-topic scientific-computing \
  --add-topic computational-science \
  --add-topic numerical-analysis \
  --add-topic c \
  --add-topic python \
  --add-topic openmp \
  --add-topic parallel-computing \
  --add-topic pattern-matching \
  --add-topic benchmarking \
  --add-topic hpc \
  --add-topic portfolio
```
