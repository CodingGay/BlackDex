#include "dobby_internal.h"

#include "MachUtility.h"

#include "PlatformUtil/ProcessRuntimeUtility.h"

#include <unistd.h>
#include <stdlib.h>

#include <iostream>

#include "misc-helper/async_logger.h"

extern char *mach_msg_to_str(mach_msg_header_t *msg);

#if 0
typeof(mach_msg) *orig_mach_msg = NULL;

mach_msg_return_t fake_mach_msg(mach_msg_header_t *msg, mach_msg_option_t option, mach_msg_size_t send_size,
                                mach_msg_size_t rcv_size, mach_port_name_t rcv_name, mach_msg_timeout_t timeout,
                                mach_port_name_t notify) {
  char buffer[256] = {0};
  char *mach_msg_name = mach_msg_to_str(msg);
  if(mach_msg_name) {
    sprintf(buffer, "[%d][mach_msg] %s\n",i++, mach_msg_name);
    async_logger_print(buffer);
  }
#if 0
  {
    write(STDOUT_FILENO, buffer, strlen(buffer) + 1);
  }
#endif
  return orig_mach_msg(msg, option, send_size, rcv_size, rcv_name, timeout, notify);
}

void mach_system_call_monitor() {
  void *mach_msg_ptr = (void *)DobbySymbolResolver(NULL, "mach_msg");
  log_set_level(1);
  DobbyHook(mach_msg_ptr, (void *)fake_mach_msg, (void **)&orig_mach_msg);
}
#endif

static addr_t getCallFirstArg(RegisterContext *ctx) {
  addr_t result;
#if defined(_M_X64) || defined(__x86_64__)
#if defined(_WIN32)
  result = ctx->general.regs.rcx;
#else
  result = ctx->general.regs.rdi;
#endif
#elif defined(__arm64__) || defined(__aarch64__)
  result = ctx->general.regs.x0;
#elif defined(__arm__)
  result = ctx->general.regs.r0;
#else
#error "Not Support Architecture."
#endif
  return result;
}

static void common_handler(RegisterContext *ctx, const HookEntryInfo *info) {
  addr_t caller = get_caller_from_main_binary(ctx);
  if (caller == 0)
    return;

  char buffer[256] = {0};
  mach_msg_header_t *msg = (typeof(msg))getCallFirstArg(ctx);
  char *mach_msg_name = mach_msg_to_str(msg);
  if (mach_msg_name) {
    sprintf(buffer, "[mach msg %p] %s\n", caller, mach_msg_name);
  } else {
    buffer[0] = 0;
  }
  if (buffer[0])
    async_logger_print(buffer);
}

void mach_system_call_monitor() {
  void *mach_msg_ptr = (void *)DobbySymbolResolver(NULL, "mach_msg");
  log_set_level(1);
  DobbyInstrument(mach_msg_ptr, common_handler);
}
