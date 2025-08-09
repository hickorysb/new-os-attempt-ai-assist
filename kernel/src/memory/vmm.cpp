#include "vmm.h"
#include "pmm.h"
#include "paging.h"
#include "../devices/fb.h"
#include "../lib/memory/memory.h"

// Global pointer to the kernel's PML4 table (virtual address).
static PageTable* kernel_pml4_virt = nullptr;

// This function now takes the PHYSICAL address of the PML4 table.
static void load_pml4(uint64_t pml4_phys) {
    asm volatile ("mov %0, %%cr3" :: "r"(pml4_phys));
}

// Maps a single virtual page to a physical page.
void vmm_map_page(PageTable* pml4_virt, uintptr_t virt, uintptr_t phys, uint64_t flags) {
    uint64_t pml4_index = (virt >> 39) & 0x1FF;
    uint64_t pdpt_index = (virt >> 30) & 0x1FF;
    uint64_t pd_index = (virt >> 21) & 0x1FF;
    uint64_t pt_index = (virt >> 12) & 0x1FF;

    extern volatile struct limine_hhdm_request hhdm_request;
    uint64_t hhdm_offset = hhdm_request.response->offset;

    // Get a virtual pointer to the PML4 entry.
    PageTableEntry* pml4e = &pml4_virt->entries[pml4_index];
    PageTable* pdpt_virt;
    if (!(pml4e->value & PTE_PRESENT)) {
        uint64_t pdpt_phys = (uint64_t)pmm_alloc_frame();
        if (!pdpt_phys) return;
        pdpt_virt = (PageTable*)(pdpt_phys + hhdm_offset);
        memset(pdpt_virt, 0, PAGE_SIZE);
        pml4e->set_address(pdpt_phys);
        pml4e->set_flags(PTE_PRESENT | PTE_WRITABLE);
    } else {
        pdpt_virt = (PageTable*)(pml4e->get_address() + hhdm_offset);
    }

    PageTableEntry* pdpte = &pdpt_virt->entries[pdpt_index];
    PageTable* pd_virt;
    if (!(pdpte->value & PTE_PRESENT)) {
        uint64_t pd_phys = (uint64_t)pmm_alloc_frame();
        if (!pd_phys) return;
        pd_virt = (PageTable*)(pd_phys + hhdm_offset);
        memset(pd_virt, 0, PAGE_SIZE);
        pdpte->set_address(pd_phys);
        pdpte->set_flags(PTE_PRESENT | PTE_WRITABLE);
    } else {
        pd_virt = (PageTable*)(pdpte->get_address() + hhdm_offset);
    }

    PageTableEntry* pde = &pd_virt->entries[pd_index];
    PageTable* pt_virt;
    if (!(pde->value & PTE_PRESENT)) {
        uint64_t pt_phys = (uint64_t)pmm_alloc_frame();
        if (!pt_phys) return;
        pt_virt = (PageTable*)(pt_phys + hhdm_offset);
        memset(pt_virt, 0, PAGE_SIZE);
        pde->set_address(pt_phys);
        pde->set_flags(PTE_PRESENT | PTE_WRITABLE);
    } else {
        pt_virt = (PageTable*)(pde->get_address() + hhdm_offset);
    }

    PageTableEntry* pte = &pt_virt->entries[pt_index];
    pte->set_address(phys);
    pte->set_flags(flags);
}

// Initializes the Virtual Memory Manager.
void vmm_init(limine_memmap_response *memmap, limine_hhdm_response *hhdm, limine_executable_address_response *executable_addr) {
    fb_print_string("VMM Initializing...\n", 0x00FFFF00);

    // 1. Allocate a physical frame for the new PML4 table.
    uint64_t kernel_pml4_phys = (uint64_t)pmm_alloc_frame();
    if (!kernel_pml4_phys) {
        fb_print_string("VMM FATAL: Failed to allocate PML4!\n", 0xFF0000);
        for(;;);
    }
    
    // Get its virtual address via the HHDM so we can write to it.
    kernel_pml4_virt = (PageTable*)(kernel_pml4_phys + hhdm->offset);
    memset(kernel_pml4_virt, 0, PAGE_SIZE);

    // 2. Map all of physical memory into the higher half of the new address space.
    // This creates the HHDM and ensures the stack, framebuffer, etc., remain accessible.
    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        limine_memmap_entry* entry = memmap->entries[i];
        // This logic is corrected to only map valid, existing memory regions.
        if (entry->type == LIMINE_MEMMAP_USABLE || 
            entry->type == LIMINE_MEMMAP_ACPI_RECLAIMABLE || 
            entry->type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE || 
            entry->type == LIMINE_MEMMAP_EXECUTABLE_AND_MODULES ||
            entry->type == LIMINE_MEMMAP_FRAMEBUFFER || 
            entry->type == LIMINE_MEMMAP_ACPI_NVS) {
            
            for (uint64_t j = 0; j < entry->length; j += PAGE_SIZE) {
                uintptr_t phys_addr = entry->base + j;
                uintptr_t virt_addr = phys_addr + hhdm->offset;
                vmm_map_page(kernel_pml4_virt, virt_addr, phys_addr, PTE_PRESENT | PTE_WRITABLE);
            }
        }
    }

    // 3. Map the kernel's executable sections to their final, linked virtual addresses.
    uintptr_t phys_base = executable_addr->physical_base;
    uintptr_t virt_base = executable_addr->virtual_base;

    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        limine_memmap_entry* entry = memmap->entries[i];
        if (entry->type == LIMINE_MEMMAP_EXECUTABLE_AND_MODULES) {
            for (uint64_t j = 0; j < entry->length; j += PAGE_SIZE) {
                uintptr_t phys_addr = entry->base + j;
                uintptr_t virt_addr = phys_addr - phys_base + virt_base;
                vmm_map_page(kernel_pml4_virt, virt_addr, phys_addr, PTE_PRESENT | PTE_WRITABLE);
            }
        }
    }

    // 4. Load the PHYSICAL address of the new page map into CR3.
    load_pml4(kernel_pml4_phys);
    fb_print_string("New page map loaded.\n", 0x00FFFF00);
}
