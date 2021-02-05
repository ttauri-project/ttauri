// Copyright 2019 Pokitec
// All rights reserved.

#include "ttauri/Path.hpp"
#include "ttauri/bezier_curve.hpp"
#include <gtest/gtest.h>
#include <iostream>
#include <string>

using namespace std;
using namespace tt;

TEST(PathTests, getBeziersOfLayer) {
    auto path = Path();
    path.moveTo(f32x4::point( 1, 1 ));
    path.lineTo(f32x4::point( 2, 1 ));
    path.lineTo(f32x4::point( 2, 2 ));
    path.lineTo(f32x4::point( 1, 2 ));
    path.closeContour();

    ttlet beziers = path.getBeziers();
    ASSERT_EQ(beziers.size(), 4);
    ASSERT_EQ(beziers[0], bezier_curve(f32x4::point( 1,1 ), f32x4::point( 2,1 )));
    ASSERT_EQ(beziers[1], bezier_curve(f32x4::point( 2,1 ), f32x4::point( 2,2 )));
    ASSERT_EQ(beziers[2], bezier_curve(f32x4::point( 2,2 ), f32x4::point( 1,2 )));
    ASSERT_EQ(beziers[3], bezier_curve(f32x4::point( 1,2 ), f32x4::point( 1,1 )));
}

TEST(PathTests, getbezier_pointsOfContour) {
    auto path = Path();
    path.moveTo(f32x4::point( 1, 1 ));
    path.lineTo(f32x4::point( 2, 1 ));
    path.lineTo(f32x4::point( 2, 2 ));
    path.lineTo(f32x4::point( 1, 2 ));
    path.closeContour();

    ttlet points = path.getbezier_pointsOfContour(0);
    ASSERT_EQ(points.size(), 4);
    ASSERT_EQ(points[0], bezier_point(f32x4::point( 1,1 ), bezier_point::Type::Anchor));
    ASSERT_EQ(points[1], bezier_point(f32x4::point( 2,1 ), bezier_point::Type::Anchor));
    ASSERT_EQ(points[2], bezier_point(f32x4::point( 2,2 ), bezier_point::Type::Anchor));
    ASSERT_EQ(points[3], bezier_point(f32x4::point( 1,2 ), bezier_point::Type::Anchor));
}
