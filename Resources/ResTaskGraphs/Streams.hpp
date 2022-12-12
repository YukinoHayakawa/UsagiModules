#pragma once

#include <boost/iostreams/stream.hpp>

namespace usagi
{
boost::iostreams::stream<boost::iostreams::array_source>
    stream_from_string_view(std::string_view view);
}
