#ifndef __AC_GFXFILTERHELPERS_H
#define __AC_GFXFILTERHELPERS_H

struct MouseGetPosCallbackImpl : IMouseGetPosCallback {
protected:
    ScalingGFXFilter *_callbackFilter;

public:
    MouseGetPosCallbackImpl(ScalingGFXFilter *filter)
    {
        _callbackFilter = filter;
    }

    virtual void AdjustPosition(int *x, int *y)
    {
        _callbackFilter->AdjustPosition(x, y);
    }
};

#endif // __AC_GFXFILTERHELPERS_H