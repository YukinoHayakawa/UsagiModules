#pragma once

namespace usagi
{
/**
 * \brief
 */
struct ServiceRuntimeCompilation
{
    using ServiceT = ServiceRuntimeCompilation;

    ServiceT & get_service() { return *this; }
};
}
