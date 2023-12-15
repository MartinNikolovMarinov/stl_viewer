#include "t-index.h"

const char* __cmd_flag_test_input_1[] = {
    "das.exe", "adsad", "zxczxc",
    "-b", "asd",
    "-basd", "-123",
    "--bb",
    "-cc", "asdasd",
    "--bb",
    "--zz"
};
const addr_size __cmd_flag_test_input_1_len = sizeof(__cmd_flag_test_input_1) / sizeof(__cmd_flag_test_input_1[0]);

const char* __cmd_flag_test_input_2[] = {
    "name_of_program",
    "-bool-1 ", " t",
    "-bool-2   ", "T",
    "-bool-3  ", " true",
    "  -bool-4", "  TRUE",
    " -bool-5  ", "True  ",
    "    -bool-6", "1",
    "-bool-7", "false",
    "-bool-8", "zxcasdasd",

    " -int32", "0004",
    "-int64  ", "  -13",
    "\t-uint32", "19 ",
    "   -uint64", "\t99asad",

    "-string", "  banicata   fsa  ",

    "-float32-1", "  1.2",
    "-float32-2", "  .5. ",
    "-float32-3", " 1. ",
    "-float32-4", "  -1.2cvxc",
    "-float64-5", " 1.2.",
    "-float64-6", "7...  ",
    "-float64-7", "-1.2",
    "-float64-8", "00012.000005",
};
const addr_size __cmd_flag_test_input_2_len = sizeof(__cmd_flag_test_input_2) / sizeof(__cmd_flag_test_input_2[0]);

template <typename TAllocator>
i32 cmdFlagParserSymbolParsingTest() {
    using CmdFlagParser = core::CmdFlagParser<TAllocator>;
    using core::sv;

    CmdFlagParser parser;

    auto ret = parser.parse(__cmd_flag_test_input_1_len, __cmd_flag_test_input_1);
    Assert(!ret.hasErr());

    auto& symbols = parser.parsedSymbols();

    Assert(symbols.len() == __cmd_flag_test_input_1_len);

    // Check the program name:
    {
        Assert(symbols[0].type == CmdFlagParser::ParsedSymbolType::ProgramName);
        Assert(symbols[0].value.eq(sv("das.exe")));
        Assert(parser.programName().eq(sv("das.exe")));
    }

    // Check the program argument list:
    {
        Assert(symbols[1].type == CmdFlagParser::ParsedSymbolType::Argument);
        Assert(symbols[1].value.eq(sv("adsad")));
        Assert(symbols[2].type == CmdFlagParser::ParsedSymbolType::Argument);
        Assert(symbols[2].value.eq(sv("zxczxc")));

        parser.arguments([](core::StrView arg, addr_size idx) -> bool {
            switch (idx) {
                case 0: Assert(arg.eq(sv("adsad"))); break;
                case 1: Assert(arg.eq(sv("zxczxc"))); break;
                default: Assert(false); break;
            }
            return true;
        });
    }

    // Check the rest:
    {
        Assert(symbols[3].type == CmdFlagParser::ParsedSymbolType::FlagName);
        Assert(symbols[3].value.eq(sv("b")));
        Assert(symbols[4].type == CmdFlagParser::ParsedSymbolType::FlagValue);
        Assert(symbols[4].value.eq(sv("asd")));
        Assert(symbols[5].type == CmdFlagParser::ParsedSymbolType::FlagName);
        Assert(symbols[5].value.eq(sv("basd")));
        Assert(symbols[6].type == CmdFlagParser::ParsedSymbolType::FlagValue);
        Assert(symbols[6].value.eq(sv("-123")));
        Assert(symbols[7].type == CmdFlagParser::ParsedSymbolType::OptionName);
        Assert(symbols[7].value.eq(sv("bb")));
        Assert(symbols[8].type == CmdFlagParser::ParsedSymbolType::FlagName);
        Assert(symbols[8].value.eq(sv("cc")));
        Assert(symbols[9].type == CmdFlagParser::ParsedSymbolType::FlagValue);
        Assert(symbols[9].value.eq(sv("asdasd")));
        Assert(symbols[10].type == CmdFlagParser::ParsedSymbolType::OptionName);
        Assert(symbols[10].value.eq(sv("bb")));
        Assert(symbols[11].type == CmdFlagParser::ParsedSymbolType::OptionName);
        Assert(symbols[11].value.eq(sv("zz")));

        // Check the flags:
        {
            i32 flagsChecker[] = { 0, 0, 0 };
            parser.flags([&flagsChecker](core::StrView flag, core::StrView value) -> bool {
                if (flag.eq(sv("b"))) {
                    Assert(value.eq(sv("asd")));
                    flagsChecker[0]++;
                }
                else if (flag.eq(sv("basd"))) {
                    Assert(value.eq(sv("-123")));
                    flagsChecker[1]++;
                }
                else if (flag.eq(sv("cc"))) {
                    Assert(value.eq(sv("asdasd")));
                    flagsChecker[2]++;
                }
                else {
                    Assert(false, "Parsed somthing that shouldn't have been parsed!");
                }

                return true;
            });

            // Check that all flags were parsed and iterated over exactly once:
            for (auto v : flagsChecker) {
                Assert(v == 1);
            }
        }

        // Check the options:
        {
            i32 optionsChecker[] = { 0, 0 };
            parser.options([&optionsChecker](core::StrView option) -> bool {
                if (option.eq(sv("bb"))) {
                    optionsChecker[0]++;
                }
                else if (option.eq(sv("zz"))) {
                    optionsChecker[1]++;
                }
                else {
                    Assert(false, "Parsed somthing that shouldn't have been parsed!");
                }

                return true;
            });

            // Check that all options were parsed and iterated over exactly once:
            Assert(optionsChecker[0] == 2); // this should be set twice.
            Assert(optionsChecker[1] == 1);
        }
    }

    return 0;
}

template <typename TAllocator>
i32 cmdFlagParserSymbolParsingLongerTest() {
    using CmdFlagParser = core::CmdFlagParser<TAllocator>;
    using core::sv;

    CmdFlagParser parser;

    auto ret = parser.parse(__cmd_flag_test_input_2_len, __cmd_flag_test_input_2);
    Assert(!ret.hasErr());

    auto& symbols = parser.parsedSymbols();

    Assert(symbols.len() == __cmd_flag_test_input_2_len);

    // Check the program name:
    {
        Assert(symbols[0].type == CmdFlagParser::ParsedSymbolType::ProgramName);
        Assert(symbols[0].value.eq(sv("name_of_program")));
        Assert(parser.programName().eq(sv("name_of_program")));
    }

    // Check argument list:
    {
        i32 none = 0;
        parser.arguments([&none](core::StrView, addr_size) -> bool {
            none++;
            return true;
        });
        Assert(none == 0);
    }

    // Check the flags:
    {
        Assert(symbols[1].type == CmdFlagParser::ParsedSymbolType::FlagName);
        Assert(symbols[1].value.eq(sv("bool-1")));
        Assert(symbols[2].type == CmdFlagParser::ParsedSymbolType::FlagValue);
        Assert(symbols[2].value.eq(sv("t")));
        Assert(symbols[3].type == CmdFlagParser::ParsedSymbolType::FlagName);
        Assert(symbols[3].value.eq(sv("bool-2")));
        Assert(symbols[4].type == CmdFlagParser::ParsedSymbolType::FlagValue);
        Assert(symbols[4].value.eq(sv("T")));
        Assert(symbols[5].type == CmdFlagParser::ParsedSymbolType::FlagName);
        Assert(symbols[5].value.eq(sv("bool-3")));
        Assert(symbols[6].type == CmdFlagParser::ParsedSymbolType::FlagValue);
        Assert(symbols[6].value.eq(sv("true")));
        Assert(symbols[7].type == CmdFlagParser::ParsedSymbolType::FlagName);
        Assert(symbols[7].value.eq(sv("bool-4")));
        Assert(symbols[8].type == CmdFlagParser::ParsedSymbolType::FlagValue);
        Assert(symbols[8].value.eq(sv("TRUE")));
        Assert(symbols[9].type == CmdFlagParser::ParsedSymbolType::FlagName);
        Assert(symbols[9].value.eq(sv("bool-5")));
        Assert(symbols[10].type == CmdFlagParser::ParsedSymbolType::FlagValue);
        Assert(symbols[10].value.eq(sv("True")));
        Assert(symbols[11].type == CmdFlagParser::ParsedSymbolType::FlagName);
        Assert(symbols[11].value.eq(sv("bool-6")));
        Assert(symbols[12].type == CmdFlagParser::ParsedSymbolType::FlagValue);
        Assert(symbols[12].value.eq(sv("1")));
        Assert(symbols[13].type == CmdFlagParser::ParsedSymbolType::FlagName);
        Assert(symbols[13].value.eq(sv("bool-7")));
        Assert(symbols[14].type == CmdFlagParser::ParsedSymbolType::FlagValue);
        Assert(symbols[14].value.eq(sv("false")));
        Assert(symbols[15].type == CmdFlagParser::ParsedSymbolType::FlagName);
        Assert(symbols[15].value.eq(sv("bool-8")));
        Assert(symbols[16].type == CmdFlagParser::ParsedSymbolType::FlagValue);
        Assert(symbols[16].value.eq(sv("zxcasdasd")));
        Assert(symbols[17].type == CmdFlagParser::ParsedSymbolType::FlagName);
        Assert(symbols[17].value.eq(sv("int32")));
        Assert(symbols[18].type == CmdFlagParser::ParsedSymbolType::FlagValue);
        Assert(symbols[18].value.eq(sv("0004")));
        Assert(symbols[19].type == CmdFlagParser::ParsedSymbolType::FlagName);
        Assert(symbols[19].value.eq(sv("int64")));
        Assert(symbols[20].type == CmdFlagParser::ParsedSymbolType::FlagValue);
        Assert(symbols[20].value.eq(sv("-13")));
        Assert(symbols[21].type == CmdFlagParser::ParsedSymbolType::FlagName);
        Assert(symbols[21].value.eq(sv("uint32")));
        Assert(symbols[22].type == CmdFlagParser::ParsedSymbolType::FlagValue);
        Assert(symbols[22].value.eq(sv("19")));
        Assert(symbols[23].type == CmdFlagParser::ParsedSymbolType::FlagName);
        Assert(symbols[23].value.eq(sv("uint64")));
        Assert(symbols[24].type == CmdFlagParser::ParsedSymbolType::FlagValue);
        Assert(symbols[24].value.eq(sv("99asad")));
        Assert(symbols[25].type == CmdFlagParser::ParsedSymbolType::FlagName);
        Assert(symbols[25].value.eq(sv("string")));
        Assert(symbols[26].type == CmdFlagParser::ParsedSymbolType::FlagValue);
        Assert(symbols[26].value.eq(sv("banicata   fsa")));
        Assert(symbols[27].type == CmdFlagParser::ParsedSymbolType::FlagName);
        Assert(symbols[27].value.eq(sv("float32-1")));
        Assert(symbols[28].type == CmdFlagParser::ParsedSymbolType::FlagValue);
        Assert(symbols[28].value.eq(sv("1.2")));
        Assert(symbols[29].type == CmdFlagParser::ParsedSymbolType::FlagName);
        Assert(symbols[29].value.eq(sv("float32-2")));
        Assert(symbols[30].type == CmdFlagParser::ParsedSymbolType::FlagValue);
        Assert(symbols[30].value.eq(sv(".5.")));
        Assert(symbols[31].type == CmdFlagParser::ParsedSymbolType::FlagName);
        Assert(symbols[31].value.eq(sv("float32-3")));
        Assert(symbols[32].type == CmdFlagParser::ParsedSymbolType::FlagValue);
        Assert(symbols[32].value.eq(sv("1.")));
        Assert(symbols[33].type == CmdFlagParser::ParsedSymbolType::FlagName);
        Assert(symbols[33].value.eq(sv("float32-4")));
        Assert(symbols[34].type == CmdFlagParser::ParsedSymbolType::FlagValue);
        Assert(symbols[34].value.eq(sv("-1.2cvxc")));
        Assert(symbols[35].type == CmdFlagParser::ParsedSymbolType::FlagName);
        Assert(symbols[35].value.eq(sv("float64-5")));
        Assert(symbols[36].type == CmdFlagParser::ParsedSymbolType::FlagValue);
        Assert(symbols[36].value.eq(sv("1.2.")));
        Assert(symbols[37].type == CmdFlagParser::ParsedSymbolType::FlagName);
        Assert(symbols[37].value.eq(sv("float64-6")));
        Assert(symbols[38].type == CmdFlagParser::ParsedSymbolType::FlagValue);
        Assert(symbols[38].value.eq(sv("7...")));
        Assert(symbols[39].type == CmdFlagParser::ParsedSymbolType::FlagName);
        Assert(symbols[39].value.eq(sv("float64-7")));
        Assert(symbols[40].type == CmdFlagParser::ParsedSymbolType::FlagValue);
        Assert(symbols[40].value.eq(sv("-1.2")));
        Assert(symbols[41].type == CmdFlagParser::ParsedSymbolType::FlagName);
        Assert(symbols[41].value.eq(sv("float64-8")));
        Assert(symbols[42].type == CmdFlagParser::ParsedSymbolType::FlagValue);
        Assert(symbols[42].value.eq(sv("00012.000005")));
    }

    return 0;
}

template <typename TAllocator>
i32 cmdFlagParserBasicErrorsTest() {
    using CmdFlagParser = core::CmdFlagParser<TAllocator>;

    CmdFlagParser parser;

    {
        const char* input[] = { "lies" };
        auto ret = parser.parse(CmdFlagParser::MAX_ARG_COUNT + 1, input);
        Assert(ret.hasErr());
        Assert(ret.err() == CmdFlagParser::ParseError::ArgumentListTooLong);
    }
    {
        auto ret = parser.parse(5, nullptr);
        Assert(ret.hasErr());
        Assert(ret.err() == CmdFlagParser::ParseError::NothingToParse);
    }
    {
        const char* input[] = { "lies" };
        auto ret = parser.parse(0, input);
        Assert(ret.hasErr());
        Assert(ret.err() == CmdFlagParser::ParseError::NothingToParse);
    }
    {
        const char* input[] = { nullptr };
        constexpr addr_size len = sizeof(input) / sizeof(input[0]);
        auto ret = parser.parse(len, input);
        Assert(ret.hasErr());
        Assert(ret.err() == CmdFlagParser::ParseError::InvalidArgvElement);
    }
    {
        const char* input[] = { "a", "asd", nullptr };
        constexpr addr_size len = sizeof(input) / sizeof(input[0]);
        auto ret = parser.parse(len, input);
        Assert(ret.hasErr());
        Assert(ret.err() == CmdFlagParser::ParseError::InvalidArgvElement);
    }
    {
        const char* input[] = { "a", "-a", nullptr };
        constexpr addr_size len = sizeof(input) / sizeof(input[0]);
        auto ret = parser.parse(len, input);
        Assert(ret.hasErr());
        Assert(ret.err() == CmdFlagParser::ParseError::InvalidArgvElement);
    }
    {
        // Values must be associated with flags not options.
        const char* input[] = { "a", "--b", "c" };
        constexpr addr_size len = sizeof(input) / sizeof(input[0]);
        auto ret = parser.parse(len, input);
        Assert(ret.hasErr());
        Assert(ret.err() == CmdFlagParser::ParseError::IncorrectValue);
    }
    {
        // Multiple values for a single flag are not allowed.
        const char* input[] = { "a", "-b", "c", "d", "--f" };
        constexpr addr_size len = sizeof(input) / sizeof(input[0]);
        auto ret = parser.parse(len, input);
        Assert(ret.hasErr());
        Assert(ret.err() == CmdFlagParser::ParseError::IncorrectValue);
    }
    {
        // Dangling flag.
        const char* input[] = { "a", "-b", "c", "-d" };
        constexpr addr_size len = sizeof(input) / sizeof(input[0]);
        auto ret = parser.parse(len, input);
        Assert(ret.hasErr());
        Assert(ret.err() == CmdFlagParser::ParseError::FlagWithoutValue);
    }
    {
        // Dangling flag.
        const char* input[] = { "a", "-b" };
        constexpr addr_size len = sizeof(input) / sizeof(input[0]);
        auto ret = parser.parse(len, input);
        Assert(ret.hasErr());
        Assert(ret.err() == CmdFlagParser::ParseError::FlagWithoutValue);
    }

    return 0;
}

template <typename TAllocator>
i32 cmdFlagParserDoubleParsingTest() {
    using CmdFlagParser = core::CmdFlagParser<TAllocator>;

    CmdFlagParser parser;

    {
        const char* input[] = { "a", "-b", "c" };
        auto res = parser.parse(3, input);
        Assert(!res.hasErr());
        Assert(parser.parsedSymbols().len() == 3);
    }
    {
        const char* input[] = { "a", "-b", "c", "--f", "-d", "e" };
        auto res = parser.parse(6, input);
        Assert(!res.hasErr());
        Assert(parser.parsedSymbols().len() == 6);
    }

    return 0;
}

template <typename TAllocator>
i32 cmdFlagParserFriendlyInputMatchingTest() {
    using CmdFlagParser = core::CmdFlagParser<TAllocator>;
    using core::sv;

    CmdFlagParser parser;

    i32 a = 0;
    i64 b = 0;
    u32 c = 0;
    u64 d = 0;

    parser.setFlagInt32(&a, sv("int32"));
    parser.setFlagInt64(&b, sv("int64"));
    parser.setFlagUint32(&c, sv("uint32"));
    parser.setFlagUint64(&d, sv("uint64"));

    char* cptrArg = nullptr;
    parser.setFlagCptr(&cptrArg, sv("string"));

    bool bool_1 = false;
    bool bool_2 = false;
    bool bool_3 = false;
    bool bool_4 = false;
    bool bool_5 = false;
    bool bool_6 = false;
    bool bool_7 = false;
    bool bool_8 = false;
    parser.setFlagBool(&bool_1, sv("bool-1"));
    parser.setFlagBool(&bool_2, sv("bool-2"));
    parser.setFlagBool(&bool_3, sv("bool-3"));
    parser.setFlagBool(&bool_4, sv("bool-4"));
    parser.setFlagBool(&bool_5, sv("bool-5"));
    parser.setFlagBool(&bool_6, sv("bool-6"));
    parser.setFlagBool(&bool_7, sv("bool-7"));
    parser.setFlagBool(&bool_8, sv("bool-8"));

    f32 fa = 0, fb = 0, fc = 0, fd = 0;
    f64 fe = 0, ff = 0, fg = 0, fh = 0;
    parser.setFlagFloat32(&fa, sv("float32-1"));
    parser.setFlagFloat32(&fb, sv("float32-2"));
    parser.setFlagFloat32(&fc, sv("float32-3"));
    parser.setFlagFloat32(&fd, sv("float32-4"));
    parser.setFlagFloat64(&fe, sv("float64-5"));
    parser.setFlagFloat64(&ff, sv("float64-6"));
    parser.setFlagFloat64(&fg, sv("float64-7"));
    parser.setFlagFloat64(&fh, sv("float64-8"));

    {
        auto ret = parser.parse(__cmd_flag_test_input_2_len, __cmd_flag_test_input_2);
        Assert(!ret.hasErr());
    }

    {
        auto ret = parser.matchFlags();
        Assert(!ret.hasErr());
    }

    // Check integer parsing:
    {
        Assert(a == 4);
        Assert(b == -13);
        Assert(c == 19);
        Assert(d == 99);
    }

    // Check string parsing:
    {
        Assert(cptrArg != nullptr);
        Assert(core::cptrEq(cptrArg, "banicata   fsa", core::cptrLen("banicata   fsa")));
    }

    // Check boolean parsing:
    {
        Assert(bool_1);
        Assert(bool_2);
        Assert(bool_3);
        Assert(bool_4);
        Assert(bool_5);
        Assert(bool_6);
        Assert(!bool_7);
        Assert(!bool_8);
    }

    // Check float parsing:
    {
        Assert(core::safeEq(fa, 1.2f, 0.00001f));
        Assert(core::safeEq(fb, 0.5f, 0.00001f));
        Assert(core::safeEq(fc, 1.0f, 0.00001f));
        Assert(core::safeEq(fd, -1.2f, 0.00001f));
        Assert(core::safeEq(fe, 1.2, 0.00001));
        Assert(core::safeEq(ff, 7.0, 0.00001));
        Assert(core::safeEq(fg, -1.2, 0.00001));
        Assert(core::safeEq(fh, 12.000005, 0.00001));

    }

    return 0;
}

template <typename TAllocator>
i32 cmdFlagParserMatchingEdgecasesTest() {
    using CmdFlagParser = core::CmdFlagParser<TAllocator>;
    using core::sv;

    {
        CmdFlagParser parser;
        i32 a = 0;

        parser.setFlagInt32(&a, sv("int32"));

        {
            const char* input[] = { "exe", "-int32 5", "8" };
            constexpr addr_size len = sizeof(input) / sizeof(input[0]);
            auto ret = parser.parse(len, input);
            Assert(!ret.hasErr());
        }
        {
            auto ret = parser.matchFlags();
            Assert(ret.hasErr());
            Assert(ret.err() == CmdFlagParser::MatchError::UnknownFlag);
        }
    }

    {
        CmdFlagParser parser;
        i32 a = 0;

        {
            const char* input[] = { "exe", "-int32", "8" };
            constexpr addr_size len = sizeof(input) / sizeof(input[0]);
            auto ret = parser.parse(len, input);
            Assert(!ret.hasErr());
        }
        {
            auto ret = parser.matchFlags();
            Assert(ret.hasErr());
            Assert(ret.err() == CmdFlagParser::MatchError::UnknownFlag);
        }
        parser.setFlagInt32(&a, sv("int32"));
        {
            // Match again after setting the flag.
            auto ret = parser.matchFlags();
            Assert(!ret.hasErr());
        }
    }

    // Test required flag predicated:
    {
        CmdFlagParser parser;
        i32 a;
        i64 b;

        parser.setFlagInt32(&a, sv("int32"), true);
        parser.setFlagInt64(&b, sv("int64"), false);

        {
            const char* input[] = { "exe", "-int64", "8" };
            constexpr addr_size len = sizeof(input) / sizeof(input[0]);
            auto ret = parser.parse(len, input);
            Assert(!ret.hasErr());
        }
        {
            auto ret = parser.matchFlags();
            Assert(ret.hasErr());
            Assert(ret.err() == CmdFlagParser::MatchError::MissingRequiredFlag);
        }
        {
            const char* input[] = { "exe", "-int32", "8" };
            constexpr addr_size len = sizeof(input) / sizeof(input[0]);
            auto ret = parser.parse(len, input);
            Assert(!ret.hasErr());
        }
        {
            auto ret = parser.matchFlags();
            Assert(!ret.hasErr());
        }
    }

    // Test allow unknown flags
    {
        CmdFlagParser parser;

        {
            const char* input[] = { "exe", "-int32", "8", "-int64", "8" };
            constexpr addr_size len = sizeof(input) / sizeof(input[0]);
            auto ret = parser.parse(len, input);
            Assert(!ret.hasErr());
        }
        {
            auto ret = parser.matchFlags();
            Assert(ret.hasErr());
            Assert(ret.err() == CmdFlagParser::MatchError::UnknownFlag);
        }
        parser.allowUnknownFlags(true);
        {
            auto ret = parser.matchFlags();
            Assert(!ret.hasErr());
        }
    }

    return 0;
}

template <typename TAllocator>
i32 cmdParserValidationRulesTest() {
    using CmdFlagParser = core::CmdFlagParser<TAllocator>;
    using core::sv;

    CmdFlagParser parser;
    i32 a, b;

    // Add some rules
    parser.setFlagInt32(&a, sv("a"), false, [](void* _a) -> bool {
        i32 v = *reinterpret_cast<i32*>(_a);
        return (v > 0);
    });
    parser.setFlagInt32(&b, sv("b"), false, [](void* _b) -> bool {
        i32 v = *reinterpret_cast<i32*>(_b);
        return (v <= 10);
    });

    // Check if the rules catch errors, when they should.

    {
        const char* input[] = { "exe", "-a", "1", "-b", "5" };
        constexpr addr_size len = sizeof(input) / sizeof(input[0]);
        Expect(parser.parse(len, input));
        Expect(parser.matchFlags());
    }
    {
        const char* input[] = { "exe", "-a", "0", "-b", "5" };
        constexpr addr_size len = sizeof(input) / sizeof(input[0]);
        Expect(parser.parse(len, input));
        auto res = parser.matchFlags();
        Assert(res.hasErr());
        Assert(res.err() == CmdFlagParser::MatchError::ValidationFailed);
    }
    {
        const char* input[] = { "exe", "-a", "1", "-b", "10" };
        constexpr addr_size len = sizeof(input) / sizeof(input[0]);
        Expect(parser.parse(len, input));
        Expect(parser.matchFlags());
    }
    {
        const char* input[] = { "exe", "-a", "0", "-b", "11" };
        constexpr addr_size len = sizeof(input) / sizeof(input[0]);
        Expect(parser.parse(len, input));
        auto res = parser.matchFlags();
        Assert(res.hasErr());
        Assert(res.err() == CmdFlagParser::MatchError::ValidationFailed);
    }
    {
        const char* input[] = { "exe", "-a", "1", "-b", "10" };
        constexpr addr_size len = sizeof(input) / sizeof(input[0]);
        Expect(parser.parse(len, input));
        Expect(parser.matchFlags());
    }
    {
        const char* input[] = { "bin", "-a", "1" }; // b is not required, the custom validator should not fail this case!
        constexpr addr_size len = sizeof(input) / sizeof(input[0]);
        Expect(parser.parse(len, input));
        Expect(parser.matchFlags());
    }

    return 0;
}

template <typename TAllocator>
i32 cmdParserAliasTest() {
    using CmdFlagParser = core::CmdFlagParser<TAllocator>;
    using core::sv;

    // Basic aliasing:
    {
        CmdFlagParser parser;
        i32 arg1 = -1;
        bool arg2 = false;

        parser.setFlagInt32(&arg1, core::sv("full_name"));
        parser.setFlagBool(&arg2, core::sv("full_name_2"));
        parser.alias(core::sv("full_name"), core::sv("a"));
        parser.alias(core::sv("full_name_2"), core::sv("b"));

        const char* input[] = { "bin", "-a", "1", "-b", "true" };
        constexpr addr_size len = sizeof(input) / sizeof(input[0]);
        Expect(parser.parse(len, input));
        Expect(parser.matchFlags());

        Assert(arg1 == 1);
        Assert(arg2 == true);
    }

    // Multiple aliases to the same flag:
    {
        CmdFlagParser parser;
        char* doubleAliased = nullptr;

        parser.setFlagCptr(&doubleAliased, core::sv("full_name"));

        parser.alias(core::sv("full_name"), core::sv("a"));
        parser.alias(core::sv("full_name"), core::sv("b"));

        const char* input[] = { "bin", "-a", "value", "-b", "override" };
        constexpr addr_size len = sizeof(input) / sizeof(input[0]);
        Expect(parser.parse(len, input));
        Expect(parser.matchFlags());

        Assert(core::cptrEq(doubleAliased, "override", core::cptrLen("override")));
    }

    return 0;
}

i32 runCmdParserTestsSuite() {
    constexpr addr_size BUFF_SIZE = KILOBYTE * 10;
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
        RunTest_DisplayMemAllocs(cmdFlagParserSymbolParsingTest<core::StdAllocator>, core::StdAllocator);
        RunTest_DisplayMemAllocs(cmdFlagParserSymbolParsingTest<core::StdStatsAllocator>, core::StdStatsAllocator);
        RunTest_DisplayMemAllocs(cmdFlagParserSymbolParsingTest<core::BumpAllocator>, core::BumpAllocator);
        core::BumpAllocator::clear();
        checkLeaks();
    }
    {
        RunTest_DisplayMemAllocs(cmdFlagParserSymbolParsingLongerTest<core::StdAllocator>, core::StdAllocator);
        RunTest_DisplayMemAllocs(cmdFlagParserSymbolParsingLongerTest<core::StdStatsAllocator>, core::StdStatsAllocator);
        RunTest_DisplayMemAllocs(cmdFlagParserSymbolParsingLongerTest<core::BumpAllocator>, core::BumpAllocator);
        core::BumpAllocator::clear();
        checkLeaks();
    }
    {
        RunTest_DisplayMemAllocs(cmdFlagParserBasicErrorsTest<core::StdAllocator>, core::StdAllocator);
        RunTest_DisplayMemAllocs(cmdFlagParserBasicErrorsTest<core::StdStatsAllocator>, core::StdStatsAllocator);
        RunTest_DisplayMemAllocs(cmdFlagParserBasicErrorsTest<core::BumpAllocator>, core::BumpAllocator);
        core::BumpAllocator::clear();
        checkLeaks();
    }
    {
        RunTest_DisplayMemAllocs(cmdFlagParserDoubleParsingTest<core::StdAllocator>, core::StdAllocator);
        RunTest_DisplayMemAllocs(cmdFlagParserDoubleParsingTest<core::StdStatsAllocator>, core::StdStatsAllocator);
        RunTest_DisplayMemAllocs(cmdFlagParserDoubleParsingTest<core::BumpAllocator>, core::BumpAllocator);
        core::BumpAllocator::clear();
        checkLeaks();
    }
    {
        RunTest_DisplayMemAllocs(cmdFlagParserFriendlyInputMatchingTest<core::StdAllocator>, core::StdAllocator);
        RunTest_DisplayMemAllocs(cmdFlagParserFriendlyInputMatchingTest<core::StdStatsAllocator>, core::StdStatsAllocator);
        RunTest_DisplayMemAllocs(cmdFlagParserFriendlyInputMatchingTest<core::BumpAllocator>, core::BumpAllocator);
        core::BumpAllocator::clear();
        checkLeaks();
    }
    {
        RunTest_DisplayMemAllocs(cmdFlagParserMatchingEdgecasesTest<core::StdAllocator>, core::StdAllocator);
        RunTest_DisplayMemAllocs(cmdFlagParserMatchingEdgecasesTest<core::StdStatsAllocator>, core::StdStatsAllocator);
        RunTest_DisplayMemAllocs(cmdFlagParserMatchingEdgecasesTest<core::BumpAllocator>, core::BumpAllocator);
        core::BumpAllocator::clear();
        checkLeaks();
    }
    {
        RunTest_DisplayMemAllocs(cmdParserValidationRulesTest<core::StdAllocator>, core::StdAllocator);
        RunTest_DisplayMemAllocs(cmdParserValidationRulesTest<core::StdStatsAllocator>, core::StdStatsAllocator);
        RunTest_DisplayMemAllocs(cmdParserValidationRulesTest<core::BumpAllocator>, core::BumpAllocator);
        core::BumpAllocator::clear();
        checkLeaks();
    }
    {
        RunTest_DisplayMemAllocs(cmdParserAliasTest<core::StdAllocator>, core::StdAllocator);
        RunTest_DisplayMemAllocs(cmdParserAliasTest<core::StdStatsAllocator>, core::StdStatsAllocator);
        RunTest_DisplayMemAllocs(cmdParserAliasTest<core::BumpAllocator>, core::BumpAllocator);
        core::BumpAllocator::clear();
        checkLeaks();
    }

    return 0;
}
