#include "AssetBuilderYamlObject.hpp"

#include <Usagi/Modules/Services/Asset/AssetManager.hpp>

namespace usagi
{
AssetBuilderYamlObject::AssetBuilderYamlObject(
    AssetManager *manager,
    std::u8string locator)
    : mManager(manager)
    , mLocator(std::move(locator))
{
}

AssetHandle AssetBuilderYamlObject::hash()
{
    // todo: use canonical path
    return crc32c(0, mLocator.data(), mLocator.size());
}

MemoryRegion AssetBuilderYamlObject::build()
{
    auto source = mManager->request_asset(
        AssetPriority::NORMAL,
        true,
        mLocator
    );
    source->wait();

    // todo use vm allocator
    std::unique_ptr<ryml::Tree> tree(new ryml::Tree());

    const auto blob = source->blob();
    ryml::parse(
        c4::csubstr { static_cast<char*>(blob.base_address), blob.length },
        tree.get()
    );

    return { tree.release(), sizeof(ryml::Tree) };
}

void AssetBuilderYamlObject::free(const MemoryRegion &mem)
{
    delete static_cast<ryml::Tree*>(mem.base_address);
}
}
