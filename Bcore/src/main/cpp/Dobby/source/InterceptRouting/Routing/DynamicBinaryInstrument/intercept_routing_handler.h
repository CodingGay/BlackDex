#ifndef INTERCEPT_ROUTING_HANDLER_H
#define INTERCEPT_ROUTING_HANDLER_H

#include "dobby_internal.h"

#include "TrampolineBridge/ClosureTrampolineBridge/AssemblyClosureTrampoline.h"

extern "C" {
void instrument_routing_dispatch(RegisterContext *ctx, ClosureTrampolineEntry *entry);
}

#endif