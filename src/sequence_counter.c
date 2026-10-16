#define _POSIX_C_SOURCE 200809L

#include "sequence_counter.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _OPENMP
#include <omp.h>
#endif

static double wall_time_seconds(void)
{
#ifdef _OPENMP
    return omp_get_wtime();
#else
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
    }
    return (double)clock() / (double)CLOCKS_PER_SEC;
#endif
}

static size_t count_occurrences(const char *haystack,
                                size_t haystack_len,
                                const char *needle,
                                size_t needle_len)
{
    return count_occurrences_owned(haystack, haystack_len, needle, needle_len, 0,
                                   haystack_len);
}

size_t count_occurrences_owned(const char *digits,
                               size_t length,
                               const char *needle,
                               size_t needle_len,
                               size_t owned_begin,
                               size_t owned_end)
{
    size_t count = 0;
    size_t i;
    size_t begin;
    size_t end;
    size_t last_start;

    if (digits == NULL || needle == NULL || needle_len == 0) {
        return 0;
    }
    if (length < needle_len || owned_begin >= owned_end) {
        return 0;
    }

    /* Starts may read past owned_end; only the start index is owned. */
    last_start = length - needle_len + 1;
    begin = owned_begin;
    end = owned_end;
    if (begin >= last_start) {
        return 0;
    }
    if (end > last_start) {
        end = last_start;
    }
    if (begin >= end) {
        return 0;
    }

    for (i = begin; i < end; ++i) {
        if (memcmp(digits + i, needle, needle_len) == 0) {
            ++count;
        }
    }

    return count;
}

static int fill_pattern_metadata(const char *const *patterns,
                                 size_t n_patterns,
                                 CountReport *report,
                                 size_t *max_len)
{
    size_t p;
    size_t longest = 0;

    for (p = 0; p < n_patterns; ++p) {
        size_t plen;

        if (patterns[p] == NULL) {
            return 1;
        }
        plen = strlen(patterns[p]);
        if (plen == 0 || plen >= MAX_PATTERN_LEN) {
            return 1;
        }
        strncpy(report->patterns[p].pattern, patterns[p], MAX_PATTERN_LEN - 1);
        report->patterns[p].pattern[MAX_PATTERN_LEN - 1] = '\0';
        report->patterns[p].length = plen;
        report->patterns[p].count = 0;
        report->patterns[p].elapsed_seconds = 0.0;
        if (plen > longest) {
            longest = plen;
        }
    }

    if (max_len != NULL) {
        *max_len = longest;
    }
    return 0;
}

int count_patterns(const char *digits,
                   size_t length,
                   const char *const *patterns,
                   size_t n_patterns,
                   CountReport *report)
{
    size_t p;
    clock_t total_start;
    clock_t total_end;

    if (digits == NULL || patterns == NULL || report == NULL) {
        return 1;
    }
    if (n_patterns == 0 || n_patterns > MAX_PATTERNS) {
        return 1;
    }

    memset(report, 0, sizeof(*report));
    report->pattern_count = n_patterns;
    report->digits_scanned = length;

    if (fill_pattern_metadata(patterns, n_patterns, report, NULL) != 0) {
        return 1;
    }

    total_start = clock();

    for (p = 0; p < n_patterns; ++p) {
        clock_t start;
        clock_t end;

        start = clock();
        report->patterns[p].count = count_occurrences(
            digits, length, patterns[p], report->patterns[p].length);
        end = clock();
        report->patterns[p].elapsed_seconds =
            (double)(end - start) / (double)CLOCKS_PER_SEC;
    }

    total_end = clock();
    report->total_elapsed_seconds =
        (double)(total_end - total_start) / (double)CLOCKS_PER_SEC;

    return 0;
}

int count_patterns_partitioned(const char *digits,
                               size_t length,
                               const char *const *patterns,
                               size_t n_patterns,
                               size_t n_partitions,
                               int parallel,
                               CountReport *report)
{
    size_t p;
    size_t max_len = 0;
    double wall_start;
    double wall_end;
    size_t *counts = NULL;

    if (digits == NULL || patterns == NULL || report == NULL) {
        return 1;
    }
    if (n_patterns == 0 || n_patterns > MAX_PATTERNS || n_partitions == 0) {
        return 1;
    }

    memset(report, 0, sizeof(*report));
    report->pattern_count = n_patterns;
    report->digits_scanned = length;

    if (fill_pattern_metadata(patterns, n_patterns, report, &max_len) != 0) {
        return 1;
    }

    counts = (size_t *)calloc(n_patterns, sizeof(size_t));
    if (counts == NULL) {
        return 1;
    }

    wall_start = wall_time_seconds();

#ifdef _OPENMP
    if (parallel) {
#pragma omp parallel
        {
            size_t local[MAX_PATTERNS];
            size_t part;
            size_t q;

            for (q = 0; q < n_patterns; ++q) {
                local[q] = 0;
            }

#pragma omp for schedule(static) nowait
            for (part = 0; part < n_partitions; ++part) {
                size_t owned_begin = (part * length) / n_partitions;
                size_t owned_end = ((part + 1) * length) / n_partitions;

                for (q = 0; q < n_patterns; ++q) {
                    local[q] += count_occurrences_owned(
                        digits,
                        length,
                        patterns[q],
                        report->patterns[q].length,
                        owned_begin,
                        owned_end);
                }
            }

#pragma omp critical
            {
                for (q = 0; q < n_patterns; ++q) {
                    counts[q] += local[q];
                }
            }
        }
    } else
#endif
    {
        size_t part;
        (void)parallel;
        for (part = 0; part < n_partitions; ++part) {
            size_t owned_begin = (part * length) / n_partitions;
            size_t owned_end = ((part + 1) * length) / n_partitions;

            for (p = 0; p < n_patterns; ++p) {
                counts[p] += count_occurrences_owned(
                    digits,
                    length,
                    patterns[p],
                    report->patterns[p].length,
                    owned_begin,
                    owned_end);
            }
        }
    }

    wall_end = wall_time_seconds();

    for (p = 0; p < n_patterns; ++p) {
        report->patterns[p].count = counts[p];
        report->patterns[p].elapsed_seconds = 0.0;
    }
    report->total_elapsed_seconds = wall_end - wall_start;

    free(counts);
    (void)max_len;
    return 0;
}

size_t load_digits(const char *path, char **out_digits)
{
    FILE *fp;
    long file_size;
    char *buffer;
    char *digits;
    size_t n_read;
    size_t i;
    size_t n_digits = 0;

    if (path == NULL || out_digits == NULL) {
        return (size_t)-1;
    }

    *out_digits = NULL;

    fp = fopen(path, "rb");
    if (fp == NULL) {
        return (size_t)-1;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return (size_t)-1;
    }

    file_size = ftell(fp);
    if (file_size < 0) {
        fclose(fp);
        return (size_t)-1;
    }

    rewind(fp);

    buffer = (char *)malloc((size_t)file_size + 1);
    if (buffer == NULL) {
        fclose(fp);
        return (size_t)-1;
    }

    n_read = fread(buffer, 1, (size_t)file_size, fp);
    fclose(fp);
    buffer[n_read] = '\0';

    digits = (char *)malloc(n_read + 1);
    if (digits == NULL) {
        free(buffer);
        return (size_t)-1;
    }

    for (i = 0; i < n_read; ++i) {
        if (isdigit((unsigned char)buffer[i])) {
            digits[n_digits++] = buffer[i];
        }
    }
    digits[n_digits] = '\0';

    free(buffer);
    *out_digits = digits;
    return n_digits;
}

void write_report_csv(FILE *out, const CountReport *report)
{
    size_t i;

    if (out == NULL || report == NULL) {
        return;
    }

    fprintf(out, "pattern,length,count,elapsed_seconds\n");
    for (i = 0; i < report->pattern_count; ++i) {
        fprintf(out,
                "%s,%zu,%zu,%.9f\n",
                report->patterns[i].pattern,
                report->patterns[i].length,
                report->patterns[i].count,
                report->patterns[i].elapsed_seconds);
    }
}
