/// Copyright 2025 INRIA

#include "coal/shape/geometric_shapes.h"
#include "fwd.h"
#include "coal/BVH/BVH_model.h"

#include "pickle.hh"
#include "serializable.hh"

#include "coal/serialization/BVH_model.h"
#include "coal/serialization/AABB.h"
#include "coal/serialization/BVH_model.h"
#include "coal/serialization/hfield.h"
#include "coal/serialization/geometric_shapes.h"
#include "coal/serialization/convex.h"

#include <boost/python/list.hpp>

using namespace coal;
using namespace nb::literals;
using namespace boost::python;

typedef std::vector<Vec3s> Vec3ss;
typedef std::vector<Triangle> Triangles;

// Copypasted from /python

struct ConvexBaseWrapper {
  typedef Eigen::Matrix<Scalar, Eigen::Dynamic, 3, Eigen::RowMajor> RowMatrixX3;
  typedef Eigen::Map<RowMatrixX3> MapRowMatrixX3;
  typedef Eigen::Ref<RowMatrixX3> RefRowMatrixX3;
  typedef Eigen::Map<VecXs> MapVecXs;
  typedef Eigen::Ref<VecXs> RefVecXs;

  static Vec3s& point(const ConvexBase& convex, unsigned int i) {
    if (i >= convex.num_points)
      throw std::out_of_range("index is out of range");
    return (*(convex.points))[i];
  }

  static RefRowMatrixX3 points(const ConvexBase& convex) {
    return MapRowMatrixX3((*(convex.points))[0].data(), convex.num_points, 3);
  }

  static Vec3s& normal(const ConvexBase& convex, unsigned int i) {
    if (i >= convex.num_normals_and_offsets)
      throw std::out_of_range("index is out of range");
    return (*(convex.normals))[i];
  }

  static RefRowMatrixX3 normals(const ConvexBase& convex) {
    return MapRowMatrixX3((*(convex.normals))[0].data(),
                          convex.num_normals_and_offsets, 3);
  }

  static Scalar offset(const ConvexBase& convex, unsigned int i) {
    if (i >= convex.num_normals_and_offsets)
      throw std::out_of_range("index is out of range");
    return (*(convex.offsets))[i];
  }

  static RefVecXs offsets(const ConvexBase& convex) {
    return MapVecXs(convex.offsets->data(), convex.num_normals_and_offsets, 1);
  }

  static list neighbors(const ConvexBase& convex, unsigned int i) {
    if (i >= convex.num_points)
      throw std::out_of_range("index is out of range");
    list n;
    const std::vector<ConvexBase::Neighbors>& neighbors_ = *(convex.neighbors);
    for (unsigned char j = 0; j < neighbors_[i].count(); ++j)
      n.append(neighbors_[i][j]);
    return n;
  }

  static ConvexBase* convexHull(const Vec3ss& points, bool keepTri,
                                const char* qhullCommand) {
    return ConvexBase::convexHull(points.data(), (unsigned int)points.size(),
                                  keepTri, qhullCommand);
  }
};

template <typename PolygonT>
struct ConvexWrapper {
  typedef Convex<PolygonT> Convex_t;

  static PolygonT polygons(const Convex_t& convex, unsigned int i) {
    if (i >= convex.num_polygons)
      throw std::out_of_range("index is out of range");
    return (*convex.polygons)[i];
  }

  static shared_ptr<Convex_t> constructor(const Vec3ss& _points,
                                          const Triangles& _tris) {
    std::shared_ptr<std::vector<Vec3s>> points(
        new std::vector<Vec3s>(_points.size()));
    std::vector<Vec3s>& points_ = *points;
    for (std::size_t i = 0; i < _points.size(); ++i) points_[i] = _points[i];

    std::shared_ptr<std::vector<Triangle>> tris(
        new std::vector<Triangle>(_tris.size()));
    std::vector<Triangle>& tris_ = *tris;
    for (std::size_t i = 0; i < _tris.size(); ++i) tris_[i] = _tris[i];
    return shared_ptr<Convex_t>(new Convex_t(points,
                                             (unsigned int)_points.size(), tris,
                                             (unsigned int)_tris.size()));
  }
};

// End of Copypasted from /python

void exposeShapes(nb::module_& m) {
  nb::class_<ShapeBase, CollisionGeometry>(m, "ShapeBase")
      .def("setSweptSphereRadius", &ShapeBase::setSweptSphereRadius)
      .def("getSweptSphereRadius", &ShapeBase::getSweptSphereRadius);

  nb::class_<Box, ShapeBase>(m, "Box")
      .def(nb::init<>())
      .def(nb::init<const Box&>(), "other"_a)
      .def(nb::init<Scalar, Scalar, Scalar>(), "x"_a, "y"_a, "z"_a)
      .def(nb::init<const Vec3s&>(), "side_"_a)
      .DEF_RW_CLASS_ATTRIB(Box, halfSide)
      .def("clone", &Box::clone, nb::rv_policy::take_ownership)
      // .def(python::v2::PickleVisitor<Box>())
      // .def(python::v2::SerializableVisitor<Box>())
      // #if EIGENPY_VERSION_AT_LEAST(3, 8, 0)
      //       .def(eigenpy::IdVisitor<Box>())
      // #endif
      ;

  nb::class_<Capsule, ShapeBase>(m, "Capsule")
      .def(nb::init<>())
      .def(nb::init<Scalar, Scalar>(), "radius"_a, "lz_"_a)
      .def(nb::init<const Capsule&>(), "other_"_a)
      .DEF_RW_CLASS_ATTRIB(Capsule, radius)
      .DEF_RW_CLASS_ATTRIB(Capsule, halfLength)
      .def("clone", &Capsule::clone, nb::rv_policy::take_ownership)
      // .def(python::v2::PickleVisitor<Capsule>())
      // .def(python::v2::SerializableVisitor<Capsule>())
      // #if EIGENPY_VERSION_AT_LEAST(3, 8, 0)
      //       .def(eigenpy::IdVisitor<Capsule>())
      // #endif
      ;

  nb::class_<Cone, ShapeBase>(m, "Cone")
      .def(nb::init<>())
      .def(nb::init<Scalar, Scalar>(), "radius"_a, "lz_"_a)
      .def(nb::init<const Cone&>(), "other_"_a)
      .DEF_RW_CLASS_ATTRIB(Cone, radius)
      .DEF_RW_CLASS_ATTRIB(Cone, halfLength)
      .def("clone", &Cone::clone, nb::rv_policy::take_ownership)
      // .def(python::v2::PickleVisitor<Cone>())
      // .def(python::v2::SerializableVisitor<Cone>())
      // #if EIGENPY_VERSION_AT_LEAST(3, 8, 0)
      //       .def(eigenpy::IdVisitor<Cone>())
      // #endif
      ;

  nb::class_<ConvexBase, ShapeBase>(m, "ConvexBase")
      .DEF_RO_CLASS_ATTRIB(ConvexBase, center)
      .DEF_RO_CLASS_ATTRIB(ConvexBase, num_points)
      .DEF_RO_CLASS_ATTRIB(ConvexBase, num_normals_and_offsets)
      .def("point", &ConvexBaseWrapper::point, "index"_a,
           "Retrieve the point given by its index.",
           nb::rv_policy::reference_internal)
      .def("points", &ConvexBaseWrapper::point, "index"_a,
           "Retrieve the point given by its index.",
           nb::rv_policy::reference_internal)
      .def("points", &ConvexBaseWrapper::points, "Retrieve all the points.")
      .def("normal", &ConvexBaseWrapper::normal, "index"_a,
           "Retrieve the normal given by its index.",
           nb::rv_policy::reference_internal)
      .def("normals", &ConvexBaseWrapper::normals, "Retrieve all the normals.")
      .def("offset", &ConvexBaseWrapper::offset, "index"_a,
           "Retrieve the offset given by its index.")
      .def("offsets", &ConvexBaseWrapper::offsets, "Retrieve all the offsets.")
      .def("neighbors", &ConvexBaseWrapper::neighbors)
      .def_static("convexHull", &ConvexBaseWrapper::convexHull,
                  nb::rv_policy::take_ownership)
      .def("clone", &ConvexBase::clone, nb::rv_policy::take_ownership);

  nb::class_<Convex<Triangle>, ConvexBase>(m, "Convex")
      .def(nb::init<>())
      .def(nb::init<const Convex<Triangle>>(), "other_"_a)
      .DEF_RO_CLASS_ATTRIB(Convex<Triangle>, num_polygons)
      .def("polygons", &ConvexWrapper<Triangle>::polygons)
      // .def(python::v2::PickleVisitor<Convex<Triangle>>())
      // .def(python::v2::SerializableVisitor<Convex<Triangle>>())
      // #if EIGENPY_VERSION_AT_LEAST(3, 8, 0)
      //       .def(eigenpy::IdVisitor<Convex<Triangle>>())
      // #endif
      ;

  nb::class_<Cylinder, ShapeBase>(m, "Cylinder")
      .def(nb::init<>())
      .def(nb::init<Scalar, Scalar>(), "radius"_a, "lz_"_a)
      .def(nb::init<const Cylinder&>(), "other_"_a)
      .DEF_RW_CLASS_ATTRIB(Cylinder, radius)
      .DEF_RW_CLASS_ATTRIB(Cylinder, halfLength)
      .def("clone", &Cylinder::clone, nb::rv_policy::take_ownership)
      // .def(python::v2::PickleVisitor<Cylinder>())
      // .def(python::v2::SerializableVisitor<Cylinder>())
      // #if EIGENPY_VERSION_AT_LEAST(3, 8, 0)
      //       .def(eigenpy::IdVisitor<Cylinder>())
      // #endif
      ;

  nb::class_<Halfspace, ShapeBase>(m, "Halfspace")
      .def(nb::init<>())
      .def(nb::init<const Vec3s&, Scalar>(), "n_"_a, "d_"_a)
      .def(nb::init<const Halfspace&>(), "other_"_a)
      .def(nb::init<Scalar, Scalar, Scalar, Scalar>(), "a"_a, "b"_a, "c"_a,
           "d_"_a)
      .DEF_RW_CLASS_ATTRIB(Halfspace, n)
      .DEF_RW_CLASS_ATTRIB(Halfspace, d)
      .def("clone", &Halfspace::clone, nb::rv_policy::take_ownership)
      // .def(python::v2::PickleVisitor<Halfspace>())
      // .def(python::v2::SerializableVisitor<Halfspace>())
      // #if EIGENPY_VERSION_AT_LEAST(3, 8, 0)
      //       .def(eigenpy::IdVisitor<Halfspace>())
      // #endif
      ;

  nb::class_<Plane, ShapeBase>(m, "Plane")
      .def(nb::init<>())
      .def(nb::init<const Vec3s&, Scalar>(), "n_"_a, "d_"_a)
      .def(nb::init<const Plane&>(), "other_"_a)
      .def(nb::init<Scalar, Scalar, Scalar, Scalar>(), "a"_a, "b"_a, "c"_a,
           "d_"_a)
      .DEF_RW_CLASS_ATTRIB(Plane, n)
      .DEF_RW_CLASS_ATTRIB(Plane, d)
      .def("clone", &Plane::clone, nb::rv_policy::take_ownership)
      // .def(python::v2::PickleVisitor<Plane>())
      // .def(python::v2::SerializableVisitor<Plane>())
      // #if EIGENPY_VERSION_AT_LEAST(3, 8, 0)
      //       .def(eigenpy::IdVisitor<Plane>())
      // #endif
      ;

  nb::class_<Sphere, ShapeBase>(m, "Sphere")
      .def(nb::init<>())
      .def(nb::init<Scalar>(), "radius_"_a)
      .def(nb::init<const Sphere&>(), "other_"_a)
      .DEF_RW_CLASS_ATTRIB(Sphere, radius)
      .def("clone", &Sphere::clone, nb::rv_policy::take_ownership)
      // .def(python::v2::PickleVisitor<Sphere>())
      // .def(python::v2::SerializableVisitor<Sphere>())
      // #if EIGENPY_VERSION_AT_LEAST(3, 8, 0)
      //       .def(eigenpy::IdVisitor<Sphere>())
      // #endif
      ;

  nb::class_<Ellipsoid, ShapeBase>(m, "Ellipsoid")
      .def(nb::init<>())
      .def(nb::init<Scalar, Scalar, Scalar>(), "rx"_a, "ry"_a, "rz"_a)
      .def(nb::init<Vec3s>(), "radii"_a)
      .def(nb::init<const Ellipsoid&>(), "other_"_a)
      .DEF_RW_CLASS_ATTRIB(Ellipsoid, radii)
      .def("clone", &Ellipsoid::clone, nb::rv_policy::take_ownership)
      // .def(python::v2::PickleVisitor<Ellipsoid>())
      // .def(python::v2::SerializableVisitor<Ellipsoid>())
      // #if EIGENPY_VERSION_AT_LEAST(3, 8, 0)
      //       .def(eigenpy::IdVisitor<Ellipsoid>())
      // #endif
      ;

  nb::class_<TriangleP, ShapeBase>(m, "TriangleP")
      .def(nb::init<>())
      .def(nb::init<const Vec3s&, const Vec3s&, const Vec3s&>(), "a_"_a, "b_"_a,
           "c_"_a)
      .def(nb::init<const TriangleP&>(), "other_"_a)
      .DEF_RW_CLASS_ATTRIB(TriangleP, a)
      .DEF_RW_CLASS_ATTRIB(TriangleP, b)
      .DEF_RW_CLASS_ATTRIB(TriangleP, c)
      .def("clone", &TriangleP::clone, nb::rv_policy::take_ownership)
      // .def(python::v2::PickleVisitor<TriangleP>())
      // .def(python::v2::SerializableVisitor<TriangleP>())
      // #if EIGENPY_VERSION_AT_LEAST(3, 8, 0)
      //       .def(eigenpy::IdVisitor<TriangleP>())
      // #endif
      ;
}