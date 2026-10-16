# Data

| File | Role |
| --- | --- |
| `pi_digits.txt` | Default digit stream (~$2.53\times 10^6$ digits), committed |
| `pi_digits_large.txt` | Optional larger corpus from `make generate-digits` (gitignored) |

The counter strips non-digit characters on load, so a leading `3.` or whitespace does not bias $N$ or $C_k$.

```bash
N=10000000 make generate-digits
./bin/pi_counter data/pi_digits_large.txt --patterns 1,12,123,1234,12345
```
