/// Copyright 2025 INRIA

#include "fwd.h"
#include <nanobind/eigen/dense.h>
#include <nanobind/operators.h>

#include "coal/fwd.hh"
#include "coal/shape/geometric_shapes.h"
#include "coal/shape/convex.h"
#include "coal/hfield.h"
#include "coal/serialization/memory.h"
#include "coal/serialization/AABB.h"

#include "pickle.hh"
#include "serializable.hh"

using namespace coal;
using namespace nb::literals;
using Triangles = std::vector<Triangle>;

void exposeShapes(nb::module_& m);
void exposeBVHModels(nb::module_& m);
void exposeHeightFields(nb::module_& m);
void exposeComputeMemoryFootprint(nb::module_& m);

void exposeCollisionGeometries(nb::module_& m) {
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

  nb::class_<AABB>(
      m, "AABB",
      "A class describing the AABB collision structure, which is a "
      "box in 3D space determined by two diagonal points")
      .def(nb::init<>())
      .def(nb::init<AABB>(), "other"_a)
      .def(nb::init<Vec3s>(), "v"_a)
      .def(nb::init<Vec3s, Vec3s>(), "a"_a, "b"_a)
      .def(nb::init<AABB, Vec3s>(), "core"_a, "delta"_a)
      .def(nb::init<Vec3s, Vec3s, Vec3s>(), "a"_a, "b"_a, "c"_a)

      .def(
          "contain",
          [](const AABB& self, const Vec3s& p) { return self.contain(p); },
          "p"_a, "Check whether the AABB contains a point p.")
      .def(
          "contain",
          [](const AABB& self, const AABB& other) {
            return self.contain(other);
          },
          "other"_a, "Check whether the AABB contains another AABB.")
      .def(
          "overlap",
          [](const AABB& self, const AABB& other) {
            return self.overlap(other);
          },
          "other"_a, "Check whether two AABB overlap.")
      .def(
          "overlap",
          [](const AABB& self, const AABB& other, AABB& overlapping_part) {
            return self.overlap(other, overlapping_part);
          },
          "other"_a, "overlapping_part"_a,
          "Check whether two AABB overlap and return the overlapping part if "
          "true.")
      .def(
          "distance",
          [](const AABB& self, const AABB& other) {
            return self.distance(other);
          },
          "other"_a, "Distance between two AABBs.")

      .def_prop_rw(
          "min_", [](AABB& self) -> Vec3s& { return self.min_; },
          [](AABB& self, const Vec3s& min_) { self.min_ = min_; },
          "The min point in the AABB.")
      .def_prop_rw(
          "max_", [](AABB& self) -> Vec3s& { return self.max_; },
          [](AABB& self, const Vec3s& max_) { self.max_ = max_; },
          "The max point in the AABB.")

      .def(nb::self == nb::self)
      .def(nb::self != nb::self)
      .def(nb::self + nb::self)
      .def(nb::self += nb::self)
      .def(nb::self += Vec3s())

      .def("size", &AABB::volume)
      .def("center", &AABB::center)
      .def("width", &AABB::width)
      .def("height", &AABB::height)
      .def("depth", &AABB::depth)
      .def("volume", &AABB::volume)

      .def(
          "expand",
          [](AABB& self, const AABB& other, Scalar scalar) -> AABB& {
            return self.expand(other, scalar);
          },
          nb::rv_policy::reference_internal)
      .def(
          "expand",
          [](AABB& self, Scalar scalar) -> AABB& {
            return self.expand(scalar);
          },
          nb::rv_policy::reference_internal)
      .def(
          "expand",
          [](AABB& self, const Vec3s& vec) -> AABB& {
            return self.expand(vec);
          },
          nb::rv_policy::reference_internal)

      .def(python::v2::PickleVisitor<AABB>())        // TODO: TOFIX
      .def(python::v2::SerializableVisitor<AABB>())  // TODO: TOFIX
      // #if EIGENPY_VERSION_AT_LEAST(3, 8, 0)
      //       .def(eigenpy::IdVisitor<AABB>())
      // #endif
      ;

  m.def(
      "translate",
      [](const AABB& aabb, const Vec3s& t) { return translate(aabb, t); },
      "aabb"_a, "t"_a, "Translate the center of AABB by t");

  m.def(
      "rotate",
      [](const AABB& aabb, const Matrix3s& R) { return rotate(aabb, R); },
      "aabb"_a, "R"_a, "Rotate the AABB object by R");

  nb::class_<CollisionGeometry>(m, "CollisionGeometry")
      .DEF_CLASS_FUNC(CollisionGeometry, getObjectType)
      .DEF_CLASS_FUNC(CollisionGeometry, getNodeType)

      .DEF_CLASS_FUNC(CollisionGeometry, computeLocalAABB)
      .DEF_CLASS_FUNC(CollisionGeometry, computeCOM)
      .DEF_CLASS_FUNC(CollisionGeometry, computeMomentofInertia)
      .DEF_CLASS_FUNC(CollisionGeometry, computeVolume)
      .DEF_CLASS_FUNC(CollisionGeometry, computeMomentofInertiaRelatedToCOM)

      .def_rw("aabb_radius", &CollisionGeometry::aabb_radius)
      .def_rw("aabb_center", &CollisionGeometry::aabb_center)
      .def_rw("aabb_local", &CollisionGeometry::aabb_local)

      .def("isOccupied", &CollisionGeometry::isOccupied)
      .def("isFree", &CollisionGeometry::isFree)
      .def("isUncertain", &CollisionGeometry::isUncertain)

      .def_rw("cost_density", &CollisionGeometry::cost_density)
      .def_rw("threshold_occupied", &CollisionGeometry::threshold_occupied)
      .def_rw("threshold_free", &CollisionGeometry::threshold_free)

      .def(nb::self == nb::self)
      .def(nb::self != nb::self);

  exposeShapes(m);
  exposeBVHModels(m);
  exposeHeightFields(m);
  exposeComputeMemoryFootprint(m);
}

void exposeCollisionObject(nb::module_& m) {
  nb::class_<CollisionObject>(m, "CollisionObject")
      .def(nb::init<const CollisionGeometryPtr_t&, bool>(), "cgeom"_a,
           "compute_local_aabb"_a = true)
      .def(nb::init<const CollisionGeometryPtr_t&, const Transform3s&, bool>(),
           "cgeom"_a, "tf"_a, "compute_local_aabb"_a = true)
      .def(nb::init<const CollisionGeometryPtr_t&, const Matrix3s&,
                    const Vec3s&, bool>(),
           "cgeom"_a, "R"_a, "t"_a, "compute_local_aabb"_a = true)
      .def(nb::self == nb::self)
      .def(nb::self != nb::self)

      .DEF_CLASS_FUNC(CollisionObject, getObjectType)
      .DEF_CLASS_FUNC(CollisionObject, getNodeType)

      // properties
      .def_prop_rw("translation", &CollisionObject::getTranslation,
                   &CollisionObject::setTranslation)
      .def_prop_rw("rotation", &CollisionObject::getRotation,
                   &CollisionObject::setRotation)
      .def("getTransform", &CollisionObject::getTransform)
      .def("setTransform", [](CollisionObject& o,
                              const Transform3s& tf) { o.setTransform(tf); })
      .def("setTransform", [](CollisionObject& o, const Matrix3s& R,
                              const Vec3s& t) { o.setTransform(R, t); })

      .def("isIdentityTransform", &CollisionObject::isIdentityTransform)
      .def("setIdentityTransform", &CollisionObject::setIdentityTransform)

      .def(
          "getAABB", [](CollisionObject& o) -> AABB& { return o.getAABB(); },
          nb::rv_policy::automatic_reference)
      .DEF_CLASS_FUNC(CollisionObject, computeAABB)

      .def("setCollisionGeometry", &CollisionObject::setCollisionGeometry,
           "cgeom"_a, "compute_local_aabb"_a = true)
      .def("collisionGeometry",
           [](CollisionObject& o) { return o.collisionGeometry(); });
}
