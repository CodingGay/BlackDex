#include "./NearMemoryArena.h"

#include "dobby_internal.h"

#include "UserMode/PlatformUtil/ProcessRuntimeUtility.h"

#include <iostream>
#include <vector>

using namespace zz;

#define KB (1024uLL)
#define MB (1024uLL * KB)
#define GB (1024uLL * MB)

LiteMutableArray *NearMemoryArena::page_chunks;

#if defined(WIN32)
static const void* memmem(const void* haystack, size_t haystacklen, const void* needle, size_t needlelen)
{
  if (!haystack || !needle) {
    return haystack;
  } else {
    const char* h = (const char*)haystack;
    const char* n = (const char*)needle;
    size_t l = needlelen;
    const char* r = h;
    while (l && (l <= haystacklen)) {
      if (*n++ != *h++) {
        r = h;
        n = (const char*)needle;
        l = needlelen;
      } else {
        --l;
      }
      --haystacklen;
    }
    return l ? NULL : r;
  }
}
#endif

#if 1
static addr_t search_near_blank_page(addr_t pos, size_t alloc_range) {
  addr_t min_page_addr, max_page_addr;
  min_page_addr = ALIGN((pos - alloc_range), OSMemory::PageSize()) + OSMemory::PageSize();
  max_page_addr = ALIGN((pos + alloc_range), OSMemory::PageSize()) - OSMemory::PageSize();

  // region.start sorted
  std::vector<MemoryRegion> process_memory_layout = ProcessRuntimeUtility::GetProcessMemoryLayout();

  /*
   * min_page_addr/--special-blank--/==region==/--right-blank--/max_page_addr
   */

  addr_t resultPageAddr = 0, assumePageAddr = min_page_addr;

  // check first region
  addr_t first_region_start = (addr_t)process_memory_layout[0].address;
  if (min_page_addr < first_region_start) {
    resultPageAddr = first_region_start - OSMemory::PageSize();
    resultPageAddr =
        (addr_t)OSMemory::Allocate((void *)assumePageAddr, OSMemory::PageSize(), MemoryPermission::kReadExecute);
    if (resultPageAddr)
      return resultPageAddr;
  }

  // check last region
  MemoryRegion last_region = process_memory_layout[process_memory_layout.size() - 1];
  addr_t last_region_end = (addr_t)last_region.address + last_region.length;
  if (max_page_addr < last_region_end) {
    resultPageAddr = last_region_end + OSMemory::PageSize();
    resultPageAddr =
        (addr_t)OSMemory::Allocate((void *)assumePageAddr, OSMemory::PageSize(), MemoryPermission::kReadExecute);
    if (resultPageAddr)
      return resultPageAddr;
  }

  for (int i = 0; i < process_memory_layout.size(); ++i) {
    MemoryRegion region = process_memory_layout[i];
    // check if assume-page-addr in memory-layout
    addr_t region_end = (addr_t)region.address + region.length;
    addr_t region_start = (addr_t)region.address;

    if (region_end < max_page_addr) {
      if (region_start >= min_page_addr) {

        // find the region locate in the [min_page_addr, max_page_addr]
        if (assumePageAddr == min_page_addr) {
          MemoryRegion prev_region;
          prev_region = process_memory_layout[i - 1];
          addr_t prev_region_end = (addr_t)prev_region.address + prev_region.length;
          // check if have blank cave page
          if (region_start > prev_region_end) {
            assumePageAddr = min_page_addr > prev_region_end ? min_page_addr : prev_region_end;
            resultPageAddr = (addr_t)OSMemory::Allocate((void *)assumePageAddr, OSMemory::PageSize(),
                                                        MemoryPermission::kReadExecute);
            if (resultPageAddr)
              break;
          }
        }

        // right-blank
        MemoryRegion next_region = process_memory_layout[i + 1];
        // check if have blank cave page
        if (region_end < (addr_t)next_region.address) {
          assumePageAddr = (addr_t)region.address + region.length;
          resultPageAddr =
              (addr_t)OSMemory::Allocate((void *)assumePageAddr, OSMemory::PageSize(), MemoryPermission::kReadExecute);
          if (resultPageAddr)
            break;
        }
      }
    }
  }
  return resultPageAddr;
}

NearMemoryArena::NearMemoryArena() {
}

static addr_t search_near_blank_memory_chunk(addr_t pos, size_t alloc_range, int alloc_size) {
  addr_t min_page_addr, max_page_addr;
  min_page_addr = ALIGN((pos - alloc_range), OSMemory::PageSize()) + OSMemory::PageSize();
  max_page_addr = ALIGN((pos + alloc_range), OSMemory::PageSize()) - OSMemory::PageSize();

  std::vector<MemoryRegion> process_memory_layout = ProcessRuntimeUtility::GetProcessMemoryLayout();

  uint8_t *blank_chunk_addr = NULL;
  for (auto region : process_memory_layout) {
    // check if assume-page-addr in memory-layout
    if (region.permission == kReadExecute || region.permission == kRead) {
      if (((addr_t)region.address + region.length) <= max_page_addr) {
        if ((addr_t)region.address >= min_page_addr) {
#if defined(__APPLE__)
          if (*(uint32_t *)region.address == 0xfeedfacf)
            continue;
#endif
          char *blank_memory = (char *)malloc(alloc_size);
          memset(blank_memory, 0, alloc_size);
#if defined(__arm__) || defined(__aarch64__)
          alloc_size += (4 - 1);
          blank_chunk_addr = (uint8_t *)memmem(region.address, region.length, blank_memory, alloc_size);
          if (blank_chunk_addr) {
            int off = 4 - ((addr_t)blank_chunk_addr % 4);
            blank_chunk_addr += off;
          }
#else
          blank_chunk_addr = (uint8_t *)memmem(region.address, region.length, blank_memory, alloc_size);
#endif

          if (blank_chunk_addr)
            break;
        }
      }
    }
  }
  return (addr_t)blank_chunk_addr;
}
#endif

int NearMemoryArena::PushPage(addr_t page_addr, MemoryPermission permission) {
  PageChunk *newPage = new PageChunk;
  newPage->page.address = (void *)page_addr;
  newPage->page.length = OSMemory::PageSize();
  newPage->page_cursor = page_addr;
  newPage->permission = permission;
  newPage->chunks = new LiteMutableArray(8);
  NearMemoryArena::page_chunks->pushObject(reinterpret_cast<LiteObject *>(newPage));
  return RT_SUCCESS;
}

WritableDataChunk *NearMemoryArena::AllocateDataChunk(addr_t position, size_t alloc_range, int alloc_size) {
  return NearMemoryArena::AllocateChunk(position, alloc_range, alloc_size, kReadWrite);
}

AssemblyCodeChunk *NearMemoryArena::AllocateCodeChunk(addr_t position, size_t alloc_range, int alloc_size) {
  return NearMemoryArena::AllocateChunk(position, alloc_range, alloc_size, kReadExecute);
}

MemoryChunk *NearMemoryArena::AllocateChunk(addr_t position, size_t alloc_range, int alloc_size,
                                            MemoryPermission permission) {

  if (page_chunks == NULL) {
    page_chunks = new LiteMutableArray(8);
  }
  MemoryChunk *result = NULL;

search_once_more:
  LiteCollectionIterator iter(NearMemoryArena::page_chunks);
  PageChunk *page = NULL;
  while ((page = reinterpret_cast<PageChunk *>(iter.getNextObject())) != NULL) {
    if (page->permission == permission) {
      if (llabs((intptr_t)(page->page_cursor - position)) < alloc_range) {
        if ((page->page_cursor + alloc_size) < ((addr_t)page->page.address + page->page.length)) {
          break;
        }
      }
    }
  }

  MemoryChunk *chunk = NULL;
  if (page) {
    chunk = new MemoryChunk;
    chunk->address = (void *)page->page_cursor;
    chunk->length = alloc_size;

    // update page cursor
    page->chunks->pushObject(reinterpret_cast<LiteObject *>(chunk));
    page->page_cursor += alloc_size;
    return chunk;
  }

  addr_t blank_page_addr = 0;
  blank_page_addr = search_near_blank_page(position, alloc_range);
  if (blank_page_addr) {
    OSMemory::SetPermission((void *)blank_page_addr, OSMemory::PageSize(), permission);
    NearMemoryArena::PushPage(blank_page_addr, permission);
    goto search_once_more;
  }

  // TODO: fix up
  if (permission == kReadWrite) {
    return NULL;
  }

  addr_t blank_chunk_addr = 0;
  blank_chunk_addr = search_near_blank_memory_chunk(position, alloc_range, alloc_size);
  if (blank_chunk_addr) {
    MemoryChunk *chunk = NULL;
    chunk = new MemoryChunk;
    chunk->address = (void *)blank_chunk_addr;
    chunk->length = alloc_size;
    return chunk;
  }

  return NULL;
}
