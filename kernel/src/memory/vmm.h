#pragma once

#include <limine.h>
#include "paging.h"

// Initializes the Virtual Memory Manager.
// Reverted to use limine_executable_address_response
void vmm_init(limine_memmap_response *memmap, limine_hhdm_response *hhdm, limine_executable_address_response *executable_addr);

// Maps a virtual page to a physical page in the given page map (PML4).
void vmm_map_page(PageTable* pml4, uintptr_t virt, uintptr_t phys, uint64_t flags);

// Unmaps a virtual page.
void vmm_unmap_page(PageTable* pml4, uintptr_t virt);
