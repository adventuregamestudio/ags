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
// Helper matrix functions, to ease use of GLM in a simulated 2D space.
//
//=============================================================================
#ifndef __AGS_CN_UTIL__MATRIX_H
#define __AGS_CN_UTIL__MATRIX_H

#include <algorithm>
#include <glm/ext/matrix_transform.hpp>
#include "util/geometry.h"

namespace AGS
{
namespace Common
{

namespace glmex
{
    // Construct a 3D-compatible vec4 from (x, y)
    inline glm::vec4 vec4(float x, float y) { return glm::vec4(x, y, 0.0, 1.f); }

    // Create identity matrix
    inline glm::mat4 identity() { return glm::mat4(1.f); }
    // Translate matrix in 2D plane
    inline glm::mat4 translate(float x, float y) { return glm::translate(glmex::identity(), { x, y, 0.f }); }
    inline glm::mat4 translate(const glm::mat4 &m, float x, float y) { return glm::translate(m, {x, y, 0.f}); }
    // Scale matrix in 2D plane
    inline glm::mat4 scale(float sx, float sy) { return glm::scale(glmex::identity(), { sx, sy, 1.f }); }
    inline glm::mat4 scale(const glm::mat4 &m, float sx, float sy) { return glm::scale(m, { sx, sy, 1.f }); }
    // Rotate matrix in 2D plane (around Z)
    inline glm::mat4 rotatez(float angle) { return glm::rotate(glmex::identity(), angle, { 0.f, 0.f, 1.f }); }
    inline glm::mat4 rotatez(const glm::mat4 &m, float angle) { return glm::rotate(m, angle, { 0.f, 0.f, 1.f }); }
    // Setup full 2D transformation matrix
    inline glm::mat4 transform2d(const glm::mat4 &m_, float x, float y, float sx, float sy, float anglez, float pivotx = 0.0, float pivoty = 0.0)
    {
        glm::mat4 m = glmex::translate(m_, x - pivotx, y - pivoty);
        m = glmex::rotatez(m, anglez);
        m = glmex::translate(m, pivotx, pivoty);
        m = glmex::scale(m, sx, sy);
        return m;
    }
    inline glm::mat4 make_transform2d(float x, float y, float sx, float sy, float anglez, float pivotx = 0.0, float pivoty = 0.0)
    {
        return glmex::transform2d(glmex::identity(), x, y, sx, sy, anglez, pivotx, pivoty);
    }
    // Setup inverse 2D transformation matrix
    inline glm::mat4 inv_transform2d(const glm::mat4 &m_, float x, float y, float sx, float sy, float anglez, float pivotx = 0.0, float pivoty = 0.0)
    {
        glm::mat4 m = glmex::scale(m_, sx, sy);
        m = glmex::translate(m, pivotx, pivoty);
        m = glmex::rotatez(m, anglez);
        m = glmex::translate(m, x - pivotx, y - pivoty);
        return m;
    }
    inline glm::mat4 make_inv_transform2d(float x, float y, float sx, float sy, float anglez, float pivotx = 0.0, float pivoty = 0.0)
    {
        return inv_transform2d(glmex::identity(), x, y, sx, sy, anglez, pivotx, pivoty);
    }

    // Setup Direct3D-compatible orthographic projection
    // CHECKME: glm seem to supply some ortho variants meant for Direct3D,
    // but none of them worked right in the engine.
    inline glm::mat4 ortho_d3d(float width, float height)
    {
        return glm::mat4((2.f / width), 0.f, 0.f, 0.f,
                         0.f, (2.f / height), 0.f, 0.f,
                         0.f, 0.f, 0.f, 0.f,
                         0.f, 0.f, 0.f, 1.f);
    }


    // Linearly transform a rectangle using the given matrix;
    // This is an optimized case where rotation is not expected.
    inline Rect linear_transform(const Rect &r, const glm::mat4 &m)
    {
        glm::vec4 v1 = m * vec4((float)r.Left, (float)r.Top);
        glm::vec4 v2 = m * vec4((float)r.Right, (float)r.Bottom);
        // TODO: better rounding
        return Rect((int)v1.x, (int)v1.y, (int)v2.x, (int)v2.y);
    }

    // Transform a rectangle using the given matrix;
    // This is a full transform case which assumes rotation may be included.
    inline Rect full_transform(const Rect &r, const glm::mat4 &m)
    {
        // TODO: search for the faster AABB transform algorithm
        glm::vec4 p1 = m * glmex::vec4((float)r.Left, (float)r.Top);
        glm::vec4 p2 = m * glmex::vec4((float)r.Right, (float)r.Top);
        glm::vec4 p3 = m * glmex::vec4((float)r.Left, (float)r.Bottom);
        glm::vec4 p4 = m * glmex::vec4((float)r.Right, (float)r.Bottom);
        float xmin = std::min(p1.x, std::min(p2.x, std::min(p3.x, p4.x)));
        float ymin = std::min(p1.y, std::min(p2.y, std::min(p3.y, p4.y)));
        float xmax = std::max(p1.x, std::max(p2.x, std::max(p3.x, p4.x)));
        float ymax = std::max(p1.y, std::max(p2.y, std::max(p3.y, p4.y)));
        // TODO: better rounding
        return Rect((int)xmin, (int)ymin, (int)xmax, (int)ymax);
    }

} // namespace glmex

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__MATRIX_H
