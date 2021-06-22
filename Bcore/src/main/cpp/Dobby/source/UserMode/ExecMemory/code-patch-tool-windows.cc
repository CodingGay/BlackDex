#include "dobby_internal.h"

#include <windows.h>

using namespace zz;

PUBLIC MemoryOperationError CodePatch(void *address, uint8_t *buffer, uint32_t buffer_size) {
  DWORD oldProtect;
  int pageSize;

  // Get page size
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  pageSize = si.dwPageSize;

  void *addressPageAlign = (void *)ALIGN(address, pageSize);

  if (!VirtualProtect(addressPageAlign, pageSize, PAGE_EXECUTE_READWRITE, &oldProtect))
    return kMemoryOperationError;

  memcpy(address, buffer, buffer_size);

  if (!VirtualProtect(addressPageAlign, pageSize, oldProtect, &oldProtect))
    return kMemoryOperationError;

  return kMemoryOperationSuccess;
}
