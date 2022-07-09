#include "RbAssetImage.hpp"

#include <webp/decode.h>

#include <Usagi/Modules/Resources/ResScratchBuffer/RbScratchBuffer.hpp>
#include <Usagi/Modules/Runtime/Asset/RbAssetMemoryView.hpp>
#include <Usagi/Modules/Runtime/HeapManager/HeapManager.hpp>

namespace usagi
{
ResourceState RbAssetImage::construct(
    ResourceConstructDelegate<ProductT> &delegate,
    const AssetPath &path)
{
    // fetch the raw asset. ref count is maintained by the returned pointer.
    const auto memory = delegate.resource<RbAssetMemoryView>(path).await();

    // validate image format
    int img_x, img_y;
    const int suc = WebPGetInfo(
        memory->as<std::uint8_t>(),
        memory->size(),
        &img_x,
        &img_y
    );
    USAGI_ASSERT_THROW(suc, std::runtime_error("Invalid WebP image."));

    const std::size_t num_pixels = img_x * img_y;
    const std::size_t buf_size = num_pixels * sizeof(ProductT::PixelT);

    // allocate a scratch buffer
    const auto staging_buf = delegate.resource_transient<
        RbScratchBuffer<ProductT::PixelT>>(
        delegate.make_unique_descriptor(),
        num_pixels
    ).get();

    const auto out = WebPDecodeRGBAInto(
        memory->as<std::uint8_t>(),
        memory->size(),
        reinterpret_cast<uint8_t *>(staging_buf->get()),
        buf_size,
        sizeof(ProductT::PixelT) * img_x
    );
    USAGI_ASSERT_THROW(out, std::runtime_error("Failed to decode WebP image."));

    delegate.emplace(*staging_buf, Vector2u32 { img_x, img_y });

    return ResourceState::READY;
}
}
