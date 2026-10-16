#include "sequence_counter.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *DEFAULT_PATTERNS[] = {
    "1",
    "12",
    "123",
    "1234",
    "12345",
};

static void print_usage(const char *prog)
{
    fprintf(stderr,
            "Usage: %s <digits_file> [--csv <path>] [--patterns p1,p2,...]\n"
            "\n"
            "Count digit-sequence occurrences in a text file of pi digits.\n"
            "Default patterns: 1,12,123,1234,12345\n",
            prog);
}

static int parse_patterns(char *csv, const char ***out_patterns, size_t *out_n)
{
    const char **list;
    size_t n = 0;
    size_t capacity = 8;
    char *cursor = csv;

    list = (const char **)malloc(capacity * sizeof(char *));
    if (list == NULL) {
        return 1;
    }

    while (*cursor != '\0') {
        char *start = cursor;
        char *end;

        while (*cursor != '\0' && *cursor != ',') {
            ++cursor;
        }
        end = cursor;
        if (*cursor == ',') {
            *cursor = '\0';
            ++cursor;
        }

        if (end == start) {
            continue;
        }

        if (n >= capacity) {
            const char **grown;
            capacity *= 2;
            grown = (const char **)realloc(list, capacity * sizeof(char *));
            if (grown == NULL) {
                free(list);
                return 1;
            }
            list = grown;
        }
        list[n++] = start;
    }

    if (n == 0 || n > MAX_PATTERNS) {
        free(list);
        return 1;
    }

    *out_patterns = list;
    *out_n = n;
    return 0;
}

int main(int argc, char **argv)
{
    const char *digits_path = NULL;
    const char *csv_path = NULL;
    char *patterns_arg = NULL;
    const char *const *patterns = DEFAULT_PATTERNS;
    size_t n_patterns = sizeof(DEFAULT_PATTERNS) / sizeof(DEFAULT_PATTERNS[0]);
    const char **owned_patterns = NULL;
    char *digits = NULL;
    size_t n_digits;
    CountReport report;
    FILE *csv_out = NULL;
    size_t i;
    int argi;

    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    for (argi = 1; argi < argc; ++argi) {
        if (strcmp(argv[argi], "--csv") == 0) {
            if (argi + 1 >= argc) {
                print_usage(argv[0]);
                return 1;
            }
            csv_path = argv[++argi];
        } else if (strcmp(argv[argi], "--patterns") == 0) {
            if (argi + 1 >= argc) {
                print_usage(argv[0]);
                return 1;
            }
            patterns_arg = argv[++argi];
        } else if (strcmp(argv[argi], "--help") == 0 ||
                   strcmp(argv[argi], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (argv[argi][0] == '-') {
            fprintf(stderr, "Unknown option: %s\n", argv[argi]);
            print_usage(argv[0]);
            return 1;
        } else if (digits_path == NULL) {
            digits_path = argv[argi];
        } else {
            fprintf(stderr, "Unexpected argument: %s\n", argv[argi]);
            print_usage(argv[0]);
            return 1;
        }
    }

    if (digits_path == NULL) {
        print_usage(argv[0]);
        return 1;
    }

    if (patterns_arg != NULL) {
        if (parse_patterns(patterns_arg, &owned_patterns, &n_patterns) != 0) {
            fprintf(stderr, "Invalid --patterns list\n");
            return 1;
        }
        patterns = (const char *const *)owned_patterns;
    }

    n_digits = load_digits(digits_path, &digits);
    if (n_digits == (size_t)-1 || digits == NULL) {
        fprintf(stderr, "Failed to load digits from %s\n", digits_path);
        free(owned_patterns);
        return 1;
    }

    if (count_patterns(digits, n_digits, patterns, n_patterns, &report) != 0) {
        fprintf(stderr, "Pattern counting failed\n");
        free(digits);
        free(owned_patterns);
        return 1;
    }

    printf("digits_scanned=%zu\n", report.digits_scanned);
    printf("total_elapsed_seconds=%.9f\n", report.total_elapsed_seconds);
    printf("%-10s %8s %12s %16s\n", "pattern", "length", "count", "seconds");
    for (i = 0; i < report.pattern_count; ++i) {
        printf("%-10s %8zu %12zu %16.9f\n",
               report.patterns[i].pattern,
               report.patterns[i].length,
               report.patterns[i].count,
               report.patterns[i].elapsed_seconds);
    }

    if (csv_path != NULL) {
        csv_out = fopen(csv_path, "w");
        if (csv_out == NULL) {
            fprintf(stderr, "Could not write CSV to %s\n", csv_path);
            free(digits);
            free(owned_patterns);
            return 1;
        }
        write_report_csv(csv_out, &report);
        fclose(csv_out);
    }

    free(digits);
    free(owned_patterns);
    return 0;
}
