
#ifndef MemoryAllocator_NearMemoryArena_h
#define MemoryAllocator_NearMemoryArena_h

#include "MemoryAllocator/MemoryArena.h"

class NearMemoryArena : public MemoryArena {
public:
  NearMemoryArena();

  static MemoryChunk *AllocateChunk(addr_t position, size_t alloc_range, int alloc_size, MemoryPermission permission);

  static WritableDataChunk *AllocateDataChunk(addr_t position, size_t alloc_range, int alloc_size);

  static AssemblyCodeChunk *AllocateCodeChunk(addr_t position, size_t alloc_range, int alloc_size);

  static int PushPage(addr_t page_addr, MemoryPermission permission);

  static void Destroy(MemoryChunk *chunk);

private:
  static LiteMutableArray *page_chunks;
};

#endif
