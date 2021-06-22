#include "dobby_internal.h"

#include "InterceptRouting/InterceptRouting.h"
#include "InterceptRouting/RoutingPlugin/RoutingPlugin.h"

using namespace zz;

void InterceptRouting::Prepare() {
}

// Generate relocated code
bool InterceptRouting::GenerateRelocatedCode(int tramp_size) {
  // generate original code
  AssemblyCodeChunk *origin = NULL;
  origin = AssemblyCodeBuilder::FinalizeFromAddress((addr_t)entry_->target_address, tramp_size);
  origin_ = origin;

  // generate the relocated code
  AssemblyCodeChunk *relocated = NULL;
  relocated = AssemblyCodeBuilder::FinalizeFromAddress(0, 0);
  relocated_ = relocated;

  void *relocate_buffer = NULL;
  relocate_buffer = entry_->target_address;

  GenRelocateCodeAndBranch(relocate_buffer, origin, relocated);
  if (relocated->raw_instruction_start() == 0)
    return false;

  // set the relocated instruction address
  entry_->relocated_origin_instructions = (void *)relocated->raw_instruction_start();
  DLOG(0, "[insn relocate] origin %p - %d", origin->raw_instruction_start(), origin->raw_instruction_size());
  DLOG(0, "[insn relocate] relocated %p - %d", relocated->raw_instruction_start(), relocated->raw_instruction_size());

  // save original prologue
  memcpy((void *)entry_->origin_chunk_.chunk_buffer, (void *)origin_->raw_instruction_start(),
         origin_->raw_instruction_size());
  entry_->origin_chunk_.chunk.re_init_region_range(origin_);
  return true;
}

bool InterceptRouting::GenerateTrampolineBuffer(void *src, void *dst) {
  CodeBufferBase *trampoline_buffer = NULL;
  // if near branch trampoline plugin enabled
  if (RoutingPluginManager::near_branch_trampoline) {
    RoutingPluginInterface *plugin = NULL;
    plugin = reinterpret_cast<RoutingPluginInterface *>(RoutingPluginManager::near_branch_trampoline);
    if (plugin->GenerateTrampolineBuffer(this, src, dst) == false) {
      DLOG(0, "Failed enable near branch trampoline plugin");
    }
  }

  if (this->GetTrampolineBuffer() == NULL) {
    trampoline_buffer = GenerateNormalTrampolineBuffer((addr_t)src, (addr_t)dst);
    this->SetTrampolineBuffer(trampoline_buffer);

    DLOG(0, "[trampoline] Generate trampoline buffer %p -> %p", src, dst);
  }
  return true;
}

// Active routing, will patch the origin insturctions, and forward to our custom routing.
// Patch the address with branch instr
void InterceptRouting::Active() {
  void *patch_address = NULL;
  patch_address = (void *)origin_->raw_instruction_start();

  CodePatch(patch_address, (uint8_t *)trampoline_buffer_->getRawBuffer(), trampoline_buffer_->getSize());
  DLOG(0, "[intercept routing] Active patch %p", patch_address);
}

void InterceptRouting::Commit() {
  this->Active();
}

#if 0
int InterceptRouting::PredefinedTrampolineSize() {
#if __arm64__
  return 12;
#elif __arm__
  return 8;
#endif
}
#endif

HookEntry *InterceptRouting::GetHookEntry() {
  return entry_;
};
