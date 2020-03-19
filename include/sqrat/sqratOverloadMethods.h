// Sqrat: altered version by Gaijin Entertainment Corp.
// SqratOverloadMethods: Overload Methods
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
#if !defined(_SQRAT_OVERLOAD_METHODS_H_)
#define _SQRAT_OVERLOAD_METHODS_H_

#include <squirrel.h>
#include <sqstdaux.h>
#include "sqratTypes.h"
#include "sqratUtil.h"
#include "sqratGlobalMethods.h"
#include "sqratMemberMethods.h"

namespace Sqrat {


// Overload name generator
class SqOverloadName {
public:

    static string Get(const SQChar* name, int args) {
      int l = SQRAT_SPRINTF(nullptr, 0, _SC("__overload_%s%d"), name, args);
      string overloadName(l+1, '\0');
      SQRAT_SPRINTF(&overloadName[0], overloadName.size(), _SC("__overload_%s%d"), name, args);
      return overloadName;
    }
};


// Squirrel Overload Functions
template <class R>
class SqOverload {
public:

    static SQInteger Func(HSQUIRRELVM vm) {
        // Get the arg count
        int argCount = sq_gettop(vm) - 2;

        const SQChar* funcName = _SC("n/a");
        sq_getstring(vm, -1, &funcName); // get the function name (free variable)

        string overloadName = SqOverloadName::Get(funcName, argCount);

        sq_pushstring(vm, overloadName.c_str(), -1);

        if (SQ_FAILED(sq_get(vm, 1))) { // Lookup the proper overload
            return sq_throwerror(vm, _SC("wrong number of parameters"));
        }

        // Push the args again
        for (int i = 1; i <= argCount + 1; ++i) {
            sq_push(vm, i);
        }

        SQRESULT result = sq_call(vm, argCount + 1, true, SQTrue);
        return SQ_SUCCEEDED(result) ? 1 : SQ_ERROR;
    }
};


//
// void return specialization
//
template <>
class SqOverload<void> {
public:

    static SQInteger Func(HSQUIRRELVM vm) {
        // Get the arg count
        int argCount = sq_gettop(vm) - 2;

        const SQChar* funcName;
        sq_getstring(vm, -1, &funcName); // get the function name (free variable)

        string overloadName = SqOverloadName::Get(funcName, argCount);

        sq_pushstring(vm, overloadName.c_str(), -1);

        if (SQ_FAILED(sq_get(vm, 1))) { // Lookup the proper overload
            return sq_throwerror(vm, _SC("wrong number of parameters"));
        }

        // Push the args again
        for (int i = 1; i <= argCount + 1; ++i) {
            sq_push(vm, i);
        }

        SQRESULT result = sq_call(vm, argCount + 1, false, SQTrue);
        return SQ_SUCCEEDED(result) ? 0 : SQ_ERROR;
    }
};


//
// Global Overloaded Function Resolvers
//
template<class Function>
SQFUNCTION SqGlobalOverloadedFunc()
{
  return &SqThunkGen<Function>::template Func<2, true>;
}

//
// Member Global Overloaded Function Resolvers
//
template<class Function>
SQFUNCTION SqMemberGlobalOverloadedFunc()
{
  return &SqThunkGen<Function>::template Func<1, true>;
}

//
// Member Overloaded Function Resolvers
//
template<class C, class MemberFunc>
SQFUNCTION SqMemberOverloadedFunc()
{
  return &SqMemberThunkGen<C, MemberFunc>::template Func<true>;
}

//
// Overload handler resolver
//
template<class F>
SQFUNCTION SqOverloadFunc()
{
  return &SqOverload<result_of_t<get_callable_function_t<F>>>::Func;
}

template<class F>
int SqGetArgCount()
{
  return function_args_num_v<get_callable_function_t<F>>;
}

/// @endcond

}

#endif
