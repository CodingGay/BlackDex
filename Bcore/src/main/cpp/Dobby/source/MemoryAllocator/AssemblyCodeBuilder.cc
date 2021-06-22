#include "MemoryAllocator/AssemblyCodeBuilder.h"

#include "dobby_internal.h"
#include "PlatformUnifiedInterface/ExecMemory/CodePatchTool.h"

AssemblyCodeChunk *AssemblyCodeBuilder::FinalizeFromAddress(addr_t address, int size) {
  AssemblyCodeChunk *result = NULL;
  result = new AssemblyCodeChunk;
  result->init_region_range(address, size);
  return result;
}

AssemblyCodeChunk *AssemblyCodeBuilder::FinalizeFromTurboAssembler(AssemblerBase *assembler) {
  AssemblyCodeChunk *result = NULL;

  CodeBufferBase *buffer = NULL;
  buffer = (CodeBufferBase *)assembler->GetCodeBuffer();

  void *realized_address = assembler->GetRealizedAddress();
  if (realized_address == NULL) {
    int buffer_size = 0;
    {
      buffer_size = buffer->getSize();
#if TARGET_ARCH_ARM64 || TARGET_ARCH_ARM
      // FIXME: need it ? actually ???
      // extra bytes for align needed
      buffer_size += 4;
#endif
    }

    // assembler without specific memory address
    result = MemoryArena::AllocateCodeChunk(buffer_size);
    if (result == NULL)
      return NULL;

    realized_address = (void *)result->raw_instruction_start();
    assembler->SetRealizedAddress(realized_address);
  } else {
    result = AssemblyCodeBuilder::FinalizeFromAddress((addr_t)realized_address, buffer->getSize());
  }

  // Realize(Relocate) the buffer_code to the executable_memory_address, remove the ExternalLabels, etc, the pc-relative
  // instructions
  CodePatch(realized_address, (uint8_t *)buffer->getRawBuffer(), buffer->getSize());

  return result;
}