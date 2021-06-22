#ifndef CLOSURE_TRAMPOLINE_COMMON_HANDLER_H
#define CLOSURE_TRAMPOLINE_COMMON_HANDLER_H

#include "dobby_internal.h"

#include "Interceptor.h"
#include "TrampolineBridge/ClosureTrampolineBridge/AssemblyClosureTrampoline.h"

extern "C" {
void intercept_routing_common_bridge_handler(RegisterContext *ctx, ClosureTrampolineEntry *entry);
}

void get_routing_bridge_next_hop(RegisterContext *ctx, void *address);

void set_routing_bridge_next_hop(RegisterContext *ctx, void *address);

#endif