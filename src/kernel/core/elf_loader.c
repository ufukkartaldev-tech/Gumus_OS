#include "kernel.h"
#include "elf.h"
#include "fs.h"
#include "memory.h"
#include "task.h"
#include "string.h"

int elf_load_and_run(const char* path) {
    uint32_t file_size = 0;
    uint8_t* elf_data = fs_read_bin(path, &file_size);
    
    if (!elf_data) {
        // print("[ELF] Dosya okunamadi\n");
        return -1;
    }

    if (file_size < sizeof(elf_header_t)) {
        kfree(elf_data);
        return -1;
    }

    elf_header_t* header = (elf_header_t*)elf_data;
    
    // Check Magic
    if (*(uint32_t*)header->e_ident != ELF_MAGIC) {
        kfree(elf_data);
        return -1;
    }

    // Create a new process directory
    page_directory_t* proc_dir = create_process_directory();
    
    // Iterate through program headers
    elf_program_header_t* phdr = (elf_program_header_t*)(elf_data + header->e_phoff);
    
    for (int i = 0; i < header->e_phnum; i++, phdr++) {
        if (phdr->p_type == PT_LOAD) {
            // Allocate and map memory
            uint32_t start_vaddr = phdr->p_vaddr;
            uint32_t end_vaddr = phdr->p_vaddr + phdr->p_memsz;
            
            // Map each page in the segment
            for (uint32_t v = start_vaddr & 0xFFFFF000; v < end_vaddr; v += PAGE_SIZE) {
                void* frame = pmm_alloc_frame();
                map_page_in_dir(proc_dir, frame, (void*)v, 0x7); // User, RW, Present
                
                // Temporarily map this frame to kernel identity area to zero it out
                // or use a safe way to write to it.
                // Since GümüşOS has identity mapping for 0-16MB, and PMM usually gives frames there:
                memset(frame, 0, PAGE_SIZE);
            }
            
            // Copy data from ELF buffer to the newly mapped virtual address
            switch_page_directory(proc_dir);
            
            // Filesize kadar veriyi kopyala
            if (phdr->p_filesz > 0) {
                memcpy((void*)phdr->p_vaddr, elf_data + phdr->p_offset, phdr->p_filesz);
            }
            
            // BSS (p_memsz > p_filesz) ise geri kalanı sıfırla
            if (phdr->p_memsz > phdr->p_filesz) {
                memset((void*)(phdr->p_vaddr + phdr->p_filesz), 0, phdr->p_memsz - phdr->p_filesz);
            }
            
            // Switch back to kernel directory
            switch_page_directory(kernel_directory);
        }
    }

    // Create the task
    // We need a modified create_user_process that takes a directory and entry point
    create_elf_task(header->e_entry, proc_dir);

    kfree(elf_data);
    return 0;
}
