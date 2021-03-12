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
// Helper matrix functions, to ease use of GLM in a simulated 2D space.
//
//=============================================================================
#ifndef __AGS_CN_UTIL__MATRIX_H
#define __AGS_CN_UTIL__MATRIX_H

#include <glm/ext/matrix_transform.hpp>

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
    inline glm::mat4 translate(const glm::mat4 &m, float x, float y) { return glm::translate(m, {x, y, 0.f}); }
    // Scale matrix in 2D plane
    inline glm::mat4 scale(const glm::mat4 &m, float sx, float sy) { return glm::scale(m, { sx, sy, 1.f }); }
    // Rotate matrix in 2D plane (around Z)
    inline glm::mat4 rotatez(const glm::mat4 &m, float angle) { return glm::rotate(m, angle, { 0.f, 0.f, 1.f }); }
    // Setup full 2D transformation matrix
    inline glm::mat4 make_transform2d(float x, float y, float sx, float sy, float anglez, float pivotx = 0.0, float pivoty = 0.0)
    {
        glm::mat4 m = glmex::identity();
        m = glmex::translate(m, x - pivotx, y - pivoty);
        m = glmex::rotatez(m, anglez);
        m = glmex::translate(m, pivotx, pivoty);
        m = glmex::scale(m, sx, sy);
        return m;
    }
    // Setup inverse 2D transformation matrix
    inline glm::mat4 make_inv_transform2d(float x, float y, float sx, float sy, float anglez, float pivotx = 0.0, float pivoty = 0.0)
    {
        glm::mat4 m = glmex::identity();
        m = glmex::scale(m, sx, sy);
        m = glmex::translate(m, pivotx, pivoty);
        m = glmex::rotatez(m, anglez);
        m = glmex::translate(m, x - pivotx, y - pivoty);
        return m;
    }
} // namespace glmex

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__MATRIX_H
