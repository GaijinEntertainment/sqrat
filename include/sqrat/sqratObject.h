// Sqrat: altered version by Gaijin Entertainment Corp.
// SqratObject: Referenced Quirrel Object Wrapper
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
#if !defined(_SQRAT_OBJECT_H_)
#define _SQRAT_OBJECT_H_

#include <squirrel.h>
#include <sqdirect.h>
#include <string.h>
#include <EASTL/type_traits.h>

#include "sqratAllocator.h"
#include "sqratTypes.h"
#include "sqratOverloadMethods.h"
#include "sqratUtil.h"

namespace Sqrat {

class Table;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// The base class for classes that represent Squirrel objects
///
/// \remarks
/// All Object and derived classes MUST be destroyed before calling sq_close or your application will crash when exiting.
///
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Object {
protected:

    HSQUIRRELVM vm;
    HSQOBJECT obj;
    bool release;

public:

    Object(HSQUIRRELVM v, bool releaseOnDestroy = true) : vm(v), release(releaseOnDestroy) {
        sq_resetobject(&obj);
    }

    Object() : vm(0), release(true) {
        sq_resetobject(&obj);
    }

    Object(const Object& so) : vm(so.vm), obj(so.obj), release(so.release) {
        sq_addref(vm, &obj);
    }

    Object(Object && so) : vm(so.vm), obj(so.obj), release(so.release) {
      sq_resetobject(&so.obj);
    }

    Object(HSQOBJECT o, HSQUIRRELVM v) : vm(v), obj(o), release(true) {
        sq_addref(vm, &obj);
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Constructs an Object from a known/bound type
    ///
    /// \param t        If t is pointer to a C++ class instance then it has to be bound already, otherwise it's ref to value of known type
    /// \param v        VM that the object will exist in
    ///
    /// \tparam T       Type
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template <class T, typename eastl::disable_if<eastl::is_arithmetic<T>::value, bool>::type = false>
    Object(const T &t, HSQUIRRELVM v) : vm(v), release(true) {
      Var<T>::push(vm, t);
      SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, -1, &obj)));
      sq_addref(vm, &obj);
      sq_poptop(vm);
    }

    Object(const SQChar *str, HSQUIRRELVM v, SQInteger str_len = -1) : vm(v), release(true) {
      if (str) {
        sq_pushstring(vm, str, str_len);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, -1, &obj)));
        sq_addref(vm, &obj);
        sq_poptop(vm);
      }
      else
        sq_resetobject(&obj);
    }

    template <class T, typename eastl::enable_if<(eastl::is_integral<T>::value && !eastl::is_same<T, bool>::value), bool>::type = false>
    Object(T t, HSQUIRRELVM v) : vm(v), release(false) {
      obj._type = OT_INTEGER;
      obj._unVal.nInteger = (SQInteger)t;
    }

    template <class T, typename eastl::enable_if<eastl::is_floating_point<T>::value, bool>::type = false>
    Object(T t, HSQUIRRELVM v) : vm(v), release(false) {
      obj._type = OT_FLOAT;
      obj._unVal.fFloat = (SQFloat)t;
    }

    template <class T, typename eastl::enable_if<eastl::is_same<T, bool>::value, bool>::type = false>
    Object(T t, HSQUIRRELVM v) : vm(v), release(false) {
      obj._type = OT_BOOL;
      obj._unVal.nInteger = t ? 1 : 0;
    }

    virtual ~Object() {
        if(release) {
            Release();
            release = false;
        }
    }

    Object& operator=(const Object& so) {
        if(release) {
            Release();
        }
        vm = so.vm;
        obj = so.obj;
        release = so.release;
        sq_addref(vm, &GetObject());
        return *this;
    }

    Object& operator=(Object && so) {
        if(release) {
            Release();
        }
        vm = so.vm;
        obj = so.obj;
        release = so.release;
        sq_resetobject(&so.obj);
        return *this;
    }

    HSQUIRRELVM& GetVM() {
        return vm;
    }

    HSQUIRRELVM GetVM() const {
        return vm;
    }

    SQObjectType GetType() const {
        return GetObject()._type;
    }

    bool IsNull() const {
        return sq_isnull(GetObject());
    }

    virtual HSQOBJECT GetObject() const {
        return obj;
    }

    virtual HSQOBJECT& GetObject() {
        return obj;
    }

    operator HSQOBJECT&() {
        return GetObject();
    }

    void Release() {
        sq_release(vm, &obj);
        sq_resetobject(&obj);
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Attempts to get the value of a slot from the object
    /// \return An Object representing the value of the slot (can be a null object if nothing was found)
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Object GetSlot(const SQChar* slot, bool raw=false) const {
        HSQOBJECT slotObj;
        sq_pushobject(vm, GetObject());
        sq_pushstring(vm, slot, -1);

        SQRESULT res = raw ? sq_rawget_noerr(vm, -2) : sq_get_noerr(vm, -2);
        if(SQ_FAILED(res)) {
            sq_pop(vm, 1);
            return Object(vm); // Return a NULL object
        } else {
            SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, -1, &slotObj)));
            Object ret(slotObj, vm); // must addref before the pop!
            sq_pop(vm, 2);
            return ret;
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Attempts to get the value of an index from the object
    /// \return An Object representing the value of the slot (can be a null object if nothing was found)
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Object GetSlot(SQInteger index, bool raw=false) const {
        HSQOBJECT slotObj;
        sq_pushobject(vm, GetObject());
        sq_pushinteger(vm, index);

        SQRESULT res = raw ? sq_rawget_noerr(vm, -2) : sq_get_noerr(vm, -2);
        if(SQ_FAILED(res)) {
            sq_pop(vm, 1);
            return Object(vm); // Return a NULL object
        } else {
            SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, -1, &slotObj)));
            Object ret(slotObj, vm); // must addref before the pop!
            sq_pop(vm, 2);
            return ret;
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Attempts to get the value of an key from the object
    /// \param slot Key (of any type) of the slot
    /// \return An Object representing the value of the slot (can be a null object if nothing was found)
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Object GetSlot(const Object& slot, bool raw=false) const {
        SQRAT_ASSERT(slot.IsNull() || slot.GetVM() == vm);
        HSQOBJECT res;
        sq_direct_get(vm, &obj, &slot.obj, &res, raw);
        Object ret(res, vm);
        return ret;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Casts the object to a certain C++ type
    /// \tparam T Type to cast to
    /// \return A copy of the value of the Object with the given type
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template <class T>
    T Cast() const {
        static_assert(VarControlsValueLifeTime<T>::value == 0,
                      "direct cast to T failed due to value is bound to Var<T>. use GetVar() instead");
        return GetVar<T>().value;
    }

    template<class T>
    Var<T>  GetVar() const
    {
        sq_pushobject(vm, GetObject());
        Var<T> ret(vm, -1);
        sq_pop(vm, 1);
        return ret;
    }

    /// Gets object slot value as a certain C++ type
    template<class T> T GetSlotValue(const SQChar* slot, T def_val, bool raw=false) const {
        static_assert(VarControlsValueLifeTime<T>::value == 0,
                      "direct cast to T failed due to value is bound to Var<T>");
        sq_pushobject(vm, GetObject());
        sq_pushstring(vm, slot, -1);

        SQRESULT res = raw ? sq_rawget_noerr(vm, -2) : sq_get_noerr(vm, -2);
        if(SQ_FAILED(res)) {
            sq_pop(vm, 1);
            return def_val; // Return a NULL object
        } else {
            T ret = Var<T>(vm, -1).value;
            sq_pop(vm, 2);
            return ret;
        }
    }

    template<class T> T GetSlotValue(SQInteger slot, T def_val, bool raw=false) const {
        static_assert(VarControlsValueLifeTime<T>::value == 0,
                      "direct cast to T failed due to value is bound to Var<T>");
        sq_pushobject(vm, GetObject());
        sq_pushinteger(vm, slot);

        SQRESULT res = raw ? sq_rawget_noerr(vm, -2) : sq_get_noerr(vm, -2);
        if(SQ_FAILED(res)) {
            sq_pop(vm, 1);
            return def_val; // Return a NULL object
        } else {
            T ret = Var<T>(vm, -1).value;
            sq_pop(vm, 2);
            return ret;
        }
    }

    /// Gets object slot value as a certain C++ type
    template<class T> T GetSlotValue(const Object &key, T def_val, bool raw=false) const {
        SQRAT_ASSERT(key.IsNull() || key.GetVM() == vm);
        static_assert(VarControlsValueLifeTime<T>::value == 0,
                      "direct cast to T failed due to value is bound to Var<T>");
        HSQOBJECT res;
        if (SQ_FAILED(sq_direct_get(vm, &obj, &key.obj, &res, raw)))
          return def_val;

        sq_pushobject(vm, res);
        T ret = Var<T>(vm, -1).value;
        sq_pop(vm, 1);
        return ret;
    }

    template <class T>
    inline Object operator[](T slot)
    {
        return GetSlot(slot);
    }

    template <class T>
    const Object operator[](T slot) const
    {
        return GetSlot(slot);
    }

    SQInteger GetSize() const {
        sq_pushobject(vm, GetObject());
        SQInteger ret = sq_getsize(vm, -1);
        sq_pop(vm, 1);
        return ret;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Iterator for going over the slots in the object using Object::Next
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    struct iterator
    {
        friend class Object;

        iterator()
        {
            Index = 0;
            sq_resetobject(&Key);
            sq_resetobject(&Value);
            Key._type = OT_NULL;
            Value._type = OT_NULL;
        }

        const SQChar* getName() { return sq_objtostring(&Key); }
        HSQOBJECT getKey() { return Key; }
        HSQOBJECT getValue() { return Value; }
    private:

        HSQOBJECT Key;
        HSQOBJECT Value;
        SQInteger Index;
    };

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Used to go through all the slots in an Object (same limitations as sq_next)
    ///
    /// \param iter An iterator being used for going through the slots
    ///
    /// \return Whether there is a next slot
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    bool Next(iterator& iter) const
    {
        sq_pushobject(vm,obj);
        sq_pushinteger(vm,iter.Index);
        if(SQ_SUCCEEDED(sq_next(vm,-2)))
        {
            SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm,-1,&iter.Value)));
            SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm,-2,&iter.Key)));
            sq_getinteger(vm,-3,&iter.Index);
            sq_pop(vm,4);
            return true;
        }
        else
        {
            sq_pop(vm,2);
            return false;
        }
    }

protected:
    template<class Func>
    void BindFunc(const SQChar* name, Func func, SQFUNCTION func_thunk, SQInteger nparamscheck, bool staticVar = false)
    {
      sq_pushobject(vm, GetObject());
      sq_pushstring(vm, name, -1);

      SQUserPointer funcPtr = sq_newuserdata(vm, sizeof(func));
      new (funcPtr) Func(func);
      sq_setreleasehook(vm, -1, ImplaceFreeReleaseHook<Func>);

      sq_newclosure(vm, func_thunk, 1);
      if (nparamscheck > 0)
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_setparamscheck(vm, nparamscheck, nullptr)));
      SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, staticVar)));
      sq_pop(vm,1); // pop table
    }

    template<class Func>
    void BindFunc(SQInteger index, Func func, SQFUNCTION func_thunk, SQInteger nparamscheck, bool staticVar = false)
    {
      sq_pushobject(vm, GetObject());
      sq_pushinteger(vm, index);

      SQUserPointer funcPtr = sq_newuserdata(vm, sizeof(func));
      new (funcPtr) Func(func);
      sq_setreleasehook(vm, -1, ImplaceFreeReleaseHook<Func>);

      sq_newclosure(vm, func_thunk, 1);
       if (nparamscheck > 0)
         SQRAT_VERIFY(SQ_SUCCEEDED(sq_setparamscheck(vm, nparamscheck, nullptr)));
      SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, staticVar)));
      sq_pop(vm,1); // pop table
    }

    // Bind a function and it's associated Squirrel closure to the object
    template<class Func>
    void BindOverload(const SQChar* name, Func func, SQFUNCTION func_thunk, SQFUNCTION overload, int argCount, bool staticVar = false) {
        string overloadName = SqOverloadName::Get(name, argCount);

        sq_pushobject(vm, GetObject());

        // Bind overload handler
        sq_pushstring(vm, name, -1);
        sq_pushstring(vm, name, -1); // function name is passed as a free variable
        sq_newclosure(vm, overload, 1);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, staticVar)));

        // Bind overloaded function
        sq_pushstring(vm, overloadName.c_str(), -1);

        SQUserPointer funcPtr = sq_newuserdata(vm, sizeof(func));
        new (funcPtr) Func(func);
        sq_setreleasehook(vm, -1, ImplaceFreeReleaseHook<Func>);

        sq_newclosure(vm, func_thunk, 1);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, staticVar)));

        sq_pop(vm,1); // pop table
    }

    /// Set the value of a variable on the object. Changes to values set this way are not reciprocated
    template<class V>
    inline void BindValue(const SQChar* name, const V& val, bool staticVar = false) {
        sq_pushobject(vm, GetObject());
        sq_pushstring(vm, name, -1);
        PushVar(vm, val);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, staticVar)));
        sq_pop(vm,1); // pop table
    }

    template<class V>
    inline void BindValue(const SQInteger index, const V& val, bool staticVar = false) {
        sq_pushobject(vm, GetObject());
        sq_pushinteger(vm, index);
        PushVar(vm, val);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, staticVar)));
        sq_pop(vm,1); // pop table
    }

    template<class V>
    inline void BindValue(const Object &key, const V& val, bool staticVar = false) {
        sq_pushobject(vm, GetObject());
        sq_pushobject(vm, key.GetObject());
        PushVar(vm, val);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, staticVar)));
        sq_pop(vm,1); // pop table
    }

    // Set the value of an instance on the object. Changes to values set this way are reciprocated back to the source instance
    template<class V>
    inline void BindInstance(const SQChar* name, V* val, bool staticVar = false) {
        sq_pushobject(vm, GetObject());
        sq_pushstring(vm, name, -1);
        PushVar(vm, val);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, staticVar)));
        sq_pop(vm,1); // pop table
    }

    template<class V>
    inline void BindInstance(const SQInteger index, V* val, bool staticVar = false) {
        sq_pushobject(vm, GetObject());
        sq_pushinteger(vm, index);
        PushVar(vm, val);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, staticVar)));
        sq_pop(vm,1); // pop table
    }

    template<class V>
    inline void BindInstance(const Object &key, V* val, bool staticVar = false) {
        sq_pushobject(vm, GetObject());
        sq_pushobject(vm, key.GetObject());
        PushVar(vm, val);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, staticVar)));
        sq_pop(vm,1); // pop table
    }
};

// Optimized no-stack-push-and-pop versions
template<> inline int32_t Object::Cast() const {
    return sq_direct_tointeger(&obj);
}

template<> inline int64_t Object::Cast() const {
    return sq_direct_tointeger(&obj);
}

template<> inline float Object::Cast() const {
    return sq_direct_tofloat(&obj);
}


template<>
inline void Object::BindValue<int>(const SQChar* name, const int & val, bool staticVar /* = false */) {
    sq_pushobject(vm, GetObject());
    sq_pushstring(vm, name, -1);
    PushVar<int>(vm, val);
    SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, staticVar)));
    sq_pop(vm,1); // pop table
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Used to get and push Object instances to and from the stack as references (Object is always a reference)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<>
struct Var<Object> {

    Object value; ///< The actual value of get operations

    /// Attempts to get the value off the stack at idx as an Object
    Var(HSQUIRRELVM vm, SQInteger idx) {
        HSQOBJECT sqValue;
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, idx, &sqValue)));
        value = Object(sqValue, vm);
    }

    /// Called by Sqrat::PushVar to put an Object on the stack
    static void push(HSQUIRRELVM vm, const Object& value) {
        sq_pushobject(vm, value.GetObject());
    }

    static const SQChar * getVarTypeName() { return _SC("object"); }
    static bool check_type(HSQUIRRELVM /*vm*/, SQInteger /*idx*/) {
        return true;
    }
};

template<>
struct Var<Object&> : Var<Object> {Var(HSQUIRRELVM vm, SQInteger idx) : Var<Object>(vm, idx) {}};

template<>
struct Var<const Object&> : Var<Object> {Var(HSQUIRRELVM vm, SQInteger idx) : Var<Object>(vm, idx) {}};

}

#endif
