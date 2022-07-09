#pragma once

#include <Usagi/Modules/Runtime/HeapManager/ResourceBuilder.hpp>
#include <Usagi/Modules/Common/Color/Color.hpp>
#include <Usagi/Modules/Content/Texture/TextureStatic.hpp>
#include <Usagi/Modules/Runtime/Asset/AssetPath.hpp>

namespace usagi
{
class RbAssetImage : public ResourceBuilderDecl<
    // output 8bit rgba pixels
    TextureStatic<Color4u8>,
    const AssetPath &>
{
public:
    ResourceState construct(
        ResourceConstructDelegate<ProductT> &delegate,
        const AssetPath &path) override;
};
}
