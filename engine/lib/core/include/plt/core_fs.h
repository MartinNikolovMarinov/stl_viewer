#pragma once

#include "core_alloc.h"
#include "core_API.h"
#include "core_arr.h"
#include "core_expected.h"
#include "core_str_builder.h"
#include "core_traits.h"
#include "core_types.h"
#include "plt/core_path.h"
#include "plt/core_plt_error.h"

// TODO: A few more functions that will be necessary:
//       - fileCopy
//       - fileMove
//       - getCurrWorkingDir
//       - changeCurrWorkingDir
//       - flush a file descriptor to disc (if possible)

namespace core {

using namespace coretypes;

struct CORE_API_EXPORT FileDesc {
    void* handle; // raw platform handle

    NO_COPY(FileDesc);

    FileDesc();

    FileDesc(FileDesc&& other);
    FileDesc& operator=(FileDesc&& other);

    bool isValid() const;
};

enum struct FileType : u8 {
    None,

    Regular,
    Directory,
    Symlink,
    Other,

    SENTINEL
};

constexpr const char* fileTypeToCptr(FileType type) {
    switch (type) {
        case FileType::None:      return "None";
        case FileType::Regular:   return "Regular";
        case FileType::Directory: return "Directory";
        case FileType::Symlink:   return "Symlink";
        case FileType::Other:     return "Other";
        case FileType::SENTINEL:  return "SENTINEL";
    }

    Assert(false, "Invalid FileType");
    return "None";
}

struct CORE_API_EXPORT FileStat {
    FileType  type;
    addr_size size;

    // TODO: [TIME] Add time for last modification once some time abstraction is in place.
};

enum struct OpenMode : u8 {
    Default  = 0,
    Read     = 1 << 0,
    Write    = 1 << 1,
    Append   = 1 << 2,
    Create   = 1 << 3,
    Truncate = 1 << 4,
};

constexpr OpenMode operator|(OpenMode lhs, OpenMode rhs) { return OpenMode(u8(lhs) | u8(rhs)); }
constexpr OpenMode operator|(OpenMode lhs, u8 rhs)       { return OpenMode(u8(lhs) | rhs); }
constexpr OpenMode operator|(u8 lhs, OpenMode rhs)       { return OpenMode(lhs | u8(rhs)); }
constexpr OpenMode operator&(OpenMode lhs, OpenMode rhs) { return OpenMode(u8(lhs) & u8(rhs)); }
constexpr OpenMode operator&(OpenMode lhs, u8 rhs)       { return OpenMode(u8(lhs) & rhs); }
constexpr OpenMode operator&(u8 lhs, OpenMode rhs)       { return OpenMode(lhs & u8(rhs)); }

enum struct SeekMode : u8 {
    Begin,
    Current,
    End
};

CORE_API_EXPORT core::expected<FileDesc, PltErrCode>  fileOpen(const char* path, OpenMode mode = OpenMode::Default);
CORE_API_EXPORT core::expected<PltErrCode>            fileClose(FileDesc& file);
CORE_API_EXPORT core::expected<PltErrCode>            fileDelete(const char* path);
CORE_API_EXPORT core::expected<PltErrCode>            fileRename(const char* path, const char* newPath);
CORE_API_EXPORT core::expected<addr_size, PltErrCode> fileWrite(FileDesc& file, const void* in, addr_size size);
CORE_API_EXPORT core::expected<addr_size, PltErrCode> fileRead(FileDesc& file, void* out, addr_size size);
CORE_API_EXPORT core::expected<addr_off, PltErrCode>  fileSeek(FileDesc& file, addr_off offset, SeekMode mode = SeekMode::Begin);
CORE_API_EXPORT core::expected<PltErrCode>            fileStat(const char* path, FileStat& out);
CORE_API_EXPORT core::expected<addr_size, PltErrCode> fileSize(FileDesc& file);

CORE_API_EXPORT core::expected<PltErrCode> dirCreate(const char* path);
CORE_API_EXPORT core::expected<PltErrCode> dirDelete(const char* path);
CORE_API_EXPORT core::expected<PltErrCode> dirRename(const char* path, const char* newPath);

// TODO2: [PERFORMANCE] The read and write entire file functions are NOT an attempt on doing performant file I/O!
//        They are just a simple way to read/write a file in one go.
//        At some point I might revisit them, and use some kind of memory mapped file I/O, or at lest do block-sized
//        read/write operations.

template <typename TAlloc>
core::expected<PltErrCode> fileReadEntire(const char* path, core::Arr<u8, TAlloc>& out) {
    using DataTypePtr = typename core::Arr<u8, TAlloc>::DataType*;

    FileDesc file;
    {
        auto res = fileOpen(path, OpenMode::Read);
        if (res.hasErr()) {
            return core::unexpected(res.err());
        }
        file = core::move(res.value());
    }

    addr_size size = 0;
    {
        auto res = fileSize(file);
        if (res.hasErr()) {
            return core::unexpected(res.err());
        }
        size = res.value();
    }

    if (size == 0) return {};

    if (out.len() < size) {
        // Deliberately avoiding zeroing out memory here!
        auto data = reinterpret_cast<DataTypePtr>(TAlloc::alloc(size * sizeof(u8)));
        out.reset(data, size);
    }
    else {
        out.clear();
    }

    {
        auto res = fileRead(file, out.data(), out.len());
        if (res.hasErr()) {
            return core::unexpected(res.err());
        }
    }

    return {};
}

template <typename TAlloc>
core::expected<PltErrCode> fileWriteEntire(const char* path, const core::Arr<u8, TAlloc>& in) {
    if (in.len() == 0) return {};

    FileDesc file;
    {
        auto res = fileOpen(path, OpenMode::Write | OpenMode::Create | OpenMode::Truncate);
        if (res.hasErr()) {
            return core::unexpected(res.err());
        }
        file = core::move(res.value());
    }

    {
        auto res = fileWrite(file, in.data(), in.len());
        if (res.hasErr()) {
            return core::unexpected(res.err());
        }
    }

    return {};
}

struct DirEntry {
    FileType type;
    const char* name;
};

template <typename TCallback>
core::expected<PltErrCode> dirWalk(const char* path, TCallback cb);

template <typename TAlloc>
core::expected<PltErrCode> dirDeleteRec(const char* path) {
    // TODO: [File/Directory Lock] When deleting a directory it is not a bad idea to lock it first,
    //       or a quick rename of the top level directory before deletion starts to prevent other
    //       processes from accessing it.

    using Sb = core::StrBuilder<TAlloc>;
    using DirectoryNames = core::Arr<Sb, TAlloc>;

    Sb fileNameTmpSb;
    DirectoryNames dirNames;
    dirNames.append(Sb(path));
    addr_size workIdx = 0;

    // Delete all files in the directory tree.
    while (workIdx < dirNames.len()) {
        PltErrCode fileDelErrCode = ERR_PLT_NONE;

        // NOTE: Using dirNames[workIdx] instead of a reference to it, because the array might be reallocated inside the
        //       walk callback.
        auto res = dirWalk(dirNames[workIdx].view().data(), [&](const DirEntry& entry, addr_size) {
            const auto& curr = dirNames[workIdx];

            if (entry.type == FileType::Directory) {
                Sb newDirName = curr.copy();
                newDirName.append(PATH_SEPARATOR);
                newDirName.append(entry.name);
                dirNames.append(core::move(newDirName));
            }
            else {
                fileNameTmpSb.clear();
                fileNameTmpSb.append(curr.view());
                fileNameTmpSb.append(PATH_SEPARATOR);
                fileNameTmpSb.append(entry.name);
                const char* fullFilePath = fileNameTmpSb.view().data();
                if (auto dres = fileDelete(fullFilePath); dres.hasErr()) {
                    fileDelErrCode = core::move(dres.err());
                    return false;
                }
            }

            return true;
        });

        if (res.hasErr()) {
            return core::unexpected(res.err());
        }

        if (fileDelErrCode != ERR_PLT_NONE) {
            return core::unexpected(fileDelErrCode);
        }

        workIdx++;
    }

    // All directories should be empty by now, and thus deletable.
    for (addr_size i = dirNames.len(); i > 0; i--) {
        const char* dirPath = dirNames[i - 1].view().data();
        if (auto dres = dirDelete(dirPath); dres.hasErr()) {
            return core::unexpected(dres.err());
        }
    }

    return {};
}

} // namespace core

#if defined(OS_WIN) && OS_WIN == 1
    #include <plt/win/win_fs.h>
#elif defined(OS_LINUX) && OS_LINUX == 1
    #include <plt/unix/unix_fs.h>
#elif defined(OS_MAC) && OS_MAC == 1
    #include <plt/unix/unix_fs.h>
#endif
