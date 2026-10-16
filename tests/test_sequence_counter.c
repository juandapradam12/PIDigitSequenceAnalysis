#include "sequence_counter.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures = 0;

static void expect_eq_size(const char *name, size_t got, size_t want)
{
    if (got != want) {
        fprintf(stderr, "FAIL %s: got %zu want %zu\n", name, got, want);
        ++failures;
    } else {
        printf("ok   %s\n", name);
    }
}

static void expect_eq_int(const char *name, int got, int want)
{
    if (got != want) {
        fprintf(stderr, "FAIL %s: got %d want %d\n", name, got, want);
        ++failures;
    } else {
        printf("ok   %s\n", name);
    }
}

static void test_overlapping_single_digit(void)
{
    const char *digits = "111";
    const char *patterns[] = {"1"};
    CountReport report;

    expect_eq_int("count_patterns returns 0",
                  count_patterns(digits, 3, patterns, 1, &report),
                  0);
    expect_eq_size("overlapping '1' in 111", report.patterns[0].count, 3);
}

static void test_ascending_patterns(void)
{
    const char *digits = "xx12345yy123zz1";
    const char *patterns[] = {"1", "12", "123", "1234", "12345"};
    CountReport report;
    /* load_digits strips non-digits; here we pass a pre-filtered stream. */
    const char *filtered = "123451231";

    expect_eq_int("ascending patterns ok",
                  count_patterns(filtered, strlen(filtered), patterns, 5, &report),
                  0);
    expect_eq_size("count of 1", report.patterns[0].count, 3);
    expect_eq_size("count of 12", report.patterns[1].count, 2);
    expect_eq_size("count of 123", report.patterns[2].count, 2);
    expect_eq_size("count of 1234", report.patterns[3].count, 1);
    expect_eq_size("count of 12345", report.patterns[4].count, 1);
    (void)digits;
}

static void test_no_match(void)
{
    const char *digits = "99999";
    const char *patterns[] = {"12"};
    CountReport report;

    count_patterns(digits, 5, patterns, 1, &report);
    expect_eq_size("no match for 12", report.patterns[0].count, 0);
}

static void test_load_digits_filters(void)
{
    const char *path = "tests/fixtures/sample_digits.txt";
    char *digits = NULL;
    size_t n;

    n = load_digits(path, &digits);
    expect_eq_size("load_digits length", n, 10);
    if (digits != NULL) {
        expect_eq_int("load_digits content", strcmp(digits, "3141592653"), 0);
        free(digits);
    } else {
        fprintf(stderr, "FAIL load_digits returned NULL\n");
        ++failures;
    }
}

static void test_null_args(void)
{
    CountReport report;
    const char *patterns[] = {"1"};

    expect_eq_int("null digits rejected",
                  count_patterns(NULL, 0, patterns, 1, &report),
                  1);
    expect_eq_int("null report rejected",
                  count_patterns("1", 1, patterns, 1, NULL),
                  1);
}

static void test_owned_range_stitch(void)
{
    /* "12" starts at index 1 and spans the midpoint between halves [0,2)+[2,5). */
    const char *digits = "31245";
    const char *patterns[] = {"12"};
    CountReport full;
    CountReport parts;

    count_patterns(digits, 5, patterns, 1, &full);
    expect_eq_size("full count of 12", full.patterns[0].count, 1);

    /* Ownership of the start index + full-buffer look-ahead recovers the hit. */
    expect_eq_size("owned left captures seam start",
                   count_occurrences_owned(digits, 5, "12", 2, 0, 2),
                   1);
    expect_eq_size("owned right does not double-count",
                   count_occurrences_owned(digits, 5, "12", 2, 2, 5),
                   0);

    expect_eq_int("partitioned stitched equals full",
                  count_patterns_partitioned(digits, 5, patterns, 1, 2, 0, &parts),
                  0);
    expect_eq_size("stitched partition count", parts.patterns[0].count, 1);
}

static void test_partitioned_matches_full(void)
{
    const char *digits = "1234512312345";
    const char *patterns[] = {"1", "12", "123", "1234", "12345"};
    CountReport full;
    CountReport parts;
    size_t i;

    count_patterns(digits, strlen(digits), patterns, 5, &full);
    count_patterns_partitioned(digits, strlen(digits), patterns, 5, 4, 0, &parts);

    for (i = 0; i < 5; ++i) {
        char name[64];
        snprintf(name, sizeof(name), "partitioned equals full [%zu]", i);
        expect_eq_size(name, parts.patterns[i].count, full.patterns[i].count);
    }
}

int main(void)
{
    test_overlapping_single_digit();
    test_ascending_patterns();
    test_no_match();
    test_load_digits_filters();
    test_null_args();
    test_owned_range_stitch();
    test_partitioned_matches_full();

    if (failures > 0) {
        fprintf(stderr, "%d test(s) failed\n", failures);
        return 1;
    }
    printf("all tests passed\n");
    return 0;
}
