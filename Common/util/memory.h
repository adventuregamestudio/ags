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
// Memory utils and algorithms
//
//=============================================================================
#ifndef __AGS_CN_UTIL__MEMORY_H
#define __AGS_CN_UTIL__MEMORY_H

namespace AGS
{
namespace Common
{

namespace Memory
{
    // Construct number of elements between two pointers [begin;end)
    template <class T>
    void Construct(T *begin_ptr, T *end_ptr, const T *p_value = NULL)
    {
        if (p_value)
        {
            const T &value = *p_value;
            while (begin_ptr < end_ptr)
            {
                new (begin_ptr++) T(value);
            }
        }
        else
        {
            while (begin_ptr < end_ptr)
            {
                new (begin_ptr++) T();
            }
        }
    }
    // Deconstruct number of elements between two pointers [begin;end)
    template <class T>
    void Deconstruct(T *begin_ptr, T *end_ptr)
    {
        while (begin_ptr < end_ptr)
        {
            (begin_ptr++)->~T();
        }
    }

    template <class T>
    inline void Copy(T *dst_ptr, const T *src_ptr, size_t elem_count)
    {
        memcpy(dst_ptr, src_ptr, elem_count * sizeof(T));
    }

    template <class T>
    inline void Move(T *dst_ptr, const T *src_ptr, size_t elem_count)
    {
        memmove(dst_ptr, src_ptr, elem_count * sizeof(T));
    }

    template <class T>
    void UninitializedCopy(T *dst_ptr, const T *src_ptr, size_t elem_count)
    {
        const T *src_end_ptr = src_ptr + elem_count;
        while (src_ptr < src_end_ptr)
        {
            new (dst_ptr++) T(*src_ptr++);
        }
    }

    template <class T>
    void UninitializedMove(T *dst_begin, const T *src_begin, size_t elem_count)
    {
        T *dst_end               = dst_begin + elem_count;
        const T *const src_end   = src_begin + elem_count;

        if (dst_end <= src_begin || dst_begin >= src_end)
        {
            const T *src_ptr = src_begin;
            while (src_ptr < src_end)
            {
                new (dst_begin++) T(*src_ptr);
                (src_ptr++)->~T();
            }
        }
        else if (dst_begin < src_begin) // && dst_end > src_begin
        {
            const T *src_ptr = src_begin;
            while (dst_begin < src_begin)
            {
                new (dst_begin++) T(*src_ptr++);
            }
            while (src_ptr < dst_end)
            {
                *dst_begin++ = *src_ptr++;
            }
            while (src_ptr < src_end)
            {
                *dst_begin++ = *src_ptr;
                (src_ptr++)->~T();
            }
        }
        else // dst_begin >= src_ptr && dst_begin < src_end
        {
            const T *src_ptr = src_end;
            while (dst_end > src_end)
            {
                new (--dst_end) T(*--src_ptr);
            }
            while (src_ptr > dst_begin)
            {
                *--dst_end = *--src_ptr;
            }
            while (src_ptr > src_begin)
            {
                *--dst_end = *--src_ptr;
                src_ptr->~T();
            }
        }
    }

    template <class T>
    T *GenericMoveAndInsertSpace(T *dst_ptr, const T *src_ptr, size_t elem_count, size_t insert_at, size_t insert_count,
        void (*pfn_memmove)(T*, const T*, size_t))
    {
        if (insert_at > 0 && insert_at < elem_count && insert_count > 0)
        {
            pfn_memmove(dst_ptr, src_ptr, insert_at);
            pfn_memmove(dst_ptr + insert_at + insert_count, src_ptr + insert_at, (elem_count - insert_at));
        }
        else
        {
            pfn_memmove(insert_at == 0 ? dst_ptr + insert_count : dst_ptr, src_ptr, elem_count);
        }
        return dst_ptr;
    }

    template <class T>
    inline T *CopyAndInsertSpace(T *dst_ptr, const T *src_ptr, size_t elem_count, size_t insert_at = 0, size_t insert_count = 0)
    {
        return GenericMoveAndInsertSpace<T>(dst_ptr, src_ptr, elem_count, insert_at, insert_count, Memory::Copy<T>);
    }

    template <class T>
    inline T *MoveAndInsertSpace(T *dst_ptr, const T *src_ptr, size_t elem_count, size_t insert_at = 0, size_t insert_count = 0)
    {
        return GenericMoveAndInsertSpace<T>(dst_ptr, src_ptr, elem_count, insert_at, insert_count, Memory::Move<T>);
    }

    template <class T>
    inline T *ObjectCopyAndInsertSpace(T *dst_ptr, const T *src_ptr, size_t elem_count, size_t insert_at = 0, size_t insert_count = 0)
    {
        return GenericMoveAndInsertSpace<T>(dst_ptr, src_ptr, elem_count, insert_at, insert_count, UninitializedCopy<T>);
    }

    template <class T>
    inline T *ObjectMoveAndInsertSpace(T *dst_ptr, const T *src_ptr, size_t elem_count, size_t insert_at = 0, size_t insert_count = 0)
    {
        return GenericMoveAndInsertSpace<T>(dst_ptr, src_ptr, elem_count, insert_at, insert_count, UninitializedMove<T>);
    }

    template <class T>
    T *GenericMovePartial(T *dst_ptr, const T *src_ptr, size_t elem_count, size_t remove_at, size_t remove_count,
        void (*pfn_memmove)(T*, const T*, size_t))
    {
        if (remove_at > 0 && remove_at < elem_count && remove_count > 0)
        {
            pfn_memmove(dst_ptr, src_ptr, remove_at);
            pfn_memmove(dst_ptr + remove_at, src_ptr + remove_at + remove_count, (elem_count - remove_at - remove_count));
        }
        else
        {
            if (remove_at == 0)
            {
                src_ptr += remove_count;
                elem_count -= remove_count;
            }
            pfn_memmove(dst_ptr, src_ptr, elem_count);
        }
        return dst_ptr;
    }

    template <class T>
    inline T *CopyPartial(T *dst_ptr, const T *src_ptr, size_t elem_count, size_t insert_at = 0, size_t insert_count = 0)
    {
        return GenericMovePartial<T>(dst_ptr, src_ptr, elem_count, insert_at, insert_count, Memory::Copy<T>);
    }

    template <class T>
    inline T *MovePartial(T *dst_ptr, const T *src_ptr, size_t elem_count, size_t insert_at = 0, size_t insert_count = 0)
    {
        return GenericMovePartial<T>(dst_ptr, src_ptr, elem_count, insert_at, insert_count, Memory::Move<T>);
    }

    template <class T>
    inline T *ObjectCopyPartial(T *dst_ptr, const T *src_ptr, size_t elem_count, size_t insert_at = 0, size_t insert_count = 0)
    {
        return GenericMovePartial<T>(dst_ptr, src_ptr, elem_count, insert_at, insert_count, UninitializedCopy<T>);
    }

    template <class T>
    inline T *ObjectMovePartial(T *dst_ptr, const T *src_ptr, size_t elem_count, size_t insert_at = 0, size_t insert_count = 0)
    {
        return GenericMovePartial<T>(dst_ptr, src_ptr, elem_count, insert_at, insert_count, UninitializedMove<T>);
    }

    template <class T>
    T *GenericExpandDataInBuffer(T *buffer, size_t buffer_size, T *data, size_t elem_count, size_t insert_at, size_t insert_count,
        void (*pfn_memmove)(T*, const T*, size_t))
    {
        const size_t head_space = data - buffer;
        const size_t tail_space = (buffer + buffer_size) - (data + elem_count);
        T *          data_new_ptr = data;

        // insertion at the head
        if (insert_at == 0)
        {
            if (head_space < insert_count)
            {
                data_new_ptr = buffer;
                pfn_memmove(buffer + insert_count, data, elem_count);
            }
            else
            {
                data_new_ptr -= insert_count;
            }
        }
        // insertion at the tail
        else if (insert_at >= elem_count)
        {
            if (tail_space < insert_count)
            {
                data_new_ptr = buffer;
                pfn_memmove(buffer, data, elem_count);
            }
        }
        else // insertion in the middle
        {
            const bool can_move_piece_of_data_left  = head_space >= insert_count;
            const bool can_move_piece_of_data_right = tail_space >= insert_count;
            const bool should_try_move_left = insert_at <= (elem_count >> 1);

            if (can_move_piece_of_data_left &&
                (should_try_move_left || !can_move_piece_of_data_right))
            {
                data_new_ptr = data - insert_count;
                pfn_memmove(data_new_ptr, data, insert_at);
            }
            else if (can_move_piece_of_data_right &&
                (!should_try_move_left || !can_move_piece_of_data_left))
            {
                data_new_ptr = data + insert_at + insert_count;
                pfn_memmove(data_new_ptr, data + insert_at, insert_count);
            }
            else
            {
                data_new_ptr = buffer;
                pfn_memmove(buffer, data, insert_at);
                pfn_memmove(buffer + insert_at + insert_count, data + insert_at, (elem_count - insert_at));
            }
        }
        return data_new_ptr;
    }

    template <class T>
    inline T *ExpandDataInBuffer(T *buffer, size_t buffer_size, T *data, size_t elem_count, size_t insert_at, size_t insert_count)
    {
        return GenericExpandDataInBuffer<T>(buffer, buffer_size, data, elem_count, insert_at, insert_count, Memory::Move<T>);
    }

    template <class T>
    inline T *ExpandObjectsInBuffer(T *buffer, size_t buffer_size, T *data, size_t elem_count, size_t insert_at, size_t insert_count)
    {
        return GenericExpandDataInBuffer<T>(buffer, buffer_size, data, elem_count, insert_at, insert_count, Memory::UninitializedMove<T>);
    }

    template <class T>
    T *GenericExcludeDataInBuffer(T *data, size_t elem_count, size_t remove_at, size_t remove_count,
        void (*pfn_memmove)(T*, const T*, size_t), void (*pfn_deconstruct)(T*, T*) = NULL)
    {
        T *data_new_ptr = data;
        Math::ClampLength<size_t>(0, elem_count, remove_at, remove_count);
        // removal at the head
        if (remove_at == 0)
        {
            data_new_ptr = data + remove_count;
            if (pfn_deconstruct)
            {
                pfn_deconstruct(data, data_new_ptr);
            }
        }
        // removal at the tail
        else if (remove_at + remove_count >= elem_count)
        {
            if (pfn_deconstruct)
            {
                pfn_deconstruct(data + elem_count - remove_count, data + elem_count);
            }
        }
        else // removal in the middle
        {
            const bool should_move_left = remove_at < elem_count - (remove_at + remove_count);

            if (should_move_left)
            {
                data_new_ptr = data + remove_count;
                pfn_memmove(data_new_ptr, data, remove_at);
            }
            else
            {
                pfn_memmove(data + remove_at, data + remove_at + remove_count, elem_count - (remove_at + remove_count));
            }
        }
        return data_new_ptr;
    }

    template <class T>
    inline T *ExcludeDataInBuffer(T *data, size_t elem_count, size_t remove_at, size_t remove_count)
    {
        return GenericExcludeDataInBuffer<T>(data, elem_count, remove_at, remove_count,
            Memory::Move<T>, (void (*)(T*, T*))NULL);
    }

    template <class T>
    inline T *ExcludeObjectsInBuffer(T *data, size_t elem_count, size_t remove_at, size_t remove_count)
    {
        return GenericExcludeDataInBuffer<T>(data, elem_count, remove_at, remove_count,
            Memory::UninitializedMove<T>, Memory::Deconstruct<T>);
    }
}

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__MEMORY_H
