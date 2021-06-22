#ifndef MemoryAllocator_MemoryArena_h
#define MemoryAllocator_MemoryArena_h

#include "xnucxx/LiteMutableArray.h"

#include "PlatformUnifiedInterface/StdMemory.h"

struct MemoryChunk : MemoryRange {
  inline void init_region_range(addr_t address, int size) {
    this->address = (void *)address;
    this->length = size;
  }

  inline void re_init_region_range(addr_t address, int size) {
    init_region_range(address, size);
  }

  inline void re_init_region_range(MemoryChunk *chunk) {
    init_region_range((addr_t)chunk->address, chunk->length);
  }

  inline addr_t raw_instruction_start() {
    return (addr_t)address;
  };

  inline size_t raw_instruction_size() {
    return length;
  };
};

typedef MemoryChunk AssemblyCodeChunk, WritableDataChunk;

typedef struct {
  MemoryChunk page;
  addr_t page_cursor;
  MemoryPermission permission;
  LiteMutableArray *chunks;
} PageChunk;

class MemoryArena {
public:
  static MemoryChunk *AllocateChunk(int alloc_size, MemoryPermission permission);

  static WritableDataChunk *AllocateDataChunk(int alloc_size);

  static AssemblyCodeChunk *AllocateCodeChunk(int alloc_size);

  static void Destroy(MemoryChunk *chunk);

public:
  static LiteMutableArray *page_chunks;
};

#endif
