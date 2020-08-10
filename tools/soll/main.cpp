// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include "soll/AST/AST.h"
#include "soll/Frontend/CompilerInstance.h"
#include "soll/Frontend/CompilerInvocation.h"
#include "soll/Frontend/TextDiagnosticPrinter.h"
#include "soll/FrontendTool/Utils.h"
#include <llvm/ADT/SmallVector.h>
#include <llvm/Support/Process.h>
#include <llvm/Support/TargetSelect.h>

using namespace soll;

int main(int argc, const char **argv) {
  //  Declared In: Process.h
  //  namespace llvm::sys
  //  public: static std::error_code Process::FixupStandardFileDescriptors()
  //  This functions ensures that the standard file descriptors (input, output,
  //  and error) are properly mapped to a file descriptor before we use any of
  //  them.
  //  This should only be called by standalone programs,
  //  library components should not call this.
  if (llvm::sys::Process::FixupStandardFileDescriptors()) {
    return EXIT_FAILURE;
  }

  // This is a 'vector' (really, a variable-sized array),
  // optimized for the case when the array is small.
  // It contains some number of elements in-place,
  // which allows it to avoid heap allocation when the actual number of elements
  // is below that threshold.
  // This allows normal "small" cases to be fast without losing generality for
  // large inputs.Note that this does not attempt to be exception safe.
  llvm::SmallVector<const char *, 256> Args(argv, argv + argc);

  // unique_ptr与所指对象的内存紧密地绑定
  // 不能与其他的unique_ptr类型的指针对象共享所指向对象的内存
  // include/soll/Frontend/CompilerInstance.h
  std::unique_ptr Soll = std::make_unique<CompilerInstance>();

  // A smart pointer to a reference-counted object that
  // inherits from RefCountedBase or ThreadSafeRefCountedBase.
  // This class increments its pointee's reference count when it is created,
  // and decrements its refcount when it's destroyed
  // (or is changed to point to a different object).
  // include/soll/Basic/DiagnosticIDs.h
  llvm::IntrusiveRefCntPtr<DiagnosticIDs> DiagID(new DiagnosticIDs());

  // include/soll/Basic/DiagnosticOptions.h
  llvm::IntrusiveRefCntPtr<DiagnosticOptions> DiagOpts =
      new DiagnosticOptions();

  // include/soll/Basic/Diagnostic.h
  DiagnosticsEngine Diags(DiagID, &*DiagOpts);

  // CompilerInvocation &CompilerInstance::getInvocation() {
  //  assert(Invocation.get() != nullptr);
  //  return *Invocation;
  // }
  // lib/Frontend/CompilerInvocation.cpp
  // process Input command, options and files
  if (!Soll->getInvocation().ParseCommandLineOptions(Args, Diags)) {
    return EXIT_FAILURE;
  }

  // InitializeAllTargets - The main program should call this function
  // if it wants access to all available target machines that LLVM is configured
  // to support, to make them available via the TargetRegistry.
  // It is legal for a client to make multiple calls to this function.
  llvm::InitializeAllTargets();

  // InitializeAllTargetMCs - The main program should call this function
  // if it wants access to all available target MC that LLVM is configured
  // to support, to make them available via the TargetRegistry.
  // It is legal for a client to make multiple calls to this function.
  llvm::InitializeAllTargetMCs();

  // InitializeAllAsmPrinters - The main program should call this function
  // if it wants all asm printers that LLVM is configured to support,
  // to make them available via the TargetRegistry.
  // It is legal for a client to make multiple calls to this function.
  llvm::InitializeAllAsmPrinters();

  // InitializeAllAsmParsers - The main program should call this function
  // if it wants all asm parsers that LLVM is configured to support,
  // to make them available via the TargetRegistry.
  // It is legal for a client to make multiple calls to this function.
  llvm::InitializeAllAsmParsers();

  // bool ExecuteCompilerInvocation(CompilerInstance *Soll) {
  //  std::unique_ptr<FrontendAction> Act(CreateFrontendAction(*Soll));
  //  if (!Act)
  //    return false;
  //  bool Success = Soll->ExecuteAction(*Act);
  //  return Success;
  // }
  // Soll.get(): Return the stored pointer.
  if (!ExecuteCompilerInvocation(Soll.get())) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
