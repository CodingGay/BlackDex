#ifndef UNIFIED_INTERFACE_STD_MEMORY_H
#define UNIFIED_INTERFACE_STD_MEMORY_H

#include "common_header.h"

enum MemoryPermission { kNoAccess, kRead, kReadWrite, kReadWriteExecute, kReadExecute };

typedef struct _MemoryRange {
  void *address;
  size_t length;
} MemoryRange;

typedef struct _MemoryRegion {
  void *address;
  size_t length;
  MemoryPermission permission;
} MemoryRegion;

typedef MemoryRegion MemoryPage;

#endif
