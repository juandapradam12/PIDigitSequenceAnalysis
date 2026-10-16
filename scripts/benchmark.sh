#!/usr/bin/env bash
# Sequential chunked-processing benchmark for pi digit counting.
# Runs TRIALS wall-clock measurements per partition count and records mean ± std.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DIGITS="${ROOT}/data/pi_digits.txt"
COUNTER="${ROOT}/bin/pi_counter"
OUT_CSV="${ROOT}/results/partition_benchmark.csv"
TMP_DIR="$(mktemp -d "${TMPDIR:-/tmp}/pi_bench.XXXXXX")"
TRIALS="${TRIALS:-5}"
PARTITIONS=(1 2 5 10 20 50 100)

cleanup() {
  rm -rf "${TMP_DIR}"
}
trap cleanup EXIT

if [[ ! -x "${COUNTER}" ]]; then
  echo "Missing binary: ${COUNTER}. Run 'make' first." >&2
  exit 1
fi

if [[ ! -f "${DIGITS}" ]]; then
  echo "Missing digit file: ${DIGITS}" >&2
  exit 1
fi

mkdir -p "${ROOT}/results"

echo "partitions,mean_seconds,std_seconds,trials,digits_scanned" > "${OUT_CSV}"

for n in "${PARTITIONS[@]}"; do
  part_dir="${TMP_DIR}/n${n}"
  mkdir -p "${part_dir}"
  rm -f "${part_dir}"/part*.txt

  if [[ "${n}" -eq 1 ]]; then
    cp "${DIGITS}" "${part_dir}/part00.txt"
  else
    # Byte-equal chunks; patterns spanning boundaries are intentionally not
    # stitched — this experiment measures chunking overhead, not full recall.
    split -n "${n}" -d --additional-suffix=.txt "${DIGITS}" "${part_dir}/part"
  fi

  times_file="${TMP_DIR}/times_${n}.txt"
  : > "${times_file}"
  digits_total=0

  for ((trial = 1; trial <= TRIALS; trial++)); do
    start_ns="$(date +%s%N)"
    digits_total=0

    for part in "${part_dir}"/part*.txt; do
      summary="$("${COUNTER}" "${part}" 2>/dev/null | sed -n '1,2p')"
      part_digits="$(printf '%s\n' "${summary}" | sed -n 's/^digits_scanned=//p')"
      digits_total=$((digits_total + part_digits))
    done

    end_ns="$(date +%s%N)"
    elapsed="$(awk -v s="${start_ns}" -v e="${end_ns}" 'BEGIN { printf "%.9f", (e - s) / 1e9 }')"
    echo "${elapsed}" >> "${times_file}"
  done

  stats="$(awk '
    { sum += $1; sumsq += $1 * $1; n++ }
    END {
      mean = sum / n
      var = (n > 1) ? (sumsq - (sum * sum) / n) / (n - 1) : 0
      if (var < 0) var = 0
      printf "%.9f %.9f", mean, sqrt(var)
    }
  ' "${times_file}")"
  mean="$(echo "${stats}" | awk '{print $1}')"
  std="$(echo "${stats}" | awk '{print $2}')"

  echo "${n},${mean},${std},${TRIALS},${digits_total}" >> "${OUT_CSV}"
  echo "partitions=${n} mean=${mean}s std=${std}s trials=${TRIALS} digits=${digits_total}"
done

echo "Wrote ${OUT_CSV}"
