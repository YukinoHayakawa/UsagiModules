#pragma once

#include <string_view>

#include <Usagi/Runtime/Memory/Region.hpp>

namespace usagi
{
/**
 * \brief Provide access to raw binary data of assets.
 */
class AssetSource
{
public:
    AssetSource() = default;
    virtual ~AssetSource() = default;

    AssetSource(const AssetSource &other) = delete;
    AssetSource(AssetSource &&other) noexcept = default;
    AssetSource & operator=(const AssetSource &other) = delete;
    AssetSource & operator=(AssetSource &&other) noexcept = default;

    /**
     * \brief Checks whether this source has the specified asset.
     * \param name
     * \return
     */
    virtual bool has_asset(std::u8string_view name) const = 0;

    /**
     * \brief Load specified asset into memory. Returns the raw memory
     * referring to the loaded asset and its size, which could be used as
     * an indicator for memory consumption.
     * \param name
     * \return
     */
    virtual MemoryRegion load(std::u8string_view name) = 0;

    /**
     * \brief Remove the asset from memory and invalidate any memory reference
     * to it. The memory is released.
     * \param name
     */
    virtual void unload(std::u8string_view name) = 0;
};
}
