// Sqrat: altered version by Gaijin Entertainment Corp.
// SqratTable: Table Binding
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
#if !defined(_SQRAT_TABLE_H_)
#define _SQRAT_TABLE_H_

#include <squirrel.h>
#include <string.h>

#include "sqratObject.h"
#include "sqratFunction.h"
#include "sqratGlobalMethods.h"

namespace Sqrat {

class TableBase : public Object {
public:
    TableBase() {
    }

    TableBase(HSQUIRRELVM v) : Object(v, true) {
    }

    TableBase(const Object& obj) : Object(obj) {
    }

    TableBase(HSQOBJECT o, HSQUIRRELVM v) : Object(o, v) {
    }

    /// Binds a Table or Class to the Table (can be used to facilitate namespaces)
    void Bind(const SQChar* name, Object& object) {
        sq_pushobject(vm, GetObject());
        sq_pushstring(vm, name, -1);
        sq_pushobject(vm, object.GetObject());
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, false)));
        sq_pop(vm,1); // pop table
    }

    /// Binds a raw Squirrel closure to the Table
    TableBase& SquirrelFunc(const SQChar* name, SQFUNCTION func, SQInteger nparamscheck=0, const SQChar *typemask=nullptr) {
        sq_pushobject(vm, GetObject());
        sq_pushstring(vm, name, -1);
        sq_newclosure(vm, func, 0);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_setparamscheck(vm, nparamscheck, typemask)));
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, false)));
        sq_pop(vm,1); // pop table
        return *this;
    }

    template<class V>
    TableBase& SetValue(const SQChar* name, const V& val) {
        BindValue<V>(name, val, false);
        return *this;
    }

    template<class V>
    TableBase& SetValue(const SQInteger index, const V& val) {
        BindValue<V>(index, val, false);
        return *this;
    }

    template<class V>
    TableBase& SetValue(const Object &key, const V& val) {
        BindValue<V>(key, val, false);
        return *this;
    }

    template<class V>
    TableBase& SetInstance(const SQChar* name, V* val) {
        BindInstance<V>(name, val, false);
        return *this;
    }

    template<class V>
    TableBase& SetInstance(const SQInteger index, V* val) {
        BindInstance<V>(index, val, false);
        return *this;
    }

    template<class V>
    TableBase& SetInstance(const Object &key, V* val) {
        BindInstance<V>(key, val, false);
        return *this;
    }

    /// Sets a key in the Table to a specific function
    template<class F>
    TableBase& Func(const SQChar* name, F method) {
        BindFunc<F>(name, method, SqGlobalThunk<F>(), 1+SqGetArgCount<F>());
        return *this;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Sets a key in the Table to a specific function and allows the key to be overloaded with functions of a different amount of arguments
    ///
    /// \remarks
    /// Overloading in Sqrat does not work for functions with the same amount of arguments (just like in Squirrel).
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template<class F>
    TableBase& Overload(const SQChar* name, F method) {
        BindOverload(name, method, SqGlobalOverloadedFunc<F>(), SqOverloadFunc<F>(), SqGetArgCount<F>());
        return *this;
    }

    bool HasKey(const SQChar* name) const
    {
        sq_pushobject(vm, obj);
        sq_pushstring(vm, name, -1);
        if (SQ_FAILED(sq_get_noerr(vm, -2))) {
            sq_pop(vm, 1);
            return false;
        }
        sq_pop(vm, 2);
        return true;
    }

    bool HasKey(const Object &key, bool raw=false) const
    {
        SQRAT_ASSERT(key.IsNull() || key.GetVM() == vm);
        const HSQOBJECT &hSelf = GetObject(), &hKey = key.GetObject();
        HSQOBJECT out;
        return SQ_SUCCEEDED(sq_direct_get(vm, &hSelf, &hKey, &out, raw));
    }


    Function GetFunction(const SQChar* name) const {
        HSQOBJECT funcObj;
        sq_pushobject(vm, GetObject());
        sq_pushstring(vm, name, -1);
        if(SQ_FAILED(sq_get_noerr(vm, -2))) {
            sq_pop(vm, 1);
            return Function();
        }
        SQObjectType value_type = sq_gettype(vm, -1);
        if (value_type != OT_CLOSURE && value_type != OT_NATIVECLOSURE) {
            sq_pop(vm, 2);
            return Function();
        }

        SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, -1, &funcObj)));
        Function ret(vm, GetObject(), funcObj); // must addref before the pop!
        sq_pop(vm, 2);
        return ret;
    }

    Function GetFunction(const SQInteger index) const {
        HSQOBJECT funcObj;
        sq_pushobject(vm, GetObject());
        sq_pushinteger(vm, index);
        if(SQ_FAILED(sq_get_noerr(vm, -2))) {
            sq_pop(vm, 1);
            return Function();
        }
        SQObjectType value_type = sq_gettype(vm, -1);
        if (value_type != OT_CLOSURE && value_type != OT_NATIVECLOSURE) {
            sq_pop(vm, 2);
            return Function();
        }
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, -1, &funcObj)));
        Function ret(vm, GetObject(), funcObj); // must addref before the pop!
        sq_pop(vm, 2);
        return ret;
    }

    Function GetFunction(const Object& key, bool raw=false) const {
        SQRAT_ASSERT(key.IsNull() || key.GetVM() == vm);
        const HSQOBJECT &hSelf = GetObject(), &hKey = key.GetObject();
        HSQOBJECT funcObj;
        if (SQ_FAILED(sq_direct_get(vm, &hSelf, &hKey, &funcObj, raw))
          || (funcObj._type != OT_CLOSURE && funcObj._type != OT_NATIVECLOSURE))
        {
          return Function();
        }

        Function ret(vm, GetObject(), funcObj); // must addref before the pop!
        return ret;
    }

    bool DeleteSlot(const SQChar* name) const
    {
        sq_pushobject(vm, obj);
        sq_pushstring(vm, name, -1);
        if (SQ_FAILED(sq_deleteslot(vm, -2, false))) {
            sq_pop(vm, 1);
            return false;
        }
        sq_pop(vm, 1);
        return true;
    }

    bool DeleteSlot(const Object &key) const
    {
        sq_pushobject(vm, obj);
        sq_pushobject(vm, key.GetObject());
        if (SQ_FAILED(sq_deleteslot(vm, -2, false))) {
            sq_pop(vm, 1);
            return false;
        }
        sq_pop(vm, 1);
        return true;
    }

    SQInteger Length() const
    {
        sq_pushobject(vm, obj);
        SQInteger r = sq_getsize(vm, -1);
        sq_pop(vm, 1);
        return r;
    }

    bool Clear() {
        sq_pushobject(vm, GetObject());
        bool ok = SQ_SUCCEEDED(sq_clear(vm, -1));
        sq_pop(vm, 1);
        return ok;
    }
};

class Table : public TableBase {
public:
    Table(HSQUIRRELVM v) : TableBase(v) {
        sq_newtable(vm);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm,-1,&obj)));
        sq_addref(vm, &obj);
        sq_pop(vm,1);
    }

    Table() {
    }

    Table(const Object& obj) : TableBase(obj) {
    }

    Table(HSQOBJECT o, HSQUIRRELVM v) : TableBase(o, v) {
    }
};


class RootTable : public TableBase {
public:
    RootTable(HSQUIRRELVM v) : TableBase(v) {
        sq_pushroottable(vm);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm,-1,&obj)));
        sq_addref(vm, &obj);
        sq_pop(v,1); // pop root table
    }
};


class RegistryTable : public TableBase {
public:
    RegistryTable(HSQUIRRELVM v) : TableBase(v) {
        sq_pushregistrytable(v);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm,-1,&obj)));
        sq_addref(vm, &obj);
        sq_pop(v,1); // pop the registry table
    }
};

/// Used to get and push Table instances to and from the stack as references (tables are always references in Squirrel)
template<>
struct Var<Table> {

    Table value; ///< The actual value of get operations

    /// Attempts to get the value off the stack at idx as a Table
    Var(HSQUIRRELVM vm, SQInteger idx) {
        HSQOBJECT obj;
        sq_resetobject(&obj);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm,idx,&obj)));
        value = Table(obj, vm);

        SQObjectType value_type = sq_gettype(vm, idx);
        if (value_type != OT_TABLE && value_type != OT_NULL) {
            SQRAT_ASSERTF(0, FormatTypeError(vm, idx, _SC("table")).c_str());
        }
    }

    /// Called by Sqrat::PushVar to put an Table reference on the stack
    static void push(HSQUIRRELVM vm, const Table& value) {
        HSQOBJECT obj;
        sq_resetobject(&obj);
        obj = value.GetObject();
        sq_pushobject(vm,obj);
    }

    static const SQChar * getVarTypeName() { return _SC("table"); }
    static bool check_type(HSQUIRRELVM vm, SQInteger idx) {
        return sq_gettype(vm, idx) == OT_TABLE || sq_gettype(vm, idx) == OT_NULL;
    }
};


template<>
struct Var<Table&> : Var<Table> {Var(HSQUIRRELVM vm, SQInteger idx) : Var<Table>(vm, idx) {}};

template<>
struct Var<const Table&> : Var<Table> {Var(HSQUIRRELVM vm, SQInteger idx) : Var<Table>(vm, idx) {}};

}

#endif
