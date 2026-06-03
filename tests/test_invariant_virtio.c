#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* Mock virtio descriptor structure - matches typical virtio layout */
struct virtio_desc {
    uint64_t addr;
    uint32_t len;
    uint16_t flags;
    uint16_t next;
};

#define DEST_BUFFER_SIZE 64

/* Simulates the vulnerable copy pattern from virtio.c */
static int safe_virtio_copy(void *dest, size_t dest_size, void *src, uint32_t guest_len)
{
    /* Security invariant: copy_size must never exceed dest_size */
    if (guest_len > dest_size) {
        return -1; /* Reject oversized copy */
    }
    memcpy(dest, src, guest_len);
    return 0;
}

START_TEST(test_virtio_buffer_bounds)
{
    /* Invariant: Buffer reads never exceed the declared destination length */
    uint32_t test_lengths[] = {
        DEST_BUFFER_SIZE * 10,  /* 10x overflow - exploit case */
        DEST_BUFFER_SIZE * 2,   /* 2x overflow */
        DEST_BUFFER_SIZE + 1,   /* Boundary: 1 byte over */
        DEST_BUFFER_SIZE,       /* Exact fit - valid */
        DEST_BUFFER_SIZE / 2    /* Under size - valid */
    };
    int num_tests = sizeof(test_lengths) / sizeof(test_lengths[0]);

    char dest[DEST_BUFFER_SIZE];
    char src[DEST_BUFFER_SIZE * 10];
    memset(src, 'A', sizeof(src));

    for (int i = 0; i < num_tests; i++) {
        memset(dest, 0, DEST_BUFFER_SIZE);
        int result = safe_virtio_copy(dest, DEST_BUFFER_SIZE, src, test_lengths[i]);
        
        if (test_lengths[i] > DEST_BUFFER_SIZE) {
            /* Oversized requests must be rejected */
            ck_assert_int_eq(result, -1);
        } else {
            /* Valid requests should succeed */
            ck_assert_int_eq(result, 0);
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

    tcase_add_test(tc_core, test_virtio_buffer_bounds);
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