#include "../t-index.h"

constexpr const char* testDirectory = PATH_TO_TEST_DATA;

constexpr const char* testNamesTable[] = {
    "File Name With Spaces.txt",
    "VeryVeryLongFileNameThatExceedsNormalLengthsAndKeepsGoingAndGoingAndGoingSeeIfThisWorks.txt",

    "测试文件.txt",                        // (Chinese characters)
    "テストファイル.txt",                   // (Japanese characters)
    "테스트파일.txt",                       // (Korean characters)
    "ПримеренФайл.txt",                  // (Cyrillic characters - Bulgarian)
    "ΔοκιμαστικόΑρχείο.txt",             // (Greek characters)
    "Prüfungdatei.txt",                  // (German Umlaut)
    "FichierÉpreuve.txt",                // (French accented characters)
    "ArquivoDeTeste.txt",                // (Portuguese characters)
    "ArchivoDePrueba.txt",               // (Spanish characters)
    "टेस्टफाइल.txt",                        // (Hindi characters)
    "קובץבדיקה.txt",                       // (Hebrew characters)
    "ملفالاختبار.txt",                    // (Arabic characters)
    "🚀Rocket🌟Star.txt",                // (Emojis)
    "File&Name@Special#Char$.txt",       // (Special characters)
    "🎵MusicNote♫.txt",                  // (Musical symbols)
    "🧪💡ExperimentLightBulb.txt",       // (Multiple emojis)
    "🐱‍👓HackerCat.txt",                 // (Compound emoji)
    "ƒîlèNäméŵîthŠpecîålČhåråčterš.txt", // (Mixed accented characters)
};

void closeAndDeleteFile(core::FileDesc&& fd, const char* path) {
    {
        auto res = core::fileClose(fd);
        Assert(!res.hasErr(), "File closing failed");
    }
    {
        auto res = core::fileDelete(path);
        Assert(!res.hasErr(), "File deletion failed");
    }
}

struct TestPathBuilder {
    static constexpr char FILE_SEPARATOR = '/';

    char buff[256];
    addr_size dirPathLen;

    TestPathBuilder() : buff{}, dirPathLen(0) {}

    void setDirPath(const char* dirPath) {
        dirPathLen = core::cptrLen(dirPath);
        core::memcopy(buff, dirPath, dirPathLen);
    }

    void appendToDirPath(const char* dirPath) {
        buff[dirPathLen] = FILE_SEPARATOR;
        dirPathLen += 1;
        addr_size len = core::cptrLen(dirPath);
        core::memcopy(buff + dirPathLen, dirPath, len);
        dirPathLen += len;
    }

    const char* fileName() const { return buff + dirPathLen + 1; }

    const char* path() const { return buff; }

    char* filePart() { return buff + dirPathLen; }

    addr_size filePartLen() { return core::cptrLen(filePart()); }

    void setFilePart(const char* fpath) {
        addr_size len = core::cptrLen(fpath);
        core::memcopy(filePart(), fpath, len);
        filePart()[len] = '\0';
    }

    void setFileName(const char* fname) {
        filePart()[0] = FILE_SEPARATOR;
        addr_size len = core::cptrLen(fname);
        core::memcopy(filePart() + 1, fname, len);
        filePart()[len + 1] = '\0';
    }

    void resetFilePart() {
        core::memset(filePart(), 0, filePartLen());
    }

    void reset() {
        core::memset(buff, 0, sizeof(buff));
        dirPathLen = 0;
    }
};


void tryOpenFileWithMostCommonModeCombinations(const char* path) {
    //  With Default Mode:
    {
        core::FileDesc f;
        {
            auto res = core::fileOpen(path);
            Assert(!res.hasErr());
            f = core::move(res.value());
        }
        {
            auto res = core::fileClose(f);
            Assert(!res.hasErr());
        }
    }

    // With Read Mode:
    {
        core::FileDesc f;
        {
            auto res = core::fileOpen(path, core::OpenMode::Read);
            Assert(!res.hasErr());
            f = core::move(res.value());
        }
        {
            auto res = core::fileClose(f);
            Assert(!res.hasErr());
        }
    }

    // With Write Mode:
    {
        core::FileDesc f;
        {
            auto res = core::fileOpen(path, core::OpenMode::Write);
            Assert(!res.hasErr());
            f = core::move(res.value());
        }
        {
            auto res = core::fileClose(f);
            Assert(!res.hasErr());
        }
    }

    // With Read and Write Mode:
    {
        core::FileDesc f;
        {
            auto res = core::fileOpen(path, core::OpenMode::Read | core::OpenMode::Write);
            Assert(!res.hasErr());
            f = core::move(res.value());
        }
        {
            auto res = core::fileClose(f);
            Assert(!res.hasErr());
        }
    }

    // With Append Mode:
    {
        core::FileDesc f;
        {
            auto res = core::fileOpen(path, core::OpenMode::Append);
            Assert(!res.hasErr());
            f = core::move(res.value());
        }
        {
            auto res = core::fileClose(f);
            Assert(!res.hasErr());
        }
    }

    // With Append and Read Mode:
    {
        core::FileDesc f;
        {
            auto res = core::fileOpen(path, core::OpenMode::Append | core::OpenMode::Read);
            Assert(!res.hasErr());
            f = core::move(res.value());
        }
        {
            auto res = core::fileClose(f);
            Assert(!res.hasErr());
        }
    }

    // With Append and Write Mode:
    {
        core::FileDesc f;
        {
            auto res = core::fileOpen(path, core::OpenMode::Append | core::OpenMode::Write);
            Assert(!res.hasErr());
            f = core::move(res.value());
        }
        {
            auto res = core::fileClose(f);
            Assert(!res.hasErr());
        }
    }

    // With Create Mode:
    {
        core::FileDesc f;
        {
            auto res = core::fileOpen(path, core::OpenMode::Create);
            Assert(!res.hasErr());
            f = core::move(res.value());
        }
        {
            auto res = core::fileClose(f);
            Assert(!res.hasErr());
        }
    }

    // With Truncate Mode:
    {
        core::FileDesc f;
        {
            auto res = core::fileOpen(path, core::OpenMode::Truncate);
            Assert(!res.hasErr());
            f = core::move(res.value());
        }
        {
            auto res = core::fileClose(f);
            Assert(!res.hasErr());
        }
    }

    // All possible:
    {
        core::FileDesc f;
        {
            auto res = core::fileOpen(path, core::OpenMode::Read |
                                            core::OpenMode::Write |
                                            core::OpenMode::Append |
                                            core::OpenMode::Create |
                                            core::OpenMode::Truncate);
            Assert(!res.hasErr());
            f = core::move(res.value());
        }
        {
            auto res = core::fileClose(f);
            Assert(!res.hasErr());
        }
    }
};

// ############################### Tests start from here ###############################################################

i32 createAndDeleteFileTest() {
    core::FileDesc f;
    TestPathBuilder pb;

    core::memset(pb.buff, 1, 256);

    pb.setDirPath(testDirectory);
    pb.setFileName("test.txt");

    {
        auto res = core::fileOpen(pb.path(), core::OpenMode::Create);
        Assert(!res.hasErr(), "File creation failed");
        f = core::move(res.value());
    }

    closeAndDeleteFile(core::move(f), pb.path());

    return 0;
}

i32 createFilesAndAssertTheyExistTest() {
    TestPathBuilder pb;
    pb.setDirPath(testDirectory);

    executeTestTable("alignTest failed at index: ", testNamesTable, [&](auto& c, const char* cErr) {
        pb.resetFilePart();
        pb.setFileName(c);

        core::FileDesc f;

        // Open with create
        {
            auto res = core::fileOpen(pb.path(), core::OpenMode::Create);
            Assert(!res.hasErr(), cErr);
            f = core::move(res.value());
        }

        // Close
        {
            auto res = core::fileClose(f);
            Assert(!res.hasErr(), cErr);
        }

        tryOpenFileWithMostCommonModeCombinations(pb.path());

        // Delete
        {
            auto res = core::fileDelete(pb.path());
            Assert(!res.hasErr(), cErr);
        }
    });

    return 0;
}

i32 commonOpenErrorsTest() {
    {
        auto res = core::fileOpen("does not exist");
        Assert(res.hasErr(), "File should not exist");
    }
    {
        auto res = core::fileOpen("does not exist", core::OpenMode::Read);
        Assert(res.hasErr(), "File should not exist");
    }
    {
        auto res = core::fileOpen("does not exist", core::OpenMode::Write);
        Assert(res.hasErr(), "File should not exist");
    }
    {
        auto res = core::fileOpen("does not exist", core::OpenMode::Append);
        Assert(res.hasErr(), "File should not exist");
    }
    {
        auto res = core::fileOpen("does not exist", core::OpenMode::Truncate);
        Assert(res.hasErr(), "File should not exist");
    }
    return 0;
}

i32 edgeErrorCasesTest() {
    TestPathBuilder pb;
    pb.setDirPath(testDirectory);
    pb.setFileName("test.txt");

    // Files opened for reading should fail to write
    {
        core::FileDesc f;
        {
            auto res = core::fileOpen(pb.path(), core::OpenMode::Read | core::OpenMode::Create);
            Assert(!res.hasErr());
            f = core::move(res.value());
        }
        {
            auto res = core::fileWrite(f, "should fail", core::cptrLen("should fail"));
            Assert(res.hasErr());
        }
        closeAndDeleteFile(core::move(f), pb.path());
    }

    // Files oppned for writing should fail to read
    {
        core::FileDesc f;
        {
            auto res = core::fileOpen(pb.path(), core::OpenMode::Write | core::OpenMode::Create);
            Assert(!res.hasErr());
            f = core::move(res.value());
        }
        {
            char buff[256] = {};
            auto res = core::fileRead(f, buff, 256);
            Assert(res.hasErr());
        }
        closeAndDeleteFile(core::move(f), pb.path());
    }

    return 0;
}

i32 directoriesCreateRenameAndDeleteTest() {
    TestPathBuilder pb;
    pb.setDirPath(testDirectory);

    executeTestTable("alignTest failed at index: ", testNamesTable, [&](auto& c, const char* cErr) {
        pb.resetFilePart();
        pb.setFileName(c);

        // Create directory
        {
            auto res = core::dirCreate(pb.path());
            Assert(!res.hasErr(), cErr);
        }

        // Rename It
        {
            auto res = core::dirRename(pb.path(), "renamed");
            Assert(!res.hasErr(), cErr);
        }

        // Delete It
        {
            auto res = core::dirDelete("renamed");
            Assert(!res.hasErr(), cErr);
        }
    });

    return 0;
}

i32 mostBasicReadAndWriteTest() {
    TestPathBuilder pb;
    pb.setDirPath(testDirectory);
    pb.setFileName("test.txt");

    core::FileDesc f;
    {
        auto res = core::fileOpen(pb.path(), core::OpenMode::Write | core::OpenMode::Read | core::OpenMode::Create);
        Assert(!res.hasErr());
        f = core::move(res.value());
    }

    // Write "hello"
    {
        auto res = core::fileWrite(f, "hello", 5);
        Assert(!res.hasErr());
        Assert(res.value() == 5);
    }

    // Seek to beginning
    {
        auto res = core::fileSeek(f, 0, core::SeekMode::Begin);
        Assert(!res.hasErr());
        Assert(res.value() == 0, "Offset should be 0 after seeking to beginning");
    }

    // Read "hello"
    {
        char buff[6] = {};
        auto res = core::fileRead(f, buff, 5);
        Assert(!res.hasErr());
        Assert(res.value() == 5);
        Assert(core::cptrEq(buff, "hello", 5));
    }

    closeAndDeleteFile(core::move(f), pb.path());

    return 0;
}

i32 basicListDirectoryContentsTest() {
    TestPathBuilder pb;

    pb.setDirPath(testDirectory);
    pb.appendToDirPath("test_directory");

    const char* basicFileNames[] = {
        "test0.txt",
        "test1.txt",
        "test2.txt",
    };
    constexpr addr_size basicFileNamesLen = sizeof(basicFileNames) / sizeof(basicFileNames[0]);

    const char* basicDirNames[] = {
        "test_sub_directory_0",
        "test_sub_directory_1",
    };
    constexpr addr_size basicDirNamesLen = sizeof(basicDirNames) / sizeof(basicDirNames[0]);

    // Create directory
    {
        auto res = core::dirCreate(pb.path());
        Assert(!res.hasErr());
    }

    // Create the test files inside the directory
    {
        for (addr_size i = 0; i < basicFileNamesLen; ++i) {
            pb.resetFilePart();
            pb.setFileName(basicFileNames[i]);

            core::FileDesc f;
            {
                auto res = core::fileOpen(pb.path(), core::OpenMode::Create);
                Assert(!res.hasErr());
                f = core::move(res.value());
            }
            {
                auto res = core::fileClose(f);
                Assert(!res.hasErr());
            }
        }

        for (addr_size i = 0; i < basicDirNamesLen; ++i) {
            pb.resetFilePart();
            pb.setFileName(basicDirNames[i]);

            auto res = core::dirCreate(pb.path());
            Assert(!res.hasErr());
        }

        pb.resetFilePart();
    }

    // List directory contents
    {
        addr_size fileCount = 0;
        addr_size dirCount = 0;
        auto res = core::dirWalk(pb.path(), [&](const core::DirEntry& de, addr_size) -> bool {
            const char* got = de.name;

            if (de.type == core::FileType::Regular) {
                addr_off foundIdx = core::find(basicFileNames, basicFileNamesLen,
                    [&] (const char* elem, addr_off) -> bool { return core::cptrEq(elem, got, core::cptrLen(elem)); });
                Assert(foundIdx != -1, "File not found");
                fileCount++;
            }
            else if (de.type == core::FileType::Directory) {
                addr_off foundIdx = core::find(basicDirNames, basicDirNamesLen,
                    [&] (const char* elem, addr_off) -> bool { return core::cptrEq(elem, got, core::cptrLen(elem)); });
                Assert(foundIdx != -1, "Directory not found");
                dirCount++;
            }
            else {
                Assert(false, "Unknown file type");
            }

            return true;
        });

        Assert(!res.hasErr());
        Assert(fileCount == basicFileNamesLen);
        Assert(dirCount == basicDirNamesLen);
    }

    // Delete directory
    {
        constexpr addr_size BUF_SIZE = core::KILOBYTE;
        char buff[BUF_SIZE];
        core::BumpAllocator::init(nullptr, buff, BUF_SIZE);
        auto res = core::dirDeleteRec<core::BumpAllocator>(pb.path());
        Assert(!res.hasErr());
    }

    return 0;
}

// TODO: Create a table test for fileReadEntire and fileWriteEntire with stat and size checks.
//        This will make sure that the API works on a basic level.

i32 runPltFileSystemTestsSuite() {
    // TODO: At some point I should create a test folder and check if it is empty after every one of these tests.
    //       Somthing like the memory leak tests I do elsewhere.
    //       Also on starting the test suite this directory should be cleaned.

    RunTest(createAndDeleteFileTest);
    RunTest(createFilesAndAssertTheyExistTest);
    RunTest(commonOpenErrorsTest);
    RunTest(edgeErrorCasesTest);
    RunTest(directoriesCreateRenameAndDeleteTest);
    RunTest(mostBasicReadAndWriteTest);
    RunTest(basicListDirectoryContentsTest);

    return 0;
}
