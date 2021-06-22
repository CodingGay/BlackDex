#ifndef GET_PROCESS_MODULE_MAP_H
#define GET_PROCESS_MODULE_MAP_H

#include "PlatformUnifiedInterface/StdMemory.h"

#include <vector>
#include <algorithm>

typedef struct _RuntimeModule {
  char path[1024];
  void *load_address;
} RuntimeModule;

class ProcessRuntimeUtility {
public:
  // ================================================================
  // Process Memory

  static std::vector<MemoryRegion> GetProcessMemoryLayout();

  // ================================================================
  // Process Module

  static std::vector<RuntimeModule> GetProcessModuleMap();

  static RuntimeModule GetProcessModule(const char *name);
};

#endif