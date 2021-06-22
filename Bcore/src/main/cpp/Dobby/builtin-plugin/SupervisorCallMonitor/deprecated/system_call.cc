#include "dobby_internal.h"

#include "PlatformUtil/ProcessRuntimeUtility.h"

#include <iostream>

#include <unistd.h>
#include <stdlib.h>

#include <sys/syscall.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "misc-helper/async_logger.h"

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

extern const char *syscall_num_to_str(int num);

extern const char *mach_syscall_num_to_str(int num);

extern char *mach_msg_to_str(mach_msg_header_t *msg);

static void common_handler(RegisterContext *ctx, const HookEntryInfo *info) {
  char buffer[256] = {0};
  int syscall_rum = ctx->general.regs.x16;
  if (syscall_rum == 0) {
    syscall_rum = (int)getCallFirstArg(ctx);
    sprintf(buffer, "[syscall svc-%d] %s\n", syscall_rum, syscall_num_to_str(syscall_rum));
  } else if (syscall_rum == -31) {
    // mach_msg_trap
    mach_msg_header_t *msg = (typeof(msg))getCallFirstArg(ctx);
    char *mach_msg_name = mach_msg_to_str(msg);
    if (mach_msg_name) {
      sprintf(buffer, "[mach msg svc] %s\n", mach_msg_name);
    } else {
      buffer[0] = 0;
    }
  } else if (syscall_rum > 0) {
    sprintf(buffer, "[svc-%d] %s\n", syscall_rum, syscall_num_to_str(syscall_rum));
  } else {
    sprintf(buffer, "[mach svc-%d] %s\n", syscall_rum, mach_syscall_num_to_str(syscall_rum));
  }
  async_logger_print(buffer);
}

typedef int32_t arm64_instr_t;

void monitor_libsystem_kernel_dylib() {
  auto libsystem_c = ProcessRuntimeUtility::GetProcessModule("libsystem_kernel.dylib");
  addr_t libsystem_c_header = (addr_t)libsystem_c.load_address;
  auto text_section =
      mach_kit::macho_get_section_by_name_64((struct mach_header_64 *)libsystem_c_header, "__TEXT", "__text");

  addr_t shared_cache_load_addr = (addr_t)mach_kit::macho_get_shared_cache();
  addr_t insn_addr = shared_cache_load_addr + (addr_t)text_section->offset;
  addr_t insn_addr_end = insn_addr + text_section->size;

  addr_t write_svc_addr = (addr_t)DobbySymbolResolver("libsystem_kernel.dylib", "write");
  write_svc_addr += 4;

  addr_t __psynch_mutexwait_svc_addr = (addr_t)DobbySymbolResolver("libsystem_kernel.dylib", "__psynch_mutexwait");
  __psynch_mutexwait_svc_addr += 4;

  for (; insn_addr < insn_addr_end; insn_addr += sizeof(arm64_instr_t)) {
    if (*(arm64_instr_t *)insn_addr == 0xd4001001) {
      dobby_enable_near_branch_trampoline();
      if (insn_addr == write_svc_addr)
        continue;

      if (insn_addr == __psynch_mutexwait_svc_addr)
        continue;
      DobbyInstrument((void *)insn_addr, common_handler);
      LOG(2, "instrument svc at %p", insn_addr);
    }
  }
}

void monitor_main_binary() {
  auto main = ProcessRuntimeUtility::GetProcessModuleMap()[0];
  addr_t main_header = (addr_t)main.load_address;
  auto text_section = mach_kit::macho_get_section_by_name_64((struct mach_header_64 *)main_header, "__TEXT", "__text");

  addr_t insn_addr = main_header + (addr_t)text_section->offset;
  addr_t insn_addr_end = insn_addr + text_section->size;

  for (; insn_addr < insn_addr_end; insn_addr += sizeof(arm64_instr_t)) {
    if (*(arm64_instr_t *)insn_addr == 0xd4001001) {
      DobbyInstrument((void *)insn_addr, common_handler);
      LOG(2, "instrument svc at %p", insn_addr);
    }
  }
}

void system_call_monitor() {
#if 0
  monitor_libsystem_kernel_dylib();
#endif

  monitor_main_binary();
}
