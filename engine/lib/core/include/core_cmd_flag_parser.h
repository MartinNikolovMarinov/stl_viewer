#pragma once

#include <core_alloc.h>
#include <core_arr.h>
#include <core_cptr_conv.h>
#include <core_cptr.h>
#include <core_expected.h>
#include <core_hash_map.h>
#include <core_str_builder.h>
#include <core_types.h>

namespace core {

using namespace coretypes;

template <typename TAllocator = CORE_DEFAULT_ALLOCATOR()>
struct CmdFlagParser {
    struct ParsedSymbol;
    struct FlagData;

    using sb = StrBuilder<TAllocator>;
    using ParsedSymbols = Arr<ParsedSymbol, TAllocator>;
    using FlagDataMap = HashMap<StrView, FlagData, TAllocator>;
    using FlagValidationFn =  bool (*)(void* val);

    static constexpr u32 MAX_ARG_COUNT = 100;

    enum struct ParseError : u8 {
        None,

        NothingToParse,
        ArgumentListTooLong,
        InvalidArgvElement,
        IncorrectValue,
        FlagWithoutValue,

        SENTINEL
    };

    enum struct MatchError : u8 {
        None,

        NoSymbolsToMatch,
        MissingRequiredFlag,
        ValidationFailed,
        UnknownFlag,

        SENTINEL
    };

    enum struct ParsedSymbolType : u8 {
        None,

        ProgramName,
        Argument,
        FlagName,
        FlagValue,
        OptionName,

        SENTINEL
    };

    struct ParsedSymbol {
        ParsedSymbolType type;
        sb value;
    };

    enum struct FlagType {
        None,

        Bool,
        Int32,
        Int64,
        Uint32,
        Uint64,
        Float32,
        Float64,
        CPtr,

        SENTINEL
    };

    struct FlagData {
        sb name;
        FlagType type = FlagType::None;
        void* data = nullptr;
        bool isSet = false;
        bool isRequired = false;
        FlagValidationFn validationFn = nullptr;
    };

    void allowUnknownFlags(bool allow) {
        m_allowUnknownFlags = allow;
    }

    core::expected<ParseError> parse(addr_size argc, const char** argv) {
        if (argv == nullptr) return core::unexpected(ParseError::NothingToParse);
        if (argc == 0) return core::unexpected(ParseError::NothingToParse);
        if (argc > MAX_ARG_COUNT) return core::unexpected(ParseError::ArgumentListTooLong);

        // Pre-allocate at enough space for the worst case scenario:
        m_parsedSymbols.clear();
        m_parsedSymbols.adjustCap(argc*2);

        auto trailingWhitespaceIdx = [](const char* str, addr_size len) -> addr_size {
            if (!core::isWhiteSpace(str[len - 1])) return len;
            addr_size idx = len;
            while (idx > 0 && core::isWhiteSpace(str[idx - 1])) {
                idx--;
            }
            return idx;
        };

        i32 state = 0;
        bool isValue = false;

        for (addr_size i = 0; i < argc; i++) {
            const char* chunk = argv[i];
            if (chunk == nullptr) return core::unexpected(ParseError::InvalidArgvElement);

            chunk = core::cptrSkipSpace(chunk);

            bool isFlag = false;
            bool isOption = false;
            if (chunk[0] == '-' && isValue == false) {
                chunk++; // skip first -
                if (chunk[0] == '-') {
                    chunk++; // skip second -
                    isOption = true;
                }
                else {
                    isFlag = true;
                }
            }

            addr_size trailingIdx = trailingWhitespaceIdx(chunk, core::cptrLen(chunk));

            switch (state) {
                case 0: // in program name
                {
                    ParsedSymbol s = { ParsedSymbolType::ProgramName, core::sv(chunk, trailingIdx) };
                    m_parsedSymbols.append(core::move(s));
                    state = 1;
                    break;
                }

                case 1: // in argument list
                {
                    if (!isFlag && !isOption) {
                        ParsedSymbol s = { ParsedSymbolType::Argument, core::sv(chunk, trailingIdx) };
                        m_parsedSymbols.append(core::move(s));
                    }
                    else if (isFlag) {
                        ParsedSymbol s = { ParsedSymbolType::FlagName, core::sv(chunk, trailingIdx) };
                        m_parsedSymbols.append(core::move(s));
                        isValue = true; // next symbol is a value.
                        state = 2; // end of argument list, found first flag.
                    }
                    else {
                        ParsedSymbol s = { ParsedSymbolType::OptionName, core::sv(chunk, trailingIdx) };
                        m_parsedSymbols.append(core::move(s));
                        state = 2; // end of argument list, found first option.
                    }

                    break;
                }

                case 2:
                {
                    if (isFlag) {
                        ParsedSymbol s = { ParsedSymbolType::FlagName, core::sv(chunk, trailingIdx) };
                        m_parsedSymbols.append(core::move(s));
                        isValue = true; // next symbol is a value.
                    }
                    else if (isOption && !isValue) {
                        ParsedSymbol s = { ParsedSymbolType::OptionName, core::sv(chunk, trailingIdx) };
                        m_parsedSymbols.append(core::move(s));
                    }
                    else if (isValue) {
                        ParsedSymbol s = { ParsedSymbolType::FlagValue, core::sv(chunk, trailingIdx) };
                        m_parsedSymbols.append(core::move(s));
                        isValue = false;
                    }
                    else {
                        return core::unexpected(ParseError::IncorrectValue);
                    }

                    break;
                }

                default:
                {
                    Panic(false, "Implementation bug: invalid state");
                }
            };
        }

        if (m_parsedSymbols.last().type == ParsedSymbolType::FlagName) {
            // Dangling flag name without a value.
            return core::unexpected(ParseError::FlagWithoutValue);
        }

        return {};
    }

    const ParsedSymbols& parsedSymbols() const {
        return m_parsedSymbols;
    }

    StrView programName() const {
        auto ret = !m_parsedSymbols.empty() ? m_parsedSymbols[0].value.view() : sv();
        return ret;
    }

    template <typename TCallback>
    inline void arguments(TCallback cb) const {
        for (addr_size i = 1; i < m_parsedSymbols.len(); i++) {
            auto& s = m_parsedSymbols[i];
            if (s.type == ParsedSymbolType::Argument) {
                if (!cb(s.value.view(), i - 1)) {
                    return;
                }
            }
        }
    }

    template <typename TCallback>
    inline void flags(TCallback cb) const {
        for (addr_size i = 1; i < m_parsedSymbols.len(); i++) {
            auto& s = m_parsedSymbols[i];
            if (s.type == ParsedSymbolType::FlagName) {
                const ParsedSymbol* next = (i + 1 < m_parsedSymbols.len()) ? &m_parsedSymbols[i + 1] : nullptr;
                StrView nextValue = next ? next->value.view() : sv();
                if (!cb(s.value.view(), nextValue)) {
                    return;
                }
            }
        }
    }

    template <typename TCallback>
    inline void options(TCallback cb) const {
        for (addr_size i = 1; i < m_parsedSymbols.len(); i++) {
            auto& s = m_parsedSymbols[i];
            if (s.type == ParsedSymbolType::OptionName) {
                if (!cb(s.value.view())) {
                    return;
                }
            }
        }
    }

    core::expected<MatchError> matchFlags() {
        if (m_parsedSymbols.empty()) return core::unexpected(MatchError::NoSymbolsToMatch);

        MatchError err = MatchError::None;

        // Reset the flag data:

        m_flagData.values([](FlagData& fd) -> bool {
            fd.isSet = false;
            return true;
        });

        // Match the flags:

        flags([&, this](core::StrView flag, core::StrView value) -> bool {
            if (auto fd = m_flagData.get(flag); fd) {
                switch (fd->type) {
                    case FlagType::Bool:
                    {
                        addr_size vLen = value.len();
                        bool v = false;

                        if (vLen >= 4) {
                            v = core::cptrCmp(value.data(), 4, "true", 4) == 0 ||
                                core::cptrCmp(value.data(), 4, "True", 4) == 0 ||
                                core::cptrCmp(value.data(), 4, "TRUE", 4) == 0;
                            if (vLen > 4) {
                                // This checks that the words ['true', 'True', 'TRUE'] are not a prefix of a longer word.
                                v &= core::isWhiteSpace(value[4]) || value[4] == '\0';
                            }
                        }
                        else if (vLen >= 1) {
                            v = value[0] == '1' || value[0] == 't' || value[0] == 'T';
                            if (vLen > 1) {
                                // This checks that the symbols ['1', 't', 'T'] are not a prefix of a longer word.
                                v &= core::isWhiteSpace(value[1]) || value[1] == '\0';
                            }
                        }

                        *reinterpret_cast<bool*>(fd->data) = v;
                        fd->isSet = true;
                        break;
                    }
                    case FlagType::Int32:
                    {
                        i32 v = core::cptrToInt<i32>(value.data());
                        *reinterpret_cast<i32*>(fd->data) = v;
                        fd->isSet = true;
                        break;
                    }
                    case FlagType::Int64:
                    {
                        i64 v = core::cptrToInt<i64>(value.data());
                        *reinterpret_cast<i64*>(fd->data) = v;
                        fd->isSet = true;
                        break;
                    }
                    case FlagType::Uint32:
                    {
                        u32 v = core::cptrToInt<u32>(value.data());
                        *reinterpret_cast<u32*>(fd->data) = v;
                        fd->isSet = true;
                        break;
                    }
                    case FlagType::Uint64:
                    {
                        u64 v = core::cptrToInt<u64>(value.data());
                        *reinterpret_cast<u64*>(fd->data) = v;
                        fd->isSet = true;
                        break;
                    }
                    case FlagType::Float32:
                    {
                        f32 v = core::cptrToFloat<f32>(value.data());
                        *reinterpret_cast<f32*>(fd->data) = v;
                        fd->isSet = true;
                        break;
                    }
                    case FlagType::Float64:
                    {
                        f64 v = core::cptrToFloat<f64>(value.data());
                        *reinterpret_cast<f64*>(fd->data) = v;
                        fd->isSet = true;
                        break;
                    }
                    case FlagType::CPtr:
                    {
                        *reinterpret_cast<const char**>(fd->data) = value.data();
                        fd->isSet = true;
                        break;
                    }

                    case FlagType::None:     [[fallthrough]];
                    case FlagType::SENTINEL: Panic(false, "Implementation bug: invalid flag type");
                }
            }
            else {
                if (!m_allowUnknownFlags) {
                    err = MatchError::UnknownFlag;
                    return false;
                }
            }

            return true;
        });

        if (err != MatchError::None) {
            return core::unexpected(err);
        }

        // Run any validation functions:

        m_flagData.values([&](FlagData& fd) -> bool {
            if (fd.isRequired && !fd.isSet) {
                err = MatchError::MissingRequiredFlag;
                return false;
            }

            if (fd.isSet && fd.validationFn) {
                if (!fd.validationFn(fd.data)) {
                    err = MatchError::ValidationFailed;
                    return false;
                }
            }
            return true;
        });

        if (err != MatchError::None) {
            return core::unexpected(err);
        }

        return {};
    }

    void setFlagCptr(char** out, StrView flagName, bool required = false, FlagValidationFn validation = nullptr) {
        _insertFlag(out, flagName, required, validation, FlagType::CPtr);
    }

    void setFlagBool(bool* out, StrView flagName, bool required = false, FlagValidationFn validation = nullptr) {
        _insertFlag(out, flagName, required, validation, FlagType::Bool);
    }

    void setFlagInt32(i32* out, StrView flagName, bool required = false, FlagValidationFn validation = nullptr) {
        _insertFlag(out, flagName, required, validation, FlagType::Int32);
    }

    void setFlagInt64(i64* out, StrView flagName, bool required = false, FlagValidationFn validation = nullptr) {
        _insertFlag(out, flagName, required, validation, FlagType::Int64);
    }

    void setFlagUint32(u32* out, StrView flagName, bool required = false, FlagValidationFn validation = nullptr) {
        _insertFlag(out, flagName, required, validation, FlagType::Uint32);
    }

    void setFlagUint64(u64* out, StrView flagName, bool required = false, FlagValidationFn validation = nullptr) {
        _insertFlag(out, flagName, required, validation, FlagType::Uint64);
    }

    void setFlagFloat32(f32* out, StrView flagName, bool required = false, FlagValidationFn validation = nullptr) {
        _insertFlag(out, flagName, required, validation, FlagType::Float32);
    }

    void setFlagFloat64(f64* out, StrView flagName, bool required = false, FlagValidationFn validation = nullptr) {
        _insertFlag(out, flagName, required, validation, FlagType::Float64);
    }

    void alias(StrView flagName, StrView aliasName) {
        FlagData* existing = m_flagData.get(flagName);
        if (existing) {
            FlagData fd;
            fd.name = sb(aliasName);
            fd.type = existing->type;
            fd.data = existing->data;
            fd.isSet = existing->isSet;
            fd.isRequired = existing->isRequired;
            fd.validationFn = existing->validationFn;

            auto key = fd.name.view();
            m_flagData.put(key, core::move(fd));
        }
        else {
            Panic(false, "Alias target does not exist");
        }
    }

private:

    template <typename T>
    void _insertFlag(T* out, StrView flagName, bool required, FlagValidationFn validation, FlagType type) {
        FlagData* existing = m_flagData.get(flagName);
        if (existing) {
            Panic(false, "Inserting duplicate flags is not allowed");
        }

        FlagData fd;
        fd.name = sb(flagName);
        fd.type = type;
        fd.data = out;
        fd.isSet = false;
        fd.isRequired = required;
        fd.validationFn = validation;

        auto key = fd.name.view();
        m_flagData.put(key, core::move(fd));
    }

    ParsedSymbols m_parsedSymbols;
    FlagDataMap m_flagData;
    bool m_allowUnknownFlags = false;
};

} // namespace core
