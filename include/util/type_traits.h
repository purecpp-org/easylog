#pragma once
#include <functional>
#include <memory>
#include <string>
#include <string_view>

template <class T> struct remove_cvref {
  typedef std::remove_cv_t<std::remove_reference_t<T>> type;
};
template <class T> using remove_cvref_t = typename remove_cvref<T>::type;

template <class T>
constexpr inline bool c_array_v =
    std::is_array_v<remove_cvref_t<T>> &&std::extent_v<remove_cvref_t<T>> > 0;

template <typename Type, typename = void> struct is_array : std::false_type {};

template <typename T>
struct is_array<
    T, std::void_t<decltype(std::declval<T>().size()),
                   typename std::enable_if_t<(std::tuple_size<T>::value != 0)>>>
    : std::true_type {};

template <typename T>
constexpr inline bool array_v = is_array<remove_cvref_t<T>>::value;

template <typename Type>
constexpr inline bool fixed_array_v = c_array_v<Type> || array_v<Type>;

template <template <typename...> class U, typename T>
struct is_template_instant_of : std::false_type {};

template <template <typename...> class U, typename... args>
struct is_template_instant_of<U, U<args...>> : std::true_type {};

template <typename T>
constexpr inline bool string_view_v =
    is_template_instant_of<std::basic_string_view, remove_cvref_t<T>>::value;

template <typename T>
constexpr inline bool string_v =
    is_template_instant_of<std::basic_string, remove_cvref_t<T>>::value;

namespace coro_rpc {
template <typename Function> struct function_traits;

template <typename Return, typename... Arguments>
struct function_traits<Return (*)(Arguments...)> {
  using parameters_type = std::tuple<remove_cvref_t<Arguments>...>;
  using return_type = Return;
};

template <typename Return, typename... Arguments>
struct function_traits<Return (*)(Arguments...) noexcept> {
  using parameters_type = std::tuple<remove_cvref_t<Arguments>...>;
  using return_type = Return;
};

template <typename Return, typename... Arguments>
struct function_traits<Return(Arguments...)> {
  using parameters_type = std::tuple<remove_cvref_t<Arguments>...>;
  using return_type = Return;
};

template <typename Return, typename... Arguments>
struct function_traits<Return(Arguments...) noexcept> {
  using parameters_type = std::tuple<remove_cvref_t<Arguments>...>;
  using return_type = Return;
};

template <typename This, typename Return, typename... Arguments>
struct function_traits<Return (This::*)(Arguments...)> {
  using parameters_type = std::tuple<remove_cvref_t<Arguments>...>;
  using return_type = Return;
  using class_type = This;
};

template <typename This, typename Return, typename... Arguments>
struct function_traits<Return (This::*)(Arguments...) noexcept> {
  using parameters_type = std::tuple<remove_cvref_t<Arguments>...>;
  using return_type = Return;
  using class_type = This;
};

template <typename This, typename Return, typename... Arguments>
struct function_traits<Return (This::*)(Arguments...) const> {
  using parameters_type = std::tuple<remove_cvref_t<Arguments>...>;
  using return_type = Return;
  using class_type = This;
};

template <typename This, typename Return, typename... Arguments>
struct function_traits<Return (This::*)(Arguments...) const noexcept> {
  using parameters_type = std::tuple<remove_cvref_t<Arguments>...>;
  using return_type = Return;
  using class_type = This;
};

template <typename Return> struct function_traits<Return (*)()> {
  using parameters_type = void;
  using return_type = Return;
};

template <typename Return> struct function_traits<Return (*)() noexcept> {
  using parameters_type = void;
  using return_type = Return;
};

template <typename Return> struct function_traits<Return (&)()> {
  using parameters_type = void;
  using return_type = Return;
};

template <typename Return> struct function_traits<Return (&)() noexcept> {
  using parameters_type = void;
  using return_type = Return;
};

template <typename Return> struct function_traits<Return()> {
  using parameters_type = void;
  using return_type = Return;
};

template <typename Return> struct function_traits<Return() noexcept> {
  using parameters_type = void;
  using return_type = Return;
};

template <typename This, typename Return>
struct function_traits<Return (This::*)()> {
  using parameters_type = void;
  using return_type = Return;
  using class_type = This;
};

template <typename This, typename Return>
struct function_traits<Return (This::*)() noexcept> {
  using parameters_type = void;
  using return_type = Return;
  using class_type = This;
};

template <typename This, typename Return>
struct function_traits<Return (This::*)() const> {
  using parameters_type = void;
  using return_type = Return;
  using class_type = This;
};

template <typename This, typename Return>
struct function_traits<Return (This::*)() const noexcept> {
  using parameters_type = void;
  using return_type = Return;
  using class_type = This;
};

// Support function object and lambda expression
template <class Function>
struct function_traits : function_traits<decltype(&Function::operator())> {};

template <typename Function>
using function_parameters_t =
    typename function_traits<remove_cvref_t<Function>>::parameters_type;

template <typename Function>
using last_parameters_type_t =
    std::tuple_element_t<std::tuple_size_v<function_parameters_t<Function>> - 1,
                         function_parameters_t<Function>>;

template <typename Function>
using function_return_type_t =
    typename function_traits<remove_cvref_t<Function>>::return_type;

template <typename Function>
using class_type_t =
    typename function_traits<remove_cvref_t<Function>>::class_type;

template <typename F, typename... Args>
struct is_invocable
    : std::is_constructible<
          std::function<void(std::remove_reference_t<Args>...)>,
          std::reference_wrapper<typename std::remove_reference<F>::type>> {};

template <typename F, typename... Args>
inline constexpr bool is_invocable_v = is_invocable<F, Args...>::value;

template <typename T> struct remove_first { using type = T; };

template <class First, class... Second>
struct remove_first<std::tuple<First, Second...>> {
  using type = std::tuple<Second...>;
};

template <typename T> using remove_first_t = typename remove_first<T>::type;

template <bool has_conn, typename T> inline auto get_args() {
  if constexpr (has_conn) {
    using args_type = remove_first_t<T>;
    return args_type{};
  } else {
    return T{};
  }
}

template <typename Test, template <typename...> class Ref>
struct is_specialization : std::false_type {};

template <template <typename...> class Ref, typename... Args>
struct is_specialization<Ref<Args...>, Ref> : std::true_type {};

template <typename Test, template <typename...> class Ref>
inline constexpr bool is_specialization_v = is_specialization<Test, Ref>::value;

} // namespace coro_rpc
