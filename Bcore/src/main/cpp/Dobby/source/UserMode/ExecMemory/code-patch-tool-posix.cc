
#include "dobby_internal.h"
#include "core/arch/Cpu.h"

#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

#if !defined(__APPLE__)
PUBLIC MemoryOperationError CodePatch(void *address, uint8_t *buffer, uint32_t buffer_size) {

  int page_size = (int)sysconf(_SC_PAGESIZE);
  uintptr_t page_align_address = ALIGN_FLOOR(address, page_size);
  int offset = (uintptr_t)address - page_align_address;

#if defined(__ANDROID__) || defined(__linux__)

  // change page permission as rwx
  mprotect((void *)page_align_address, page_size, PROT_READ | PROT_WRITE | PROT_EXEC);

  // patch buffer
  memcpy((void *)((addr_t)page_align_address + offset), buffer, buffer_size);

  // restore page permission
  mprotect((void *)page_align_address, page_size, PROT_READ | PROT_EXEC);
#endif

  addr_t clear_start_ = (addr_t)page_align_address + offset;
  ClearCache((void *)clear_start_, (void *)(clear_start_ + buffer_size));

  return kMemoryOperationSuccess;
}

#endif