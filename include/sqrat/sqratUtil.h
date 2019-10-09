// Sqrat: altered version by Gaijin Entertainment Corp.
// SqratUtil: Quirrel Utilities
//

//
// Copyright (c) 2009 Brandon Jones
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented; you must not
//  claim that you wrote the original software. If you use this software
//  in a product, an acknowledgment in the product documentation would be
//  appreciated but is not required.
//
//  2. Altered source versions must be plainly marked as such, and must not be
//  misrepresented as being the original software.
//
//  3. This notice may not be removed or altered from any source
//  distribution.
//

#pragma once
#if !defined(_SQRAT_UTIL_H_)
#define _SQRAT_UTIL_H_

#ifdef USE_SQRAT_CONFIG

  #include "sqratConfig.h"

#else

  #include <assert.h>

  #define SQRAT_ASSERT assert
  #define SQRAT_ASSERTF(cond, msg, ...) assert((cond) && (msg))
  #define SQRAT_VERIFY(cond) do { if (!(cond)) assert(#cond); } while(0)

#endif

#include <squirrel.h>
#include <sqstdaux.h>
#include <EASTL/tuple.h>
#include <EASTL/string.h>
#include <EASTL/hash_map.h>
#include <EASTL/type_traits.h>

#if (defined(_MSC_VER) && _MSC_VER >= 1900) || (__cplusplus >= 201402L) || (__cplusplus == 201300L)
#else
  #error C++14 support required
#endif

EA_DISABLE_ALL_VC_WARNINGS()

namespace Sqrat {

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Define an unordered map for Sqrat to use
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template<class Key, class T>
    struct unordered_map {
        typedef eastl::hash_map<Key, T> type;
    };

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Define an inline function to avoid MSVC's "conditional expression is constant" warning
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef _MSC_VER
    template <typename T>
    inline T _c_def(T value) { return value; }
    #define SQRAT_CONST_CONDITION(value) _c_def(value)
#else
    #define SQRAT_CONST_CONDITION(value) value
#endif


template <typename T>
void SQRAT_UNUSED(const T&) {}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Defines a string that is definitely compatible with the version of Squirrel being used (normally this is std::string)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef eastl::basic_string<SQChar> string;


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Tells Sqrat whether Squirrel error handling should be used
///
/// \remarks
/// If true, if a runtime error occurs during the execution of a call, the VM will invoke its error handler.
///
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ErrorHandling {
private:

    static bool& errorHandling() {
        static bool eh = true;
        return eh;
    }

public:

    static bool IsEnabled() {
        return errorHandling();
    }

    static void Enable(bool enable) {
        errorHandling() = enable;
    }
};


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Returns a string that has been formatted to give a nice type error message (for usage with Class::SquirrelFunc)
///
/// \param vm           VM the error occurred with
/// \param idx          Index on the stack of the argument that had a type error
/// \param expectedType The name of the type that the argument should have been
///
/// \return String containing a nice type error message
///
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline string FormatTypeError(HSQUIRRELVM vm, SQInteger idx, const SQChar *expectedType) {
    string err(string::CtorSprintf{}, _SC("wrong type (%s expected)"), expectedType);
    if (SQ_SUCCEEDED(sq_typeof(vm, idx))) {
        const SQChar* actualType = _SC("n/a");
        sq_tostring(vm, -1);
        sq_getstring(vm, -1, &actualType);
        sq_pop(vm, 2);
        err = err + _SC(", got ") + actualType + _SC(")");
    } else {
        err = err + _SC(", got unknown)");
    }
    return err;
}


/// Returns the last error that occurred with a Squirrel VM (not associated with Sqrat errors)
inline string LastErrorString(HSQUIRRELVM vm) {
    const SQChar* sqErr = _SC("n/a");
    sq_getlasterror(vm);
    if (sq_gettype(vm, -1) == OT_NULL) {
        sq_pop(vm, 1);
        return string();
    }
    sq_tostring(vm, -1);
    sq_getstring(vm, -1, &sqErr);
    sq_pop(vm, 2);
    return string(sqErr);
}

template<class Obj>
SQInteger ImplaceFreeReleaseHook(SQUserPointer p, SQInteger)
{
  static_cast<Obj*>(p)->~Obj();
  return 1;
}

template<class T, class = void> struct Var;

// utilities for manipulations with variadic templates arguments
namespace vargs {
  template<class... T>
  struct TailElem
  {
    typedef eastl::tuple<T...> tuple;
    typedef typename eastl::tuple_element<eastl::tuple_size<tuple>::value - 1, tuple>::type type;
  };

  template<class Head, class... T>
  struct HeadElem
  {
    typedef Head type;
  };

  template<class... T>
  using TailElem_t = typename TailElem<T...>::type;

  template<class... T>
  using HeadElem_t = typename HeadElem<T...>::type;

  template<class Arg>
  constexpr Arg tail(Arg&& arg)
  {
    return arg;
  }

  template<class Head, class... Tail>
  constexpr TailElem_t<Head, Tail...> tail(Head&&, Tail&&... args)
  {
    return tail(eastl::forward<Tail>(args)...);
  }

  template<class Arg>
  constexpr Arg head(Arg&& arg)
  {
    return head;
  }

  template<class Head, class... Tail>
  constexpr Head head(Head&& head, Tail&&... args)
  {
    return head;
  }

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4100)
#endif
  template <class Func, class Tuple, size_t... Indexes>
  auto apply_helper(Func pf, eastl::index_sequence<Indexes...>, Tuple &args)
  {
    return pf(eastl::get<Indexes>(args).value...);
  }

  template <class Func, class Tuple>
  auto apply(Func pf, Tuple &&args)
  {
    constexpr auto argsN = eastl::tuple_size<eastl::decay_t<Tuple>>::value;
    return apply_helper(pf, eastl::make_index_sequence<argsN>(), args);
  }

  template <class C, class Member, class Tuple, size_t... Indexes>
  auto apply_member_helper(C *ptr, Member pf, eastl::index_sequence<Indexes...>,
                           Tuple &args)
  {
    return (ptr->*pf)(eastl::get<Indexes>(args).value...);
  }

  template <class C, class Member, class Tuple>
  auto apply_member(C *ptr, Member pf, Tuple &&args)
  {
    constexpr auto argsN = eastl::tuple_size<eastl::decay_t<Tuple>>::value;
    return apply_member_helper(ptr, pf, eastl::make_index_sequence<argsN>(), args);
  }
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
}

// add meta-functions aliases that are missing in eastl
template <typename T>
using remove_pointer_t = typename eastl::remove_pointer<T>::type;

template <typename T>
using remove_const_t = typename eastl::remove_const<T>::type;

struct void_type
{
  using type = void;
};

template<typename T>
struct is_function :
          eastl::disjunction<eastl::is_function<T>,
                             eastl::is_function<remove_pointer_t<eastl::remove_reference_t<T>>>
                             >
{
};


template <typename T>
struct member_function_signature
{
  using type = void;
};

template <typename C, typename R, typename... A>
struct member_function_signature<R (C::*)(A...)>
{
  using type = R(A...);
};

template <typename C, typename R, typename... A>
struct member_function_signature<R (C::*)(A...) const>
{
  using type = R(A...);
};

template <typename C, typename R, typename... A>
struct member_function_signature<R (C::*)(A...) volatile>
{
  using type = R(A...);
};

template <typename C, typename R, typename... A>
struct member_function_signature<R (C::*)(A...) volatile const>
{
  using type = R(A...);
};

template<typename T>
using member_function_signature_t = typename member_function_signature<T>::type;

template<class T>
struct get_class_callop_signature
{
  using type = member_function_signature_t<decltype(&eastl::remove_reference_t<T>::operator())>;
};

template<class T>
using get_class_callop_signature_t = typename get_class_callop_signature<T>::type;

template<class F>
struct get_function_signature
{
  using type = void;
};

template <typename R, typename... A>
struct get_function_signature<R(A...)>
{
  using type = R(A...);
};

template <typename R, typename... A>
struct get_function_signature<R (&)(A...)>
{
  using type = R(A...);
};

template <typename R, typename... A>
struct get_function_signature<R (*)(A...)>
{
  using type = R(A...);
};

template <typename R, typename... A>
struct get_function_signature<R (*&)(A...)>
{
  using type = R(A...);
};

template<typename Func>
using get_function_signature_t = typename get_function_signature<Func>::type;


template<typename T>
struct get_callable_function : eastl::conditional_t<eastl::is_member_function_pointer<T>::value,
                                                    member_function_signature<T>,
                                                    eastl::conditional_t<is_function<T>::value,
                                                                          get_function_signature<remove_pointer_t<T>>,
                                                                          eastl::conditional_t<eastl::is_class<T>::value,
                                                                                            get_class_callop_signature<T>,
                                                                                            void_type>
                                                                        >

                                                    >
{
};


template<typename T>
using get_callable_function_t = typename get_callable_function<T>::type;

template<typename Function>
struct result_of;

template<typename R, typename... A>
struct result_of<R(A...)>
{
  using type = R;
};

template<typename F>
using result_of_t = typename result_of<F>::type;

template<typename Function>
struct function_args_num;

template<typename R, typename... A>
struct function_args_num<R(A...)>
{
  static size_t const value = sizeof...(A);
};

template<typename F>
constexpr size_t function_args_num_v = function_args_num<F>::value;

template<typename T>
struct has_call_operator
{
  using yes = char[2];
  using no  = char[1];

  struct Fallback { void operator()();};
  struct Derived : T, Fallback { Derived() {} };

  template<typename U>
  static no& test(decltype(&U::operator())*);

  template<typename U>
  static yes& test(...);

  static bool const value = sizeof(test<Derived>(0)) == sizeof(yes);
};

template<typename T>
struct is_callable :
  eastl::conditional<eastl::is_class<T>::value,
                      has_call_operator<T>,
                      is_function<T>>::type
{
};

template<typename T>
constexpr int is_callable_v = is_callable<T>::value;

}

EA_RESTORE_ALL_VC_WARNINGS()

#endif
