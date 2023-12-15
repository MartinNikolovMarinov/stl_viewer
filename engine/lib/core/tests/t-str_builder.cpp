#include "t-index.h"

template<typename TAllocator>
constexpr i32 initializeStrBuilderTest() {
    using StrBuilder = core::StrBuilder<TAllocator>;

    {
        StrBuilder s1;
        Assert(s1.len() == 0);
        Assert(s1.cap() == 0);
        Assert(s1.byteLen() == 0);
        Assert(s1.byteCap() == 0);
        Assert(s1.empty());
        Assert(s1.view().buff == nullptr);
    }

    {
        StrBuilder str(2);
        Assert(str.len() == 2);
        Assert(str.byteLen() == 2 * sizeof(typename StrBuilder::DataType));
        Assert(str.cap() == 3);
        Assert(str.byteCap() == 3 * sizeof(typename StrBuilder::DataType));
        Assert(!str.empty());

        Assert(str.first() == core::term_char);
        Assert(str.last() == core::term_char);
        Assert(str[0] == core::term_char);
        Assert(str[1] == core::term_char);
        Assert(str.at(0) == core::term_char);
        Assert(str.at(1) == core::term_char);
    }

    {
        StrBuilder str(2, 5);
        Assert(str.len() == 2);
        Assert(str.byteLen() == 2 * sizeof(typename StrBuilder::DataType));
        Assert(str.cap() == 5);
        Assert(str.byteCap() == 5 * sizeof(typename StrBuilder::DataType));
        Assert(!str.empty());

        Assert(str.first() == core::term_char);
        Assert(str[0] == core::term_char);
        Assert(str[1] == core::term_char);
        Assert(str.at(0) == core::term_char);
        Assert(str.at(1) == core::term_char);
    }

    {
        StrBuilder str("hello");

        Assert(str.len() == 5);
        Assert(str.byteLen() == 5 * sizeof(typename StrBuilder::DataType));
        Assert(str.cap() == 6);
        Assert(str.byteCap() == 6 * sizeof(typename StrBuilder::DataType));

        Assert(str.first() == 'h');
        Assert(str.last() == 'o');
        Assert(str[0] == 'h');
        Assert(str[1] == 'e');
        Assert(str[2] == 'l');
        Assert(str[3] == 'l');
        Assert(str[4] == 'o');
        Assert(str.at(0) == 'h');
        Assert(str.at(1) == 'e');
        Assert(str.at(2) == 'l');
        Assert(str.at(3) == 'l');
        Assert(str.at(4) == 'o');

        auto sview = str.view();
        Assert(core::cptrEq(sview.buff, "hello", sview.len()));
    }

    {
        StrBuilder str;
        auto prevCap = str.cap();

        str = "hello";

        Assert(str.len() == 5);
        Assert(str.byteLen() == 5 * sizeof(typename StrBuilder::DataType));
        Assert(str.cap() == prevCap*2 + core::cptrLen("hello") + 1);
        Assert(str.byteCap() == (prevCap*2 + core::cptrLen("hello") + 1) * sizeof(typename StrBuilder::DataType));

        Assert(str.first() == 'h');
        Assert(str.last() == 'o');
        Assert(str[0] == 'h');
        Assert(str[1] == 'e');
        Assert(str[2] == 'l');
        Assert(str[3] == 'l');
        Assert(str[4] == 'o');
        Assert(str.at(0) == 'h');
        Assert(str.at(1) == 'e');
        Assert(str.at(2) == 'l');
        Assert(str.at(3) == 'l');
        Assert(str.at(4) == 'o');

        auto sview = str.view();
        Assert(core::cptrEq(sview.buff, "hello", sview.len()));
        Assert(core::cptrLen(sview.buff) == sview.len(), "string view should be correctly null terminated");

        constexpr const char* testMsg = "should not memory leak this";
        prevCap = str.cap();
        str = testMsg;
        Assert(str.len() == 27);
        Assert(str.byteLen() == 27 * sizeof(typename StrBuilder::DataType));
        Assert(str.cap() == prevCap*2 + core::cptrLen(testMsg) + 1);
        Assert(str.byteCap() == (prevCap*2 + core::cptrLen(testMsg) + 1) * sizeof(typename StrBuilder::DataType));

        sview = str.view();
        Assert(core::cptrEq(sview.buff, testMsg, sview.len()));
        Assert(core::cptrLen(sview.buff) == sview.len(), "string view should be correctly null terminated");
    }

    return 0;
}

template<typename TAllocator>
constexpr i32 moveAndCopyStrBuilderTest() {
    using StrBuilder = core::StrBuilder<TAllocator>;

    {
        StrBuilder str1("hello");
        StrBuilder str2 = core::move(str1);
        StrBuilder str3;
        StrBuilder str4;

        Assert(str1.len() == 0);
        Assert(str1.byteLen() == 0);
        Assert(str1.cap() == 0);
        Assert(str1.byteCap() == 0);
        Assert(str1.empty());
        Assert(str1.view().buff == nullptr);

        Assert(str2.len() == 5);
        Assert(str2.byteLen() == 5 * sizeof(typename StrBuilder::DataType));
        Assert(str2.cap() == 6);
        Assert(str2.byteCap() == 6 * sizeof(typename StrBuilder::DataType));
        Assert(!str2.empty());
        Assert(core::cptrEq(str2.view().buff, "hello", str2.view().len()));
        Assert(core::cptrLen(str2.view().buff) == str2.view().len(), "string view should be correctly null terminated");

        str3 = core::move(str2);
        str4 = str3.copy();

        Assert(str2.len() == 0);
        Assert(str2.byteLen() == 0);
        Assert(str2.cap() == 0);
        Assert(str2.byteCap() == 0);
        Assert(str2.empty());
        Assert(str2.view().buff == nullptr);

        Assert(str3.len() == 5);
        Assert(str3.byteLen() == 5 * sizeof(typename StrBuilder::DataType));
        Assert(str3.cap() == 6);
        Assert(str3.byteCap() == 6 * sizeof(typename StrBuilder::DataType));
        Assert(!str3.empty());
        Assert(core::cptrEq(str3.view().buff, "hello", str3.view().len()));
        Assert(core::cptrLen(str3.view().buff) == str3.view().len(), "string view should be correctly null terminated");

        Assert(str4.len() == 5);
        Assert(str4.byteLen() == 5 * sizeof(typename StrBuilder::DataType));
        Assert(str4.cap() == 6);
        Assert(str4.byteCap() == 6 * sizeof(typename StrBuilder::DataType));
        Assert(!str4.empty());
        Assert(core::cptrEq(str4.view().buff, "hello", str4.view().len()));
        Assert(core::cptrLen(str4.view().buff) == str4.view().len(), "string view should be correctly null terminated");

        Assert(str3.view().buff != str4.view().buff);
    }

    {
        // Move assignment of self.
        StrBuilder a = "hello";
        a = core::move(a);

        Assert(a.len() == 5);
        Assert(a.byteLen() == 5 * sizeof(typename StrBuilder::DataType));
        Assert(a.cap() == 6);
        Assert(a.byteCap() == 6 * sizeof(typename StrBuilder::DataType));
        Assert(!a.empty());
        Assert(core::cptrEq(a.view().buff, "hello", a.view().len()));
    }

    {
        // Assigning char pointer when there is enough memory to fit it and when ther isn't.

        constexpr const char* testMsg = "Hello world!";
        StrBuilder a;
        auto prevCap = a.cap();

        a = testMsg;

        Assert(a.len() == core::cptrLen(testMsg));
        Assert(a.byteLen() == core::cptrLen(testMsg) * sizeof(typename StrBuilder::DataType));
        Assert(a.cap() == prevCap*2 + core::cptrLen(testMsg) + 1);
        Assert(a.byteCap() == (prevCap*2 + core::cptrLen(testMsg) + 1) * sizeof(typename StrBuilder::DataType));

        constexpr const char* testMsg2 = "This should fit!";
        prevCap = a.cap();
        a = testMsg2;
        Assert(a.len() == core::cptrLen(testMsg2));
        Assert(a.cap() == prevCap*2 + core::cptrLen(testMsg2) + 1, "should not resize when char ptr fits in current capacity");
        Assert(a.byteCap() == (prevCap*2 + core::cptrLen(testMsg2) + 1) * sizeof(typename StrBuilder::DataType));
    }

    return 0;
}

template<typename TAllocator>
i32 resizeStrBuilderTest() {
    using StrBuilder = core::StrBuilder<TAllocator>;

    StrBuilder s;
    Assert(s.len() == 0);
    Assert(s.cap() == 0);
    Assert(s.view().buff == nullptr);
    Assert(s.empty());

    s.adjustCap(10);
    Assert(s.len() == 0);
    Assert(s.cap() == 10);
    Assert(s.view().buff != nullptr);
    Assert(s.empty());

    s.adjustCap(0);
    Assert(s.len() == 0);
    Assert(s.cap() == 0);
    Assert(s.view().buff != nullptr);
    Assert(s.empty());

    return 0;
}

template<typename TAllocator>
i32 fillStrBuilderTest() {
    using StrBuilder = core::StrBuilder<TAllocator>;

    {
        StrBuilder s;
        s.fill('a'); // should not crash
        Assert(s.len() == 0);
        Assert(s.cap() == 0);
        Assert(s.view().buff == nullptr);
    }

    {
        StrBuilder s(10);

        s.fill(0);
        for (addr_size i = 0; i < s.len(); ++i) {
            Assert(s[i] == 0);
        }

        s.fill('a');
        for (addr_size i = 0; i < s.len(); ++i) {
            Assert(s[i] == 'a');
        }
    }

    {
        StrBuilder s(1, 5);

        s.fill(0);
        for (addr_size i = 0; i < s.len(); ++i) {
            Assert(s[i] == 0);
            Assert(i < 1, "Fill should use the length of the string builder, not the capacity.");
        }

        s.fill(1);
        for (addr_size i = 0; i < s.len(); ++i) {
            Assert(s[i] == 1);
            Assert(i < 1, "Fill should use the length of the string builder, not the capacity.");
        }
    }

    return 0;
}

template<typename TAllocator>
i32 appendStrBuilderTest() {
    using StrBuilder = core::StrBuilder<TAllocator>;

    {
        StrBuilder s;
        s.append('a'); // should not crash
        Assert(s.len() == 1);
        Assert(s.cap() == 2);
        Assert(s.view().buff != nullptr);
        Assert(s[0] == 'a');
    }

    {
        StrBuilder s(2);

        s.append('b');
        Assert(s.len() == 3);
        Assert(s.cap() == 6);
        Assert(s.view().buff != nullptr);
        Assert(s[0] == 0);
        Assert(s[1] == 0);
        Assert(s[2] == 'b');

        s.append('a');
        Assert(s.len() == 4);
        Assert(s.cap() == 6);
        Assert(s.view().buff != nullptr);
        Assert(s[0] == 0);
        Assert(s[1] == 0);
        Assert(s[2] == 'b');
        Assert(s[3] == 'a');
    }

    {
        StrBuilder s(1, 5);

        s.append('b');
        Assert(s.len() == 2);
        Assert(s.cap() == 5);
        Assert(s.view().buff != nullptr);
        Assert(s[0] == 0);
        Assert(s[1] == 'b');

        s.append('a');
        Assert(s.len() == 3);
        Assert(s.cap() == 5);
        Assert(s.view().buff != nullptr);
        Assert(s[0] == 0);
        Assert(s[1] == 'b');
        Assert(s[2] == 'a');
    }

    {
        // Appending past the specified capacity.
        StrBuilder s(0, 2);

        s.append('a').append('b').append('c');

        Assert(s.len() == 3);
        Assert(s.cap() == 4);
        Assert(s.view().buff != nullptr);
        Assert(s[0] == 'a');
        Assert(s[1] == 'b');
        Assert(s[2] == 'c');

        s.clear();

        Assert(s.len() == 0);
        Assert(s.cap() == 4);
        Assert(s.view().buff != nullptr);

        s.append('e').append('f').append('g');

        Assert(s.len() == 3);
        Assert(s.cap() == 4);
        Assert(s.view().buff != nullptr);
        Assert(s[0] == 'e');
        Assert(s[1] == 'f');
        Assert(s[2] == 'g');
    }

    {
        // Append whole string.
        StrBuilder s(0, 2);

        {
            auto prevCap = s.cap();
            s.append("abc");
            Assert(s.len() == 3);
            Assert(s.cap() == prevCap * 2 + core::cptrLen("abc") + 1);
            Assert(s.view().buff != nullptr);
            Assert(s[0] == 'a');
            Assert(s[1] == 'b');
            Assert(s[2] == 'c');

            s.clear();
        }
        {
            auto prevCap = s.cap();
            char buf[] = "efg";
            s.append(buf);
            Assert(s.len() == 3);
            Assert(s.cap() == prevCap, "No need to resize, there is enough space.");
            Assert(s.view().buff != buf);
            Assert(s.view().len() == 3);
            Assert(core::cptrEq(s.view().buff, buf, s.view().len()));

            s.clear();

            Assert(core::cptrEq(buf, "efg", 3), "clear must not affect the original buffer");
        }
        {
            auto prevCap = s.cap();
            const char buf[] = "higklmn";
            s.append(buf, 5);

            Assert(s.len() == 5);
            Assert(s.cap() == prevCap, "No need to resize, there is enough space.");
            Assert(s.view().buff != nullptr);
            Assert(s.view().len() == 5);
            Assert(core::cptrEq(s.view().buff, buf, s.view().len()));
            Assert(core::cptrLen(s.view().buff) == core::cptrLen("higkl"), "string view should be null terminated");

            s.append("opq");

            Assert(s.len() == 8);
            Assert(s.cap() == prevCap * 2, "Should just double the capacity.");
            Assert(s.view().buff != nullptr);
            Assert(s.view().len() == 8);
            Assert(s[0] == 'h');
            Assert(s[1] == 'i');
            Assert(s[2] == 'g');
            Assert(s[3] == 'k');
            Assert(s[4] == 'l');
            Assert(s[5] == 'o');
            Assert(s[6] == 'p');
            Assert(s[7] == 'q');
            Assert(core::cptrLen(s.view().buff) == core::cptrLen("higklopq"), "string view should be null terminated");

            Assert(core::cptrEq(buf, "higklmn", core::cptrLen(buf)), "append must not affect the original buffer");
            s.clear();
            Assert(core::cptrEq(buf, "higklmn", core::cptrLen(buf)), "clear must not affect the original buffer");
        }
    }

    return 0;
}

template<typename TAllocator>
i32 takeAndStealStrBuilderTest() {
    using StrBuilder = core::StrBuilder<TAllocator>;

    {
        // Basic take ownership.

        constexpr i32 allocatedSize = 25;
        char* data = reinterpret_cast<char*>(TAllocator::alloc(allocatedSize * sizeof(char)));
        core::memset(data, 0, allocatedSize);
        core::memset(data, 'a', allocatedSize - 1);

        StrBuilder s;
        s.reset(&data);
        Assert(data == nullptr);
        Assert(s.len() == allocatedSize - 1);
        Assert(s.cap() == allocatedSize);
        Assert(s.view().buff != nullptr);
        Assert(s.view().len() == allocatedSize - 1);

        for (addr_size i = 0; i < allocatedSize - 1; ++i) { Assert(s[i] == 'a'); }
    }

    {
        // Take ownership twice.

        constexpr i32 allocatedSize = 17;
        char* data = reinterpret_cast<char*>(TAllocator::alloc(allocatedSize * sizeof(char)));
        core::memset(data, 0, allocatedSize);
        core::memset(data, 'a', allocatedSize - 1);

        StrBuilder s(0, 5);
        s.reset(&data);
        Assert(data == nullptr);
        Assert(s.len() == allocatedSize - 1);
        Assert(s.cap() == allocatedSize );
        Assert(s.view().buff != nullptr);
        Assert(s.view().len() == allocatedSize - 1);

        for (addr_size i = 0; i < allocatedSize - 1; ++i) { Assert(s[i] == 'a'); }

        char* data2 = reinterpret_cast<char*>(TAllocator::alloc(allocatedSize * 2 * sizeof(char)));
        core::memset(data2, 0, allocatedSize * 2);
        core::memset(data2, 'b', allocatedSize * 2 - 1);

        s.reset(&data2);
        Assert(data2 == nullptr);
        Assert(s.len() == allocatedSize * 2 - 1);
        Assert(s.cap() == allocatedSize * 2);
        Assert(s.view().buff != nullptr);
        Assert(s.view().len() == allocatedSize * 2 - 1);

        for (addr_size i = 0; i < allocatedSize * 2 - 1; ++i) { Assert(s[i] == 'b'); }
    }

    {
        // Basic steal ownership.

        StrBuilder s(0, 5);
        s.append("abcde");

        char* data = s.steal();

        Assert(data != nullptr);
        Assert(core::cptrEq(data, "abcde", 5));

        Assert(s.len() == 0);
        Assert(s.cap() == 0);
        Assert(s.view().buff == nullptr);
        Assert(s.view().len() == 0);

        // The user code is now responsible for freeing the data.
        TAllocator::free(data);
    }

    return 0;
}

template<typename TAllocator>
i32 specialCasesRelatedToNullTerminationStrBuilderTest() {
    using StrBuilder = core::StrBuilder<TAllocator>;

    {
        StrBuilder s(0, 3);
        Assert(core::cptrLen(s.view().buff) == 0);
        s.append('a');
        Assert(core::cptrLen(s.view().buff) == 1);
        s.append('b');
        Assert(core::cptrLen(s.view().buff) == 2);
        s.append('c');
        Assert(core::cptrLen(s.view().buff) == 3);
        s.append('d');
        Assert(core::cptrLen(s.view().buff) == 4);
        s.append('e');
        Assert(core::cptrLen(s.view().buff) == 5);
        s.append('f');
        Assert(core::cptrLen(s.view().buff) == 6);
        s.append('g');
        Assert(core::cptrLen(s.view().buff) == 7);
    }

    {
        // Same length and cap. Cap should till be +1.
        StrBuilder s(1, 1);
        s[0] = 'b'; // set the first char.

        Assert(s.len() == 1);
        Assert(s.cap() == 2);
        s.append('a');
        Assert(s.len() == 2);
        Assert(s.cap() == 4);
        Assert(core::cptrLen(s.view().buff) == 2);
        Assert(s[0] == 'b');
        Assert(s[1] == 'a');
    }

    {
        // Appending strings that have a larger length than the capacity.
        constexpr const char* msg1 = "large-ish volume of text";
        StrBuilder s;
        s.append(msg1, core::cptrLen(msg1));
        Assert(s.len() == core::cptrLen(msg1));
        Assert(s.cap() == core::cptrLen(msg1) + 1, "Should resize with the length of the large-ish string.");
        addr_size currCap = s.cap();

        constexpr const char* msg2 = "small msg";
        s.append(msg2);
        Assert(s.len() == core::cptrLen(msg1) + core::cptrLen(msg2));
        Assert(s.cap() == currCap * 2);
    }

    {
        // Appending after clearing. Should be cheap and should maintain null termination.
        StrBuilder s;
        s.append("abc");
        Assert(s.len() == 3);
        Assert(s.cap() == 4);

        s.clear();
        Assert(s.len() == 0);
        Assert(s.cap() == 4);

        s.append('d');
        Assert(s.len() == 1);
        Assert(s.cap() == 4);
        Assert(core::cptrLen(s.view().buff) == 1);

        s.clear();
        Assert(s.len() == 0);
        Assert(s.cap() == 4);

        s.append("ab");
        Assert(s.len() == 2);
        Assert(s.cap() == 4);
        Assert(core::cptrLen(s.view().buff) == 2);
    }

    return 0;
}

template <typename TAllocator>
i32 runStrBuilderInArrayAppendRValueBUGTest() {
    using Sb = core::StrBuilder<TAllocator>;
    using SbArr = core::Arr<Sb, TAllocator>;

    SbArr arr;

    arr.append(Sb("hello"));

    Sb& rFromArr = arr[0];
    Sb newValue = rFromArr.copy();
    newValue.append(" world!");
    newValue[0] = 'H';

    arr.append(core::move(newValue));

    // NOTE: Remember that after this append the array will be reallocated and the rFromArr reference will become invalid.

    Assert(arr.len() == 2);
    Assert(arr[0].len() == 5);
    Assert(arr[1].len() == 12);

    Assert(core::cptrEq(arr[0].view().buff, "hello", 5));
    Assert(core::cptrEq(arr[1].view().buff, "Hello world!", 12));

    return 0;
}

i32 runStrBuilderTestsSuite() {
    constexpr addr_size BUFF_SIZE = core::KILOBYTE;
    char buf[BUFF_SIZE];

    core::StdAllocator::init(nullptr);
    core::StdStatsAllocator::init(nullptr);
    core::BumpAllocator::init(nullptr, buf, BUFF_SIZE);

    auto checkLeaks = []() {
        Assert(core::StdAllocator::usedMem() == 0);
        Assert(core::StdStatsAllocator::usedMem() == 0, "Memory leak detected!");
        Assert(core::BumpAllocator::usedMem() == 0);
    };

    {
        RunTest_DisplayMemAllocs(initializeStrBuilderTest<core::StdAllocator>, core::StdAllocator);
        RunTest_DisplayMemAllocs(initializeStrBuilderTest<core::StdStatsAllocator>, core::StdStatsAllocator);
        RunTest_DisplayMemAllocs(initializeStrBuilderTest<core::BumpAllocator>, core::BumpAllocator);
        core::BumpAllocator::clear();
        checkLeaks();
    }
    {
        RunTest_DisplayMemAllocs(moveAndCopyStrBuilderTest<core::StdAllocator>, core::StdAllocator);
        RunTest_DisplayMemAllocs(moveAndCopyStrBuilderTest<core::StdStatsAllocator>, core::StdStatsAllocator);
        RunTest_DisplayMemAllocs(moveAndCopyStrBuilderTest<core::BumpAllocator>, core::BumpAllocator);
        core::BumpAllocator::clear();
        checkLeaks();
    }
    {
        RunTest_DisplayMemAllocs(resizeStrBuilderTest<core::StdAllocator>, core::StdAllocator);
        RunTest_DisplayMemAllocs(resizeStrBuilderTest<core::StdStatsAllocator>, core::StdStatsAllocator);
        RunTest_DisplayMemAllocs(resizeStrBuilderTest<core::BumpAllocator>, core::BumpAllocator);
        core::BumpAllocator::clear();
        checkLeaks();
    }
    {
        RunTest_DisplayMemAllocs(fillStrBuilderTest<core::StdAllocator>, core::StdAllocator);
        RunTest_DisplayMemAllocs(fillStrBuilderTest<core::StdStatsAllocator>, core::StdStatsAllocator);
        RunTest_DisplayMemAllocs(fillStrBuilderTest<core::BumpAllocator>, core::BumpAllocator);
        core::BumpAllocator::clear();
        checkLeaks();
    }
    {
        RunTest_DisplayMemAllocs(appendStrBuilderTest<core::StdAllocator>, core::StdAllocator);
        RunTest_DisplayMemAllocs(appendStrBuilderTest<core::StdStatsAllocator>, core::StdStatsAllocator);
        RunTest_DisplayMemAllocs(appendStrBuilderTest<core::BumpAllocator>, core::BumpAllocator);
        core::BumpAllocator::clear();
        checkLeaks();
    }
    {
        RunTest_DisplayMemAllocs(takeAndStealStrBuilderTest<core::StdAllocator>, core::StdAllocator);
        RunTest_DisplayMemAllocs(takeAndStealStrBuilderTest<core::StdStatsAllocator>, core::StdStatsAllocator);
        RunTest_DisplayMemAllocs(takeAndStealStrBuilderTest<core::BumpAllocator>, core::BumpAllocator);
        core::BumpAllocator::clear();
        checkLeaks();
    }
    {
        RunTest_DisplayMemAllocs(specialCasesRelatedToNullTerminationStrBuilderTest<core::StdAllocator>, core::StdAllocator);
        RunTest_DisplayMemAllocs(specialCasesRelatedToNullTerminationStrBuilderTest<core::StdStatsAllocator>, core::StdStatsAllocator);
        RunTest_DisplayMemAllocs(specialCasesRelatedToNullTerminationStrBuilderTest<core::BumpAllocator>, core::BumpAllocator);
        core::BumpAllocator::clear();
        checkLeaks();
    }
    {
        RunTest_DisplayMemAllocs(runStrBuilderInArrayAppendRValueBUGTest<core::StdAllocator>, core::StdAllocator);
        RunTest_DisplayMemAllocs(runStrBuilderInArrayAppendRValueBUGTest<core::StdStatsAllocator>, core::StdStatsAllocator);
        RunTest_DisplayMemAllocs(runStrBuilderInArrayAppendRValueBUGTest<core::BumpAllocator>, core::BumpAllocator);
        core::BumpAllocator::clear();
        checkLeaks();
    }

    return 0;
}
