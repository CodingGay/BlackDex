#include "dobby_internal.h"

#include "logging/logging.h"

#include "Interceptor.h"
#include "InterceptRouting/InterceptRouting.h"

#include "function-wrapper.h"

PUBLIC int DobbyWrap(void *function_address, PreCallTy pre_call, PostCallTy post_call) {
  DLOG(0, "Initialize 'DobbyWrap' hook at %p", function_address);

  Interceptor *interceptor = Interceptor::SharedInstance();

  HookEntry *entry = new HookEntry();
  entry->id = interceptor->entries->getCount();
  entry->type = kFunctionWrapper;
  entry->function_address = function_address;

  FunctionWrapperRouting *route = new FunctionWrapperRouting(entry);
  route->DispatchRouting();
  interceptor->AddHookEntry(entry);
  route->Commit();

  DLOG(0, "Finalize %p", function_address);
  return RS_SUCCESS;
}
