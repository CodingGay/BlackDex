#include "InterceptRouting/Routing/DynamicBinaryInstrument/intercept_routing_handler.h"

#include "dobby_internal.h"

#include "InterceptRouting/Routing/DynamicBinaryInstrument/dynamic-binary-instrument.h"

#include "TrampolineBridge/ClosureTrampolineBridge/common-bridge-handler.h"

void instrument_call_forward_handler(RegisterContext *ctx, HookEntry *entry) {
  DynamicBinaryInstrumentRouting *route = (DynamicBinaryInstrumentRouting *)entry->route;
  if (route->handler) {
    DBICallTy handler;
    HookEntryInfo entry_info;
    entry_info.hook_id = entry->id;
    entry_info.instruction_address = entry->instruction_address;
    handler = (DBICallTy)route->handler;
    (*handler)(ctx, (const HookEntryInfo *)&entry_info);
  }

  // set prologue bridge next hop address with origin instructions that have been relocated(patched)
  set_routing_bridge_next_hop(ctx, entry->relocated_origin_instructions);
}

void instrument_routing_dispatch(RegisterContext *ctx, ClosureTrampolineEntry *closure_trampoline_entry) {
  HookEntry *entry = (HookEntry *)closure_trampoline_entry->carry_data;
  instrument_call_forward_handler(ctx, entry);
}
