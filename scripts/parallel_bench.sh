#!/usr/bin/env bash
# Compare serial full scan, stitched serial partitions, and OpenMP partitions.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DIGITS="${ROOT}/data/pi_digits.txt"
SERIAL="${ROOT}/bin/pi_counter"
OMP="${ROOT}/bin/pi_counter_omp"
OUT_CSV="${ROOT}/results/parallel_benchmark.csv"
TRIALS="${TRIALS:-5}"
PARTITIONS=(1 2 4 8 16 32)
THREADS="${THREADS:-$(nproc 2>/dev/null || echo 4)}"

if [[ ! -x "${SERIAL}" || ! -x "${OMP}" ]]; then
  echo "Build binaries first: make all" >&2
  exit 1
fi

mkdir -p "${ROOT}/results"
echo "mode,partitions,threads,mean_seconds,std_seconds,trials,count_12345" > "${OUT_CSV}"

run_trials() {
  local mode="$1"
  local cmd="$2"
  local partitions="$3"
  local threads="$4"
  local times_file
  times_file="$(mktemp)"
  local count_ref=""

  for ((t = 1; t <= TRIALS; t++)); do
    out="$(eval "${cmd}")"
    elapsed="$(printf '%s\n' "${out}" | sed -n 's/^total_elapsed_seconds=//p' | tail -n 1)"
    count_ref="$(printf '%s\n' "${out}" | awk '/^12345 / {print $3}')"
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
  rm -f "${times_file}"
  mean="$(echo "${stats}" | awk '{print $1}')"
  std="$(echo "${stats}" | awk '{print $2}')"
  echo "${mode},${partitions},${threads},${mean},${std},${TRIALS},${count_ref}" >> "${OUT_CSV}"
  echo "${mode} P=${partitions} threads=${threads} mean=${mean}s count_12345=${count_ref}"
}

# Baseline: single-buffer serial scan.
run_trials "serial_full" "\"${SERIAL}\" \"${DIGITS}\"" 1 1

for p in "${PARTITIONS[@]}"; do
  run_trials "stitched_serial" \
    "\"${SERIAL}\" \"${DIGITS}\" --partitions ${p}" \
    "${p}" 1

  run_trials "stitched_omp" \
    "\"${OMP}\" \"${DIGITS}\" --partitions ${p} --threads ${THREADS}" \
    "${p}" "${THREADS}"
done

echo "Wrote ${OUT_CSV}"
