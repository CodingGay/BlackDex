
#include "dobby_internal.h"

#include "logging/logging.h"

#include "intercept_routing_handler.h"

#include "function-wrapper.h"
#include "intercept_routing_handler.h"

#include "MultiThreadSupport/ThreadSupport.h"

#include "TrampolineBridge/ClosureTrampolineBridge/common-bridge-handler.h"

void pre_call_forward_handler(RegisterContext *ctx, HookEntry *entry) {
  FunctionWrapperRouting *route = (FunctionWrapperRouting *)entry->route;

  StackFrame *stackframe = new StackFrame();
  // create stack frame as common variable between pre_call and post_call
  ThreadSupport::PushStackFrame(stackframe);

  // run the `pre_call` before execute origin function which has been relocated(fixed)
  if (route->pre_call) {
    PreCallTy pre_call;
    HookEntryInfo entry_info;
    entry_info.hook_id = entry->id;
    entry_info.target_address = entry->target_address;
    pre_call = route->pre_call;
    // run the pre_call with the power of accessing all registers
    (*pre_call)(ctx, (const HookEntryInfo *)&entry_info);
  }

  // save the origin ret address, and use in `post_call_forword_handler`
  stackframe->orig_ret = get_func_ret_address(ctx);

  // set the prologue bridge next hop address with the patched instructions has been relocated
  set_routing_bridge_next_hop(ctx, entry->relocated_origin_function);

  // replace the function ret address with our epilogue_routing_dispatch
  set_func_ret_address(ctx, entry->epilogue_dispatch_bridge);
}

void post_call_forward_handler(RegisterContext *ctx, HookEntry *entry) {
  FunctionWrapperRouting *route = (FunctionWrapperRouting *)entry->route;

  // pop stack frame as common variable between pre_call and post_call
  StackFrame *stackframe = ThreadSupport::PopStackFrame();

  // run the `post_call`, and access all the register value, as the origin function done,
  if (route->post_call) {
    PostCallTy post_call;
    HookEntryInfo entry_info;
    entry_info.hook_id = entry->id;
    entry_info.target_address = entry->target_address;
    post_call = route->post_call;

    // run the post_call with the power of accessing all registers
    (*post_call)(ctx, (const HookEntryInfo *)&entry_info);
  }

  // set epilogue bridge next hop address with origin ret address, restore the call.
  set_routing_bridge_next_hop(ctx, stackframe->orig_ret);
}

// run the user handler **before run the origin-instructions(which have been relocated)**
void prologue_routing_dispatch(RegisterContext *ctx, ClosureTrampolineEntry *closure_trampoline_entry) {
  DLOG(0, "Catch prologue dispatch");
  HookEntry *entry = static_cast<HookEntry *>(closure_trampoline_entry->carry_data);
  pre_call_forward_handler(ctx, entry);
  return;
}

// run the user handler **before the function return** by replace the lr register
void epilogue_routing_dispatch(RegisterContext *ctx, ClosureTrampolineEntry *closure_trampoline_entry) {
  DLOG(0, "Catch epilogue dispatch");
  HookEntry *entry = static_cast<HookEntry *>(closure_trampoline_entry->carry_data);
  post_call_forward_handler(ctx, entry);
  return;
}