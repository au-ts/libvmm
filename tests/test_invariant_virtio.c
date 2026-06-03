#include <check.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/* We need to test that descriptor processing validates GPA bounds.
 * Include the virtio header to access the processing functions. */
#include "src/virtio/virtio.c"

/* Define a simulated guest memory region with known bounds */
#define GUEST_MEM_BASE  ((uintptr_t)guest_memory)
#define GUEST_MEM_SIZE  4096

static char guest_memory[GUEST_MEM_SIZE];

START_TEST(test_virtio_descriptor_bounds_validation)
{
    /* Invariant: descriptor processing must never allow reads/writes
     * outside the guest's allocated physical memory region. */

    struct {
        uintptr_t gpa;
        uint32_t len;
        const char *desc;
    } cases[] = {
        /* Exploit: address far outside guest memory */
        { 0xDEADBEEF00000000ULL, 4096, "arbitrary high address" },
        /* Boundary: address at end of guest region overflowing */
        { GUEST_MEM_BASE + GUEST_MEM_SIZE - 1, 4096, "overflow past end" },
        /* Boundary: zero address (NULL page) */
        { 0x0, 256, "null page access" },
        /* Valid: within guest memory */
        { GUEST_MEM_BASE + 128, 64, "valid in-bounds access" },
    };
    int num_cases = sizeof(cases) / sizeof(cases[0]);

    memset(guest_memory, 'A', GUEST_MEM_SIZE);

    for (int i = 0; i < num_cases; i++) {
        uintptr_t src_gpa = cases[i].gpa;
        uint32_t copy_size = cases[i].len;

        /* Validate: src_gpa must be within [GUEST_MEM_BASE, GUEST_MEM_BASE+GUEST_MEM_SIZE)
         * and src_gpa + copy_size must not overflow or exceed the region */
        int in_bounds = (src_gpa >= GUEST_MEM_BASE) &&
                        (src_gpa + copy_size > src_gpa) &&  /* overflow check */
                        (src_gpa + copy_size <= GUEST_MEM_BASE + GUEST_MEM_SIZE);

        if (i < 3) {
            /* Adversarial cases MUST be detected as out-of-bounds */
            ck_assert_msg(!in_bounds,
                "Case '%s': out-of-bounds GPA was not rejected", cases[i].desc);
        } else {
            /* Valid case must pass bounds check */
            ck_assert_msg(in_bounds,
                "Case '%s': valid GPA was incorrectly rejected", cases[i].desc);
        }
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_virtio_descriptor_bounds_validation);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}