#pragma once

#include <limine.h>
#include <cstddef>

// Defines the size of a single physical memory page frame.
constexpr size_t PAGE_SIZE = 4096;

// Initializes the Physical Memory Manager.
void pmm_init(limine_memmap_response *memmap, limine_hhdm_response *hhdm);

// Allocates a single physical memory frame (4KiB).
void* pmm_alloc_frame();

// Frees a previously allocated physical memory frame.
void pmm_free_frame(void* physical_address);

// Gets the physical address of the PMM's internal bitmap.
void* pmm_get_bitmap_addr();

// Gets the size (in bytes) of the PMM's internal bitmap.
size_t pmm_get_bitmap_size();
