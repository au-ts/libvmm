#pragma once

#include <microkit.h>

#define VMX_GUEST_RSP 0x0000681C
#define VMX_GUEST_RIP 0x0000681E

#define VMX_GUEST_ES_SELECTOR 0x00000800
#define VMX_GUEST_CS_SELECTOR 0x00000802
#define VMX_GUEST_SS_SELECTOR 0x00000804
#define VMX_GUEST_DS_SELECTOR 0x00000806
#define VMX_GUEST_FS_SELECTOR 0x00000808
#define VMX_GUEST_GS_SELECTOR 0x0000080A

int vcpu_write_vmcs(size_t vcpu_id, seL4_Word field, seL4_Word value);