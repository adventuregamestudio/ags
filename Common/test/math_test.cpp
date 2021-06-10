#include "gtest/gtest.h"
#include "util/scaling.h"
#include "util/math.h"

using namespace AGS::Common;

static void AxScale(int src, int dst)
{
    int x;
    AxisScaling sc;
    sc.Init(src, dst);
    x = sc.ScalePt(0);
    ASSERT_TRUE(x == 0);
    x = sc.ScalePt(src);
    ASSERT_TRUE(x == dst);
    x = sc.UnScalePt(dst);
    ASSERT_TRUE(x == src);
}

TEST(Math, AxisScaling) {
    AxScale(100, 100);

    AxScale(100, 1000);
    AxScale(320, 1280);
    AxScale(200, 400);

    AxScale(1000, 100);
    AxScale(1280, 320);
    AxScale(400, 200);

    AxScale(300, 900);
    AxScale(100, 700);
    AxScale(200, 2200);

    AxScale(900, 300);
    AxScale(700, 100);
    AxScale(2200, 200);

    for (int i = 250; i < 2000; i += 25)
    {
        AxScale(200, i);
        AxScale(i, 200);
    }
}
