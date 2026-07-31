// SPDX-License-Identifier: LGPL-2.1-or-later

#include <Mod/Measure/App/MassPropertiesResult.h>

#include <BRepBuilderAPI_MakeFace.hxx>
#include <TopoDS_Face.hxx>
#include <gp_Pln.hxx>
#include <gtest/gtest.h>

TEST(MassPropertiesResult, calculatesAreaPropertiesForFaces)
{
    const TopoDS_Face face = BRepBuilderAPI_MakeFace(gp_Pln(), 0.0, 10.0, 0.0, 20.0).Face();
    const MassPropertiesData result = CalculateMassProperties(
        {{nullptr, face, Base::Placement()}},
        MassPropertiesMode::CenterOfGravity,
        nullptr
    );

    EXPECT_TRUE(result.isSurface);
    EXPECT_DOUBLE_EQ(result.volume.getValue(), 0.0);
    EXPECT_DOUBLE_EQ(result.mass.getValue(), 0.0);
    EXPECT_NEAR(result.surfaceArea.getValue(), 200.0, 1e-10);

    EXPECT_NEAR(result.cog.x, 5.0, 1e-10);
    EXPECT_NEAR(result.cog.y, 10.0, 1e-10);
    EXPECT_NEAR(result.cog.z, 0.0, 1e-10);

    // The result is calculated around the area centroid. The planar rectangle
    // is 10 mm by 20 mm, so Ix = bh^3/12 and Iy = hb^3/12.
    EXPECT_NEAR(result.inertiaJo.x, 200.0 * 20.0 * 20.0 / 12.0, 1e-8);
    EXPECT_NEAR(result.inertiaJo.y, 200.0 * 10.0 * 10.0 / 12.0, 1e-8);
    EXPECT_NEAR(result.inertiaJo.z, 200.0 * (10.0 * 10.0 + 20.0 * 20.0) / 12.0, 1e-8);
    EXPECT_NEAR(result.inertiaJCross.x, 0.0, 1e-10);
    EXPECT_NEAR(result.inertiaJCross.y, 0.0, 1e-10);
    EXPECT_NEAR(result.inertiaJCross.z, 0.0, 1e-10);
}
