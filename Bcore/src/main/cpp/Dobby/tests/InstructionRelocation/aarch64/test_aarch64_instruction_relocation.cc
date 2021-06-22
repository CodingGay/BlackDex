#include <stdio.h>
#include <stdlib.h>

#if defined(__WIN32__) || defined(__APPLE__)
#define xcdecl(s) "_" s
#else
#define xcdecl(s) s
#endif

#define xASM(x) __asm(x)

__attribute__((naked)) void pc_relative_instructions() {
  xASM("ldr x0, #4");
  xASM("nop");
  xASM("nop");

  xASM("nop");
  xASM("nop");
  xASM("ldr x0, #-4");

  xASM("adr x0, #4");

  xASM("adrp x0, #0x1000");

  xASM("tbz x0, #8, #4");

  xASM("tbz x0, #27, #-4");

  xASM("cbz x0, #4");

  xASM("cbz x0, #-4");

  xASM("cmp x0, x0");
  xASM("b.eq #-4");
}

__attribute__((naked)) void pc_relative_instructions_end() {
}

#include "InstructionRelocation/arm64/ARM64InstructionRelocation.h"

extern zz::AssemblyCode *GenRelocateCodeAndBranch(void *buffer, int *relocate_size, addr_t from_pc, addr_t to_pc);

extern "C" {
int _main(int argc, const char **argv);
}

int _main(int argc, const char **argv) {
  LOG(1, "pc_relative_instructions: %p", pc_relative_instructions);

  char *relo_buffer = (char *)malloc(0x1000);

  int relo_size = (uint64_t)pc_relative_instructions_end - (uint64_t)pc_relative_instructions;
  zz::AssemblyCode *code = GenRelocateCodeAndBranch((void *)pc_relative_instructions, &relo_size, 0, 0);

  unsigned char *instruction_bytes = (unsigned char *)code->raw_instruction_start();
  for (int i = 0; i < code->raw_instruction_size(); i += 1) {
    printf("%.2x ", instruction_bytes[i]);
  }

  return 0;
}
