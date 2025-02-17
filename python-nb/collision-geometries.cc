#include "fwd.h"
#include "serializable.hh"

#include "coal/fwd.hh"
#include "coal/shape/geometric_shapes.h"
#include "coal/shape/convex.h"
#include "coal/BVH/BVH_model.h"
#include "coal/hfield.h"

#include "coal/serialization/memory.h"

using namespace coal;
using Triangles = std::vector<Triangle>;

void exposeCollisionGeometries(nb::module_ &m) {
  nb::enum_<BVHModelType>(m, "BVHModelType")
      .value("BVH_MODEL_UNKNOWN", BVH_MODEL_UNKNOWN)
      .value("BVH_MODEL_TRIANGLES", BVH_MODEL_TRIANGLES)
      .value("BVH_MODEL_POINTCLOUD", BVH_MODEL_POINTCLOUD)
      .export_values();

  nb::enum_<BVHBuildState>(m, "BVHBuildState")
      .value("BVH_BUILD_STATE_EMPTY", BVH_BUILD_STATE_EMPTY)
      .value("BVH_BUILD_STATE_BEGUN", BVH_BUILD_STATE_BEGUN)
      .value("BVH_BUILD_STATE_PROCESSED", BVH_BUILD_STATE_PROCESSED)
      .value("BVH_BUILD_STATE_UPDATE_BEGUN", BVH_BUILD_STATE_UPDATE_BEGUN)
      .value("BVH_BUILD_STATE_UPDATED", BVH_BUILD_STATE_UPDATED)
      .value("BVH_BUILD_STATE_REPLACE_BEGUN", BVH_BUILD_STATE_REPLACE_BEGUN)
      .export_values();

  nb::enum_<OBJECT_TYPE>(m, "OBJECT_TYPE")
      .value("OT_UNKNOWN", OT_UNKNOWN)
      .value("OT_BVH", OT_BVH)
      .value("OT_GEOM", OT_GEOM)
      .value("OT_OCTREE", OT_OCTREE)
      .value("OT_HFIELD", OT_HFIELD)
      .export_values();

  nb::enum_<NODE_TYPE>(m, "NODE_TYPE")
      .value("BV_UNKNOWN", BV_UNKNOWN)
      .value("BV_AABB", BV_AABB)
      .value("BV_OBB", BV_OBB)
      .value("BV_RSS", BV_RSS)
      .value("BV_kIOS", BV_kIOS)
      .value("BV_OBBRSS", BV_OBBRSS)
      .value("BV_KDOP16", BV_KDOP16)
      .value("BV_KDOP18", BV_KDOP18)
      .value("BV_KDOP24", BV_KDOP24)
      .value("GEOM_BOX", GEOM_BOX)
      .value("GEOM_SPHERE", GEOM_SPHERE)
      .value("GEOM_ELLIPSOID", GEOM_ELLIPSOID)
      .value("GEOM_CAPSULE", GEOM_CAPSULE)
      .value("GEOM_CONE", GEOM_CONE)
      .value("GEOM_CYLINDER", GEOM_CYLINDER)
      .value("GEOM_CONVEX", GEOM_CONVEX)
      .value("GEOM_PLANE", GEOM_PLANE)
      .value("GEOM_HALFSPACE", GEOM_HALFSPACE)
      .value("GEOM_TRIANGLE", GEOM_TRIANGLE)
      .value("GEOM_OCTREE", GEOM_OCTREE)
      .value("HF_AABB", HF_AABB)
      .value("HF_OBBRSS", HF_OBBRSS)
      .export_values();
}
