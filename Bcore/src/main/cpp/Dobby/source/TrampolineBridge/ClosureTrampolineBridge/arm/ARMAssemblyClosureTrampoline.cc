#include "platform_macro.h"
#if defined(TARGET_ARCH_ARM)

#include "dobby_internal.h"

#include "core/modules/assembler/assembler-arm.h"

#include "TrampolineBridge/ClosureTrampolineBridge/AssemblyClosureTrampoline.h"

using namespace zz;
using namespace zz::arm;

ClosureTrampolineEntry *ClosureTrampoline::CreateClosureTrampoline(void *carry_data, void *carry_handler) {
  ClosureTrampolineEntry *entry = nullptr;
  entry = new ClosureTrampolineEntry;

#ifdef ENABLE_CLOSURE_TRAMPOLINE_TEMPLATE
#define CLOSURE_TRAMPOLINE_SIZE (7 * 4)
  // use closure trampoline template code, find the executable memory and patch it.
  Code *code = Code::FinalizeCodeFromAddress(closure_trampoline_template, CLOSURE_TRAMPOLINE_SIZE);
#else

// use assembler and codegen modules instead of template_code
#include "TrampolineBridge/ClosureTrampolineBridge/AssemblyClosureTrampoline.h"
#define _ turbo_assembler_.
  TurboAssembler turbo_assembler_(0);

  PseudoLabel entry_label;
  PseudoLabel forward_bridge_label;

  _ Ldr(r12, &entry_label);
  _ Ldr(pc, &forward_bridge_label);
  _ PseudoBind(&entry_label);
  _ EmitAddress((uint32_t)entry);
  _ PseudoBind(&forward_bridge_label);
  _ EmitAddress((uint32_t)get_closure_bridge());

  AssemblyCodeChunk *code = nullptr;
  code = AssemblyCodeBuilder::FinalizeFromTurboAssembler(&turbo_assembler_);

  entry->address = (void *)code->raw_instruction_start();
  entry->size = code->raw_instruction_size();
  entry->carry_data = carry_data;
  entry->carry_handler = carry_handler;

  delete code;
  return entry;
#endif
}

#endif