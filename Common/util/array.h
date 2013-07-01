//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_CN_UTIL__ARRAY_H
#define __AGS_CN_UTIL__ARRAY_H

#include <new>
#include <memory>
#include <stdlib.h>
#include <string.h>
#include "core/types.h"
#include "debug/assert.h"
#include "util/math.h"
#include "util/memory.h"
#include "util/stream.h"

namespace AGS
{
namespace Common
{

template <class T> struct ArrayBase
{
    int32_t RefCount;   // reference count
    // Capacity and Length do not include null-terminator
    int32_t Capacity;   // available space, in characters
    int32_t Length;     // used space
    T       *Arr;       // pointer to constructed elements

    ArrayBase()
        : RefCount(0)
        , Capacity(0)
        , Length(0)
        , Arr(NULL)
    {
    }
};

//-----------------------------------------------------------------------------
//
// Array<T> specialization for basic types:
// no construction/destruction, elements are copied with memcpy/memmove.
//
//-----------------------------------------------------------------------------

template <class T> class Array
{
public:
    Array()
        : _data(NULL)
    {
    }
    Array(const Array<T> &arr)
        : _data(NULL)
    {
        *this = arr;
    }
    ~Array()
    {
        Free();
    }

    // Get element count
    inline int GetCount() const
    {
        return _meta ? _meta->Length : 0;
    }
    // Know if the array is empty
    inline bool IsEmpty() const
    {
        return _meta ? _meta->Length == 0 : true;
    }
    // Get constant pointer to underlying C-array;
    // it is generally unsafe to keep returned address for future reference,
    // because Array's buffer may eventually be reallocated.
    inline const T *GetCArr() const
    {
        return _meta ? _meta->Arr : NULL;
    }
    // Those getters are for tests only, hence ifdef _DEBUG
#ifdef _DEBUG
    inline const char *GetData() const
    {
        return _data;
    }

    inline int GetCapacity() const
    {
        return _meta ? _meta->Capacity : 0;
    }

    inline int GetRefCount() const
    {
        return _meta ? _meta->RefCount : 0;
    }
#endif
    // Make new array, constructing given N number of elements;
    // New() guarantees that the buffer will be of minimal possible size
    // to accommodate N elements.
    // The initial values of elements are undefined.
    void New(int count)
    {
        if (!_meta && count <= 0)
        {
            return;
        }
        count = Math::Max(count, 0);
        if (!_meta || _meta->RefCount > 1 || _meta->Capacity != count)
        {
            Free();
            CreateBuffer(count);
        }
        _meta->Length = count;
    }
    // Same as first New(), but also explicitly assigns initial value to
    // every element.
    void New(int count, const T &value)
    {
        if (!_meta && count <= 0)
        {
            return;
        }
        count = Math::Max(count, 0);
        if ((!_meta && count > 0) || _meta->RefCount > 1 || _meta->Capacity != count)
        {
            Free();
            CreateBuffer(count);
        }
        Construct(0, count, value);
        _meta->Length = count;
    }
    // Create new array and copy N elements from C-array
    void CreateFromCArray(const T *arr, int count)
    {
        New(count);
        if (arr && count > 0)
        {
            Memory::Copy<T>(_meta->Arr, arr, count);
        }
    }
    // Destroy all existing elements in array
    void Empty()
    {
        if (_meta)
        {
            if (_meta->RefCount > 1)
            {
                Free();
            }
            _meta->Length = 0;
        }
    }
    // Decrement ref counter and deallocate data if must.
    // Free() should be called only when buffer is not needed anymore;
    // if array must be truncated to zero length, while retaining
    // allocated buffer, call Empty() instead.
    void Free()
    {
        if (_meta)
        {
            _meta->RefCount--;
            if (!_meta->RefCount)
            {
                delete [] _data;
            }
        }
        _data = NULL;
    }

    // Get existing element at given index
    inline const T &GetAt(int index) const
    {
        assert (_meta && index >= 0 && index < _meta->Length);
        return _meta->Arr[index];
    }
    inline T &GetAt(int index)
    {
        assert (_meta && index >= 0 && index < _meta->Length);
        BecomeUnique();
        return _meta->Arr[index];
    }
    // Set new element at given index
    void SetAt(int index, const T &val)
    {
        assert (_meta && index >= 0 && index < _meta->Length);
        if (_meta && index >= 0 && index < _meta->Length)
        {
            BecomeUnique();
            _meta->Arr[index] = val;
        }
    }

    // Create new array, reading up to N elements from stream by copying raw
    // bytes over array buffer.
    // It is generally unsafe to use this function for arrays of non-POD types.
    void ReadRaw(Stream *in, int count)
    {
        if (in && count > 0)
        {
            if (MustReserve(count))
            {
                ReserveAndShift(-1, count - GetCount());
            }
            count = in->ReadArray(_meta->Arr, sizeof(T), count);
             _meta->Length = count;
        }
        else
        {
            Empty();
        }
    }
    // Reads up to N elements from stream, copying the over existing elements.
    // It is generally unsafe to use this function for arrays of non-POD types.
    void ReadRawOver(Stream *in, int from, int count)
    {
        if (_meta && _meta->Length > 0 && count != 0 && in)
        {
            count = count >= 0 ? count : _meta->Length;
            Math::ClampLength(0, _meta->Length, from, count);
            if (count > 0)
            {
                BecomeUnique();
                in->ReadArray(_meta->Arr + from, sizeof(T), count);
            }
        }
    }
    inline void ReadRawOver(Stream *in, int count = -1)
    {
        ReadRawOver(in, 0, count);
    }
    // Write array elements to stream, putting raw bytes from array buffer.
    // It is not recommended to use this function for non-POD types.
    void WriteRaw(Stream *out, int from, int count) const
    {
        if (_meta && _meta->Length > 0 && out)
        {
            count = count >= 0 ? count : _meta->Length;
            Math::ClampLength(0, _meta->Length, from, count);
            if (count > 0)
            {
                out->WriteArray(_meta->Arr, sizeof(T), _meta->Length);
            }
        }
    }
    inline void WriteRaw(Stream *out, int count = -1) const
    {
        WriteRaw(out, 0, count);
    }

    // Ensure array has at least space to store N chars;
    // this does not change array contents, nor length
    void Reserve(int max_length)
    {
        if (MustReserve(max_length))
        {
            ReserveAndShift(-1, max_length - GetCount());
        }
    }
    // Ensure array has at least space to store N additional chars
    void ReserveMore(int more_length)
    {
        ReserveAndShift(-1, more_length);
    }
    // Make array's buffer as small as possible to hold current data
    void Compact()
    {
        if (_meta && _meta->Capacity > _meta->Length)
        {
            Copy(_meta->Length);
        }
    }
    // Add single element at array's end
    void Append(const T &value)
    {
        if (MustReserveMoreRight(1))
        {
            ReserveAndShift(-1, 1);
        }
        _meta->Arr[_meta->Length++] = value;
    }
    // Add number of unitialized elements at array's end
    void AppendCount(int count)
    {
        if (count > 0)
        {
            if (MustReserveMoreRight(count))
            {
                ReserveAndShift(-1, count);
            }
            _meta->Length += count;
        }
    }
    // Add number of elements at array's end and initialize them
    // with given value
    void AppendCount(int count, const T &value)
    {
        if (count > 0)
        {
            if (MustReserveMoreRight(count))
            {
                ReserveAndShift(-1, count);
            }
            Construct(_meta->Length, _meta->Length + count, value);
            _meta->Length += count;
        }
    }
    void Insert(int at_index, const T &value)
    {
        Math::Clamp(0, GetCount(), at_index);
        ReserveAndShift(at_index, 1);
        _meta->Arr[at_index] = value;
        _meta->Length++;
    }
    void InsertCount(int at_index, int count, const T &value)
    {
        if (count > 0)
        {
            Math::Clamp(0, GetCount(), at_index);
            ReserveAndShift(at_index, count);
            Construct(at_index, at_index + count, value);
            _meta->Length += count;
        }
    }
    void Remove(int at_index, int count = 1)
    {
        if (_meta && _meta->Length > 0)
        {
            count = count >= 0 ? count : _meta->Length;
            Math::ClampLength(0, _meta->Length, at_index, count);
            if (count > 0)
            {
                if (_meta->RefCount > 1)
                {
                    Copy(_meta->Length - count, 0, false, at_index, count);
                }
                else
                {
                    _meta->Arr = Memory::ExcludeDataInBuffer<T>(_meta->Arr, _meta->Length, at_index, count);
                    _meta->Length -= count;
                }
            }
        }
    }
    // Grows or truncates array to match given N number of elements,
    // keeps up to N of previously existed elements.
    void SetLength(int count)
    {
        count = Math::Max(count, 0);
        if (count > 0)
        {
            if (MustReserve(count))
            {
                ReserveAndShift(-1, count - GetCount());
            }
            else if (_meta->RefCount > 1)
            {
                Copy(count);
            }
            _meta->Length = count;
        }
        else
        {
            Empty();
        }
    }
    // Same as first SetLength(), but but also explicitly assigns initial value to
    // every new element.
    void SetLength(int count, const T &value)
    {
        count = Math::Max(count, 0);
        if (count > 0)
        {
            if (MustReserve(count))
            {
                ReserveAndShift(-1, count - GetCount());
                Construct(_meta->Length, count, value);
            }
            else if (_meta->RefCount > 1)
            {
                Copy(count);
            }
            _meta->Length = count;
        }
        else
        {
            Empty();
        }
    }
    // Grows array to accommodate N elements; does nothing if array is already at least that large
    inline void GrowTo(int count)
    {
        if (count > GetCount())
        {
            SetLength(count);
        }
    }
    // Same as first GrowTo(), but but also explicitly assigns initial value to
    // every new element.
    inline void GrowTo(int count, const T &value)
    {
        if (count > GetCount())
        {
            SetLength(count, value);
        }
    }

    inline void QSort(int (*pfn_compare)(const T*, const T*))
    {
        if (_data)
        {
            qsort(_meta->Arr, _meta->Length, sizeof(T),
                (int (*)(const void*, const void*))pfn_compare);
        }
    }
    // Assign array by sharing data reference
    Array<T> &operator=(const Array<T>& arr)
    {
        if (_data != arr._data)
        {
            Free();
            if (arr._data && arr._meta->Length > 0)
            {
                _data = arr._data;
                if (_meta)
                {
                    _meta->RefCount++;
                }
            }
        }
        return *this;
    }
    inline const T &operator[](int index) const
    {
        assert (_meta && index >= 0 && index < _meta->Length);
        return _meta->Arr[index];
    }
    inline T &operator[](int index)
    {
        assert (_meta && index >= 0 && index < _meta->Length);
        BecomeUnique();
        return _meta->Arr[index];
    }

protected:
    // Construct number of elements between two array indexes [begin;end)
    void Construct(int begin, int end, const T &value)
    {
        const T *end_ptr = _meta->Arr + end;
        for (T *elem_ptr = _meta->Arr + begin; elem_ptr != end_ptr; ++elem_ptr)
        {
            *elem_ptr = value;
        }
    }
    // Creates new empty array with buffer enough to accommodate given length
    void CreateBuffer(int max_length)
    {
        _data = new char[sizeof(ArrayBase<T>) + max_length * sizeof(T)];
        _meta->RefCount = 1;
        _meta->Capacity = max_length;
        _meta->Length = 0;
        _meta->Arr = (T*)(_data + sizeof(ArrayBase<T>));
    }
    // Release array and copy data to the new buffer, optionally inserting
    // or removing extra space before, after or in the middle of copied array
    void Copy(int new_capacity, int copy_from = 0, bool expand = true, int change_at = 0, int change_count = 0)
    {
        char *new_data = new char[sizeof(ArrayBase<T>) + new_capacity * sizeof(T)];
        // remember, that _meta->Arr may point to any address in buffer
        T *arr_head  = (T*)(new_data + sizeof(ArrayBase<T>));
        T *new_arr;
        int copy_length = _meta->Length - copy_from;
        if (change_count == 0)
        {
            copy_length = Math::Min(copy_length, new_capacity);
            Memory::Copy<T>(arr_head, _meta->Arr + copy_from, copy_length);
            new_arr = arr_head;
        }
        else if (expand)
        {
            new_arr = Memory::CopyAndInsertSpace<T>(arr_head, _meta->Arr + copy_from, copy_length,
                                                 change_at, change_count);
        }
        else
        {
            new_arr = Memory::CopyPartial<T>(arr_head, _meta->Arr + copy_from, copy_length,
                                          change_at, change_count);
            copy_length -= change_count;
        }

        Free();
        _data = new_data;
        _meta->RefCount = 1;
        _meta->Capacity = new_capacity;
        _meta->Length = copy_length;
        _meta->Arr = new_arr;        
    }
    // Ensure this array is a compact independent copy, with ref counter = 1
    inline void BecomeUnique()
    {
        if (_meta->RefCount > 1)
        {
            Copy(_meta->Length);
        }
    }
    // TODO: more balanced appending/prepending strategy
    void ReserveAndShift(int reserve_at, int need_space)
    {
        if (_meta)
        {
            reserve_at = reserve_at < 0 ? _meta->Length : Math::Min(reserve_at, _meta->Length);
            const int total_length = _meta->Length + need_space;
            if (_meta->Capacity < total_length)
            {
                // grow by 100% or at least to total_size, or at least by 4 elements
                int grown_length = Math::Max(_meta->Capacity << 1, _meta->Capacity + 4);
                Copy(Math::Max(total_length, grown_length), 0, true, reserve_at, need_space);
            }
            else if (_meta->RefCount > 1)
            {
                Copy(total_length, 0, true, reserve_at, need_space);
            }
            else
            {
                // make sure we make use of all of our space
                _meta->Arr = Memory::ExpandDataInBuffer<T>((T*)(_data + sizeof(ArrayBase<T>)), _meta->Capacity, _meta->Arr, _meta->Length,
                    reserve_at, need_space);
            }
        }
        else
        {
            CreateBuffer(need_space);
        }
    }

    union
    {
        char         *_data;
        ArrayBase<T> *_meta;
    };

    inline T    *BufferBegin() const { return (T*)(_data + sizeof(ArrayBase<T>)); }
    inline T    *BufferEnd()   const { return (T*)(_data + sizeof(ArrayBase<T>)) + _meta->Capacity; }
    inline T    *StringBegin() const { return _meta->Arr; }
    inline T    *StringEnd()   const { return _meta->Arr + _meta->Length; }
    inline int  SpaceLeft()    const { return StringBegin() - BufferBegin(); }
    inline int  SpaceRight()   const { return BufferEnd() - StringEnd(); }
    inline bool MustReserve(int total_space)         const { return !_meta || _meta->RefCount > 1 ||
        total_space > _meta->Length && SpaceRight() < total_space - _meta->Length; }
    inline bool MustReserveMoreRight(int need_space) const { return !_meta || _meta->RefCount > 1 || SpaceRight() < need_space; }
    inline bool MustReserveMoreLeft(int need_space)  const { return !_meta || _meta->RefCount > 1 || SpaceLeft() < need_space; }
};



//-----------------------------------------------------------------------------
//
// Array<T> specialization for object types:
// construction/destruction at (de)allocation, elements are copied by
// assignment.
//
//-----------------------------------------------------------------------------

template <class T> class ObjectArray
{
public:
    ObjectArray()
        : _data(NULL)
    {
    }
    ObjectArray(const ObjectArray<T> &arr)
        : _data(NULL)
    {
        *this = arr;
    }
    ~ObjectArray()
    {
        Free();
    }

    // Get element count
    inline int GetCount() const
    {
        return _meta ? _meta->Length : 0;
    }
    // Know if the array is empty
    inline bool IsEmpty() const
    {
        return _meta ? _meta->Length == 0 : true;
    }
    // Get constant pointer to underlying C-array;
    // it is generally unsafe to keep returned address for future reference,
    // because Array's buffer may eventually be reallocated.
    inline const T *GetCArr() const
    {
        return _meta ? _meta->Arr : NULL;
    }
    // Those getters are for tests only, hence ifdef _DEBUG
#ifdef _DEBUG
    inline const char *GetData() const
    {
        return _data;
    }

    inline int GetCapacity() const
    {
        return _meta ? _meta->Capacity : 0;
    }

    inline int GetRefCount() const
    {
        return _meta ? _meta->RefCount : 0;
    }
#endif
    // Make new array, constructing given N number of elements;
    // New() guarantees that the buffer will be of minimal possible size
    // to accommodate N elements.
    // The initial values of elements are undefined.
    void New(int count)
    {
        if (!_meta && count <= 0)
        {
            return;
        }
        count = Math::Max(count, 0);
        if (_meta && _meta->RefCount == 1 && _meta->Capacity == count)
        {
            Deconstruct(0, _meta->Length);
        }
        else
        {
            Free();
            CreateBuffer(count);
        }
        Construct(0, count);
        _meta->Length = count;
    }
    // Same as first New(), but also explicitly assigns initial value to
    // every element.
    void New(int count, const T &value)
    {
        if (!_meta && count <= 0)
        {
            return;
        }
        count = Math::Max(count, 0);
        if (_meta && _meta->RefCount == 1 && _meta->Capacity == count)
        {
            Deconstruct(0, _meta->Length);
        }
        else
        {
            Free();
            CreateBuffer(count);
        }
        Construct(0, count, &value);
        _meta->Length = count;
    }
    // Create new array and copy N elements from C-array
    void CreateFromCArray(const T *arr, int count)
    {
        if (arr && count > 0)
        {
            if (_meta && _meta->RefCount == 1 && _meta->Capacity == count)
            {
                Deconstruct(0, _meta->Length);
            }
            else
            {
                Free();
                CreateBuffer(count);
            }
            const T *end_ptr = _meta->Arr + count;
            T *dest_ptr = _meta->Arr;
            while (dest_ptr < end_ptr)
            {
                *dest_ptr++ = *arr++;
            }
        }
        else
        {
            Free();
        }
    }
    // Destroy all existing elements in array
    void Empty()
    {
        if (_meta)
        {
            if (_meta->RefCount > 1)
            {
                Free();
            }
            else
            {
                Deconstruct(0, _meta->Length);
            }
            _meta->Length = 0;
        }
    }
    // Decrement ref counter and deallocate data if must.
    // Free() should be called only when buffer is not needed anymore;
    // if array must be truncated to zero length, while retaining
    // allocated buffer, call Empty() instead.
    void Free()
    {
        if (_meta)
        {
            _meta->RefCount--;
            if (!_meta->RefCount)
            {
                Deconstruct(0, _meta->Length);
                delete [] _data;
            }
        }
        _data = NULL;
    }

    // Get existing element at given index
    inline const T &GetAt(int index) const
    {
        assert (_meta && index >= 0 && index < _meta->Length);
        return _meta->Arr[index];
    }
    inline T &GetAt(int index)
    {
        assert (_meta && index >= 0 && index < _meta->Length);
        BecomeUnique();
        return _meta->Arr[index];
    }
    // Set new element at given index
    void SetAt(int index, const T &val)
    {
        assert (_meta && index >= 0 && index < _meta->Length);
        if (_meta && index >= 0 && index < _meta->Length)
        {
            BecomeUnique();
            _meta->Arr[index] = val;
        }
    }

    // Ensure array has at least space to store N chars;
    // this does not change array contents, nor length
    void Reserve(int max_length)
    {
        if (MustReserve(max_length))
        {
            ReserveAndShift(-1, max_length - GetCount());
        }
    }
    // Ensure array has at least space to store N additional chars
    void ReserveMore(int more_length)
    {
        ReserveAndShift(-1, more_length);
    }
    // Make array's buffer as small as possible to hold current data
    void Compact()
    {
        if (_meta && _meta->Capacity > _meta->Length)
        {
            Copy(_meta->Length);
        }
    }
    // Add single element at array's end
    void Append(const T &value)
    {
        if (MustReserveMoreRight(1))
        {
            ReserveAndShift(-1, 1);
        }
        new (_meta->Arr + _meta->Length++) T(value);
    }
    // Add number of elements with default values at array's end
    void AppendCount(int count)
    {
        if (count > 0)
        {
            if (MustReserveMoreRight(count))
            {
                ReserveAndShift(-1, count);
            }
            Construct(_meta->Length, _meta->Length + count);
            _meta->Length += count;
        }
    }
    // Add number of elements at array's end and initialize them
    // with given value
    void AppendCount(int count, const T &value)
    {
        if (count > 0)
        {
            if (MustReserveMoreRight(count))
            {
                ReserveAndShift(-1, count);
            }
            Construct(_meta->Length, _meta->Length + count, &value);
            _meta->Length += count;
        }
    }
    void Insert(int at_index, const T &value)
    {
        Math::Clamp(0, GetCount(), at_index);
        ReserveAndShift(at_index, 1);
        new (_meta->Arr + at_index) T(value);
        _meta->Length++;
    }
    void InsertCount(int at_index, int count, const T &value)
    {
        if (count > 0)
        {
            Math::Clamp(0, GetCount(), at_index);
            ReserveAndShift(at_index, count);
            Construct(at_index, at_index + count, value);
            _meta->Length += count;
        }
    }
    void Remove(int at_index, int count = 1)
    {
        if (_meta && _meta->Length > 0)
        {
            count = count >= 0 ? count : _meta->Length;
            Math::ClampLength(0, _meta->Length, at_index, count);
            if (count > 0)
            {
                if (_meta->RefCount > 1)
                {
                    Copy(_meta->Length - count, 0, false, at_index, count);
                }
                else
                {
                    _meta->Arr = Memory::ExcludeObjectsInBuffer<T>(_meta->Arr, _meta->Length, at_index, count);
                    _meta->Length -= count;
                }
            }
        }
    }
    // Grows or truncates array to match given N number of elements,
    // keeps up to N of previously existed elements.
    void SetLength(int count)
    {
        count = Math::Max(count, 0);
        if (count > 0)
        {
            if (MustReserve(count))
            {
                ReserveAndShift(-1, count - GetCount());
                Construct(_meta->Length, count);
            }
            else if (_meta->RefCount > 1)
            {
                Copy(count);
            }
            else if (count < _meta->Length)
            {
                Deconstruct(count, _meta->Length);
            }
            _meta->Length = count;
        }
        else
        {
            Empty();
        }
    }
    // Same as first SetLength(), but but also explicitly assigns initial value to
    // every new element.
    void SetLength(int count, const T &value)
    {
        count = Math::Max(count, 0);
        if (count > 0)
        {
            if (MustReserve(count))
            {
                ReserveAndShift(-1, count - GetCount());
                Construct(_meta->Length, count, &value);
            }
            else if (_meta->RefCount > 1)
            {
                Copy(count);
            }
            else if (count < _meta->Length)
            {
                Deconstruct(count, _meta->Length);
            }
            _meta->Length = count;
        }
        else
        {
            Empty();
        }
    }
    // Grows array to accommodate N elements; does nothing if array is already at least that large
    inline void GrowTo(int count)
    {
        if (count > GetCount())
        {
            SetLength(count);
        }
    }
    // Same as first GrowTo(), but but also explicitly assigns initial value to
    // every new element.
    inline void GrowTo(int count, const T &value)
    {
        if (count > GetCount())
        {
            SetLength(count, value);
        }
    }
    // Assign array by sharing data reference
    ObjectArray<T> &operator=(const ObjectArray<T>& arr)
    {
        if (_data != arr._data)
        {
            Free();
            if (arr._data && arr._meta->Length > 0)
            {
                _data = arr._data;
                if (_meta)
                {
                    _meta->RefCount++;
                }
            }
        }
        return *this;
    }
    inline const T &operator[](int index) const
    {
        assert (_meta && index >= 0 && index < _meta->Length);
        return _meta->Arr[index];
    }
    inline T &operator[](int index)
    {
        assert (_meta && index >= 0 && index < _meta->Length);
        BecomeUnique();
        return _meta->Arr[index];
    }

protected:
    // Construct number of elements between two array indexes [begin;end)
    void Construct(int begin, int end, const T *p_value = NULL)
    {
        const T *end_ptr = _meta->Arr + end;
        if (p_value)
        {
            const T &value = *p_value;
            for (T *elem_ptr = _meta->Arr + begin; elem_ptr != end_ptr; ++elem_ptr)
            {
                new (elem_ptr) T(value);
            }
        }
        else
        {
            for (T *elem_ptr = _meta->Arr + begin; elem_ptr != end_ptr; ++elem_ptr)
            {
                new (elem_ptr) T();
            }
        }
    }
    // Deconstruct number of elements between two array indexes (inclusive)
    void Deconstruct(int begin, int end)
    {
        const T *end_ptr = _meta->Arr + end;
        for (T *elem_ptr = _meta->Arr + begin; elem_ptr != end_ptr; ++elem_ptr)
        {
            elem_ptr->~T();
        }
    }
    // Creates new empty array with buffer enough to accommodate given length
    void CreateBuffer(int max_length)
    {
        _data = new char[sizeof(ArrayBase<T>) + max_length * sizeof(T)];
        _meta->RefCount = 1;
        _meta->Capacity = max_length;
        _meta->Length = 0;
        _meta->Arr = (T*)(_data + sizeof(ArrayBase<T>));
    }
    // Release array and copy data to the new buffer, optionally leaving
    // extra space before, after or in the middle of copied array
    void Copy(int new_capacity, int copy_from = 0, bool expand = true, int change_at = 0, int change_count = 0)
    {
        char *new_data = new char[sizeof(ArrayBase<T>) + new_capacity * sizeof(T)];
        // remember, that _meta->Arr may point to any address in buffer
        T *arr_head  = (T*)(new_data + sizeof(ArrayBase<T>));
        T *new_arr;
        int copy_length = _meta->Length - copy_from;
        if (change_count == 0)
        {
            copy_length = Math::Min(copy_length, new_capacity);
            Memory::UninitializedCopy<T>(arr_head, _meta->Arr + copy_from, copy_length);
            new_arr = arr_head;
        }
        else if (expand)
        {
            new_arr = Memory::ObjectCopyAndInsertSpace<T>(arr_head, _meta->Arr + copy_from, copy_length,
                change_at, change_count);
        }
        else
        {
            new_arr = Memory::ObjectCopyPartial<T>(arr_head, _meta->Arr + copy_from, copy_length,
                change_at, change_count);
            copy_length -= change_count;
        }

        Free();
        _data = new_data;
        _meta->RefCount = 1;
        _meta->Capacity = new_capacity;
        _meta->Length = copy_length;
        _meta->Arr = new_arr; 
    }
    // Ensure this array is a compact independent copy, with ref counter = 1
    inline void BecomeUnique()
    {
        if (_meta->RefCount > 1)
        {
            Copy(_meta->Length);
        }
    }
    // TODO: more balanced appending/prepending strategy
    void ReserveAndShift(int reserve_at, int need_space)
    {
        if (_meta)
        {
            reserve_at = reserve_at < 0 ? _meta->Length : Math::Min(reserve_at, _meta->Length);
            const int total_length = _meta->Length + need_space;
            if (_meta->Capacity < total_length)
            {
                // grow by 100% or at least to total_size, or at least by 4 elements
                int grown_length = Math::Max(_meta->Capacity << 1, _meta->Capacity + 4);
                Copy(Math::Max(total_length, grown_length), 0, true, reserve_at, need_space);
            }
            else if (_meta->RefCount > 1)
            {
                Copy(total_length, 0, true, reserve_at, need_space);
            }
            else
            {
                // make sure we make use of all of our space
                _meta->Arr = Memory::ExpandObjectsInBuffer<T>((T*)(_data + sizeof(ArrayBase<T>)), _meta->Capacity, _meta->Arr, _meta->Length,
                    reserve_at, need_space);
            }
        }
        else
        {
            CreateBuffer(need_space);
        }
    }

    union
    {
        char         *_data;
        ArrayBase<T> *_meta;
    };

    inline T    *BufferBegin() const { return (T*)(_data + sizeof(ArrayBase<T>)); }
    inline T    *BufferEnd()   const { return (T*)(_data + sizeof(ArrayBase<T>)) + _meta->Capacity; }
    inline T    *StringBegin() const { return _meta->Arr; }
    inline T    *StringEnd()   const { return _meta->Arr + _meta->Length; }
    inline int  SpaceLeft()    const { return StringBegin() - BufferBegin(); }
    inline int  SpaceRight()   const { return BufferEnd() - StringEnd(); }
    inline bool MustReserve(int total_space)         const { return !_meta || _meta->RefCount > 1 ||
        total_space > _meta->Length && SpaceRight() < total_space - _meta->Length; }
    inline bool MustReserveMoreRight(int need_space) const { return !_meta || _meta->RefCount > 1 || SpaceRight() < need_space; }
    inline bool MustReserveMoreLeft(int need_space)  const { return !_meta || _meta->RefCount > 1 || SpaceLeft() < need_space; }
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__ARRAY_H
