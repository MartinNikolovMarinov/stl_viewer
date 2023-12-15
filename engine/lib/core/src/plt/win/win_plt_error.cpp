#include <plt/core_plt_error.h>

#include <windows.h>

namespace core {

// Error descriptions provided by the OS.
bool pltErrorDescribe(PltErrCode err, char out[MAX_SYSTEM_ERR_MSG_SIZE]) {
    wchar_t wbuf[MAX_SYSTEM_ERR_MSG_SIZE];

    auto res = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                              NULL, DWORD(err), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                              wbuf, (sizeof(wbuf) / sizeof(wchar_t)), NULL);

    if (res == 0) {
        return false;
    }

    size_t convertedChars = 0;
    wcstombs_s(&convertedChars, out, MAX_SYSTEM_ERR_MSG_SIZE, wbuf, _TRUNCATE);
    if (convertedChars == 0) {
        return false; // Conversion failed
    }

    return true;
}

} // namespace core
