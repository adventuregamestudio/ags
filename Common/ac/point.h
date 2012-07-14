#ifndef __AC_POINT_H
#define __AC_POINT_H

struct _Point {
    short x, y;
};

#define MAXPOINTS 30
struct PolyPoints {
    int x[MAXPOINTS];
    int y[MAXPOINTS];
    int numpoints;
    void add_point(int xxx,int yyy);
    PolyPoints() { numpoints = 0; }

    void ReadFromFile(FILE *fp);
};

#endif // __AC_POINT_H