
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
#include "core/types.h"
#include "debug/assert.h"
#include "util/math.h"

namespace AGS
{
namespace Common
{

class Stream;

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
        count = count > 0 ? count : 0;
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
        count = count > 0 ? count : 0;
        if ((!_meta && count > 0) || _meta->RefCount > 1 || _meta->Capacity != count)
        {
            Free();
            CreateBuffer(count);
        }
        Construct(0, count, value);
        _meta->Length = count;
    }
    // Grows or truncates array to match given N number of elements,
    // keeps up to N of previously existed elements;
    // SetLength() guarantees that the buffer will be of minimal possible size
    // to accommodate N elements.
    void SetLength(int count)
    {
        if (_meta)
        {
            count = count > 0 ? count : 0;
            if (_meta->RefCount > 1 || _meta->Capacity != count)
            {
                Copy(count);
            }
            _meta->Length = count;
        }
        else if (count > 0)
        {
            CreateBuffer(count);
            _meta->Length = count;
        }
    }
    // Same as first SetLength(), but but also explicitly assigns initial value to
    // every new element.
    void SetLength(int count, const T &value)
    {
        if (_meta)
        {
            count = count > 0 ? count : 0;
            if (_meta->RefCount > 1 || _meta->Capacity != count)
            {
                Copy(count);
            }

            if (_meta->Length < count)
            {
                Construct(_meta->Length, count, value);
            }
            _meta->Length = count;
        }
        else if (count > 0)
        {
            CreateBuffer(count);
            Construct(0, count, value);
            _meta->Length = count;
        }
    }
    // Create new array and copy N elements from C-array
    void CreateFromCArray(const T *arr, int count)
    {
        New(count);
        if (arr && count > 0)
        {
            memcpy(_meta->Arr, arr, sizeof(T) * count);
        }
    }
    // Destroy all existing elements in array
    void Empty()
    {
        if (_meta)
        {
            BecomeUnique();
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
            New(count);
            in->ReadArray(_meta->Arr, sizeof(T), count);
        }
    }
    // Reads up to N elements from stream, copying the over existing elements.
    // It is generally unsafe to use this function for arrays of non-POD types.
    void ReadRawOver(Stream *in, int from, int count = -1)
    {
        if (_meta && _meta->Length > 0 && count != 0 && in)
        {
            BecomeUnique();
            count = count >= 0 ? count : _meta->Length;
            Math::ClampLength(0, _meta->Length, from, count);
            if (count > 0)
            {
                in->ReadArray(_meta->Arr + from, sizeof(T), count);
            }
        }
    }
    // Write array elements to stream, putting raw bytes from array buffer.
    // It is not recommended to use this function for non-POD types.
    void WriteRaw(Stream *out)
    {
        if (_meta && _meta->Length > 0 && out)
        {
            out->WriteArray(_meta->Arr, sizeof(T), _meta->Length);
        }
    }

    // Ensure string has at least space to store N chars;
    // this does not change string contents, nor length
    void Reserve(int max_length)
    {
        if (max_length > 0 && _meta && max_length > _meta->Capacity)
        {
            ReserveAndShift(false, max_length - _meta->Length);
        }
    }
    // Ensure string has at least space to store N additional chars
    void ReserveMore(int more_length)
    {
        ReserveAndShift(false, more_length);
    }
    // Make string's buffer as small as possible to hold current data
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
        ReserveAndShift(false, 1);
        _meta->Arr[_meta->Length++] = value;
    }
    // Add number of unitialized elements at array's end
    void AppendCount(int count)
    {
        if (count > 0)
        {
            ReserveAndShift(false, count);
            _meta->Length += count;
        }
    }
    // Add number of elements at array's end and initialize them
    // with given value
    void AppendCount(int count, const T &value)
    {
        if (count > 0)
        {
            ReserveAndShift(false, count);
            Construct(_meta->Length, _meta->Length + count, value);
            _meta->Length += count;
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
    // Copy data to the new buffer at given offset
    void Copy(int max_length, int offset = 0)
    {
        if (!_meta)
        {
            return;
        }

        char *new_data = new char[sizeof(ArrayBase<T>) + max_length * sizeof(T)];
        // remember, that _meta->Arr may point to any address in buffer
        T *arr_head = (T*)(new_data + sizeof(ArrayBase<T>)) + offset;
        int copy_length = Math::Min(_meta->Length, max_length);
        memcpy(arr_head, _meta->Arr, copy_length * sizeof(T));
        Free();
        _data = new_data;
        _meta->RefCount = 1;
        _meta->Capacity = max_length;
        _meta->Length = copy_length;
        _meta->Arr = arr_head;
    }
    // Aligns data at given offset in buffer
    void Align(int offset)
    {
        T *arr_head = (T*)(_data + sizeof(ArrayBase<T>)) + offset;
        memmove(arr_head, _meta->Arr, _meta->Length);
        _meta->Arr = arr_head;
    }
    // Ensure this array is a compact independent copy, with ref counter = 1
    inline void BecomeUnique()
    {
        if (_meta->RefCount != 1)
        {
            Copy(_meta->Length);
        }
    }
    // TODO: more balanced appending/prepending strategy
    void ReserveAndShift(bool reserve_left, int need_space)
    {
        if (_meta)
        {
            int total_length = _meta->Length + need_space;
            if (_meta->Capacity < total_length)
            {
                // grow by 100% or at least to total_size, or at least by 4 elements
                int grow_length = Math::Max(_meta->Capacity << 1, 4);
                Copy(Math::Max(total_length, grow_length), reserve_left ? need_space : 0);
            }
            else if (_meta->RefCount > 1)
            {
                Copy(_meta->Capacity, reserve_left ? need_space : 0);
            }
            else
            {
                // make sure we make use of all of our space
                const T *arrbuf_head = (const T*)(_data + sizeof(ArrayBase<T>));
                int free_space = reserve_left ?
                    _meta->Arr - arrbuf_head :
                    (arrbuf_head + _meta->Capacity) - (_meta->Arr + _meta->Length);
                if (free_space < need_space)
                {
                    Align(reserve_left ?
                        (_meta->Arr + (need_space - free_space)) - arrbuf_head :
                    0);
                }
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
        count = count > 0 ? count : 0;
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
        count = count > 0 ? count : 0;
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
    // Grows or truncates array to match given N number of elements,
    // keeps up to N of previously existed elements;
    // SetLength() guarantees that the buffer will be of minimal possible size
    // to accommodate N elements.
    void SetLength(int count)
    {
        if (_meta)
        {
            count = count > 0 ? count : 0;
            if (_meta->RefCount > 1 || _meta->Capacity != count)
            {
                Copy(count);
            }

            if (_meta->Length < count)
            {
                Construct(_meta->Length, count);
            }
            _meta->Length = count;
        }
        else if (count > 0)
        {
            CreateBuffer(count);
            Construct(0, count);
            _meta->Length = count;
        }
    }
    // Same as first SetLength(), but but also explicitly assigns initial value to
    // every new element.
    void SetLength(int count, const T &value)
    {
        if (_meta)
        {
            count = count > 0 ? count : 0;
            if (_meta->RefCount > 1 || _meta->Capacity != count)
            {
                Copy(count);
            }

            if (_meta->Length < count)
            {
                Construct(_meta->Length, count, value);
            }
            _meta->Length = count;
        }
        else if (count > 0)
        {
            CreateBuffer(count);
            Construct(0, count, value);
            _meta->Length = count;
        }
    }
    // Create new array and copy N elements from C-array
    void CreateFromCArray(const T *arr, int count)
    {
        New(count);
        if (arr && count > 0)
        {
            const T *end_ptr = _meta->Arr + count;
            for (T *dest_ptr = _meta->Arr, const T *src_ptr = arr; dest_ptr != end_ptr; ++dest_ptr, ++src_ptr)
            {
                *dest_ptr = *src_ptr;
            }
        }
    }
    // Destroy all existing elements in array
    void Empty()
    {
        if (_meta)
        {
            BecomeUnique();
            Deconstruct(0, _meta->Length);
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

    // Ensure string has at least space to store N chars;
    // this does not change string contents, nor length
    void Reserve(int max_length)
    {
        if (max_length > 0 && _meta && max_length > _meta->Capacity)
        {
            ReserveAndShift(false, max_length - _meta->Length);
        }
    }
    // Ensure string has at least space to store N additional chars
    void ReserveMore(int more_length)
    {
        ReserveAndShift(false, more_length);
    }
    // Make string's buffer as small as possible to hold current data
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
        ReserveAndShift(false, 1);
        new (_meta->Arr + _meta->Length++) T(value);
    }
    // Add number of unitialized elements at array's end
    void AppendCount(int count)
    {
        if (count > 0)
        {
            ReserveAndShift(false, count);
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
            ReserveAndShift(false, count);
            Construct(_meta->Length, _meta->Length + count, &value);
            _meta->Length += count;
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
    // Copy data to the new buffer at given offset
    void Copy(int max_length, int offset = 0)
    {
        char *new_data = new char[sizeof(ArrayBase<T>) + max_length * sizeof(T)];
        // remember, that _meta->Arr may point to any address in buffer
        T *arr_head = (T*)(new_data + sizeof(ArrayBase<T>)) + offset;
        int copy_length = Math::Min(_meta->Length, max_length);
        const T *end_ptr = arr_head + copy_length;
        for (T *dest_ptr = arr_head, *src_ptr = _meta->Arr; dest_ptr != end_ptr; ++dest_ptr, ++src_ptr)
        {
            new (dest_ptr) T(*src_ptr);
        }
        Free();
        _data = new_data;
        _meta->RefCount = 1;
        _meta->Capacity = max_length;
        _meta->Length = copy_length;
        _meta->Arr = arr_head;
    }
    // Aligns data at given offset in buffer
    void Align(int offset)
    {
        T *arr_head = (T*)(_data + sizeof(ArrayBase<T>)) + offset;
        T *dst_end_ptr = arr_head + _meta->Length;
        T *src_end_ptr = _meta->Arr + _meta->Length;
        if (arr_head < _meta->Arr)
        {
            T *dest_ptr = arr_head;
            T *src_ptr = _meta->Arr;
            for (; dest_ptr < _meta->Arr; ++dest_ptr, ++src_ptr)
            {
                new (dest_ptr) T(*src_ptr);
            }
            for (; dest_ptr < dst_end_ptr; ++dest_ptr, ++src_ptr)
            {
               *dest_ptr = *src_ptr;
            }
            for (; src_ptr < src_end_ptr; ++src_ptr)
            {
                src_ptr->~T();
            }
        }
        // This function assumes that the new offset is different
        else // arr_head > _meta->Arr
        {
            T *dest_ptr = dst_end_ptr;
            T *src_ptr = src_end_ptr;
            for (; dest_ptr >= src_end_ptr; --dest_ptr, --src_ptr)
            {
                new (dest_ptr) T(*src_ptr);
            }
            for (; dest_ptr >= arr_head; --dest_ptr, --src_ptr)
            {
                *dest_ptr = *src_ptr;
            }
            for (; src_ptr >= _meta->Arr; --src_ptr)
            {
                src_ptr->~T();
            }
        }
        _meta->Arr = arr_head;
    }
    // Ensure this array is a compact independent copy, with ref counter = 1
    inline void BecomeUnique()
    {
        if (_meta->RefCount != 1)
        {
            Copy(_meta->Length);
        }
    }
    // TODO: more balanced appending/prepending strategy
    void ReserveAndShift(bool reserve_left, int need_space)
    {
        if (_meta)
        {
            int total_length = _meta->Length + need_space;
            if (_meta->Capacity < total_length)
            {
                // grow by 100% or at least to total_size, or at least by 4 elements
                int grow_length = Math::Max(_meta->Capacity << 1, 4);
                Copy(Math::Max(total_length, grow_length), reserve_left ? need_space : 0);
            }
            else if (_meta->RefCount > 1)
            {
                Copy(_meta->Capacity, reserve_left ? need_space : 0);
            }
            else
            {
                // make sure we make use of all of our space
                const T *arrbuf_head = (const T*)(_data + sizeof(ArrayBase<T>));
                int free_space = reserve_left ?
                    _meta->Arr - arrbuf_head :
                    (arrbuf_head + _meta->Capacity) - (_meta->Arr + _meta->Length);
                if (free_space < need_space)
                {
                    Align(reserve_left ?
                        (_meta->Arr + (need_space - free_space)) - arrbuf_head :
                    0);
                }
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
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__ARRAY_H
