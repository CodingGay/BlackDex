
#ifndef PLATFORM_INTERFACE_CODE_PATCH_TOOL_H
#define PLATFORM_INTERFACE_CODE_PATCH_TOOL_H

#include "PlatformUnifiedInterface/StdMemory.h"

MemoryOperationError CodePatch(void *address, uint8_t *buffer, uint32_t buffer_size);

#endif