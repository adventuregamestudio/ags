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

#ifndef __AC_DYNAMICARRAY_H
#define __AC_DYNAMICARRAY_H

#include <cstdlib> //calloc()
#include <cstring> //memcpy()

template <typename T> struct DynamicArray {
private:
    T defaultConstructed;
    T *data;
    int datasize;

public:

    DynamicArray() {
        data = NULL;
        datasize = 0;
    }

    ~DynamicArray() {
        if (data)
            free(data);
    }

    void GrowTo(int newsize);
    void SetSizeTo(int newsize);
    T& operator[] (int index);
};

template <typename T>
void DynamicArray<T>::GrowTo(int newsize) {
    if (datasize < newsize) {
        SetSizeTo(newsize);
    }
}

template <typename T>
void DynamicArray<T>::SetSizeTo(int newsize) {
    int dsWas = datasize;
    datasize = newsize;
    if (data == NULL)
        data = (T*)calloc(sizeof(T), datasize);
    else {
        T *newdata = (T*)calloc(sizeof(T), datasize);
        if (dsWas > datasize)
            dsWas = datasize;
        memcpy(newdata, data, sizeof(T) * dsWas);
        free(data);
        data = newdata;
    }
    // "construct" the new objects by copying the default-constructed
    // object into them
    // this is necessary so that the vtables are set up correctly
    for (int qq = dsWas; qq < datasize; qq++) {
        memcpy(&data[qq], &defaultConstructed, sizeof(T));
    }
}

template <typename T>
T& DynamicArray<T>::operator[] (int index) {
    if (index < 0)
        index = 0;
    if (index >= datasize) {
        // grow it 5 bigger, so we don't have to keep reallocating
        GrowTo(index + 5);
    }
    return data[index];
}

#endif // __AC_DYNAMICARRAY_H
