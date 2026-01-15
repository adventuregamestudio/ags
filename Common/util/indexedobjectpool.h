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
// IndexedObjectPool is a container that never erases its elements from memory.
// Instead it keeps a record of which elements are "in use" and which are free.
// The new elements are added into the "free" slots so long as there is one,
// and when there's none, the container expands to make more.
// When a element is removed, it's simply marked as "unused", so there's no
// element moves when this is done.
// This behavior results in fast real-time insertions and removals, while still
// being able to use indexes for the fast access.
// 
// TODO: also keep track of the highest used index. This might optimize
// iterations over large object pools.
//
// IndexedPoolBase is a base class that takes care of free indexes only,
// but does not store elements themselves. It is picked out from the
// IndexedObjectPool for two reasons:
// * possibly reduce code generation if multiple pools use same index type.
// * provide barebones pool class if another storage method is used.
//
//=============================================================================
#ifndef __AGS_CN_UTIL__INDEXEDOBJECTPOOL_H
#define __AGS_CN_UTIL__INDEXEDOBJECTPOOL_H

#include <stack>
#include <vector>

//
// IndexedPoolBase class keeps track of which storage indexes are free and
// which are in use. Does not store any elements on its own.
// This class is suggested to be used as a parent for the actual storage
// class.
//
template <typename TIndex>
class IndexedPoolBase
{
public:
    // Constructs object pool, designating a number of fixed slots
    // that are allocated initially and never selected for use automatically.
    // (They may be still assigned an element via Set(elem, index).)
    IndexedPoolBase(size_t fixed_count)
        : _fixedCount(fixed_count)
    {
        _isFree.resize(fixed_count, true);
    }

    // Returns the count of the used (aka valid) elements
    size_t GetCount() const { return _count; }
    // Tells if particular index is free
    bool IsFree(TIndex index) const { return _isFree[index]; }
    // Tells if particular index is being used
    bool IsInUse(TIndex index) const { return !_isFree[index]; }

    // Requests an index to use for the new element, marks that index as "in use"
    TIndex Add()
    {
        return AcquireFreeSlot();
    }

    // Requests particular index to be marked as "in use"
    void Set(const TIndex &index)
    {
        AcquireSlot(index);
    }

    // Initializes whole array of indexes, some of which may be marked as "in use" and others as "free"
    void Set(size_t elem_count, const std::vector<bool> &is_used)
    {
        InitFreeIndexes(std::max(elem_count, _fixedCount), is_used);
    }

    // Mark certain index as free
    void Free(const TIndex &index)
    {
        if (!_isFree[index])
        {
            _isFree[index] = true;
            if (static_cast<size_t>(index) >= _fixedCount)
                _freeIds.push(index);
            _count--;
        }
    }

    // Resets the container
    void Clear()
    {
        _isFree.clear();
        _isFree.resize(_fixedCount, true);
        _freeIds = std::stack<TIndex>();
        _count = 0;
    }

    // Reserves memory for certain amount of indexes, useful when we expect
    // a sequential addition of very big number of elements ahead
    void Reserve(size_t size)
    {
        _isFree.reserve(size);
    }

protected:
    // Returns the next free index, allocates more if necessary
    TIndex GetFreeIndex()
    {
        if (_freeIds.empty())
        {
            const TIndex index = static_cast<TIndex>(_isFree.size());
            _isFree.resize(static_cast<size_t>(index) + 1);
            return index;
        }
        else
        {
            const TIndex index = _freeIds.top();
            _freeIds.pop();
            return index;
        }
    }

    // Finds a free index, marks it as used, increments valid elements count
    TIndex AcquireFreeSlot()
    {
        const TIndex index = GetFreeIndex();
        _isFree[index] = false;
        _count++;
        return index;
    }

    // Marks certain index as used
    void AcquireSlot(const TIndex &index)
    {
        assert(index >= 0u);
        if (static_cast<size_t>(index) < _isFree.size())
        {
            if (_isFree[index])
            {
                if (static_cast<size_t>(index) >= _fixedCount)
                {
                    // If we set element directly anywhere beyond the fixed count,
                    // then we must remove this index from the _freeIds stack.

                    // Following is very inefficient, but still (apparently) more optimal
                    // than having to double check and skip non-free ids from _freeIds stack
                    // whenever selecting the next free id in GetFreeIndex().
                    // We normally assume that Set(elem, index) will never be used to set
                    // non-fixed slot directly, so if user did so, then it's their fault :/.

                    // Move free ids from _freeIds to a backup stack, in reverse,
                    // until we find a index that we set here, and can throw that away.
                    std::stack<TIndex> backup_ids;
                    do
                    {
                        if (_freeIds.top() == index)
                        {
                            _freeIds.pop();
                            break;
                        }
                        backup_ids.push(_freeIds.top());
                        _freeIds.pop();
                    } while (!_freeIds.empty());
                    // After removing current index from "free", move backed up ids back
                    while (!backup_ids.empty())
                    {
                        _freeIds.push(backup_ids.top());
                        backup_ids.pop();
                    }
                }

                _isFree[index] = false;
                _count++;
            }
        }
        else
        {
            const size_t new_size = index + 1;
            for (TIndex free_idx = _isFree.size(); free_idx != index && static_cast<size_t>(free_idx) < new_size; ++free_idx)
                _freeIds.push(free_idx);
            _isFree.resize(new_size);
            _count++;
        }
    }

    // Initializes array of indexes as either used or free
    void InitFreeIndexes(size_t elem_count, const std::vector<bool> &is_used)
    {
        _isFree.resize(elem_count, true);
        _freeIds = std::stack<TIndex>();
        _count = 0;

        for (size_t idx = 0u; idx < _isFree.size() && idx < is_used.size(); ++idx)
        {
            _isFree[idx] = !is_used[idx];
            _count += static_cast<int>(is_used[idx]);
        }
    }

    std::vector<bool>  _isFree;
    std::stack<TIndex> _freeIds;
    size_t _count = 0u;
    const size_t _fixedCount = 0u;
};

//
// IndexedObjectPool provides a element storage and keeps record of free
// and used indexes.
//
template <typename TElem, typename TIndex = size_t>
class IndexedObjectPool : protected IndexedPoolBase<TIndex>
{
public:
    using ContainerType = std::vector<TElem>;

    IndexedObjectPool() = default;
    // Constructs object pool, designating a number of fixed slots
    // that are allocated initially and never selected for use automatically.
    // (They may be still assigned an element via Set(elem, index).)
    IndexedObjectPool(size_t fixed_count)
        : IndexedPoolBase<TIndex>(fixed_count)
    {
        _elems.resize(fixed_count);
    }

    // STL-compatibility
    typename ContainerType::iterator begin() { return _elems.begin(); }
    typename ContainerType::iterator end() { return _elems.end(); }
    typename ContainerType::const_iterator begin() const { return _elems.begin(); }
    typename ContainerType::const_iterator end() const { return _elems.end(); }
    typename ContainerType::const_iterator cbegin() const { return _elems.cbegin(); }
    typename ContainerType::const_iterator cend() const { return _elems.cend(); }

    // Returns the size of container, how many elements are in the pool;
    // this includes both used and free elements
    size_t size() const { return _elems.size(); }
    const TElem &operator [](TIndex index) const { return _elems[static_cast<size_t>(index)]; }
    TElem &operator [](TIndex index)
    {
        assert(index >= 0u && static_cast<size_t>(index) < _elems.size());
        assert(!IndexedPoolBase<TIndex>::_isFree[index]);
        return _elems[static_cast<size_t>(index)];
    }

    // Returns the count of the used (aka valid) elements
    size_t GetCount() const { return IndexedPoolBase<TIndex>::_count; }
    // Tells if particular index is free
    bool IsFree(TIndex index) const { return IndexedPoolBase<TIndex>::_isFree[index]; }
    // Tells if particular index is being used
    bool IsInUse(TIndex index) const { return !IndexedPoolBase<TIndex>::_isFree[index]; }

    // Adds new element, initialized with default value; returns its index
    TIndex Add()
    {
        TIndex index = AcquireFreeSlot();
        _elems[index] = {};
        return index;
    }

    // Adds new element by copying a value; returns its index
    TIndex Add(const TElem &elem)
    {
        TIndex index = AcquireFreeSlot();
        _elems[index] = elem;
        return index;
    }

    // Adds new element by moving a value; returns its index
    TIndex Add(TElem &&elem)
    {
        TIndex index = AcquireFreeSlot();
        _elems[index] = std::move(elem);
        return index;
    }

    // Adds new element, initialized with default value, at the specified index
    void Set(const TIndex &index)
    {
        AcquireSlot(index);
        _elems[index] = {};
    }

    // Adds new element at the specified index, copies a value over
    void Set(const TElem &elem, const TIndex &index)
    {
        AcquireSlot(index);
        _elems[index] = elem;
    }

    // Adds new element at the specified index, moves a value over
    void Set(TElem &&elem, const TIndex &index)
    {
        AcquireSlot(index);
        _elems[index] = std::move(elem);
    }

    // Initializes whole array of elements, some of the slots may be marked as "in use" and others as "free"
    void Set(const std::vector<TElem> &elems, const std::vector<bool> &is_used)
    {
        _elems = elems;
        if (_elems.size() < IndexedPoolBase<TIndex>::_fixedCount)
            _elems.resize(IndexedPoolBase<TIndex>::_fixedCount);
        IndexedPoolBase<TIndex>::InitFreeIndexes(_elems.size(), is_used);
    }

    // Initializes whole array of elements, some of the slots may be marked as "in use" and others as "free"
    void Set(std::vector<TElem> &&elems, const std::vector<bool> &is_used)
    {
        _elems = std::move(elems);
        if (_elems.size() < IndexedPoolBase<TIndex>::_fixedCount)
            _elems.resize(IndexedPoolBase<TIndex>::_fixedCount);
        IndexedPoolBase<TIndex>::InitFreeIndexes(_elems.size(), is_used);
    }

    // Resets element and marks its index as free
    void Free(const TIndex &index)
    {
        _elems[index] = {};
        IndexedPoolBase<TIndex>::Free(index);
    }

    // Resets the container
    void Clear()
    {
        _elems.clear();
        _elems.resize(IndexedPoolBase<TIndex>::_fixedCount);
        IndexedPoolBase<TIndex>::Clear();
    }

    // Reserves memory for certain amount of elements, useful when we expect
    // a sequential addition of very big number of elements ahead
    void Reserve(size_t size)
    {
        _elems.reserve(size);
        IndexedPoolBase<TIndex>::Reserve(size);
    }

private:
    // Finds a free index, marks it as used, increments valid elements count
    TIndex AcquireFreeSlot()
    {
        const TIndex index = IndexedPoolBase<TIndex>::AcquireFreeSlot();
        if (static_cast<size_t>(index) >= _elems.size())
            _elems.resize(static_cast<size_t>(index) + 1);
        return index;
    }

    // Marks certain index as used
    void AcquireSlot(const TIndex &index)
    {
        assert(index >= 0u);
        IndexedPoolBase<TIndex>::AcquireSlot(index);
        if (static_cast<size_t>(index) >= _elems.size())
            _elems.resize(index + 1u);
    }

    std::vector<TElem> _elems;
};

#endif // __AGS_CN_UTIL__OBJECTPOOL_H
