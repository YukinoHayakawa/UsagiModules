#pragma once

// remove debugbreak dependency
#pragma push_macro("NDEBUG")
#define NDEBUG
#include <ryml.hpp>
#pragma pop_macro("NDEBUG")
#pragma comment(lib, "c4core.lib")
#pragma comment(lib, "ryml.lib")

#include <Usagi/Module/Service/Asset/Asset.hpp>

namespace usagi
{
class AssetManager;

class AssetBuilderYamlObject
{
    AssetManager *mManager;
    std::u8string mLocator;

public:
    AssetBuilderYamlObject(AssetManager *manager, std::u8string locator);

    using OutputT = ryml::Tree;
    AssetHandle hash();
    MemoryRegion build();
    static void free(const MemoryRegion &mem);
};
static_assert(AssetBuilder<AssetBuilderYamlObject>);
}
