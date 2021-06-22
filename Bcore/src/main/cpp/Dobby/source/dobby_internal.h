#ifndef DOBBY_INTERNAL_H
#define DOBBY_INTERNAL_H

#include "dobby.h"

#include "logging/logging.h"
#include "logging/check_logging.h"

#include "xnucxx/LiteMemOpt.h"
#include "xnucxx/LiteMutableArray.h"
#include "xnucxx/LiteMutableBuffer.h"
#include "xnucxx/LiteIterator.h"

#include "UnifiedInterface/platform.h"

#include "PlatformUnifiedInterface/StdMemory.h"
#include "PlatformUnifiedInterface/ExecMemory/CodePatchTool.h"
#include "PlatformUnifiedInterface/ExecMemory/ClearCacheTool.h"

#include "MemoryAllocator/MemoryArena.h"
#include "MemoryAllocator/AssemblyCodeBuilder.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

typedef struct _AssemblyCodeChunkBuffer {
  AssemblyCodeChunk chunk;
  uint8_t chunk_buffer[64];
} AssemblyCodeChunkBuffer;

typedef enum { kFunctionWrapper, kFunctionInlineHook, kDynamicBinaryInstrument } HookEntryType;

typedef struct {
  int id;
  int type;

  union {
    void *target_address;
    void *function_address;
    void *instruction_address;
  };

  void *route;

  // fixed-instructions which we relocated(patched)
  union {
    void *relocated_origin_instructions;
    void *relocated_origin_function;
  };

  AssemblyCodeChunkBuffer origin_chunk_;
} HookEntry;

#endif