#include "vmm.h"
#include "pmm.h"
#include "paging.h"
#include "../devices/fb.h"
#include "../lib/memory/memory.h"

// Global pointer to the kernel's PML4 table.
static PageTable* kernel_pml4 = nullptr;

// Function to load a new PML4 into the CR3 register.
static void load_pml4(PageTable* pml4) {
    extern volatile struct limine_hhdm_request hhdm_request;
    uint64_t pml4_phys = (uint64_t)pml4 - hhdm_request.response->offset;
    asm volatile ("mov %0, %%cr3" :: "r"(pml4_phys));
}

// Maps a single virtual page to a physical page.
void vmm_map_page(PageTable* pml4, uintptr_t virt, uintptr_t phys, uint64_t flags) {
    uint64_t pml4_index = (virt >> 39) & 0x1FF;
    uint64_t pdpt_index = (virt >> 30) & 0x1FF;
    uint64_t pd_index = (virt >> 21) & 0x1FF;
    uint64_t pt_index = (virt >> 12) & 0x1FF;

    extern volatile struct limine_hhdm_request hhdm_request;
    uint64_t hhdm_offset = hhdm_request.response->offset;

    PageTableEntry* pml4e = &pml4->entries[pml4_index];
    PageTable* pdpt;
    if (!(pml4e->value & PTE_PRESENT)) {
        pdpt = (PageTable*)pmm_alloc_frame();
        if (!pdpt) return;
        memset((void*)((uint64_t)pdpt + hhdm_offset), 0, PAGE_SIZE);
        pml4e->set_address((uint64_t)pdpt);
        pml4e->set_flags(PTE_PRESENT | PTE_WRITABLE | PTE_USER);
    } else {
        pdpt = (PageTable*)(pml4e->get_address() + hhdm_offset);
    }

    PageTableEntry* pdpte = &pdpt->entries[pdpt_index];
    PageTable* pd;
    if (!(pdpte->value & PTE_PRESENT)) {
        pd = (PageTable*)pmm_alloc_frame();
        if (!pd) return;
        memset((void*)((uint64_t)pd + hhdm_offset), 0, PAGE_SIZE);
        pdpte->set_address((uint64_t)pd);
        pdpte->set_flags(PTE_PRESENT | PTE_WRITABLE | PTE_USER);
    } else {
        pd = (PageTable*)(pdpte->get_address() + hhdm_offset);
    }

    PageTableEntry* pde = &pd->entries[pd_index];
    PageTable* pt;
    if (!(pde->value & PTE_PRESENT)) {
        pt = (PageTable*)pmm_alloc_frame();
        if (!pt) return;
        memset((void*)((uint64_t)pt + hhdm_offset), 0, PAGE_SIZE);
        pde->set_address((uint64_t)pt);
        pde->set_flags(PTE_PRESENT | PTE_WRITABLE | PTE_USER);
    } else {
        pt = (PageTable*)(pde->get_address() + hhdm_offset);
    }

    PageTableEntry* pte = &pt->entries[pt_index];
    pte->set_address(phys);
    pte->set_flags(flags);
}

// Initializes the Virtual Memory Manager.
void vmm_init(limine_memmap_response *memmap, limine_hhdm_response *hhdm, limine_executable_address_response *executable_addr) {
    print_string("VMM Initializing...\n", 0x00FFFF00);

    // 1. Create a new, clean page map for the kernel.
    kernel_pml4 = (PageTable*)((uint64_t)pmm_alloc_frame() + hhdm->offset);
    memset(kernel_pml4, 0, PAGE_SIZE);

    // 2. Map all of physical RAM into the new address space.
    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        limine_memmap_entry* entry = memmap->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE || entry->type == LIMINE_MEMMAP_ACPI_RECLAIMABLE || entry->type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE || entry->type == LIMINE_MEMMAP_EXECUTABLE_AND_MODULES || entry->type == LIMINE_MEMMAP_FRAMEBUFFER || entry->type == LIMINE_MEMMAP_ACPI_NVS) {
            for (uint64_t j = 0; j < entry->length; j += PAGE_SIZE) {
                uintptr_t phys_addr = entry->base + j;
                uintptr_t virt_addr = phys_addr + hhdm->offset;
                vmm_map_page(kernel_pml4, virt_addr, phys_addr, PTE_PRESENT | PTE_WRITABLE);
            }
        }
    }

    // 3. Map the kernel's executable sections.
    for (uint64_t i = 0; i < 16 * 1024 * 1024; i += PAGE_SIZE) {
        uintptr_t phys_addr = executable_addr->physical_base + i;
        uintptr_t virt_addr = executable_addr->virtual_base + i;
        vmm_map_page(kernel_pml4, virt_addr, phys_addr, PTE_PRESENT | PTE_WRITABLE);
    }

    // 4. Load the new page map.
    load_pml4(kernel_pml4);
    print_string("New page map loaded.\n", 0x00FFFF00);
}
