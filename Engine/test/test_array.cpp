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

struct TestObjType
{
    static int ctor_call_count;
    static int copy_ctor_call_count;
    static int dtor_call_count;
    static int assign_call_count;

    TestObjType()
    {
        ctor_call_count++;
    }
    TestObjType(const TestObjType &)
    {
        copy_ctor_call_count++;
    }
    ~TestObjType()
    {
        dtor_call_count++;
    }
    TestObjType &operator=(const TestObjType &)
    {
        assign_call_count++;
        return *this;
    }
};

int TestObjType::ctor_call_count = 0;
int TestObjType::copy_ctor_call_count = 0;
int TestObjType::dtor_call_count = 0;
int TestObjType::assign_call_count = 0;


void Test_Array()
{
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
        assert(arr1.GetCapacity() == 15);
        assert(arr1[5] == 123);
        assert(arr1.GetRefCount() == 1);
        assert(arr3.GetCapacity() == 10);
        assert(arr3.GetRefCount() == 2);
        assert(arr3[5] == 123);
        arr2[5] = 321;
        arr2.SetLength(6);
        assert(arr2.GetCapacity() == 6);
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
