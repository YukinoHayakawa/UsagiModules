#include "SahGlslCompiler.hpp"

#include <glslang/Public/ShaderLang.h>
#include <glslang/Include/ResourceLimits.h>
#include <glslang/SPIRV/GlslangToSpv.h>

#include <Usagi/Modules/Common/Logging/Logging.hpp>

namespace glslang
{
extern const TBuiltInResource DefaultTBuiltInResource;
}

namespace usagi
{
void SahGlslCompiler::glslang_init()
{
    glslang::InitializeProcess();
}

void SahGlslCompiler::glslang_finalize()
{
    glslang::FinalizeProcess();
}

SahGlslCompiler::SahGlslCompiler(std::string asset_path, std::string stage)
    : mAssetPath(std::move(asset_path))
    , mStage(std::move(stage))
    , mGlslangEnv(WeakSingleton<GlslangEnv>::try_lock_construct())
{
    if(mStage != "vertex" && mStage != "fragment")
        USAGI_THROW(std::runtime_error(std::format(
            "[glslang] Unsupported shader stage: {}", mStage)));
}

std::unique_ptr<SecondaryAsset> SahGlslCompiler::construct()
{
    auto f = primary_asset_async(mAssetPath);
    auto src = f.get().region;

    LOG(info, "[glslang] Compiling {} shader: {}", mStage, mAssetPath);

    using namespace glslang;
    using namespace spv;

    // Default Vulkan version
    constexpr auto client_input_semantics_version = 100;
    constexpr auto vulkan_client_version = EShTargetVulkan_1_0;
    // Not used yet, but maps to OpenGL 4.50
    // constexpr auto opengl_client_version = EShTargetOpenGL_450;
    constexpr auto target_version = EShTargetSpv_1_0;
    constexpr auto default_version = 110; // defaults to desktop version
    constexpr auto diagnostic_level = EShMsgDefault;

    // Translate shader stage value
    EShLanguage glslang_stage;
    if(mStage == "vertex") glslang_stage = EShLangVertex;
    else if(mStage == "fragment") glslang_stage = EShLangFragment;
    else USAGI_INVALID_ENUM_VALUE();

    // Create compiler objects

    // The shader object must be released after program object because the
    // program object references the shader object.
    TShader shader(glslang_stage);
    TProgram program;

    // Configure source code
    const char *strings[] = { src.as_chars() };
    const int sizes[] = { static_cast<int>(src.length) };
    shader.setStringsWithLengths(strings, sizes, 1);

    // Configure language target
    shader.setEnvInput(EShSourceGlsl, glslang_stage, EShClientVulkan,
        client_input_semantics_version);
    shader.setEnvClient(EShClientVulkan, vulkan_client_version);
    shader.setEnvTarget(EShTargetSpv, target_version);

    // Fail all include searches
    TShader::ForbidIncluder includer;
    const bool compilation_succeeded = shader.parse(
        &DefaultTBuiltInResource,
        default_version,
        false,
        diagnostic_level,
        includer
    );

    if(shader.getInfoLog()[0])
        LOG(info, "[glslang] Info:\n{}", shader.getInfoLog());
    if(shader.getInfoDebugLog()[0])
        LOG(info, "[glslang] Debug log:\n{}", shader.getInfoDebugLog());

    program.addShader(&shader);

    const auto link_succeeded = program.link(diagnostic_level)
        && program.mapIO();

    if(program.getInfoLog()[0])
        LOG(info, "Linker output:\n{}", program.getInfoLog());
    if(program.getInfoDebugLog()[0])
        LOG(info, "Linker debug output:\n{}", program.getInfoDebugLog());

    if(!compilation_succeeded || !link_succeeded)
        USAGI_THROW(std::runtime_error(std::format(
            "[glslang] Shader compilation failed: {}", mAssetPath)));

    program.buildReflection();
    LOG(info, "Reflection database:");
    program.dumpReflection();

    // Emit SPIR-V bytecodes
    std::vector<std::uint32_t> bytecodes;
    std::string diagnostics;
    SpvBuildLogger logger;
    SpvOptions spv_options;
    spv_options.generateDebugInfo = true;
    spv_options.disableOptimizer = false;
    spv_options.optimizeSize = false;

    GlslangToSpv(*program.getIntermediate(glslang_stage),
        bytecodes, &logger, &spv_options);

    return std::make_unique<SpirvBinary>(std::move(bytecodes));
}

void SahGlslCompiler::append_features(Hasher &hasher)
{
    hasher.append(mAssetPath).append(mStage);
}
}
