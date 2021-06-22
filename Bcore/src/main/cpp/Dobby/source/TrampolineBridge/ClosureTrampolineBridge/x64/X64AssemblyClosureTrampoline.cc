#include "platform_macro.h"
#if defined(TARGET_ARCH_X64)

#include "dobby_internal.h"

#include "core/modules/assembler/assembler-x64.h"

#include "TrampolineBridge/ClosureTrampolineBridge/AssemblyClosureTrampoline.h"

using namespace zz;
using namespace zz::x64;

ClosureTrampolineEntry *ClosureTrampoline::CreateClosureTrampoline(void *carry_data, void *carry_handler) {
  ClosureTrampolineEntry *entry = nullptr;
  entry = new ClosureTrampolineEntry;

  AssemblyCodeChunk *cchunk = MemoryArena::AllocateCodeChunk(32);
  if (cchunk == nullptr) {
    return NULL;
  }
#define _ turbo_assembler_.
#define __ turbo_assembler_.GetCodeBuffer()->
  TurboAssembler turbo_assembler_(0);

  char *push_rip_6 = (char *)"\xff\x35\x06\x00\x00\x00";
  char *jmp_rip_8 = (char *)"\xff\x25\x08\x00\x00\x00";

  __ EmitBuffer(push_rip_6, 6);
  __ EmitBuffer(jmp_rip_8, 6);
  __ Emit64((uint64_t)entry);
  __ Emit64((uint64_t)get_closure_bridge());

  entry->address = (void *)cchunk->raw_instruction_start();
  entry->size = cchunk->raw_instruction_size();
  entry->carry_data = carry_data;
  entry->carry_handler = carry_handler;

  CodeBufferBase *buffer = reinterpret_cast<CodeBufferBase *>(turbo_assembler_.GetCodeBuffer());
  CodePatch(cchunk->address, (uint8_t *)buffer->getRawBuffer(), buffer->getSize());

  return entry;
}

#endif