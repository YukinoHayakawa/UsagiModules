#pragma once

#include <Usagi/Library/Memory/RawResource.hpp>
#include <Usagi/Modules/Runtime/Asset/SecondaryAssetHandler.hpp>
#include <Usagi/Runtime/WeakSingleton.hpp>

#include "SpirvBinary.hpp"

namespace usagi
{
class SahGlslCompiler : public SecondaryAssetHandler<SpirvBinary>
{
    std::string mAssetPath;
    std::string mStage;

    static void glslang_init();
    static void glslang_finalize();

    struct GlslangEnv : RawResource<>
    {
        GlslangEnv() : RawResource(&glslang_init, &glslang_finalize)
        {
        }
    };

    std::shared_ptr<GlslangEnv> mGlslangEnv;

public:
    SahGlslCompiler(std::string asset_path, std::string stage);

    constexpr static inline auto VERTEX_STAGE = "vertex";
    constexpr static inline auto FRAGMENT_STAGE = "fragment";

    std::unique_ptr<SecondaryAsset> construct() override;
    void append_features(Hasher &hasher) override;
};
}
