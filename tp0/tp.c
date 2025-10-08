/* GPLv2 (c) Airbus */
#include <debug.h>
#include <info.h>

extern info_t   *info;
extern uint32_t __kernel_start__;
extern uint32_t __kernel_end__;

void q2(multiboot_memory_map_t* entry) {
      /* print start-end and type */
      uint64_t start = entry->addr;
      uint64_t len   = entry->len;
      uint64_t end   = (len > 0) ? (start + len - 1) : start;

      const char *type_str = "MULTIBOOT_MEMORY_UNKNOWN";
      switch(entry->type) {
         case MULTIBOOT_MEMORY_AVAILABLE:
            type_str = "MULTIBOOT_MEMORY_AVAILABLE";
            break;
         case MULTIBOOT_MEMORY_RESERVED:
            type_str = "MULTIBOOT_MEMORY_RESERVED";
            break;
      }

      debug("[0x%llx - 0x%llx] %s\n", start, end, type_str);
}

void q3 () {
   int *ptr_in_available_mem;
   ptr_in_available_mem = (int*)0x0;
   debug("Available mem (0x0): before: 0x%x ", *ptr_in_available_mem); // read
   *ptr_in_available_mem = 0xaaaaaaaa;                           // write
   debug("after: 0x%x\n", *ptr_in_available_mem);                // check

   int *ptr_in_reserved_mem;
   ptr_in_reserved_mem = (int*)0xf0000;
   debug("Reserved mem (at: 0xf0000):  before: 0x%x ", *ptr_in_reserved_mem); // read
   *ptr_in_reserved_mem = 0xaaaaaaaa;                           // write
   debug("after: 0x%x\n", *ptr_in_reserved_mem);                // check
}

void q4() {
   // Read and write outside the kernel memory
   int *ptr_in_reserved_mem;
   ptr_in_reserved_mem = (int*)0xffffff;
   debug("Reserved mem (at: 0xffffff):  before: 0x%x ", *ptr_in_reserved_mem); // read
}

void tp() {
   debug("kernel mem [0x%p - 0x%p]\n", &__kernel_start__, &__kernel_end__);
   debug("MBI flags 0x%x\n", info->mbi->flags);

   multiboot_memory_map_t* entry = (multiboot_memory_map_t*)info->mbi->mmap_addr;
   while((uint32_t)entry < (info->mbi->mmap_addr + info->mbi->mmap_length)) {
      printf("----\n\n");
      q2(entry);
      printf("----\n");
      q3();
      printf("----\n");
      /* advance: size field excludes the size field itself */
      entry = (multiboot_memory_map_t*)(((uint32_t)entry) + entry->size + sizeof(entry->size));
      //entry++;
   }
   printf("--------------\n");

   q4();


}
