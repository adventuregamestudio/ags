//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// Geometry data structures and helper functions
//
//=============================================================================
#ifndef __AGS_CN_UTIL__GEOMETRY_H
#define __AGS_CN_UTIL__GEOMETRY_H

#include <cmath>
#include "util/math.h"

namespace AGSMath = AGS::Common::Math;
//namespace AGS
//{
//namespace Common
//{

// Type of alignment of a geometric item of rectangular boundaries.
enum FrameAlignment
{
    kAlignNone = 0,

    // Alignment options are representing 8 sides of a frame (rectangle);
    // they are implemented as flags that may be combined together if it
    // is wanted to define alignment to multiple sides at once.
    kAlignTopLeft       = 0x0001,
    kAlignTopCenter     = 0x0002,
    kAlignTopRight      = 0x0004,
    kAlignMiddleLeft    = 0x0008,
    kAlignMiddleCenter  = 0x0010,
    kAlignMiddleRight   = 0x0020,
    kAlignBottomLeft    = 0x0040,
    kAlignBottomCenter  = 0x0080,
    kAlignBottomRight   = 0x0100,

    // Masks are helping to determine whether alignment parameter contains
    // particular horizontal or vertical component (for example: left side
    // or bottom side)
    kMAlignLeft         = kAlignTopLeft | kAlignMiddleLeft | kAlignBottomLeft,
    kMAlignRight        = kAlignTopRight | kAlignMiddleRight | kAlignBottomRight,
    kMAlignTop          = kAlignTopLeft | kAlignTopCenter | kAlignTopRight,
    kMAlignBottom       = kAlignBottomLeft | kAlignBottomCenter | kAlignBottomRight,
    kMAlignHCenter      = kAlignTopCenter | kAlignMiddleCenter | kAlignBottomCenter,
    kMAlignVCenter      = kAlignMiddleLeft | kAlignMiddleCenter | kAlignMiddleRight
};

// Horizontal alignment; based on FrameAlignment, used to restrict alignment
// setting to left/right/center option, while keeping compatibility with any
// alignment in case it will be supported in the future.
enum HorAlignment
{
    kHAlignNone     = kAlignNone,
    kHAlignLeft     = kAlignTopLeft,
    kHAlignRight    = kAlignTopRight,
    kHAlignCenter   = kAlignTopCenter
};

enum RectPlacement
{
    kPlaceOffset,
    kPlaceCenter,
    kPlaceStretch,
    kPlaceStretchProportional,
    kNumRectPlacement
};


template <typename T>
struct PointT
{
    typedef PointT<T> Pt;

    T X{};
    T Y{};

    PointT() = default;
    PointT(T x, T y)
    {
        X = x;
        Y = y;
    }

    inline static Pt Clamp(const Pt &p, const Pt &floor, const Pt &ceil)
    {
        return Pt(AGSMath::Clamp(p.X, floor.X, ceil.X),
                    AGSMath::Clamp(p.Y, floor.Y, ceil.Y));
    }

    inline bool operator ==(const Pt &p) const
    {
        return X == p.X && Y == p.Y;
    }

    inline bool operator !=(const Pt &p) const
    {
        return X != p.X || Y != p.Y;
    }

    inline Pt operator +(const Pt &p) const
    {
        return Pt(X + p.X, Y + p.Y);
    }

    inline Pt operator -(const Pt &p) const
    {
        return Pt(X - p.X, Y - p.Y);
    }

    inline Pt operator *(T mul) const
    {
        return Pt(X * mul, Y * mul);
    }

    inline Pt operator /(T div) const
    {
        return Pt(X / div, Y / div);
    }

    inline Pt &operator *=(T mul)
    {
        X *= mul;
        Y *= mul;
        return *this;
    }

    inline Pt &operator /=(T div)
    {
        X /= div;
        Y /= div;
        return *this;
    }

    inline bool Equals(const T x, const T y) const
    {
        return X == x && Y == y;
    }
};

typedef PointT<int> Point;
typedef PointT<float> Pointf;



struct Line
{
	int X1;
	int Y1;
	int X2;
	int Y2;

	Line()
	{
		X1 = 0;
		Y1 = 0;
		X2 = 0;
		Y2 = 0;
	}

	Line(int x1, int y1, int x2, int y2)
	{
		X1 = x1;
		Y1 = y1;
		X2 = x2;
		Y2 = y2;
	}
};

// Helper factory functions
inline Line HLine(int x1, int x2, int y)
{
	return Line(x1, y, x2, y);
}

inline Line VLine(int x, int y1, int y2)
{
	return Line(x, y1, x, y2);
}

struct Size
{
    int Width;
    int Height;

    Size()
    {
        Width = 0;
        Height = 0;
    }

    Size(int width, int height)
    {
        Width = width;
        Height = height;
    }

    inline bool IsNull() const
    {
        return Width <= 0 || Height <= 0;
    }

    inline static Size Clamp(const Size &sz, const Size &floor, const Size &ceil)
    {
        return Size(AGSMath::Clamp(sz.Width, floor.Width, ceil.Width),
                    AGSMath::Clamp(sz.Height, floor.Height, ceil.Height));
    }

    // Indicates if current size exceeds other size by any metric
    inline bool ExceedsByAny(const Size &size) const
    {
        return Width > size.Width || Height > size.Height;
    }

    inline bool operator==(const Size &size) const
    {
        return Width == size.Width && Height == size.Height;
    }

    inline bool operator!=(const Size &size) const
    {
        return Width != size.Width || Height != size.Height;
    }

    inline bool operator<(const Size &other) const
    { // TODO: this implementation is silly and not universally useful; make a realistic one and replace with another function where necessary
        return Width < other.Width || (Width == other.Width && Height < other.Height);
    }

    inline Size operator +(const Size &size) const
    {
        return Size(Width + size.Width, Height + size.Height);
    }

    inline Size operator -(const Size &size) const
    {
        return Size(Width - size.Width, Height - size.Height);
    }

    inline Size operator *(int x) const
    {
        return Size(Width * x, Height * x);
    }

    inline Size operator /(int x) const
    {
        return Size(Width / x, Height / x);
    }

    inline Size &operator *=(int x)
    {
        Width *= x;
        Height *= x;
        return *this;
    }

    inline Size &operator /=(int x)
    {
        Width /= x;
        Height /= x;
        return *this;
    }
};

// TODO: consider making Rect have right-bottom coordinate with +1 offset
// to comply with many other libraries (i.e. Right - Left == Width)
struct Rect
{
	int Left;
	int Top;
	int Right;
	int Bottom;

	Rect()
	{
		Left	= 0;
		Top		= 0;
		Right	= -1;
		Bottom	= -1;
	}

	Rect(int l, int t, int r, int b)
	{
		Left	= l;
		Top		= t;
		Right	= r;
		Bottom	= b;
	}

    inline Point GetLT() const
    {
        return Point(Left, Top);
    }

    inline Point GetCenter() const
    {
        return Point(Left + GetWidth() / 2, Top + GetHeight() / 2);
    }

	inline int GetWidth() const
	{
		return Right - Left + 1;
	}

	inline int GetHeight() const
	{
		return Bottom - Top + 1;
	}

    inline Size GetSize() const
    {
        return Size(GetWidth(), GetHeight());
    }
    
    inline bool IsEmpty() const
    {
        return Right < Left || Bottom < Top;
    }

    inline bool IsInside(int x, int y) const
    {
        return x >= Left && y >= Top && (x <= Right) && (y <= Bottom);
    }

    inline bool IsInside(const Point &pt) const
    {
        return IsInside(pt.X, pt.Y);
    }

    inline void MoveToX(int x)
    {
        Right += x - Left;
        Left = x;
    }

    inline void MoveToY(int y)
    {
        Bottom += y - Top;
        Top = y;
    }

    inline void MoveTo(const Point &pt)
    {
        MoveToX(pt.X);
        MoveToY(pt.Y);
    }

    inline void SetWidth(int width)
    {
        Right = Left + width - 1;
    }

    inline void SetHeight(int height)
    {
        Bottom = Top + height - 1;
    }

    inline static Rect MoveBy(const Rect &r, int x, int y)
    {
        return Rect(r.Left + x, r.Top + y, r.Right + x, r.Bottom + y);
    }

    inline bool operator ==(const Rect &r) const
    {
        return Left == r.Left && Top == r.Top &&
            Right == r.Right && Bottom == r.Bottom;
    }
};

// Helper factory function
inline Rect RectWH(int x, int y, int width, int height)
{
	return Rect(x, y, x + width - 1, y + height - 1);
}

inline Rect RectWH(const Size &sz)
{
    return Rect(0, 0, sz.Width - 1, sz.Height - 1);
}


struct Triangle
{
	int X1;
	int Y1;
	int X2;
	int Y2;
	int X3;
	int Y3;

	Triangle()
	{
		X1 = 0;
		Y1 = 0;
		X2 = 0;
		Y2 = 0;
		X3 = 0;
		Y3 = 0;
	}

	Triangle(int x1, int y1, int x2, int y2, int x3, int y3)
	{
		X1 = x1;
		Y1 = y1;
		X2 = x2;
		Y2 = y2;
		X3 = x3;
		Y3 = y3;
	}
};

struct Circle
{
	int X;
	int Y;
	int Radius;

	Circle()
	{
		X = 0;
		Y = 0;
		Radius = 0;
	}

	Circle(int x, int y, int radius)
	{
		X = x;
		Y = y;
		Radius = radius;
	}

};


// Calculates a distance between two points
inline float DistanceBetween(const Point &p1, const Point &p2)
{
    return static_cast<float>(std::sqrt(((p2.X - p1.X) ^ 2) + ((p2.Y - p1.Y) ^ 2)));
}

// Tells if two rectangles intersect (overlap) at least partially
bool AreRectsIntersecting(const Rect &r1, const Rect &r2);
// Tells if the item is completely inside place
bool IsRectInsideRect(const Rect &place, const Rect &item);
// Calculates a distance between two axis-aligned rectangles, returns 0 if they intersect
float DistanceBetween(const Rect &r1, const Rect &r2);

// Align an item of certain width in the given horizontal frame; returns item's X position
int AlignInHRange(int frame_x1, int frame_x2, int item_offx, int item_width, FrameAlignment align);
// Align an item of certain height in the given vertical frame; returns item's Y position
int AlignInVRange(int frame_y1, int frame_y2, int item_offy, int item_height, FrameAlignment align);
// Align an item in the given frame, returns item's position as rectangle
Rect AlignInRect(const Rect &frame, const Rect &item, FrameAlignment align);

Size ProportionalStretch(int dest_w, int dest_h, int item_w, int item_h);
Size ProportionalStretch(const Size &dest, const Size &item);

Rect OffsetRect(const Rect &r, const Point off);
Rect CenterInRect(const Rect &place, const Rect &item);
Rect ClampToRect(const Rect &place, const Rect &item);
Rect PlaceInRect(const Rect &place, const Rect &item, const RectPlacement &placement);
// Sum two rectangles, the result is the rectangle bounding them both
Rect SumRects(const Rect &r1, const Rect &r2);
// Intersect two rectangles, the resolt is the rectangle bounding their intersection
Rect IntersectRects(const Rect &r1, const Rect &r2);

// Calculates the size of a rectangle necessary to accomodate the rect of original size
// if it were rotated by the given angle (in degrees)
Size RotateSize(Size sz, int degrees);

//} // namespace Common
//} // namespace AGS

#endif // __AGS_CN_UTIL__GEOMETRY_H
