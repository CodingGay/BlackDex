#include "PlatformUtil/ProcessRuntimeUtility.h"

#include <vector>

#define LINE_MAX 2048

// ================================================================
// GetProcessMemoryLayout

static bool memory_region_comparator(MemoryRegion a, MemoryRegion b) {
  return (a.address > b.address);
}

std::vector<MemoryRegion> ProcessMemoryLayout;
std::vector<MemoryRegion> ProcessRuntimeUtility::GetProcessMemoryLayout() {
  if (!ProcessMemoryLayout.empty()) {
    ProcessMemoryLayout.clear();
  }

  return ProcessMemoryLayout;
}

// ================================================================
// GetProcessModuleMap

std::vector<RuntimeModule> ProcessModuleMap;

std::vector<RuntimeModule> ProcessRuntimeUtility::GetProcessModuleMap() {
  if (!ProcessMemoryLayout.empty()) {
    ProcessMemoryLayout.clear();
  }
  return ProcessModuleMap;
}

RuntimeModule ProcessRuntimeUtility::GetProcessModule(const char *name) {
  std::vector<RuntimeModule> ProcessModuleMap = GetProcessModuleMap();
  for (auto module : ProcessModuleMap) {
    if (strstr(module.path, name) != 0) {
      return module;
    }
  }
  return RuntimeModule{0};
}