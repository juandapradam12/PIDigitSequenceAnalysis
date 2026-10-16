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
 * Count non-overlapping and overlapping occurrences of each pattern
 * in a digit string using a sliding-window scan.
 *
 * @param digits     Null-terminated digit string (non-digit chars are skipped
 *                   during load; this function expects digits only).
 * @param length     Number of digits in the buffer.
 * @param patterns   Array of pattern strings to search for.
 * @param n_patterns Number of patterns.
 * @param report     Output report (must be non-NULL).
 * @return 0 on success, non-zero on error.
 */
int count_patterns(const char *digits,
                   size_t length,
                   const char *const *patterns,
                   size_t n_patterns,
                   CountReport *report);

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
