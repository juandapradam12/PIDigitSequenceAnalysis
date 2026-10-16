#!/usr/bin/env bash
# Sequential chunked-processing benchmark for pi digit counting.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DIGITS="${ROOT}/data/pi_digits.txt"
COUNTER="${ROOT}/bin/pi_counter"
OUT_CSV="${ROOT}/results/partition_benchmark.csv"
TMP_DIR="$(mktemp -d "${TMPDIR:-/tmp}/pi_bench.XXXXXX")"

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
PARTITIONS=(1 2 5 10 20 50 100)

echo "partitions,total_elapsed_seconds,digits_scanned" > "${OUT_CSV}"

for n in "${PARTITIONS[@]}"; do
  part_dir="${TMP_DIR}/n${n}"
  mkdir -p "${part_dir}"

  if [[ "${n}" -eq 1 ]]; then
    cp "${DIGITS}" "${part_dir}/part00.txt"
  else
    # split -n produces roughly equal chunks by byte size.
    split -n "${n}" -d --additional-suffix=.txt "${DIGITS}" "${part_dir}/part"
  fi

  start_ns="$(date +%s%N)"
  digits_total=0

  for part in "${part_dir}"/part*.txt; do
    # Capture digits_scanned from counter stdout; discard pattern table.
    summary="$("${COUNTER}" "${part}" 2>/dev/null | head -n 2)"
    part_digits="$(printf '%s\n' "${summary}" | sed -n 's/^digits_scanned=//p')"
    digits_total=$((digits_total + part_digits))
  done

  end_ns="$(date +%s%N)"
  elapsed="$(awk -v s="${start_ns}" -v e="${end_ns}" 'BEGIN { printf "%.9f", (e - s) / 1e9 }')"

  echo "${n},${elapsed},${digits_total}" >> "${OUT_CSV}"
  echo "partitions=${n} elapsed=${elapsed}s digits=${digits_total}"
done

echo "Wrote ${OUT_CSV}"
