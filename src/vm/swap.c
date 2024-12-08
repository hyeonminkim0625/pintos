#include "vm/swap.h"
#include <bitmap.h>
#include "threads/synch.h"
#include "devices/block.h"
#include "threads/vaddr.h"
#include "threads/interrupt.h"
#include <stdio.h>

#define SECTOR_NUM (PGSIZE/BLOCK_SECTOR_SIZE)

static struct bitmap *swap_map;
static struct lock swaplock;
static struct block *swap_block;

void 
swap_init()
{
	lock_init(&swaplock);
	swap_block = block_get_role(BLOCK_SWAP);
	int bitmap_size = block_size(swap_block)/SECTOR_NUM;
	swap_map = bitmap_create(bitmap_size);
}

bool 
swap_in(size_t slot_idx, void *addr)
{
    int i;
	int start_sector = SECTOR_NUM * slot_idx;

    if(!bitmap_test(swap_map, slot_idx))
        return false;
    
    lock_acquire(&swaplock);
	for (i = 0; i < SECTOR_NUM; i++)
	{	
		block_read(swap_block, start_sector + i, addr + i * BLOCK_SECTOR_SIZE);
	}
	bitmap_set(swap_map, slot_idx, false);
    lock_release(&swaplock);

	return true;
}

size_t 
swap_out(void* addr)
{
	int i;

    lock_acquire(&swaplock);
	size_t swap_slot = bitmap_scan_and_flip(swap_map, 0, 1, false);
	if(swap_slot == BITMAP_ERROR)
	{
		lock_release(&swaplock);
		return BITMAP_ERROR;
	}

	int start_sector = SECTOR_NUM * swap_slot;
	for (i = 0; i < SECTOR_NUM ; i++)
	{
		block_write(swap_block, start_sector + i, addr + i * BLOCK_SECTOR_SIZE);
	}
	lock_release(&swaplock);
    return swap_slot;
}