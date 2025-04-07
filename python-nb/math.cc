/// Copyright 2025 INRIA
#include "coal/math/transform.h"
#include "coal/serialization/transform.h"

#include <nanoeigenpy/geometry.hpp>

#include "fwd.h"
#include "serializable.hh"
#include "pickle.hh"

#include <nanobind/eigen/dense.h>
#include <nanobind/stl/bind_vector.h>
#include <nanobind/operators.h>

using namespace coal;
using namespace nb::literals;

void exposeMaths(nb::module_ &m) {
  nanoeigenpy::exposeQuaternion<Scalar>(m, "Quaternion");
  nanoeigenpy::exposeAngleAxis<Scalar>(m, "AngleAxis");

  nb::class_<Transform3s>(m, "Transform3s")
      .def(nb::init<>())
      .def(nb::init<const Matrix3s &, Vec3s>(), "R"_a, "t"_a)
      .def(nb::init<const Quats &, Vec3s>(), "q"_a, "t"_a)
      .def(nb::init<const Matrix3s &>(), "R"_a)
      .def(nb::init<const Quats &>(), "q"_a)
      .def(nb::init<const Vec3s &>(), "t"_a)
      .def(nb::init<const Transform3s &>())

      .def("getQuatRotation", &Transform3s::getQuatRotation,
           nb::rv_policy::automatic_reference)
      .def_prop_rw("translation", &Transform3s::getTranslation,
                   [](Transform3s &t, Vec3s v) { t.setTranslation(v); })
      .def_prop_rw("rotation", &Transform3s::getRotation,
                   [](Transform3s &t, Matrix3s R) { t.setRotation(R); })
      .def("isIdentity", &Transform3s::isIdentity)

      .def("setQuatRotation", &Transform3s::setQuatRotation)
      // .def("setTransform", &Transform3s::setTransform<Matrix3s, Vec3s>) //
      // TODO: TOFIX .def("setTransform", [](Transform3s &self, const Quats
      // &quat, const Vec3s &vec)  // TODO: TOFIX
      //                                 { self.setTransform(quat, vec); })
      .def("setIdentity", &Transform3s::setIdentity)
      .def_static("Identity", &Transform3s::Identity)

      .def("setRandom", &Transform3s::setRandom)
      .def_static("Random", &Transform3s::Random)

      // .def("transform", &Transform3s::transform<Vec3s>) // TODO: TOFIX
      .def("inverseInPlace", &Transform3s::inverseInPlace,
           nb::rv_policy::automatic_reference)
      .def("inverse", &Transform3s::inverse)
      .def("inverseTimes", &Transform3s::inverseTimes)

      .def(nb::self * nb::self)
      .def(nb::self *= nb::self)
      .def(nb::self == nb::self)
      .def(nb::self != nb::self)

      // .def(python::v2::PickleVisitor<Transform3s>());
      // .def(python::v2::SerializableVisitor<Transform3s>());

      ;

  nb::class_<Triangle>(m, "Triangle")
      .def(nb::init<>())
      .def(nb::init<Triangle::index_type, Triangle::index_type,
                    Triangle::index_type>(),
           "p1"_a, "p2"_a, "p3"_a)
      .def("__getitem__",
           [](Triangle &m, Py_ssize_t i) {
             if (i >= 3 || i <= -3) {
               throw nb::index_error("Index out of range");
             }
             return m[static_cast<Triangle::index_type>(i % 3)];
           })
      .def("__setitem__",
           [](Triangle &m, Py_ssize_t i, Triangle::index_type v) {
             if (i >= 3 || i <= -3) {
               throw nb::index_error("Index out of range");
             }
             m[static_cast<Triangle::index_type>(i % 3)] = v;
           })
      .def("set", &Triangle::set)
      .def_static("size", &Triangle::size)
      .def(nb::self == nb::self);

  nb::bind_vector<std::vector<Triangle>>(m, "StdVec_Triangle");
  nb::bind_vector<std::vector<Vec3s>>(m, "StdVec_Vec3s");
}
