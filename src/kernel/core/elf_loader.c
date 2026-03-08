#include "kernel.h"
#include "elf.h"
#include "vfs.h"
#include "memory.h"
#include "task.h"
#include "string.h"

int elf_load_and_run(const char* path) {
    // VFS üzerinden dosya aç
    int fd = vfs_open(path, 0);
    if (fd < 0) {
        printf("[ELF] Dosya açılamadı: %s\n", path);
        return -1;
    }
    
    // Dosya boyutunu al
    vfs_node_t* node = NULL;
    // VFS'ten dosya boyutunu öğrenmek için node'u al
    // Şimdilik basit implementasyon
    
    // Dosyayı oku
    uint8_t* elf_data = kmalloc(4096); // Başlangıç için 4KB
    if (!elf_data) {
        vfs_close(fd);
        return -1;
    }
    
    int bytes_read = vfs_read(fd, elf_data, 4096);
    if (bytes_read <= 0) {
        printf("[ELF] Dosya okunamadı\n");
        kfree(elf_data);
        vfs_close(fd);
        return -1;
    }
    
    uint32_t file_size = bytes_read;

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
    uint32_t max_vaddr = 0;
    
    for (int i = 0; i < header->e_phnum; i++, phdr++) {
        if (phdr->p_type == PT_LOAD) {
            // Allocate and map memory
            uint32_t start_vaddr = phdr->p_vaddr;
            uint32_t end_vaddr = phdr->p_vaddr + phdr->p_memsz;
            
            if (end_vaddr > max_vaddr) max_vaddr = end_vaddr;
            
            // Map each page in the segment
            for (uint32_t v = start_vaddr & 0xFFFFF000; v < end_vaddr; v += PAGE_SIZE) {
                void* frame = pmm_alloc_frame();
                map_page_in_dir(proc_dir, frame, (void*)v, 0x7); // User, RW, Present
                
                // Temporarily map this frame to kernel identity area to zero it out
                // or use a safe way to write to it.
                // Since GÃ¼mÃ¼ÅŸOS has identity mapping for 0-16MB, and PMM usually gives frames there:
                memset(frame, 0, PAGE_SIZE);
            }
            
            // Copy data from ELF buffer to the newly mapped virtual address
            switch_page_directory(proc_dir);
            
            // Filesize kadar veriyi kopyala
            if (phdr->p_filesz > 0) {
                memcpy((void*)phdr->p_vaddr, elf_data + phdr->p_offset, phdr->p_filesz);
            }
            
            // BSS (p_memsz > p_filesz) ise geri kalanÄ± sÄ±fÄ±rla
            if (phdr->p_memsz > phdr->p_filesz) {
                memset((void*)(phdr->p_vaddr + phdr->p_filesz), 0, phdr->p_memsz - phdr->p_filesz);
            }
            
            // Switch back to kernel directory
            switch_page_directory(kernel_directory);
        }
    }

    // Create the task
    create_elf_task(header->e_entry, proc_dir, max_vaddr);

    kfree(elf_data);
    vfs_close(fd);
    return 0;
}
