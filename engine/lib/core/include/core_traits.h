#pragma once

#include <core_system_checks.h>
#include <core_types.h>

namespace core {

using namespace coretypes;

#pragma region True/False Type ----------------------------------------------------------------------------------------

struct true_type  { static constexpr bool value = true; };
struct false_type { static constexpr bool value = false; };
constexpr bool true_type_v  = true_type::value;
constexpr bool false_type_v = false_type::value;

template<typename> constexpr bool always_false = false;

#pragma endregion

#pragma region Type Modificators --------------------------------------------------------------------------------------

template <typename T> struct type_identity { typedef T type; };

template <typename T> using type_identity_t = typename type_identity<T>::type;

template <typename T> struct remove_cv                    { typedef T type; };
template <typename T> struct remove_cv <const T>          { typedef T type; };
template <typename T> struct remove_cv <volatile T>       { typedef T type; };
template <typename T> struct remove_cv <const volatile T> { typedef T type; };

template <typename T> using remove_cv_t = typename remove_cv<T>::type;

template<typename T> struct remove_ref      { typedef T type; };
template<typename T> struct remove_ref<T&>  { typedef T type; };
template<typename T> struct remove_ref<T&&> { typedef T type; };

template <typename T> using remove_ref_t = typename remove_ref<T>::type;

template <typename T> struct remove_ptr     { typedef T type; };
template <typename T> struct remove_ptr<T*> { typedef T type; };

template <typename T> using remove_ptr_t = typename remove_ptr<T>::type;

template <typename T> struct remove_extent                   { typedef T type; };
template <typename T> struct remove_extent<T[]>              { typedef T type; };
template <typename T, addr_size N> struct remove_extent<T[N]> { typedef T type; };

template <typename T> using remove_extent_t = typename remove_extent<T>::type;

template <typename T> struct remove_all_extents { typedef T type; };
template <typename T> struct remove_all_extents<T[]> { typedef typename remove_all_extents<T>::type type; };
template <typename T, addr_size N> struct remove_all_extents<T[N]> { typedef typename remove_all_extents<T>::type type; };

template <typename T> using remove_all_extents_t = typename remove_all_extents<T>::type;

template<typename T> struct is_lvalue     { static constexpr bool value = false_type_v; };
template<typename T> struct is_lvalue<T&> { static constexpr bool value = true_type_v; };
template<typename T> constexpr bool is_lvalue_v     = is_lvalue<T>::value;
template<typename T> constexpr bool is_lvalue_v<T&> = is_lvalue<T&>::value;

template<typename T> struct is_rvalue      { static constexpr bool value = false_type_v; };
template<typename T> struct is_rvalue<T&&> { static constexpr bool value = true_type_v; };
template<typename T> constexpr bool is_rvalue_v      = is_rvalue<T>::value;
template<typename T> constexpr bool is_rvalue_v<T&&> = is_rvalue<T&&>::value;

namespace detail {

template <typename T> auto try_add_lvalue(i32) -> type_identity<T&>;
template <typename T> auto try_add_lvalue(...) -> type_identity<T>;

template <typename T> auto try_add_rvalue(i32) -> type_identity<T&&>;
template <typename T> auto try_add_rvalue(...) -> type_identity<T>;

} // namespace detail

template <typename T> struct add_lvalue : decltype(detail::try_add_lvalue<T>(0)) {};
template <typename T> struct add_rvalue : decltype(detail::try_add_rvalue<T>(0)) {};

template<typename T>
typename add_rvalue<T>::type declval() noexcept {
    static_assert(always_false<T>, "declval not allowed in an evaluated context");
}

#pragma endregion

#pragma region Type Comparison ----------------------------------------------------------------------------------------

template <typename T, typename U> struct is_same { static constexpr bool value = false_type_v; };
template <typename T> struct is_same<T, T>       { static constexpr bool value = true_type_v; };

template <typename T, typename U> constexpr bool is_same_v = is_same<T, U>::value;

namespace detail {

template <class T, class U>
concept SameHelper = core::is_same_v<T, U>;

} // namespace detail

template <typename T, typename U>
concept same_as = detail::SameHelper<T, U> && detail::SameHelper<U, T>;

template <typename> struct _is_char  { static constexpr bool value = false_type_v; };
template <> struct _is_char<char>    { static constexpr bool value = true_type_v; };
template <> struct _is_char<uchar>   { static constexpr bool value = true_type_v; };
template <> struct _is_char<schar>   { static constexpr bool value = true_type_v; };
template <typename T> struct is_char { static constexpr bool value = _is_char<remove_cv_t<T>>::value; };

template <typename T> constexpr bool is_char_v = is_char<T>::value;

template <typename> struct _is_float  { static constexpr bool value = false_type_v; };
template <> struct _is_float<f32>     { static constexpr bool value = true_type_v; };
template <> struct _is_float<f64>     { static constexpr bool value = true_type_v; };
template <typename T> struct is_float { static constexpr bool value = _is_float<remove_cv_t<T>>::value; };

template <typename T> constexpr bool is_float_v = is_float<T>::value;

template <typename> struct _is_signed  { static constexpr bool value = false_type_v; };
template <> struct _is_signed<i8>      { static constexpr bool value = true_type_v; };
template <> struct _is_signed<i16>     { static constexpr bool value = true_type_v; };
template <> struct _is_signed<i32>     { static constexpr bool value = true_type_v; };
template <> struct _is_signed<i64>     { static constexpr bool value = true_type_v; };
template <typename T> struct is_signed { static constexpr bool value = _is_signed<remove_cv_t<T>>::value; };

template <typename T> constexpr bool is_signed_v = is_signed<T>::value;

template <typename> struct _is_unsigned  { static constexpr bool value = false_type_v; };
template <> struct _is_unsigned<u8>      { static constexpr bool value = true_type_v; };
template <> struct _is_unsigned<u16>     { static constexpr bool value = true_type_v; };
template <> struct _is_unsigned<u32>     { static constexpr bool value = true_type_v; };
template <> struct _is_unsigned<u64>     { static constexpr bool value = true_type_v; };
template <typename T> struct is_unsigned { static constexpr bool value = _is_unsigned<remove_cv_t<T>>::value; };

template <typename T> constexpr bool is_unsigned_v = is_unsigned<T>::value;

template <typename T> struct is_integral { static constexpr bool value = is_unsigned_v<T> || is_signed_v<T>; };

template <typename T> constexpr bool is_integral_v = is_integral<T>::value;

template <typename T> struct is_arithmetic { static constexpr bool value = is_integral_v<T> || is_float_v<T>; };

template <typename T> constexpr bool is_arithmetic_v = is_arithmetic<T>::value;

#pragma endregion

#pragma region Conditionals -------------------------------------------------------------------------------------------

template <bool B, typename T = void> struct enable_if {};
template <typename T> struct enable_if<true, T> { typedef T type; };

template <bool B, typename T = void> using enable_if_t = typename enable_if<B, T>::type;

template <bool B, typename T, typename F> struct conditional { typedef T type; };
template <typename T, typename F> struct conditional<false, T, F> { typedef F type; };

template <bool B, typename T, typename F> using conditional_t = typename conditional<B, T, F>::type;

#pragma endregion

#pragma region Compiletime Execution ----------------------------------------------------------------------------------

namespace detail {
constexpr bool is_const_evaluated() noexcept { return __builtin_is_constant_evaluated(); }
} // namespace detail

// Using a macro here to avoid mistakes where I put is_const_evaluated() in a constexpr if statement.
#define IS_CONST_EVALUATED if (core::detail::is_const_evaluated())

template <auto V>
constexpr auto force_consteval = V;

#pragma endregion

#pragma region Compiler Intrinsic Traits ------------------------------------------------------------------------------

template <typename T>
struct is_trivially_destructible {
#if defined(COMPILER_GCC) && COMPILER_GCC == 1
    // Gcc is a very special boy.
    static constexpr bool value = __has_trivial_destructor(T);
#else
    static constexpr bool value = __is_trivially_destructible(T);
#endif
};

template <typename T>
struct is_trivially_copyable {
    static constexpr bool value = __is_trivially_copyable(T);
};

template <typename T, typename U>
struct is_trivially_assignable {
    static constexpr bool value = __is_trivially_assignable(T, U);
};

/**
 * NOTE: Standard layout types as defined by the Microsoft documentation:
 *
 * When a class or struct does not contain certain C++ language features such as virtual functions
 * which are not found in the C language, and all members have the same access control, it is a standard-layout type. It
 * is memcopy-able and the layout is sufficiently defined that it can be consumed by C programs. Standard-layout types
 * can have user-defined special member functions. In addition, standard layout types have these characteristics:
 * - No virtual functions or virtual base classes.
 * - All non-static data members have the same access control.
 * - All non-static members of class type are standard-layout.
 * - Any base classes are standard-layout.
 * - Has no base classes of the same type as the first non-static data member.
 * - Meets one of these conditions:
 *  - No non-static data member in the most-derived class and no more than one base class with non-static data members, or
 *  - Has no base classes with non-static data members.
 */
template <typename T>
struct is_standard_layout {
    static constexpr bool value = __is_standard_layout(T);
};

/**
 * NOTE: Trivial types as defined by the Microsoft documentation:
 *
 * When a class or struct in C++ has compiler-provided or explicitly defaulted special member functions,
 * then it is a trivial type. It occupies a contiguous memory area. It can have members with different access
 * specifiers. In C++, the compiler is free to choose how to order members in this situation. Therefore, you can memcopy
 * such objects but you cannot reliably consume them from a C program. A trivial type T can be copied into an array of
 * char or unsigned char, and safely copied back into a T variable. Note that because of alignment requirements, there
 * might be padding bytes between type members.
 *
 * Trivial types have a trivial default constructor, trivial copy constructor, trivial copy assignment operator and
 * trivial destructor. In each case, trivial means the constructor/operator/destructor is not user-provided and belongs
 * to a class that has:
 * - no virtual functions or virtual base classes,
 * - no base classes with a corresponding non-trivial constructor/operator/destructor
 * - no data members of class type with a corresponding non-trivial constructor/operator/destructor
 */
template <typename T>
struct is_trivial {
    static constexpr bool value = __is_trivial(T);
};

// NOTE: Pods are both trivial and standard layout types.
template <typename T>
struct is_pod {
    // __is_pod(T) is deprecated in c++20
    static constexpr bool value = __is_standard_layout(T) && __is_trivial(T);
};

template <typename T>
inline constexpr bool is_trivially_destructible_v = is_trivially_destructible<T>::value;
template <typename T>
inline constexpr bool is_trivially_copyable_v = is_trivially_copyable<T>::value;
template <typename T, typename U>
inline constexpr bool is_trivially_assignable_v = is_trivially_assignable<T, U>::value;
template <typename T>
inline constexpr bool is_standard_layout_v = is_standard_layout<T>::value;
template <typename T>
inline constexpr bool is_trivial_v = is_trivial<T>::value;
template <typename T>
inline constexpr bool is_pod_v = is_pod<T>::value;

#pragma endregion

} // namespace core
