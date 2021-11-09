#include "CompilerFlagBuilder.hpp"

#include <fmt/format.h>

namespace usagi
{
// ReSharper disable StringLiteralTypo
std::string CompilerFlagBuilder::triple_string()
{
    const int major = _MSC_FULL_VER / 10000000;
    const int minor = _MSC_FULL_VER / 100000 - major * 100;
    const int patch = _MSC_FULL_VER - major * 10000000 - minor * 100000;
    auto triple = fmt::format(
        "x86_64-pc-windows-msvc{}.{}.{}",
        major,
        minor,
        patch
    );
    return triple;
}

std::string CompilerFlagBuilder::msvc_version_string()
{
    const int major = _MSC_FULL_VER / 10000000;
    const int minor = _MSC_FULL_VER / 100000 - major * 100;
    const int patch = _MSC_FULL_VER - major * 10000000 - minor * 100000;
    auto version = fmt::format(
        "-fms-compatibility-version={}.{}.{}",
        major,
        minor,
        patch
    );
    return version;
}

void CompilerFlagBuilder::add_compiler_options()
{
    mOptions <<
        "-cc1 "
        "-disable-free "
        "-disable-llvm-verifier ";
}

void CompilerFlagBuilder::add_lang_options()
{
    mOptions <<
        "-std=c++20 "
        "-fexceptions -fcxx-exceptions ";
}

void CompilerFlagBuilder::add_ms_compatibility_options()
{
    mOptions <<
        "-triple " << triple_string() << " "
        "-fms-compatibility " << msvc_version_string() << " "
        "-fms-extensions "
        "-fms-volatile "
        "-fdelayed-template-parsing "
        "-fexternc-nounwind ";
#ifdef _UNICODE
    mOptions << "-D _UNICODE ";
#endif
#ifdef _UNICODE
    mOptions << "-D UNICODE ";
#endif
#ifdef _HAS_CXX17
    mOptions << "-D _HAS_CXX17 ";
#endif
#ifdef _HAS_CXX20
    mOptions << "-D _HAS_CXX20 ";
#endif
#ifdef _MT
    mOptions << "-D _MT ";
#endif
#ifdef _DLL
    mOptions << "-D _DLL ";
#else
    #error The program must be linked with the DLL version of runtime so that \
        the execution engine can resolve external symbols referred by JIT codes.
#endif
    mOptions <<
        "-fdefault-calling-conv=cdecl "
        "-gcodeview ";
#ifdef _DEBUG
    mOptions <<
        "-D_DEBUG "
        "--dependent-lib=msvcrtd ";
#else
    mOptions <<
        "--dependent-lib=msvcrt ";
#endif
    mOptions <<
        "--dependent-lib=oldnames ";
}

void CompilerFlagBuilder::add_target_options()
{
    mOptions <<
        "-target-cpu haswell " // arch:avx2 todo: use avx512?
        "-mllvm -x86-asm-syntax=intel ";
}

void CompilerFlagBuilder::add_codegen_options()
{
    mOptions <<
        "-fmath-errno "
        "-fno-rounding-math "
        "-fdeprecated-macro "
        "-fno-implicit-modules "
        "-stack-protector 2 ";
}

void CompilerFlagBuilder::add_diag_options()
{
    mOptions <<
        "-mconstructor-aliases "
        "-munwind-tables "
        "-fno-use-cxa-atexit ";
}

void CompilerFlagBuilder::add_debug_options()
{
    mOptions <<
        "-O0 -Wall -Werror -Wno-pragma-pack -fcolor-diagnostics "
        "-debug-info-kind=limited ";
}
// ReSharper restore StringLiteralTypo

void CompilerFlagBuilder::add_compulsory_options()
{
    add_compiler_options();
    add_lang_options();
    add_ms_compatibility_options();
    add_target_options();
    add_codegen_options();
    add_diag_options();
    add_debug_options();
}

std::stringstream CompilerFlagBuilder::jit_options()
{
    CompilerFlagBuilder builder;
    builder.add_compulsory_options();
    builder.mOptions <<
        "-x c++ ";
    return std::move(builder.mOptions);
}
}
