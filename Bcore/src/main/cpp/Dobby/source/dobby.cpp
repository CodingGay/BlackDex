#if defined(__linux__) || defined(__APPLE__)
#include <unistd.h>
#include <syslog.h>

#include "dobby_internal.h"

#include "Interceptor.h"

__attribute__((constructor)) static void ctor() {
  DLOG(-1, "================================");
  DLOG(-1, "Dobby");
  DLOG(-1, "================================");

  DLOG(-1, "dobby in debug log mode, disable with cmake flag \"-DDOBBY_DEBUG=OFF\"");
}

PUBLIC const char *DobbyBuildVersion() {
  return __DOBBY_BUILD_VERSION__;
}

PUBLIC int DobbyDestroy(void *address) {
  // check if we already hook
  HookEntry *entry = Interceptor::SharedInstance()->FindHookEntry(address);
  if (entry) {
    uint8_t *buffer = entry->origin_chunk_.chunk_buffer;
    uint32_t buffer_size = entry->origin_chunk_.chunk.length;
#if defined(TARGET_ARCH_ARM)
    address = (void *)((addr_t)address - 1);
#endif
    CodePatch(address, buffer, buffer_size);
    Interceptor::SharedInstance()->RemoveHookEntry(address);
    return RT_SUCCESS;
  }

  return RT_FAILED;
}

#endif