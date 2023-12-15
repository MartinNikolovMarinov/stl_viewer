#pragma once

#include <core_cptr.h>
#include <plt/core_fs.h>

#include <errno.h>
#include <dirent.h>
#include <sys/types.h>

namespace core {

namespace detail {

inline DirEntry toDirEntry(const dirent& d) {
    DirEntry de = {};
    de.name = d.d_name; // d.d_name should be null terminated according to documentation.
    switch (d.d_type) {
        case DT_REG:
            de.type = FileType::Regular;
            break;
        case DT_DIR:
            de.type = FileType::Directory;
            break;
        case DT_LNK:
            de.type = FileType::Symlink;
            break;

        case DT_FIFO:
        case DT_SOCK:
        case DT_CHR:
        case DT_BLK:
        default:
            de.type = FileType::Other;
    }

    return de;
}

};

template <typename TCallback>
core::expected<PltErrCode> dirWalk(const char* path, TCallback cb) {
    if (path == nullptr) {
        return core::unexpected(PltErrCode(EINVAL));
    }

    DIR* dir = opendir(path);
    if (!dir) {
        return core::unexpected(PltErrCode(errno));
    }

    PltErrCode errCode = 0;
    struct dirent* entry;
    addr_size i = 0;

    errno = 0;

    while (true) {
        entry = readdir(dir);
        if (!entry) {
            // reached the last directory entry.
            if (errno != 0) {
                // or an error occurred.
                errCode = PltErrCode(errno);
            }

            break;
        }

        bool shouldSkip = core::cptrEq(entry->d_name, ".", 1) || core::cptrEq(entry->d_name, "..", 2);
        if (shouldSkip) {
            continue;
        }

        DirEntry de = detail::toDirEntry(*entry);
        if (!cb(de, i)) break;
        i++;
    }

    if (closedir(dir) < 0) {
        return core::unexpected(PltErrCode(errno));
    }

    if (errCode != 0) {
        return core::unexpected(errCode);
    }

    return {};
}

} // namespace core


