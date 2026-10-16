#ifndef SEQUENCE_COUNTER_H
#define SEQUENCE_COUNTER_H

#include <stddef.h>
#include <stdio.h>

#define MAX_PATTERNS 16
#define MAX_PATTERN_LEN 32

typedef struct {
    char pattern[MAX_PATTERN_LEN];
    size_t length;
    size_t count;
    double elapsed_seconds;
} PatternResult;

typedef struct {
    PatternResult patterns[MAX_PATTERNS];
    size_t pattern_count;
    size_t digits_scanned;
    double total_elapsed_seconds;
} CountReport;

/**
 * Count overlapping occurrences of each pattern over the full digit buffer.
 */
int count_patterns(const char *digits,
                   size_t length,
                   const char *const *patterns,
                   size_t n_patterns,
                   CountReport *report);

/**
 * Count patterns by splitting the stream into n_partitions owned ranges.
 *
 * Each partition p owns starts in [start_p, start_{p+1}). Matches may read
 * up to (max_pattern_len - 1) digits past the owned end (boundary stitch),
 * so the summed counts equal a full-file scan.
 *
 * When built with OpenMP and parallel != 0, partitions are processed in
 * parallel. Wall/CPU timing is still recorded in the report.
 *
 * @param n_partitions Number of owned ranges (>= 1).
 * @param parallel     Non-zero to enable OpenMP (no-op if not compiled with it).
 */
int count_patterns_partitioned(const char *digits,
                               size_t length,
                               const char *const *patterns,
                               size_t n_patterns,
                               size_t n_partitions,
                               int parallel,
                               CountReport *report);

/**
 * Count matches of needle whose start index i satisfies
 * owned_begin <= i < owned_end (requires length look-ahead past owned_end).
 */
size_t count_occurrences_owned(const char *digits,
                               size_t length,
                               const char *needle,
                               size_t needle_len,
                               size_t owned_begin,
                               size_t owned_end);

/**
 * Load digits from a text file, keeping only characters '0'..'9'.
 * Caller must free(*out_digits).
 *
 * @return Number of digits loaded, or (size_t)-1 on failure.
 */
size_t load_digits(const char *path, char **out_digits);

/**
 * Write a CountReport as CSV to an open stream.
 * Header: pattern,length,count,elapsed_seconds
 */
void write_report_csv(FILE *out, const CountReport *report);

#endif /* SEQUENCE_COUNTER_H */
