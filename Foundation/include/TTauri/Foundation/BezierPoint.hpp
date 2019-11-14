// Copyright 2019 Pokitec
// All rights reserved.

#pragma once

#include "TTauri/Foundation/geometry.hpp"
#include "TTauri/Foundation/required.hpp"
#include "TTauri/Foundation/numeric_cast.hpp"
#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vec_swizzle.hpp>
#include <vector>

namespace TTauri {

/*! A point or control-point on contour of bezier curves.
 * The bezier curves can be linear (a line), quadratic or cubic.
 */
struct BezierPoint {
    enum class Type { Anchor, QuadraticControl, CubicControl1, CubicControl2 };

    Type type;
    glm::vec2 p;

    BezierPoint(glm::vec2 p, Type type) noexcept : type(type), p(p) {}
    BezierPoint(float x, float y, Type type) noexcept : BezierPoint({x, y}, type) {}

    /*! Normalize points in a list.
     * The following normalizations are executed:
     *  - Missing anchor points between two quadratic-control-points are added.
     *  - Missing first-cubic-control-points are added by reflecting the previous
     *    second-control point around the previous anchor.
     *  - The list of points will start with an anchor.
     *  - The list will close with the first anchor.
     *
     * \param begin iterator to the start of the bezier point list.
     * \param end iterator beyond the end of the bezier point list.
     * \return a vector of bezier point that include all the anchor and control points.
     */
    [[nodiscard]] static std::vector<BezierPoint> normalizePoints(
        std::vector<BezierPoint>::const_iterator begin,
        std::vector<BezierPoint>::const_iterator end
    ) noexcept {
        std::vector<BezierPoint> r;

        required_assert((end - begin) >= 2);

        auto previousPoint = *(end - 1);
        auto previousPreviousPoint = *(end - 2);
        for (auto i = begin; i != end; i++) {
            let point = *i;

            switch (point.type) {
            case BezierPoint::Type::Anchor:
                required_assert(previousPoint.type != BezierPoint::Type::CubicControl1);
                r.push_back(point);
                break;

            case BezierPoint::Type::QuadraticControl:
                if (previousPoint.type == BezierPoint::Type::QuadraticControl) {
                    r.emplace_back(midpoint(previousPoint.p, point.p), BezierPoint::Type::Anchor);

                } else {
                    required_assert(previousPoint.type == BezierPoint::Type::Anchor);
                }
                r.push_back(point);
                break;

            case BezierPoint::Type::CubicControl1:
                r.push_back(point);
                break;

            case BezierPoint::Type::CubicControl2:
                if (previousPoint.type == BezierPoint::Type::Anchor) {
                    required_assert(previousPreviousPoint.type == BezierPoint::Type::CubicControl2);

                    r.emplace_back(glm::reflect(previousPreviousPoint.p, previousPoint.p), BezierPoint::Type::CubicControl1);
                } else {
                    required_assert(previousPoint.type == BezierPoint::Type::CubicControl1);
                }
                r.push_back(point);
                break;

            default:
                no_default;
            }

            previousPreviousPoint = previousPoint;
            previousPoint = point;
        }

        for (int i = 0; i < to_signed(r.size()); i++) {
            if (r[i].type == BezierPoint::Type::Anchor) {
                std::rotate(r.begin(), r.begin() + i, r.end());
                r.push_back(r.front());
                return r;
            }
        }

        // The result did not contain an anchor.
        required_assert(false);
    }
};

[[nodiscard]] inline bool operator==(BezierPoint const &lhs, BezierPoint const &rhs) noexcept {
    return (lhs.p == rhs.p) && (lhs.type == rhs.type);
}


[[nodiscard]] inline BezierPoint operator*(glm::mat2x2 const &lhs, BezierPoint const &rhs) noexcept {
    return { lhs * rhs.p, rhs.type };
}


[[nodiscard]] inline BezierPoint operator*(glm::mat3x3 const &lhs, BezierPoint const &rhs) noexcept {
    return { glm::xy(lhs * glm::vec3{rhs.p, 1.0f}), rhs.type };
}


inline BezierPoint &operator*=(BezierPoint &lhs, glm::mat3x3 const &rhs) noexcept {
    lhs.p = glm::xy(rhs * glm::vec3{lhs.p, 1.0f});
    return lhs;
}


[[nodiscard]] inline BezierPoint operator*(float const lhs, BezierPoint const &rhs) noexcept {
    return { glm::xy(lhs * glm::vec3{rhs.p, 1.0f}), rhs.type };
}


inline BezierPoint &operator*=(BezierPoint &lhs, float const rhs) noexcept {
    lhs.p = glm::xy(rhs * glm::vec3{lhs.p, 1.0f});
    return lhs;
}

[[nodiscard]] inline BezierPoint operator+(BezierPoint const &lhs, glm::vec2 rhs) noexcept {
    return { lhs.p + rhs, lhs.type };
}

inline BezierPoint &operator+=(BezierPoint &lhs, glm::vec2 rhs) noexcept {
    lhs.p += rhs;
    return lhs;
}

}
