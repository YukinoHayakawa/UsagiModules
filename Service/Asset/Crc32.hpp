#pragma once

#include <cstdint>
#include <cstddef>

// bug msvc specific header
// _mm_crc32_u8
#include <intrin.h>

namespace usagi
{
// some refs:
// https://blog.jiubao.org/2012/07/sse42-crc32c.html
// https://stackoverflow.com/questions/15175324/crc32c-sse-vs-boost
// https://stackoverflow.com/questions/15752770/mm-crc32-u64-poorly-defined
// https://github.com/troydhanson/uthash/issues/96
// https://code.google.com/archive/p/sse-intrinsics/
// https://code.google.com/archive/p/sse-intrinsics/wikis/PmovIntrinsicBug.wiki
// http://pzemtsov.github.io/2015/11/21/crc32-intrinsic-java-or-c-with-jni.html
inline std::uint32_t crc32c(
    std::uint32_t init,
    const void *data,
    const std::size_t len)
{
    // todo optimization
    for(std::size_t i = 0; i < len; ++i)
    {
        init = _mm_crc32_u8(init, static_cast<const char*>(data)[i]);
    }
    return init;
}

template <typename T>
std::uint32_t crc32c(const std::uint32_t init, T &&value)
{
    return crc32c(init, &value, sizeof T);
}
}
