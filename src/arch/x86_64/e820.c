#include <libvmm/arch/x86_64/e820.h>

// void e820_map_init_linux() {

// }

// void e820_map_init_firmware(uintptr_t ram_start_gpa) {
//     uint8_t *e820_entries = (uint8_t *)(ram_start_gpa + ZERO_PAGE_GPA + ZERO_PAGE_E820_ENTRIES_OFFSET);
//     *e820_entries = 3;
//     struct boot_e820_entry *e820_table = (struct boot_e820_entry *)(ram_start_gpa + ZERO_PAGE_GPA
//                                                                     + ZERO_PAGE_E820_TABLE_OFFSET);
//     assert(*e820_entries <= E820_MAX_ENTRIES_ZEROPAGE);
//     e820_table[0] = (struct boot_e820_entry) {
//         .addr = pt_objs_start_gpa,
//         .size = pt_objs_end_gpa - pt_objs_start_gpa,
//         .type = E820_RESERVED,
//     };
//     e820_table[1] = (struct boot_e820_entry) {
//         .addr = acpi_start_gpa,
//         .size = acpi_end_gpa - acpi_start_gpa,
//         .type = E820_ACPI,
//     };
//     e820_table[2] = (struct boot_e820_entry) {
//         .addr = 0,
//         .size = acpi_start_gpa,
//         .type = E820_RAM,
//     };
//     // // PCI ECAM
//     // e820_table[3] = (struct boot_e820_entry) {
//     //     .addr = 0xe0000000,
//     //     .size = 0x10000000,
//     //     .type = E820_RESERVED,
//     // };
// }
