#include "sequence_counter.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static size_t count_occurrences(const char *haystack,
                                size_t haystack_len,
                                const char *needle,
                                size_t needle_len)
{
    size_t count = 0;
    size_t i;

    if (needle_len == 0 || haystack_len < needle_len) {
        return 0;
    }

    for (i = 0; i + needle_len <= haystack_len; ++i) {
        if (memcmp(haystack + i, needle, needle_len) == 0) {
            ++count;
        }
    }

    return count;
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

    total_start = clock();

    for (p = 0; p < n_patterns; ++p) {
        clock_t start;
        clock_t end;
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

        start = clock();
        report->patterns[p].count =
            count_occurrences(digits, length, patterns[p], plen);
        end = clock();

        report->patterns[p].elapsed_seconds =
            (double)(end - start) / (double)CLOCKS_PER_SEC;
    }

    total_end = clock();
    report->total_elapsed_seconds =
        (double)(total_end - total_start) / (double)CLOCKS_PER_SEC;

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
