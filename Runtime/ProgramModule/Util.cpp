#include "Util.hpp"

#include <llvm/Demangle/Demangle.h>
#include <llvm/Demangle/MicrosoftDemangle.h>

#include <Usagi/Runtime/ErrorHandling.hpp>

namespace usagi
{
std::string demangle_msvc(std::string_view mangled)
{
    using namespace llvm;
    using namespace ms_demangle;

    Demangler D;
    OutputStream S;

    StringView Name { mangled.data(), mangled.size() };
    SymbolNode *AST = D.parse(Name);

    // We don't want anything outside the plain type definition when declaring
    // variables in JIT codes.
    const auto flags = static_cast<OutputFlags>(
        OF_NoCallingConvention |
        OF_NoTagSpecifier |
        OF_NoAccessSpecifier |
        OF_NoMemberType |
        OF_NoReturnType
    );

    char *buf = nullptr;
    int internal_status = demangle_success;
    if(D.Error)
        internal_status = demangle_invalid_mangled_name;
    else if(!initializeOutputStream(nullptr, nullptr, S, 1024))
        internal_status = demangle_memory_alloc_failure;
    else {
        AST->output(S, flags);
        S += '\0';
        buf = S.getBuffer();
    }

    USAGI_ASSERT_THROW(
        internal_status == demangle_success,
        std::runtime_error("Invalid mangled name.")
    );

    std::string demangled = buf;
    free(buf);

    // If it starts with '.' it comes from typeid.raw_name(). Remove the part
    // we don't want from the result.
    if(mangled.size() > 1 && mangled[0] == '.')
    {
        demangled.resize(
            demangled.size() - sizeof("`RTTI Type Descriptor Name'")
        );
    }
    return demangled;
}

std::string demangle_typeid(const std::type_info &ti)
{
#ifdef _MSC_VER
    return demangle(ti.raw_name());
#else
    USAGI_UNREACHABLE("unimplemented");
#endif
}

std::string demangle(std::string_view mangled)
{
#ifdef _MSC_VER
    return demangle_msvc(mangled);
#else
    USAGI_UNREACHABLE("Not implemented.");
#endif
}
}
