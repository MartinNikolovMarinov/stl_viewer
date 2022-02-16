#include "../lib/core/src/core.h"

#include <memory>
#include <string>
#include <stdexcept>

namespace stlview
{
    /**
     * Shim for string_format for versions of c++ below 20.
    */
    template<typename ... Args>
    std::string string_format( const std::string& format, Args ... args )
    {
        size_t size = snprintf(nullptr, 0, format.c_str(), args ... ) + 1; // Extra space for '\0'
        if(size <= 0) {
            Panic("Error during formatting.");
        }
        std::unique_ptr<char[]> buf( new char[ size ] );
        snprintf(buf.get(), size, format.c_str(), args ... );
        auto ret = std::string(buf.get(), buf.get() + size - 1 ); // We don't want the '\0' inside
        return ret;
    }

} // namespace stlview
