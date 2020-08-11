// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include "soll/Frontend/CompilerInvocation.h"
#include "soll/Basic/TargetOptions.h"
#include "soll/CodeGen/CodeGenAction.h"
#include "soll/Config/Config.h"
#include "soll/Frontend/CompilerInstance.h"
#include "soll/Frontend/FrontendActions.h"
#include "soll/Frontend/TextDiagnostic.h"
#include "soll/Frontend/TextDiagnosticPrinter.h"
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Process.h>
#include <llvm/Support/raw_ostream.h>

namespace cl = llvm::cl;

namespace soll {

static cl::opt<bool> Help("h", cl::desc("Alias for -help"), cl::Hidden);

static cl::OptionCategory SollCategory("SOLL options");

static cl::list<std::string> InputFilenames(cl::Positional,
                                            cl::desc("[<file> ...]"),
                                            cl::cat(SollCategory));

static cl::opt<InputKind> Language("lang", cl::Optional, cl::ValueRequired,
                                   cl::init(Sol),
                                   cl::values(clEnumVal(Sol, "")),
                                   cl::values(clEnumVal(Yul, "")),
                                   cl::cat(SollCategory));

static cl::opt<DeployPlatformKind> DeployPlatform(
    "deploy", cl::Optional, cl::ValueRequired, cl::init(Chain),
    cl::values(clEnumVal(Normal, "Normal case")),
    cl::values(clEnumVal(Chain, "EWASM vm not support keccak256 yet, so for "
                                "workaround use sha256 instead")),
    cl::cat(SollCategory));

static cl::opt<ActionKind> Action(
    "action", cl::Optional, cl::ValueRequired, cl::init(EmitWasm),
    cl::values(clEnumVal(ASTDump, "")), cl::values(clEnumVal(EmitAssembly, "")),
    cl::values(clEnumVal(EmitBC, "")), cl::values(clEnumVal(EmitLLVM, "")),
    cl::values(clEnumVal(EmitLLVMOnly, "")),
    cl::values(clEnumVal(EmitCodeGenOnly, "")),
    cl::values(clEnumVal(EmitObj, "")), cl::values(clEnumVal(EmitWasm, "")),
    cl::values(clEnumVal(EmitFuncSig, "")), cl::values(clEnumVal(EmitABI, "")),
    cl::values(clEnumVal(InitOnly, "")),
    cl::values(clEnumVal(ParseSyntaxOnly, "")), cl::cat(SollCategory));

static cl::opt<OptLevel> OptimizationLevel(
    cl::Optional, cl::init(O0), cl::desc("Optimization level"),
    cl::cat(SollCategory),
    cl::values(clEnumVal(O0, "No optimizations"),
               clEnumVal(O1, "Enable trivial optimizations"),
               clEnumVal(O2, "Enable default optimizations"),
               clEnumVal(O3, "Enable expensive optimizations"),
               clEnumVal(Os, "Enable default optimizations for size"),
               clEnumVal(Oz, "Enable expensive optimizations for size")));

static cl::opt<bool> Runtime("runtime", cl::desc("Generate for runtime code"),
                             cl::cat(SollCategory));

static cl::opt<TargetKind>
    Target("target", cl::Optional, cl::ValueRequired, cl::init(EWASM),
           cl::values(clEnumVal(EWASM, "Generate LLVM IR for Ewasm backend")),
           cl::values(clEnumVal(EVM, "Generate LLVM IR for EVM backend")),
           cl::cat(SollCategory));

static void printSOLLVersion(llvm::raw_ostream &OS) {
  OS << "SOLL version " << SOLL_VERSION_STRING << "\n";
}

// Parse input options
// ArrayRef 模板类可用做首选的在内存中只读元素的序列容器
// 使用 ArrayRef 的数据可被传递为固定长度的数组，其元素在内存中是连续的
// 类似于 StringRef，ArrayRef 不拥有元素数组本身，只是引用一个数组或数组的一个连续区段
bool CompilerInvocation::ParseCommandLineOptions(
    llvm::ArrayRef<const char *> Arg, DiagnosticsEngine &Diags) {

  // Print SOLL Version
  llvm::cl::SetVersionPrinter(printSOLLVersion);

  // Returns true on success. Otherwise, this will print the error message to
  // stderr and exit if \p Errs is not set (nullptr by default), or print the
  // error message to \p Errs and return false if \p Errs is provided.
  //
  // If EnvVar is not nullptr, command-line options are also parsed from the
  // environment variable named by EnvVar.  Precedence is given to occurrences
  // from argv.  This precedence is currently implemented by parsing argv after
  // the environment variable, so it is only implemented correctly for options
  // that give precedence to later occurrences.  If your program supports options
  // that give precedence to earlier occurrences, you will need to extend this
  // function to support it correctly.
  llvm::cl::ParseCommandLineOptions(Arg.size(), Arg.data());

  // ShowColors: bool var, default 1
  // StandardErrHasColors(): thhis function determines whether
  // the terminal connected to standard error supports colors.
  // If standard error is not connected to a terminal,this func returns false.
  DiagnosticOpts->ShowColors = llvm::sys::Process::StandardErrHasColors();

  // create renderer for text diagnostic by diagonstic options
  // location of TextDiagnostic class: include/soll/Frontend/TextDiagnostic.h
  DiagRenderer =
      std::make_unique<TextDiagnostic>(llvm::errs(), *DiagnosticOpts);

  // trversa all input files
  for (auto &Filename : InputFilenames) {

    // class FrontendOptions {
    // public:
    //  bool ShowHelp;
    //  bool ShowVersion;
    //  std::vector<FrontendInputFile> Inputs;
    //  InputKind Language = Sol;
    //
    //  /* The output file, if any.*/
    //  std::string OutputFile;
    //
    //  /* The frontend action to perform.*/
    //  ActionKind ProgramAction = EmitLLVM;
    // };
    // std::vector::emplace_back():
    // 在vector的结尾插入一个新的元素，位置为当前最后一个元素的右边，
    // 元素的值使用args作为参数传递给其构造函数构造。
    // 该方法可以快速有效率地在数组size范围内增长元素，
    // 除非当增长的元素个数大小超出了vector的ccapacity的时候才会发生重分配。
    // 元素由调用allocator_traits::construct 以及参数args构造
    FrontendOpts.Inputs.emplace_back(Filename);
  }
  FrontendOpts.ProgramAction = Action;
  FrontendOpts.Language = Language;
  if (Target == EWASM) {
    TargetOpts.DeployPlatform = DeployPlatform;
  }
  TargetOpts.BackendTarget = Target;

  CodeGenOpts.OptimizationLevel = OptimizationLevel;
  CodeGenOpts.Runtime = Runtime;
  return true;
}

DiagnosticOptions &CompilerInvocation::GetDiagnosticOptions() {
  return *DiagnosticOpts;
}

DiagnosticRenderer &CompilerInvocation::GetDiagnosticRenderer() {
  return *DiagRenderer;
}

} // namespace soll
