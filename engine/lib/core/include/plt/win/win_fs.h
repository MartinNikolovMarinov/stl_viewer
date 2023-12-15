#pragma once

#include <core_cptr.h>
#include <core_str_builder.h>
#include <plt/core_fs.h>

#include <windows.h>

namespace core {

namespace detail {

inline DirEntry toDirEntry(const WIN32_FIND_DATAA& findData) {
    DirEntry de = {};
    de.name = findData.cFileName;
    switch (findData.dwFileAttributes) {
        case FILE_ATTRIBUTE_NORMAL:
        case FILE_ATTRIBUTE_ARCHIVE:
        case FILE_ATTRIBUTE_COMPRESSED:
        case FILE_ATTRIBUTE_DEVICE:
        case FILE_ATTRIBUTE_ENCRYPTED:
        case FILE_ATTRIBUTE_HIDDEN:
        case FILE_ATTRIBUTE_INTEGRITY_STREAM:
        case FILE_ATTRIBUTE_NOT_CONTENT_INDEXED:
        case FILE_ATTRIBUTE_NO_SCRUB_DATA:
        case FILE_ATTRIBUTE_OFFLINE:
        case FILE_ATTRIBUTE_READONLY:
        case FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS:
        case FILE_ATTRIBUTE_RECALL_ON_OPEN:
        case FILE_ATTRIBUTE_SPARSE_FILE:
        case FILE_ATTRIBUTE_SYSTEM:
        case FILE_ATTRIBUTE_TEMPORARY:
        case FILE_ATTRIBUTE_VIRTUAL:
            de.type = FileType::Regular;
            break;
        case FILE_ATTRIBUTE_DIRECTORY:
            de.type = FileType::Directory;
            break;
        case FILE_ATTRIBUTE_REPARSE_POINT:
            de.type = FileType::Symlink;
            break;

        default:
            de.type = FileType::Other;
    }

    return de;
}

} // namespace detail

template <typename TCallback>
core::expected<PltErrCode> dirWalk(const char* path, TCallback cb) {
    core::StrBuilder<> pathSb(path);
    if (pathSb.empty()) {
        return core::unexpected(PltErrCode(EINVAL));
    }

    pathSb.append("/*");

    WIN32_FIND_DATAA findData;
    HANDLE findHandle = FindFirstFileA(pathSb.view().data(), &findData);
    if (findHandle == INVALID_HANDLE_VALUE) {
        return core::unexpected(PltErrCode(GetLastError()));
    }

    PltErrCode errCode = 0;
    addr_size i = 0;

    while (true) {
        if (!FindNextFileA(findHandle, &findData)) {
            if (GetLastError() != ERROR_NO_MORE_FILES) {
                errCode = PltErrCode(GetLastError());
            }

            break;
        }

        bool shouldSkip = core::cptrEq(findData.cFileName, ".", 1) || core::cptrEq(findData.cFileName, "..", 2);
        if (shouldSkip) {
            continue;
        }

        DirEntry de = detail::toDirEntry(findData);
        if (!cb(de, i)) break;
        i++;
    }

    if (FindClose(findHandle) <= 0) {
        return core::unexpected(PltErrCode(GetLastError()));
    }

    if (errCode != 0) {
        return core::unexpected(errCode);
    }

    return {};
}

} // namespace core
