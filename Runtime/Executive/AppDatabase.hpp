#pragma once

#include <filesystem>

#include <fmt/format.h>

#include <Usagi/Entity/detail/ComponentFilter.hpp>

namespace usagi
{
template <typename Database>
class AppDatabase : public Database
{
    using DatabaseT = Database;
    using StorageT = typename DatabaseT::EntityPageStorageT;

    // entity database
    constexpr static std::uint16_t MAGIC_CHECK = 0xDB01;
    bool mHeaderInitialized = false;

    void init_entity_page_storage(const std::filesystem::path &base_folder)
    {
        const auto path_page = base_folder / "pages.dat";
        const bool exists = DatabaseT::entity_pages().init(path_page);
        StorageT::template push_header<MAGIC_CHECK>(DatabaseT::mMeta, exists);
        mHeaderInitialized = true;
    }

    template <Component C>
    void init_component_storage(const std::filesystem::path &base_folder)
    {
        if constexpr (!TagComponent<C>)
        {
            // todo: file name should follow a portable format
            const auto path_component =
                fmt::format("{:016x}.dat", typeid(C).hash_code());
            DatabaseT::template component_storage<C>().init(
                base_folder / path_component
            );
        }
    }

    template <Component... Cs>
    void init_component_storage_helper(
        const std::filesystem::path &base_folder,
        ComponentFilter<Cs...>)
    {
        (..., init_component_storage<Cs>(base_folder));
    }

public:
    ~AppDatabase()
    {
        if(mHeaderInitialized)
        {
            StorageT::template pop_header<MAGIC_CHECK>(DatabaseT::mMeta, true);
        }
    }

    void init_storage(const std::filesystem::path &base_folder)
    {
        init_entity_page_storage(base_folder);
        init_component_storage_helper(
            base_folder,
            typename DatabaseT::ComponentFilterT()
        );
    }
};
}
