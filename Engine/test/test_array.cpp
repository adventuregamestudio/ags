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

#ifdef _DEBUG

#include "util/array.h"
#include "debug/assert.h"

using AGS::Common::Array;
using AGS::Common::ObjectArray;
namespace Memory = AGS::Common::Memory;

struct TestObjType
{
    static int ctor_call_count;
    static int copy_ctor_call_count;
    static int dtor_call_count;
    static int assign_call_count;

    TestObjType()
    {
        a = 1;
        b = 2;
        c = 3;
        ctor_call_count++;
    }
    TestObjType(const TestObjType &obj)
    {
        copy_ctor_call_count++;
        a = obj.a;
        b = obj.b;
        c = obj.c;
    }
    TestObjType(int _a, int _b, int _c)
    {
        a = _a;
        b = _b;
        c = _c;
    }
    ~TestObjType()
    {
        dtor_call_count++;
    }
    TestObjType &operator=(const TestObjType &obj)
    {
        assign_call_count++;
        a = obj.a;
        b = obj.b;
        c = obj.c;
        return *this;
    }

    bool operator==(const TestObjType &obj) const
    {
        return a == obj.a && b == obj.b && c == obj.c;
    }

    int a;
    int b;
    int c;
};

int TestObjType::ctor_call_count = 0;
int TestObjType::copy_ctor_call_count = 0;
int TestObjType::dtor_call_count = 0;
int TestObjType::assign_call_count = 0;


void Test_Array()
{
    // POD memory operations
    {
        int arr1[100];
        int arr2[100];
        arr1[0] = 0;
        arr1[1] = 1;
        arr1[2] = 2;
        arr1[3] = 3;
        Memory::CopyAndInsertSpace(arr2, arr1, 4);
        for (int i = 0; i < 4; ++i)
        {
            assert(arr1[i] == arr2[i]);
        }
        Memory::MoveAndInsertSpace(arr1 + 2, arr1, 4);
        for (int i = 0; i < 4; ++i)
        {
            assert(arr1[i + 2] == i);
        }
        Memory::MoveAndInsertSpace(arr1, arr1 + 2, 4);
        for (int i = 0; i < 4; ++i)
        {
            assert(arr1[i] == i);
        }
        Memory::MoveAndInsertSpace(arr2, arr1, 4);
        for (int i = 0; i < 4; ++i)
        {
            assert(arr1[i] == arr2[i]);
        }

        int arr3[100];
        Memory::CopyAndInsertSpace(arr3, arr1, 4, 0, 10);
        for (int i = 0; i < 4; ++i)
        {
            assert(arr3[i + 10] == arr1[i]);
        }
        Memory::MoveAndInsertSpace(arr3, arr3 + 10, 4, 4, 10);
        for (int i = 0; i < 4; ++i)
        {
            assert(arr3[i] == arr1[i]);
        }
        Memory::MoveAndInsertSpace(arr3, arr3, 4, 2, 2);
        assert(arr3[0] == arr1[0]);
        assert(arr3[1] == arr1[1]);
        assert(arr3[4] == arr1[2]);
        assert(arr3[5] == arr1[3]);
    }

    // Object memory operations
    {
        TestObjType objarr1[100];
        TestObjType objarr2[100];
        objarr1[0] = TestObjType(44,55,66);
        objarr1[1] = TestObjType(77,88,99);
        objarr1[2] = TestObjType(111,222,333);
        objarr1[3] = TestObjType(444,555,666);
        
        Memory::ObjectCopyAndInsertSpace(objarr2, objarr1, 4);
        for (int i = 0; i < 4; ++i)
        {
            assert(objarr1[i] == objarr2[i]);
        }
        Memory::ObjectMoveAndInsertSpace(objarr1 + 2, objarr1, 4);
        assert(objarr1[2].a == 44);
        assert(objarr1[3].a == 77);
        assert(objarr1[4].a == 111);
        assert(objarr1[5].a == 444);
        Memory::ObjectMoveAndInsertSpace(objarr1, objarr1 + 2, 4);
        assert(objarr1[0].a == 44);
        assert(objarr1[1].a == 77);
        assert(objarr1[2].a == 111);
        assert(objarr1[3].a == 444);

        TestObjType objarr3[100];
        Memory::ObjectCopyAndInsertSpace(objarr3, objarr1, 4, 0, 10);
        for (int i = 0; i < 4; ++i)
        {
            assert(objarr3[i + 10] == objarr1[i]);
        }
        Memory::ObjectMoveAndInsertSpace(objarr3, objarr3 + 10, 4, 4, 10);
        for (int i = 0; i < 4; ++i)
        {
            assert(objarr3[i] == objarr1[i]);
        }
        int prev_ctor_call_count = TestObjType::ctor_call_count;
        int prev_copy_ctor_call_count = TestObjType::copy_ctor_call_count;
        int prev_assign_call_count = TestObjType::assign_call_count;
        int prev_dtor_call_count = TestObjType::dtor_call_count;
        Memory::ObjectMoveAndInsertSpace(objarr3, objarr3, 4, 2, 2);
        assert(prev_ctor_call_count == TestObjType::ctor_call_count);
        assert(prev_copy_ctor_call_count == TestObjType::copy_ctor_call_count - 2);
        assert(prev_assign_call_count == TestObjType::assign_call_count - 2);
        assert(prev_dtor_call_count == TestObjType::dtor_call_count - 2);
        assert(objarr3[0] == objarr1[0]);
        assert(objarr3[1] == objarr1[1]);
        assert(objarr3[4] == objarr1[2]);
        assert(objarr3[5] == objarr1[3]);
    }

    // Test array's internal work
    {
        Array<int> arr1;
        Array<int> arr2;
        Array<int> arr3;
        arr1.New(0);
        assert(arr1.GetData() == NULL);
        arr1.New(10);
        arr1[5] = 123;
        arr2 = arr1;
        arr3 = arr2;
        assert(arr1.GetCapacity() == 10);
        assert(arr2.GetCapacity() == 10);
        assert(arr3.GetCapacity() == 10);
        assert(arr3.GetRefCount() == 3);
        arr1.SetLength(15);
        assert(arr1.GetCapacity() == 20);
        assert(arr1[5] == 123);
        assert(arr1.GetRefCount() == 1);
        assert(arr3.GetCapacity() == 10);
        assert(arr3.GetRefCount() == 2);
        assert(arr3[5] == 123);
        arr2[5] = 321;
        arr2.SetLength(6);
        assert(arr2.GetCapacity() == 10);
        assert(arr2[5] == 321);
        assert(arr2.GetRefCount() == 1);
        assert(arr3.GetCapacity() == 10);
        assert(arr3[5] == 123);
        assert(arr3.GetRefCount() == 1);

        Array<int> arr4;
        arr4.New(10);
        Array<int> arr5 = arr4;
        assert(arr5.GetRefCount() == 2);
        const Array<int> &arr4_ref = arr4;
        if (arr4_ref[0] == arr4_ref[1])
        {
            int el = arr4_ref[0];
        }
        else
        {
            int el = arr4_ref[1];
        }
        assert(arr5.GetRefCount() == 2);
        arr4[5] = 1;
        assert(arr5.GetRefCount() == 1);
    }

    // Object array internal work
    {
        TestObjType::ctor_call_count = 0;
        TestObjType::copy_ctor_call_count = 0;
        TestObjType::dtor_call_count = 0;
        TestObjType::assign_call_count = 0;

        Array<TestObjType> arr1;
        arr1.New(10);
        arr1.SetLength(20);
        arr1.Empty();
        assert(TestObjType::ctor_call_count == 0);
        assert(TestObjType::copy_ctor_call_count == 0);
        assert(TestObjType::dtor_call_count == 0);
        assert(TestObjType::assign_call_count == 0);

        ObjectArray<TestObjType> objarr1;
        objarr1.New(10);
        assert(TestObjType::ctor_call_count == 10);
        assert(TestObjType::copy_ctor_call_count == 0);
        assert(TestObjType::dtor_call_count == 0);
        assert(TestObjType::assign_call_count == 0);
        objarr1.SetLength(20);
        assert(TestObjType::ctor_call_count == 20);
        assert(TestObjType::copy_ctor_call_count == 10);
        assert(TestObjType::assign_call_count == 0);
        assert(TestObjType::dtor_call_count == 10);
        objarr1.Empty();
        assert(TestObjType::ctor_call_count == 20);
        assert(TestObjType::copy_ctor_call_count == 10);
        assert(TestObjType::assign_call_count == 0);
        assert(TestObjType::dtor_call_count == 30);
    }

    // Dynamic buffer operations
    {
    }
}

#endif // _DEBUG
