/// Copyright 2025 INRIA

#include "coal/fwd.hh"
#include "coal/distance.h"

#include "coal/serialization/collision_data.h"

#include "fwd.h"
#include "serializable.hh"

using namespace coal;
using namespace nb::literals;

// Copypasted from /python

struct DistanceResultWrapper {
  static Vec3s getNearestPoint1(const DistanceResult& res) {
    return res.nearest_points[0];
  }
  static Vec3s getNearestPoint2(const DistanceResult& res) {
    return res.nearest_points[1];
  }
};

// End of Copypasted from /python

void exposeDistanceAPI(nb::module_& m) {
  COAL_COMPILER_DIAGNOSTIC_PUSH
  COAL_COMPILER_DIAGNOSTIC_IGNORED_DEPRECECATED_DECLARATIONS
  nb::class_<DistanceRequest, QueryRequest>(m, "DistanceRequest")
      .def(nb::init<bool, bool, Scalar, Scalar>(),
           "enable_nearest_points"_a = true, "enable_signed_distance_"_a = true,
           "rel_err"_a = Scalar(0.0), "abs_err"_a = Scalar(0.0))
      .def_prop_rw(
          "enable_nearest_points",
          [](DistanceRequest& self) -> bool {
            return self.enable_nearest_points;
          },
          [](DistanceRequest& self, const bool value) -> void {
            self.enable_nearest_points = value;
          })
      .DEF_RW_CLASS_ATTRIB(DistanceRequest, enable_signed_distance)
      .DEF_RW_CLASS_ATTRIB(DistanceRequest, rel_err)
      .DEF_RW_CLASS_ATTRIB(DistanceRequest, abs_err)
      .def(python::v2::SerializableVisitor<DistanceRequest>());
  COAL_COMPILER_DIAGNOSTIC_POP

  nb::class_<DistanceResult, QueryResult>(m, "DistanceResult")
      .def(nb::init<>())
      .DEF_RW_CLASS_ATTRIB(DistanceResult, min_distance)
      .DEF_RW_CLASS_ATTRIB(DistanceResult, normal)
      .def("getNearestPoint1", &DistanceResultWrapper::getNearestPoint1)
      .def("getNearestPoint2", &DistanceResultWrapper::getNearestPoint2)
      .DEF_RO_CLASS_ATTRIB(DistanceResult, nearest_points)
      .DEF_RO_CLASS_ATTRIB(DistanceResult, o1)
      .DEF_RO_CLASS_ATTRIB(DistanceResult, o2)
      .DEF_RW_CLASS_ATTRIB(DistanceResult, b1)
      .DEF_RW_CLASS_ATTRIB(DistanceResult, b2)
      .def("clear", &DistanceResult::clear)
      .def(python::v2::SerializableVisitor<DistanceResult>());

  m.def(
      "distance",
      [](const CollisionObject* o1, const CollisionObject* o2,
         const DistanceRequest& request, DistanceResult& result) -> Scalar {
        return distance(o1, o2, request, result);
      },
      "o1"_a, "o2"_a, "request"_a, "result"_a);

  m.def(
      "distance",
      [](const CollisionGeometry* o1, const Transform3s& tf1,
         const CollisionGeometry* o2, const Transform3s& tf2,
         const DistanceRequest& request, DistanceResult& result) -> Scalar {
        return distance(o1, tf1, o2, tf2, request, result);
      },
      "geom1"_a, "transform1"_a, "geom2"_a, "transform2"_a, "request"_a,
      "result"_a);

  nb::class_<ComputeDistance>(m, "ComputeDistance")
      .def(nb::init<const CollisionGeometry*, const CollisionGeometry*>(),
           "o1"_a, "o2"_a)
      .def("__call__", [](const ComputeDistance& self, const Transform3s& t1,
                          const Transform3s& t2, const DistanceRequest& request,
                          DistanceResult& result) {
        return self.operator()(t1, t2, request, result);
      });
}