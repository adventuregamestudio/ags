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
// Delegate class lets subscribe multiple functions, and call them all in a
// sequence with the same list of arguments.
// 
// The registered callback is identified by a pair of a internal ID ("handle"),
// and optionally - a "user object" pointer, either of which may be used to
// unsubscribe this callback later.
// 
// Because there may be multiple subscribers and callbacks, this delegate's
// function types cannot have any return value. If it's essential to receive
// a result, then it should be wrapped into one of the parameters.
// 
// TODO: Currently the delegate is implemented in a pretty naive way,
// could be improved and expanded later. A noteable restriction is that
// you must use the separate ObjectDelegate class if you plan registering
// class methods as callbacks.
//
//=============================================================================
#ifndef __AGS_CN_UTIL__DELEGATE_H
#define __AGS_CN_UTIL__DELEGATE_H

#include <functional>
#include <vector>

namespace AGS
{
namespace Common
{

template<class TObject, class... Args>
class DelegateBase
{
public:
    typedef DelegateBase<Args...> Type;
    typedef uint32_t Handle;
    typedef std::function<void(Args...)> Function;
    typedef std::pair<std::pair<Handle, TObject*>, Function> Callback;

    bool HasCallbacks() const
    {
        return !_callbacks.empty();
    }

    // Subscribe a callback; returns registry Handle, which is used when unsubscribing
    Handle Add(Function func)
    {
        const Handle handle = ++_handleCounter;
        if (_handleCounter == 0u)
            _handleCounter++;
        _callbacks.push_back(Callback(std::make_pair(handle, nullptr), func));
        return handle;
    }

    // Subscribe a callback and provide arbitrary "user object", which may be used when unsubscribing
    Handle Add(Function func, TObject *user_object)
    {
        assert(user_object);
        if (!user_object)
            return 0u;

        auto it = std::find_if(_callbacks.begin(), _callbacks.end(), CallbackFinder(user_object));
        if (it != _callbacks.end())
            return it->first.first; // callback key exists

        const Handle handle = ++_handleCounter;
        if (_handleCounter == 0u)
            _handleCounter++;
        _callbacks.push_back(Callback(std::make_pair(handle, user_object), func));
        return handle;
    }

    // Unsubscribe a callback identified with its handle (previously returned by Add method)
    void Remove(Handle handle)
    {
        assert(handle > 0u);
        if (handle == 0u)
            return;

        auto it = std::find_if(_callbacks.begin(), _callbacks.end(), CallbackFinder(handle));
        if (it != _callbacks.end())
            _callbacks.erase(it);
    }

    // Unsubscribe a callback identified with a "user object" pointer.
    void Remove(TObject *user_object)
    {
        assert(user_object);
        if (!user_object)
            return;

        auto it = std::find_if(_callbacks.begin(), _callbacks.end(), CallbackFinder(user_object));
        if (it != _callbacks.end())
            _callbacks.erase(it);
    }

    // Erase all callbacks
    void Clear()
    {
        _callbacks.clear();
    }

    // Call all registered callbacks, passing the argument list
    void Call(Args ...args)
    {
        for (auto &c : _callbacks)
            c.second(std::forward<Args>(args)...);
    }

    inline operator bool() const
    {
        return HasCallbacks();
    }

    // Call all registered callbacks, passing the argument list
    inline void operator()(Args ...args)
    {
        Call(std::forward<Args>(args)...);
    }

protected:
    struct CallbackFinder
    {
    public:
        CallbackFinder(Handle handle) : _handle(handle) {}
        CallbackFinder(TObject *object) : _object(object) {}
        bool operator()(const Callback &c) const { return (_handle != 0u && c.first.first == _handle) || (_object && c.first.second == _object); }
    private:
        Handle   _handle = 0u;
        TObject *_object = nullptr;
    };

    std::vector<Callback> _callbacks;
    Handle _handleCounter = 0u;
};

template<class... Args>
class Delegate : public DelegateBase<void, Args...>
{
    typedef DelegateBase<void, Args...> BaseType;
};

template<class TObject, class... Args>
class ObjectDelegate : protected DelegateBase<TObject, TObject*, Args...>
{
public:
    typedef DelegateBase<TObject, TObject*, Args...> BaseType;
    typedef typename BaseType::Handle Handle;
    typedef typename BaseType::Function Function;

    bool HasCallbacks() const
    {
        return BaseType::HasCallbacks();
    }

    // Subscribe a callback as method of the given object instance;
    // returns registry Handle, which may be used when unsubscribing
    Handle Add(TObject *object, Function func)
    {
        return BaseType::Add(func, object);
    }

    // Unsubscribe a callback identified with its handle (previously returned by Add method)
    void Remove(Handle handle)
    {
        BaseType::Remove(handle);
    }

    // Unsubscribe a callback belonging to the object instance
    void Remove(TObject *object)
    {
        BaseType::Remove(object);
    }

    // Erase all callbacks
    void Clear()
    {
        BaseType::Clear();
    }

    // Call all registered callbacks, passing the argument list
    void Call(Args ...args)
    {
        for (auto &c : BaseType::_callbacks)
            c.second(c.first.second, std::forward<Args>(args)...);
    }

    inline operator bool() const
    {
        return BaseType::HasCallbacks();
    }

    // Call all registered callbacks, passing the argument list
    inline void operator()(Args ...args)
    {
        Call(std::forward<Args>(args)...);
    }
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__DELEGATE_H
