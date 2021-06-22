#include "dobby_internal.h"

#include "InterceptRouting/InterceptRouting.h"
#include "InterceptRouting/Routing/DynamicBinaryInstrument/dynamic-binary-instrument.h"

PUBLIC int DobbyInstrument(void *address, DBICallTy handler) {
  if (!address) {
    ERROR_LOG("the function address is 0x0.\n");
    return RS_FAILED;
  }

  RAW_LOG(1, "\n\n");
  DLOG(0, "[DobbyInstrument] Initialize at %p", address);

  // check if we already instruemnt
  HookEntry *entry = Interceptor::SharedInstance()->FindHookEntry(address);
  if (entry) {
    DynamicBinaryInstrumentRouting *route = (DynamicBinaryInstrumentRouting *)entry->route;
    if (route->handler == handler) {
      ERROR_LOG("instruction %s already been instrumented.", address);
      return RS_FAILED;
    }
  }

  entry = new HookEntry();
  entry->id = Interceptor::SharedInstance()->GetHookEntryCount();
  entry->type = kDynamicBinaryInstrument;
  entry->instruction_address = address;

  DynamicBinaryInstrumentRouting *route = new DynamicBinaryInstrumentRouting(entry, (void *)handler);
  route->Prepare();
  route->DispatchRouting();
  Interceptor::SharedInstance()->AddHookEntry(entry);
  route->Commit();

  return RS_SUCCESS;
}
