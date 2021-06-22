#include "dobby_internal.h"

#include "Interceptor.h"
#include "InterceptRouting/InterceptRouting.h"
#include "InterceptRouting/Routing/FunctionInlineReplace/function-inline-replace.h"

PUBLIC int DobbyHook(void *address, void *replace_call, void **origin_call) {
  if (!address) {
    ERROR_LOG("function address is 0x0");
    return RS_FAILED;
  }

  DLOG(0, "[DobbyHook] Initialize at %p", address);

  // check if already hooked
  HookEntry *entry = Interceptor::SharedInstance()->FindHookEntry(address);
  if (entry) {
    FunctionInlineReplaceRouting *route = (FunctionInlineReplaceRouting *)entry->route;
    if (route->GetTrampolineTarget() == replace_call) {
      ERROR_LOG("function %p already been hooked.", address);
      return RS_FAILED;
    }
  }

  entry = new HookEntry();
  entry->id = Interceptor::SharedInstance()->GetHookEntryCount();
  entry->type = kFunctionInlineHook;
  entry->function_address = address;

  FunctionInlineReplaceRouting *route = new FunctionInlineReplaceRouting(entry, replace_call);
  route->Prepare();
  route->DispatchRouting();
  Interceptor::SharedInstance()->AddHookEntry(entry);

  // set origin call with relocated function
  *origin_call = entry->relocated_origin_function;

  // code patch & hijack original control flow entry
  route->Commit();

  return RS_SUCCESS;
}
