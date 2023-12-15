#include <plt/core_plt_error.h>

#include <core_mem.h>
#include <core_cptr.h>

#include <string.h>

namespace core {

bool pltErrorDescribe(PltErrCode err, char out[MAX_SYSTEM_ERR_MSG_SIZE]) {
    const char* errCptr = strerror(i32(err));
    if (errCptr == nullptr) {
        errCptr = "Unknown error";
    }
    addr_size len = core::cptrLen(errCptr);
    if (len > MAX_SYSTEM_ERR_MSG_SIZE) {
        len = MAX_SYSTEM_ERR_MSG_SIZE;
    }
    core::memcopy(out, errCptr, len);
    out[len] = '\0';
    return true;
}

} // namespace core
