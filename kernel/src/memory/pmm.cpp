#include "pmm.h"
#include "../devices/fb.h"
#include "../lib/sys_utils.h"
#include "../lib/memory/memory.h"

// --- PMM State ---
static uint8_t* pmm_bitmap = nullptr;
static uint64_t total_memory = 0;
static uint64_t total_pages = 0;
static uint64_t bitmap_size = 0;
static uint64_t usable_memory = 0;
static uint64_t system_reserved_memory = 0;
static uint64_t last_allocated_page = 0;
static uint64_t pmm_bitmap_phys_addr = 0;

// --- Private Helper Functions ---

static void set_frame(uint64_t page_index) {
    if (page_index >= total_pages) return;
    uint64_t byte_index = page_index / 8;
    uint8_t bit_index = page_index % 8;
    pmm_bitmap[byte_index] |= (1 << bit_index);
}

static void clear_frame(uint64_t page_index) {
    if (page_index >= total_pages) return;
    uint64_t byte_index = page_index / 8;
    uint8_t bit_index = page_index % 8;
    pmm_bitmap[byte_index] &= ~(1 << bit_index);
}

static bool test_frame(uint64_t page_index) {
    if (page_index >= total_pages) return true;
    uint64_t byte_index = page_index / 8;
    uint8_t bit_index = page_index % 8;
    return (pmm_bitmap[byte_index] & (1 << bit_index)) != 0;
}

// --- Public PMM Functions ---

void pmm_init(limine_memmap_response *memmap, limine_hhdm_response *hhdm) {
    total_memory = 0;
    usable_memory = 0;
    system_reserved_memory = 0;
    uint64_t highest_address = 0;

    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        limine_memmap_entry* entry = memmap->entries[i];
        switch (entry->type) {
            case LIMINE_MEMMAP_USABLE:
                total_memory += entry->length;
                usable_memory += entry->length;
                break;
            case LIMINE_MEMMAP_ACPI_RECLAIMABLE:
            case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
            case LIMINE_MEMMAP_EXECUTABLE_AND_MODULES:
            case LIMINE_MEMMAP_FRAMEBUFFER:
            case LIMINE_MEMMAP_ACPI_NVS:
                total_memory += entry->length;
                system_reserved_memory += entry->length;
                break;
            default:
                break;
        }
        uint64_t top = entry->base + entry->length;
        if (top > highest_address) {
            highest_address = top;
        }
    }

    total_pages = highest_address / PAGE_SIZE;
    bitmap_size = total_pages / 8;
    if (total_pages % 8 != 0) {
        bitmap_size++;
    }

    uint64_t hhdm_offset = hhdm->offset;

    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        limine_memmap_entry* entry = memmap->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE && entry->length >= bitmap_size) {
            pmm_bitmap_phys_addr = entry->base;
            pmm_bitmap = (uint8_t*)(pmm_bitmap_phys_addr + hhdm_offset);
            break;
        }
    }

    if (pmm_bitmap == nullptr) {
        print_string("FATAL: Could not find space for PMM bitmap!\n", 0xFF0000);
        for(;;);
    }
    
    memset(pmm_bitmap, 0xFF, bitmap_size);

    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        limine_memmap_entry* entry = memmap->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE) {
            for (uint64_t j = 0; j < entry->length; j += PAGE_SIZE) {
                clear_frame((entry->base + j) / PAGE_SIZE);
            }
        }
    }
    
    uint64_t bitmap_pages = bitmap_size / PAGE_SIZE;
    if (bitmap_size % PAGE_SIZE != 0) {
        bitmap_pages++;
    }
    for (uint64_t i = 0; i < bitmap_pages; i++) {
        set_frame((pmm_bitmap_phys_addr / PAGE_SIZE) + i);
    }

    print_string("PMM Initialized.\n", 0x00FFFF00);
    char buffer[32];
    
    print_string("Total Memory: ", 0x00FFFFFF);
    u64_to_str(total_memory / 1024 / 1024, buffer);
    print_string(buffer, 0x00FFFFFF);
    print_string(" MB\n", 0x00FFFFFF);

    print_string("Usable Memory: ", 0x00FFFFFF);
    u64_to_str(usable_memory / 1024 / 1024, buffer);
    print_string(buffer, 0x00FFFFFF);
    print_string(" MB\n", 0x00FFFFFF);
    
    print_string("System Reserved: ", 0x00FFFFFF);
    u64_to_str(system_reserved_memory / 1024 / 1024, buffer);
    print_string(buffer, 0x00FFFFFF);
    print_string(" MB\n", 0x00FFFFFF);
}

void* pmm_alloc_frame() {
    for (uint64_t i = last_allocated_page; i < total_pages; i++) {
        if (!test_frame(i)) {
            set_frame(i);
            last_allocated_page = i + 1;
            return (void*)(i * PAGE_SIZE);
        }
    }
    for (uint64_t i = 0; i < last_allocated_page; i++) {
        if (!test_frame(i)) {
            set_frame(i);
            last_allocated_page = i + 1;
            return (void*)(i * PAGE_SIZE);
        }
    }
    return nullptr;
}

void pmm_free_frame(void* physical_address) {
    uint64_t page_index = (uint64_t)physical_address / PAGE_SIZE;
    clear_frame(page_index);
    if (page_index < last_allocated_page) {
        last_allocated_page = page_index;
    }
}

void* pmm_get_bitmap_addr() {
    return (void*)pmm_bitmap_phys_addr;
}

size_t pmm_get_bitmap_size() {
    return bitmap_size;
}
