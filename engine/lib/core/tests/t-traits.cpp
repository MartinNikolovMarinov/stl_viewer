#include "t-index.h"

constexpr i32 runTrueFalseTypeTraitsTest() {
    static_assert(core::true_type_v == true);
    static_assert(core::false_type_v == false);
    static_assert(core::always_false<f32> == false);
    static_assert(core::always_false<bool> == false);
    static_assert(core::always_false<void> == false);
    static_assert(core::always_false<decltype(false)> == false);

    return 0;
}

constexpr i32 runTypeComparisonTraitsTest() {

    // Test cases for is_same

    static_assert(core::is_same_v<i32, i32> == true);
    static_assert(core::is_same_v<const i32, const i32> == true);
    static_assert(core::is_same_v<volatile i32, volatile i32> == true);
    static_assert(core::is_same_v<const volatile i32, const volatile i32> == true);

    i32 a = 0, b = 0;
    static_assert(core::is_same_v<i32, decltype(a)> == true);
    static_assert(core::is_same_v<decltype(a), decltype(b)> == true);

    using TestType = i8;
    static_assert(core::is_same_v<i8, TestType> == true); // NOTE: aliasing a type is not the same as creating a new type!
    static_assert(core::is_same_v<signed char, i8> == true);

    // NOTE: Unlike other types, 'char' is neither 'unsigned' nor 'signed'
    static_assert(core::is_same_v<signed char, char> == false);
    static_assert(core::is_same_v<unsigned char, char> == false);
    static_assert(core::is_same_v<signed char, unsigned char> == false);

    // Some generic comparison false cases:
    static_assert(core::is_same_v<i8, char> == false);
    static_assert(core::is_same_v<u8, char> == false);
    static_assert(core::is_same_v<i8, bool> == false);
    static_assert(core::is_same_v<u8, bool> == false);
    static_assert(core::is_same_v<i32, f32> == false);
    static_assert(core::is_same_v<i32, i32&> == false);
    static_assert(core::is_same_v<i32, i32&&> == false);
    static_assert(core::is_same_v<i32, i32[]> == false);
    static_assert(core::is_same_v<i32, i32*> == false);
    static_assert(core::is_same_v<i32, const i32> == false);
    static_assert(core::is_same_v<const i32, i32> == false);
    static_assert(core::is_same_v<i32, volatile i32> == false);
    static_assert(core::is_same_v<volatile i32, i32> == false);
    static_assert(core::is_same_v<const i32, volatile i32> == false);
    static_assert(core::is_same_v<volatile i32, const i32> == false);

    // Test Cases for is_float

    static_assert(core::is_float_v<f32> == true);
    static_assert(core::is_float_v<const f32> == true);
    static_assert(core::is_float_v<volatile f32> == true);
    static_assert(core::is_float_v<f64> == true);
    static_assert(core::is_float_v<const f64> == true);
    static_assert(core::is_float_v<volatile f64> == true);

    static_assert(core::is_float_v<i32> == false);
    static_assert(core::is_float_v<const i32> == false);
    static_assert(core::is_float_v<i64> == false);
    static_assert(core::is_float_v<const i64> == false);

    // Test Cases for is_integral, is_signed and is_unsigned

    static_assert(core::is_integral_v<u8> == true);
    static_assert(core::is_unsigned_v<u8> == true);
    static_assert(core::is_signed_v<u8> == false);
    static_assert(core::is_integral_v<const u8> == true);
    static_assert(core::is_unsigned_v<const u8> == true);
    static_assert(core::is_signed_v<const u8> == false);
    static_assert(core::is_integral_v<volatile u8> == true);
    static_assert(core::is_unsigned_v<volatile u8> == true);
    static_assert(core::is_signed_v<volatile u8> == false);
    static_assert(core::is_integral_v<u16> == true);
    static_assert(core::is_unsigned_v<u16> == true);
    static_assert(core::is_signed_v<u16> == false);
    static_assert(core::is_integral_v<const u16> == true);
    static_assert(core::is_unsigned_v<const u16> == true);
    static_assert(core::is_signed_v<const u16> == false);
    static_assert(core::is_integral_v<volatile u16> == true);
    static_assert(core::is_unsigned_v<volatile u16> == true);
    static_assert(core::is_signed_v<volatile u16> == false);
    static_assert(core::is_integral_v<u32> == true);
    static_assert(core::is_unsigned_v<u32> == true);
    static_assert(core::is_signed_v<u32> == false);
    static_assert(core::is_integral_v<const u32> == true);
    static_assert(core::is_unsigned_v<const u32> == true);
    static_assert(core::is_signed_v<const u32> == false);
    static_assert(core::is_integral_v<volatile u32> == true);
    static_assert(core::is_unsigned_v<volatile u32> == true);
    static_assert(core::is_signed_v<volatile u32> == false);
    static_assert(core::is_integral_v<u64> == true);
    static_assert(core::is_unsigned_v<u64> == true);
    static_assert(core::is_signed_v<u64> == false);
    static_assert(core::is_integral_v<const u64> == true);
    static_assert(core::is_unsigned_v<const u64> == true);
    static_assert(core::is_signed_v<const u64> == false);
    static_assert(core::is_integral_v<volatile u64> == true);
    static_assert(core::is_unsigned_v<volatile u64> == true);
    static_assert(core::is_signed_v<volatile u64> == false);

    static_assert(core::is_integral_v<i8> == true);
    static_assert(core::is_unsigned_v<i8> == false);
    static_assert(core::is_signed_v<i8> == true);
    static_assert(core::is_integral_v<const i8> == true);
    static_assert(core::is_unsigned_v<const i8> == false);
    static_assert(core::is_signed_v<const i8> == true);
    static_assert(core::is_integral_v<volatile i8> == true);
    static_assert(core::is_unsigned_v<volatile i8> == false);
    static_assert(core::is_signed_v<volatile i8> == true);
    static_assert(core::is_integral_v<i16> == true);
    static_assert(core::is_unsigned_v<i16> == false);
    static_assert(core::is_signed_v<i16> == true);
    static_assert(core::is_integral_v<const i16> == true);
    static_assert(core::is_unsigned_v<const i16> == false);
    static_assert(core::is_signed_v<const i16> == true);
    static_assert(core::is_integral_v<volatile i16> == true);
    static_assert(core::is_unsigned_v<volatile i16> == false);
    static_assert(core::is_signed_v<volatile i16> == true);
    static_assert(core::is_integral_v<i32> == true);
    static_assert(core::is_unsigned_v<i32> == false);
    static_assert(core::is_signed_v<i32> == true);
    static_assert(core::is_integral_v<const i32> == true);
    static_assert(core::is_unsigned_v<const i32> == false);
    static_assert(core::is_signed_v<const i32> == true);
    static_assert(core::is_integral_v<volatile i32> == true);
    static_assert(core::is_unsigned_v<volatile i32> == false);
    static_assert(core::is_signed_v<volatile i32> == true);
    static_assert(core::is_integral_v<i64> == true);
    static_assert(core::is_unsigned_v<i64> == false);
    static_assert(core::is_signed_v<i64> == true);
    static_assert(core::is_integral_v<const i64> == true);
    static_assert(core::is_unsigned_v<const i64> == false);
    static_assert(core::is_signed_v<const i64> == true);
    static_assert(core::is_integral_v<volatile i64> == true);
    static_assert(core::is_unsigned_v<volatile i64> == false);
    static_assert(core::is_signed_v<volatile i64> == true);

    static_assert(core::is_integral_v<f32> == false);
    static_assert(core::is_integral_v<const f32> == false);
    static_assert(core::is_integral_v<volatile f32> == false);
    static_assert(core::is_integral_v<f64> == false);
    static_assert(core::is_integral_v<const f64> == false);
    static_assert(core::is_integral_v<volatile f64> == false);

    return 0;
}

constexpr i32 runTypeModificatorTraitsTest() {
    static_assert(core::is_same_v<core::remove_cv_t<const u8>, u8>);
    static_assert(core::is_same_v<core::remove_cv_t<volatile u8>, u8>);
    static_assert(core::is_same_v<core::remove_cv_t<const volatile u8>, u8>);

    static_assert(core::is_same_v<core::remove_ref_t<u8>, u8>);
    static_assert(core::is_same_v<core::remove_ref_t<u8&>, u8>);
    static_assert(core::is_same_v<core::remove_ref_t<u8&&>, u8>);
    static_assert(core::is_same_v<core::remove_ref_t<const u8&>, const u8>);
    static_assert(core::is_same_v<core::remove_ref_t<const u8&&>, const u8>);
    static_assert(core::is_same_v<core::remove_ref_t<volatile u8&>, volatile u8>);
    static_assert(core::is_same_v<core::remove_ref_t<volatile u8&&>, volatile u8>);

    static_assert(core::is_same_v<core::remove_ptr_t<u8>, u8>);
    static_assert(core::is_same_v<core::remove_ptr_t<u8*>, u8>);
    static_assert(core::is_same_v<core::remove_ptr_t<u8**>, u8> == false);

    static_assert(core::is_same_v<core::remove_extent_t<u8>, u8>);
    static_assert(core::is_same_v<core::remove_extent_t<u8[]>, u8>);
    static_assert(core::is_same_v<core::remove_extent_t<u8[1]>, u8>);
    static_assert(core::is_same_v<core::remove_extent_t<u8[1][2]>, u8[2]>);

    static_assert(core::is_same_v<core::remove_all_extents_t<u8>, u8>);
    static_assert(core::is_same_v<core::remove_all_extents_t<u8[]>, u8>);
    static_assert(core::is_same_v<core::remove_all_extents_t<u8[1]>, u8>);
    static_assert(core::is_same_v<core::remove_all_extents_t<u8[1][2]>, u8>);
    static_assert(core::is_same_v<core::remove_all_extents_t<u8[1][2][3]>, u8>);

    static_assert(core::is_lvalue_v<i32> == false);
    static_assert(core::is_lvalue_v<i32&> == true);
    static_assert(core::is_lvalue_v<i32&&> == false);
    static_assert(core::is_lvalue_v<i32*> == false);

    static_assert(core::is_rvalue_v<i32> == false);
    static_assert(core::is_rvalue_v<i32&> == false);
    static_assert(core::is_rvalue_v<i32&&> == true);
    static_assert(core::is_rvalue_v<i32*> == false);

    // Remove l/r value reference:
    {
        i32 v = 0;
        decltype(v)& lvalue = v;
        decltype(v)&& rvalue = 0;
        core::remove_ref<decltype(lvalue)>::type t1 = 0;
        core::remove_ref<decltype(rvalue)>::type t2 = 0;

        static_assert(core::is_lvalue_v<decltype(lvalue)> == true);
        static_assert(core::is_lvalue_v<decltype(rvalue)> == false);
        static_assert(core::is_lvalue_v<decltype(t1)> == false);
        static_assert(core::is_lvalue_v<decltype(t2)> == false);

        static_assert(core::is_rvalue_v<decltype(lvalue)> == false);
        static_assert(core::is_rvalue_v<decltype(rvalue)> == true);
        static_assert(core::is_rvalue_v<decltype(t1)> == false);
        static_assert(core::is_rvalue_v<decltype(t2)> == false);
    }

    // Add and remove l/r value references:
    {
        i32 v = 0;
        typename core::add_lvalue<decltype(v)>::type lvalue = v;
        typename core::add_rvalue<decltype(v)>::type rvalue = core::move(v);

        static_assert(core::is_lvalue_v<decltype(lvalue)> == true);
        static_assert(core::is_rvalue_v<decltype(rvalue)> == true);
    }

    return 0;
}

constexpr i32 runIsTriviallyDestructibleTest() {
    // These assumptions are important for the implementation of some data structures.

    // Is trivially destructible
    {
        struct A {};
        struct B { ~B() {} };
        struct C { C() = delete; };
        struct D { D() = default; };
        struct E { E() = default; ~E() = delete; };
        struct F { F() = delete; ~F() = delete; };
        struct G { G() = delete; ~G() = default; };
        struct H { i32 h; H(i32 a) : h(a) {} };
        struct I { virtual ~I() {} };
        struct J : I { ~J() override {} };
        struct K : I {};

        static_assert(core::is_trivially_destructible_v<A> == true);
        static_assert(core::is_trivially_destructible_v<B> == false);
        static_assert(core::is_trivially_destructible_v<C> == true);
        static_assert(core::is_trivially_destructible_v<D> == true);

        // NOTE: [COMPILER SPECIFIC] Compilers don't agree on this one. Fortunately I can't think of any good reason to
        // delete the default constructor, insteadof setting it to defaut. Which means that, for now, I don't care about
        // this difference. If I discover other differences, that I care for, I might need to unify the code
        // implementation for all compilers. That work will be ugly, tedious and it is better to just use the standard
        // library from then on.
        #if defined(COMPILER_GCC) && COMPILER_GCC == 1
            static_assert(core::is_trivially_destructible_v<E> == true);
            static_assert(core::is_trivially_destructible_v<F> == true);
        #else
            static_assert(core::is_trivially_destructible_v<E> == false);
            static_assert(core::is_trivially_destructible_v<F> == false);
        #endif

        static_assert(core::is_trivially_destructible_v<G> == true);
        static_assert(core::is_trivially_destructible_v<H> == true);
        static_assert(core::is_trivially_destructible_v<I> == false);
        static_assert(core::is_trivially_destructible_v<J> == false);
        static_assert(core::is_trivially_destructible_v<K> == false);

        static_assert(core::is_trivially_destructible_v<void> == false);
        static_assert(core::is_trivially_destructible_v<void*> == true);
        static_assert(core::is_trivially_destructible_v<bool (*)(const char*)> == true);
        static_assert(core::is_trivially_destructible_v<i32> == true);
        static_assert(core::is_trivially_destructible_v<i32&> == true);
        static_assert(core::is_trivially_destructible_v<i32&&> == true);
        static_assert(core::is_trivially_destructible_v<i32*> == true);
        static_assert(core::is_trivially_destructible_v<i32[10]> == true);
        static_assert(core::is_trivially_destructible_v<bool> == true);
    }

    return 0;
}

i32 runTraitsTestsSuite() {
    RunTest(runTrueFalseTypeTraitsTest);
    RunTest(runTypeComparisonTraitsTest);
    RunTest(runTypeModificatorTraitsTest);
    RunTest(runIsTriviallyDestructibleTest);

    return 0;
}

constexpr i32 runCompiletimeTraitsTestsSuite() {
    RunTestCompileTime(runTrueFalseTypeTraitsTest);
    RunTestCompileTime(runTypeComparisonTraitsTest);
    RunTestCompileTime(runTypeModificatorTraitsTest);
    RunTestCompileTime(runIsTriviallyDestructibleTest);

    return 0;
}
