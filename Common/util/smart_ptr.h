//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// Custom smart pointer types and helpers.
//
//=============================================================================
#ifndef __AGS_CN_UTIL__SMARTPTR_H
#define __AGS_CN_UTIL__SMARTPTR_H

#include <cstddef>

//-----------------------------------------------------------------------------
// ComPtr is the RAII wrapper over a IUnknown* and deriatives
// (or any interface that has a compatible syntax), which ensures a call to
// AddRef on copy and Release on destruction.
// Supports copy and move operations.
// Lets to wraps raw IUnknown pointers without a call to AddRef (for cases when
// a ref was added by factory methods).
// See also:
// https://learn.microsoft.com/en-us/archive/msdn-magazine/2015/february/windows-with-c-com-smart-pointers-revisited
//-----------------------------------------------------------------------------
template<typename TInterface>
class ComPtr
{
public:
    typedef ComPtr<TInterface> MyType;
    // IGuardFace is a trick for hiding AddRef/Release from ComPtr's users
    class IGuardFace : public TInterface
    {
        ULONG __stdcall AddRef();
        ULONG __stdcall Release();
    };

    // Implicitly init with nullptr
    ComPtr() = default;
    // Explicitly init with nullptr
    explicit ComPtr(std::nullptr_t)
    {
        _obj = nullptr;
    }
    // Attach a compatible raw interface ptr
    explicit ComPtr(TInterface *obj)
    {
        _obj = obj;
    }
    // Copy another compatible ComPtr with AddRef
    ComPtr(const ComPtr<TInterface> &iptr)
    {
        _obj = iptr._obj;
        AddRefImpl();
    }
    // Move another compatible ComPtr (pass ptr ownership)
    ComPtr(ComPtr<TInterface> &&iptr)
    {
        _obj = iptr._obj;
        iptr._obj = nullptr; // no AddRef or Release, because we moved
    }
    ~ComPtr()
    {
        ReleaseImpl();
    }

    // Attach() explicitly wraps a raw IUnknown pointer without resetting,
    // this is primarily for assigning a return value from a factory method.
    void Attach(TInterface *obj)
    {
        if (obj != _obj)
        {
            ReleaseImpl();
            _obj = obj;
        }
    }
    // Detach() gives the ownership up and returns raw interface pointer.
    TInterface *Detach()
    {
        TInterface *obj = _obj;
        _obj = nullptr;
        return obj;
    }
    // Acquire() is a *UNSAFE* helper method, that lets to directly assign
    // a pointer field. This is meant exclusively for passing a field address
    // to a factory method which assigns a new pointer value (object address).
    // For example:
    //
    //    ComPtr<Type> ptr;
    //    // this will write a new address into ComPtr
    //    GetSomeObject(ptr.Acquire());
    //
    TInterface **Acquire()
    {
        assert(_obj == nullptr);
        ReleaseImpl(); // because we expect a new value to be written into _obj
        return &_obj;
    }
    // ReleaseAndCheck() releases wrapped object, and returns new ref count.
    // You normally do not need to call this, should assign nullptr instead.
    // The primary purpose of this helper method is to check ref count.
    uint64_t ReleaseAndCheck()
    {
        if (!_obj)
            return 0u;
        uint64_t ref_cnt = _obj->Release();
        _obj = nullptr;
        return ref_cnt;
    }

    TInterface * const get() const { return _obj; }

    operator bool() const { return _obj != nullptr; }
    IGuardFace *operator ->() const { return static_cast<IGuardFace*>(_obj); }

    MyType &operator =(std::nullptr_t)
    {
        ReleaseImpl();
        return *this;
    }
    // Copy another compatible ComPtr with AddRef
    MyType &operator =(const ComPtr<TInterface> &iptr)
    {
        if (_obj != iptr._obj)
        {
            ReleaseImpl();
            _obj = iptr._obj;
            AddRefImpl();
        }
        return *this;
    }
    // Move another compatible ComPtr (pass ptr ownership)
    MyType &operator =(ComPtr<TInterface> &&iptr)
    {
        if (_obj != iptr._obj)
        {
            ReleaseImpl();
            _obj = iptr._obj;
            iptr._obj = nullptr; // no AddRef or Release, because we moved
        }
        return *this;
    }

private:
    void AddRefImpl()
    {
        if (_obj)
            _obj->AddRef();
    }
    void ReleaseImpl()
    {
        TInterface *obj = _obj;
        if (obj)
        {
            _obj = nullptr;
            obj->Release();
        }
    }

    TInterface *_obj = nullptr;
};

// Various comparison operators for ComPtr<T>
template <typename T>
bool operator ==(const ComPtr<T> &iptr, std::nullptr_t) { return iptr.get() == nullptr; }
template <typename T>
bool operator !=(const ComPtr<T> &iptr, std::nullptr_t) { return iptr.get() != nullptr; }
template <typename T1, typename T2>
bool operator ==(const ComPtr<T1> &iptr1, const ComPtr<T2> &iptr2) { return iptr1.get() == iptr2.get(); }
template <typename T1, typename T2>
bool operator !=(const ComPtr<T1> &iptr1, const ComPtr<T2> &iptr2) { return iptr1.get() != iptr2.get(); }

#endif // __AGS_CN_UTIL__SMARTPTR_H
