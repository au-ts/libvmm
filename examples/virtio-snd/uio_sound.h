#pragma once
#define UIO_SND_FAULT_ADDRESS 0x301000
#include <sddf/sound/sound.h>

typedef struct vm_shared_state {
    sound_shared_state_t sound;
    uintptr_t data_paddr;
} vm_shared_state_t;
