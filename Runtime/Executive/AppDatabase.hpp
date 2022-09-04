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
    // PagedStorageFileBacked?
    using EntityPageStorageT = typename DatabaseT::EntityPageStorageT;

    // entity database
    constexpr static std::uint16_t MAGIC_CHECK = 0xABBE'5BCEFAFF'DB01;

    std::uint64_t mHeaderOffset = -1;

    using HeaderT = typename Database::Meta;

    HeaderT & header() override
    {
        return *EntityPageStorageT::template header<HeaderT>(mHeaderOffset);
    }

    // todo: make this more neat?
    void init_entity_page_storage(const std::filesystem::path &base_folder)
    {
        const auto path_page = base_folder / "pages.dat";
        DatabaseT::entity_pages().init(path_page);
        std::tie(mHeaderOffset, std::ignore) =
            EntityPageStorageT::template init_or_restore_header<
                MAGIC_CHECK, HeaderT>();
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
