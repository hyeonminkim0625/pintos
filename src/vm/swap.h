#ifndef VM_SWAP_H
#define VM_SWAP_H

#include <stddef.h>
#include <stdbool.h>

void swap_init();
bool swap_in(size_t slot_idx, void *addr);
size_t swap_out(void* addr);

#endif