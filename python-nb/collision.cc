/// Copyright 2025 INRIA

#include "coal/fwd.hh"
COAL_COMPILER_DIAGNOSTIC_PUSH
COAL_COMPILER_DIAGNOSTIC_IGNORED_DEPRECECATED_DECLARATIONS
#include "coal/collision.h"
#include "coal/serialization/collision_data.h"
COAL_COMPILER_DIAGNOSTIC_POP

#include <nanobind/eigen/dense.h>
#include <nanobind/operators.h>

#include "serializable.hh"

using namespace coal;
using namespace nb::literals;

// Copypasted from /python

template <int index>
const CollisionGeometry* geto(const Contact& c) {
  return index == 1 ? c.o1 : c.o2;
}

struct ContactWrapper {
  static Vec3s getNearestPoint1(const Contact& contact) {
    return contact.nearest_points[0];
  }
  static Vec3s getNearestPoint2(const Contact& contact) {
    return contact.nearest_points[1];
  }
};

// End of Copypasted from /python

void exposeCollisionAPI(nb::module_& m) {
  nb::enum_<CollisionRequestFlag>(m, "CollisionRequestFlag")
      .value("CONTACT", CONTACT)
      .value("DISTANCE_LOWER_BOUND", DISTANCE_LOWER_BOUND)
      .value("NO_REQUEST", NO_REQUEST)
      .export_values();

  nb::class_<CPUTimes>(m, "CPUTimes")
      .def_ro("wall", &CPUTimes::wall, "wall time in micro seconds (us)")
      .def_ro("user", &CPUTimes::user, "user time in micro seconds (us)")
      .def_ro("system", &CPUTimes::system, "system time in micro seconds (us)")
      .def("clear", &CPUTimes::clear, "Reset the time values.");

  COAL_COMPILER_DIAGNOSTIC_PUSH
  COAL_COMPILER_DIAGNOSTIC_IGNORED_DEPRECECATED_DECLARATIONS
  nb::class_<QueryRequest>(m, "QueryRequest")
      .DEF_RW_CLASS_ATTRIB(QueryRequest, gjk_tolerance)
      .DEF_RW_CLASS_ATTRIB(QueryRequest, gjk_max_iterations)
      .DEF_RW_CLASS_ATTRIB(QueryRequest, gjk_variant)
      .DEF_RW_CLASS_ATTRIB(QueryRequest, gjk_convergence_criterion)
      .DEF_RW_CLASS_ATTRIB(QueryRequest, gjk_convergence_criterion_type)
      .DEF_RW_CLASS_ATTRIB(QueryRequest, gjk_initial_guess)
      .DEF_RW_CLASS_ATTRIB(QueryRequest, enable_cached_gjk_guess)
      .def_prop_rw(
          "enable_cached_gjk_guess",
          [](QueryRequest& self) -> bool {
            return self.enable_cached_gjk_guess;
          },
          [](QueryRequest& self, bool value) {
            self.enable_cached_gjk_guess = value;
          })
      .DEF_RW_CLASS_ATTRIB(QueryRequest, cached_gjk_guess)
      .DEF_RW_CLASS_ATTRIB(QueryRequest, cached_support_func_guess)
      .DEF_RW_CLASS_ATTRIB(QueryRequest, epa_max_iterations)
      .DEF_RW_CLASS_ATTRIB(QueryRequest, epa_tolerance)
      .DEF_RW_CLASS_ATTRIB(QueryRequest, enable_timings)
      .DEF_CLASS_FUNC(QueryRequest, updateGuess);
  COAL_COMPILER_DIAGNOSTIC_POP

  COAL_COMPILER_DIAGNOSTIC_PUSH
  COAL_COMPILER_DIAGNOSTIC_IGNORED_DEPRECECATED_DECLARATIONS
  nb::class_<CollisionRequest, QueryRequest>(m, "CollisionRequest")
      .def(nb::init<>())
      .def(nb::init<const CollisionRequestFlag, size_t>(), "flag"_a,
           "num_max_contacts_"_a)
      .DEF_RW_CLASS_ATTRIB(CollisionRequest, num_max_contacts)
      .DEF_RW_CLASS_ATTRIB(CollisionRequest, enable_contact)
      .def_prop_rw(
          "enable_distance_lower_bound",
          [](CollisionRequest& self) -> bool {
            return self.enable_distance_lower_bound;
          },
          [](CollisionRequest& self, bool value) {
            self.enable_distance_lower_bound = value;
          })
      .DEF_RW_CLASS_ATTRIB(CollisionRequest, security_margin)
      .DEF_RW_CLASS_ATTRIB(CollisionRequest, break_distance)
      .DEF_RW_CLASS_ATTRIB(CollisionRequest, distance_upper_bound);
  // .def(python::v2::SerializableVisitor<CollisionRequest>());
  COAL_COMPILER_DIAGNOSTIC_POP

  nb::class_<Contact>(m, "Contact")
      .def(nb::init<>())
      .def(nb::init<const CollisionGeometry*, const CollisionGeometry*, int,
                    int>(),
           "o1_"_a, "o2_"_a, "b1_"_a, "b2_"_a)
      .def(nb::init<const CollisionGeometry*, const CollisionGeometry*, int,
                    int, const Vec3s&, const Vec3s&, Scalar>(),
           "o1_"_a, "o2_"_a, "b1_"_a, "b2_"_a, "pos_"_a, "normal_"_a,
           "depth_"_a)
      .def_prop_ro("o1",
                   [](Contact& self) -> CollisionGeometry* {
                     return const_cast<CollisionGeometry*>(geto<1>(self));
                   })
      .def_prop_ro("o2",
                   [](Contact& self) -> CollisionGeometry* {
                     return const_cast<CollisionGeometry*>(geto<2>(self));
                   })
      .def("getNearestPoint1", &ContactWrapper::getNearestPoint1)
      .def("getNearestPoint2", &ContactWrapper::getNearestPoint2)
      .DEF_RW_CLASS_ATTRIB(Contact, b1)
      .DEF_RW_CLASS_ATTRIB(Contact, b2)
      .DEF_RW_CLASS_ATTRIB(Contact, normal)
      .DEF_RW_CLASS_ATTRIB(Contact, nearest_points)
      .DEF_RW_CLASS_ATTRIB(Contact, pos)
      .DEF_RW_CLASS_ATTRIB(Contact, penetration_depth)
      .def(nb::self == nb::self)
      .def(nb::self != nb::self);

  nb::class_<QueryResult>(m, "QueryResult")
      .DEF_RW_CLASS_ATTRIB(QueryResult, cached_gjk_guess)
      .DEF_RW_CLASS_ATTRIB(QueryResult, cached_support_func_guess)
      .DEF_RW_CLASS_ATTRIB(QueryResult, timings);

  nb::class_<CollisionResult, QueryResult>(m, "CollisionResult")
      .def(nb::init<>())
      .DEF_CLASS_FUNC(CollisionResult, isCollision)
      .DEF_CLASS_FUNC(CollisionResult, numContacts)
      .DEF_CLASS_FUNC(CollisionResult, addContact)
      .DEF_CLASS_FUNC(CollisionResult, clear)
      .def("getContact", &CollisionResult::getContact, nb::rv_policy::copy)
      .def("getContacts",
           [](const CollisionResult& self, std::vector<Contact>& contacts) {
             self.getContacts(contacts);
           })
      .def(
          "getContacts",
          [](const CollisionResult& self) -> const std::vector<Contact>& {
            return self.getContacts();
          },
          nb::rv_policy::reference_internal)
      .DEF_RW_CLASS_ATTRIB(CollisionResult, distance_lower_bound);
  // .def(python::v2::SerializableVisitor<CollisionResult>());

  nb::class_<ComputeCollision>(m, "ComputeCollision")
      .def(nb::init<const CollisionGeometry*, const CollisionGeometry*>(),
           "o1"_a, "o2"_a)
      .def("call",
           [](const ComputeCollision& self, const Transform3s& tf1,
              const Transform3s& tf2, const CollisionRequest& request,
              CollisionResult& result) -> std::size_t {
             return self(tf1, tf2, request, result);
           });
}