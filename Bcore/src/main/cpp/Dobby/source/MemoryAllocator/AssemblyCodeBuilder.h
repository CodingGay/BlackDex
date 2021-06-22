#ifndef MemoryAllocator_AssemblyCodeBuilder_h
#define MemoryAllocator_AssemblyCodeBuilder_h

#include "MemoryAllocator/MemoryArena.h"

#include "core/modules/assembler/assembler.h"

using namespace zz;

class AssemblyCodeBuilder {
public:
  // realize the buffer address to runtime code, and create a corresponding Code Object
  static AssemblyCodeChunk *FinalizeFromAddress(addr_t address, int size);

  // realize the buffer address to runtime code, and create a corresponding Code Object
  static AssemblyCodeChunk *FinalizeFromTurboAssembler(AssemblerBase *assembler);

  // inline void init_region_range(addr_t address, int size) {
  //   code_range_.address = (void *)address;
  //   code_range_.length  = size;
  // }

  // inline void re_init_region_range(addr_t address, int size) {
  //   this->init_region_range(address, size);
  // }

  // inline addr_t raw_instruction_start() {
  //   return (addr_t)code_range_.address;
  // };

  // inline int raw_instruction_size() {
  //   return (int)code_range_.length;
  // };

private:
  // AssemblyCodeChunk code_range_;
};

#endif
