set(dobby.SOURCE_FILE_LIST
  # cpu
  source/core/arch/CpuFeature.cc
  source/core/arch/CpuRegister.cc

  # cpu - x86
  source/core/arch/x86/cpu-x86.cc

  # assembler
  source/core/modules/assembler/assembler.cc
  source/core/modules/assembler/assembler-arm.cc
  source/core/modules/assembler/assembler-arm64.cc
  source/core/modules/assembler/assembler-ia32.cc
  source/core/modules/assembler/assembler-x64.cc

  # codegen
  source/core/modules/codegen/codegen-arm.cc
  source/core/modules/codegen/codegen-arm64.cc
  source/core/modules/codegen/codegen-ia32.cc
  source/core/modules/codegen/codegen-x64.cc

  # executable memory - code buffer
  source/MemoryAllocator/CodeBuffer/CodeBufferBase.cc
  source/MemoryAllocator/CodeBuffer/code-buffer-arm.cc
  source/MemoryAllocator/CodeBuffer/code-buffer-arm64.cc
  source/MemoryAllocator/CodeBuffer/code-buffer-x86.cc
  source/MemoryAllocator/CodeBuffer/code-buffer-x64.cc

  # executable memory
  source/MemoryAllocator/AssemblyCodeBuilder.cc
  source/MemoryAllocator/MemoryArena.cc

  # instruction relocation
  source/InstructionRelocation/arm/ARMInstructionRelocation.cc
  source/InstructionRelocation/arm64/ARM64InstructionRelocation.cc
  source/InstructionRelocation/x86/X86InstructionRelocation.cc
  source/InstructionRelocation/x64/X64InstructionRelocation.cc

  source/InstructionRelocation/x86/x86_insn_decode/x86_insn_decode.c

  # intercept routing
  source/InterceptRouting/InterceptRouting.cpp

  # intercept routing trampoline
  source/TrampolineBridge/Trampoline/arm/trampoline-arm.cc
  source/TrampolineBridge/Trampoline/arm64/trampoline-arm64.cc
  source/TrampolineBridge/Trampoline/x86/trampoline-x86.cc
  source/TrampolineBridge/Trampoline/x64/trampoline-x64.cc

  # intercept routing plugin (buildin)
  source/InterceptRouting/Routing/FunctionInlineReplace/function-inline-replace.cc
  source/InterceptRouting/Routing/FunctionInlineReplace/FunctionInlineReplaceExport.cc

  # plugin register
  source/InterceptRouting/RoutingPlugin/RoutingPlugin.cc

  # unified interface

  # platform util
  source/UserMode/PlatformUtil/${platform2}/ProcessRuntimeUtility.cc

  # user mode - platform interface
  source/UserMode/UnifiedInterface/platform-${platform1}.cc

  # user mode - executable memory
  source/UserMode/ExecMemory/code-patch-tool-${platform1}.cc
  source/UserMode/ExecMemory/clear-cache-tool-all.c

  # main
  source/dobby.cpp
  source/Interceptor.cpp
  )

if(FunctionWrapper OR DynamicBinaryInstrument)
  set(dobby.SOURCE_FILE_LIST ${dobby.SOURCE_FILE_LIST}
    # closure trampoline bridge
    source/TrampolineBridge/ClosureTrampolineBridge/common-bridge-handler.cc

    source/TrampolineBridge/ClosureTrampolineBridge/arm/helper-arm.cc
    source/TrampolineBridge/ClosureTrampolineBridge/arm/closure-bridge-arm.cc
    source/TrampolineBridge/ClosureTrampolineBridge/arm/ARMAssemblyClosureTrampoline.cc

    source/TrampolineBridge/ClosureTrampolineBridge/arm64/helper-arm64.cc
    source/TrampolineBridge/ClosureTrampolineBridge/arm64/closure-bridge-arm64.cc
    source/TrampolineBridge/ClosureTrampolineBridge/arm64/ARM64AssemblyClosureTrampoline.cc

    source/TrampolineBridge/ClosureTrampolineBridge/x64/helper-x64.cc
    source/TrampolineBridge/ClosureTrampolineBridge/x64/closure-bridge-x64.cc
    source/TrampolineBridge/ClosureTrampolineBridge/x64/X64AssemblyClosureTrampoline.cc

    # user mode - multi thread support
    source/UserMode/MultiThreadSupport/ThreadSupport.cpp
    source/UserMode/Thread/PlatformThread.cc
    source/UserMode/Thread/platform-thread-${platform1}.cc
    )
endif()