#pragma once

#include <sstream>

namespace usagi
{
class CompilerFlagBuilder
{
    std::stringstream mOptions;

    static std::string triple_string();
    static std::string msvc_version_string();

    void add_compiler_options();
    void add_lang_options();
    void add_ms_compatibility_options();
    void add_target_options();
    void add_codegen_options();
    void add_diag_options();
    void add_debug_options();

    void add_compulsory_options();

    CompilerFlagBuilder() = default;

public:
    // todo preprpocessing requires library headers but we don't know their location
    // static std::string preprocess_options();
    // static std::string pch_options();
    static std::stringstream jit_options();
};
}
