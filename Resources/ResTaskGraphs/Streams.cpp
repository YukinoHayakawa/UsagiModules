#include "Streams.hpp"

namespace usagi
{
boost::iostreams::stream<boost::iostreams::array_source>
    stream_from_string_view(std::string_view view)
{
    return { view.data(), view.size() };
}
}
