#include "InterceptRouting/Routing/DynamicBinaryInstrument/dynamic-binary-instrument.h"

#include "dobby_internal.h"

#include "TrampolineBridge/ClosureTrampolineBridge/AssemblyClosureTrampoline.h"

#include "InterceptRouting/Routing/DynamicBinaryInstrument/intercept_routing_handler.h"

void DynamicBinaryInstrumentRouting::DispatchRouting() {
  BuildDynamicBinaryInstrumentRouting();

  // generate relocated code which size == trampoline size
  GenerateRelocatedCode(trampoline_buffer_->getSize());
}

// Add dbi_call handler before running the origin instructions
void DynamicBinaryInstrumentRouting::BuildDynamicBinaryInstrumentRouting() {
  // create closure trampoline jump to prologue_routing_dispath with the `entry_` data
  ClosureTrampolineEntry *closure_trampoline;

  void *handler = (void *)instrument_routing_dispatch;
#if __APPLE__
#if __has_feature(ptrauth_calls)
  handler = __builtin_ptrauth_strip(handler, ptrauth_key_asia);
#endif
#endif

  closure_trampoline = ClosureTrampoline::CreateClosureTrampoline(entry_, handler);
  this->SetTrampolineTarget(closure_trampoline->address);
  DLOG(0, "[closure bridge] Carry data %p ", entry_);
  DLOG(0, "[closure bridge] Create prologue_dispatch_bridge %p", closure_trampoline->address);

  // generate trampoline buffer, run before `GenerateRelocatedCode`
  GenerateTrampolineBuffer(entry_->target_address, GetTrampolineTarget());
}

#if 0
void *DynamicBinaryInstrumentRouting::GetTrampolineTarget() {
  return this->prologue_dispatch_bridge;
}
#endif