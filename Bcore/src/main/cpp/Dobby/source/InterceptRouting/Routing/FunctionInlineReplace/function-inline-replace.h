#ifndef FUNCTION_INLINE_REPLACE_H
#define FUNCTION_INLINE_REPLACE_H

#include "dobby_internal.h"

#include "InterceptRouting/InterceptRouting.h"
#include "Interceptor.h"

#include "TrampolineBridge/ClosureTrampolineBridge/AssemblyClosureTrampoline.h"

class FunctionInlineReplaceRouting : public InterceptRouting {
public:
  FunctionInlineReplaceRouting(HookEntry *entry, void *replace_call) : InterceptRouting(entry) {
    this->replace_call = replace_call;
  }

  void DispatchRouting() override;

private:
  virtual void BuildReplaceRouting();

private:
  void *replace_call;
};

#endif
