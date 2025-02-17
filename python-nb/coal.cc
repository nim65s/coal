/// Copyright 2025 INRIA
#include <nanobind/nanobind.h>
#include "coal/config.hh"

namespace nb = nanobind;
using namespace nb::literals;

inline constexpr bool checkVersionAtLeast(int major, int minor, int patch) {
  return COAL_VERSION_AT_LEAST(major, minor, patch);
}

inline constexpr bool checkVersionAtMost(int major, int minor, int patch) {
  return COAL_VERSION_AT_MOST(major, minor, patch);
}

void exposeVersion(nb::module_ &m) {
  m.attr("__version__") = COAL_VERSION;
  m.attr("COAL_MAJOR_VERSION") = COAL_MAJOR_VERSION;
  m.attr("COAL_MINOR_VERSION") = COAL_MINOR_VERSION;
  m.attr("COAL_PATCH_VERSION") = COAL_PATCH_VERSION;

  m.attr("WITH_QHULL") =
#if COAL_HAS_QHULL
      true;
#else
      false;
#endif

  m.attr("WITH_OCTOMAP") =
#if COAL_HAS_OCTOMAP
      true;
#else
      false;
#endif

  m.def("checkVersionAtLeast", &checkVersionAtLeast, "major"_a, "minor"_a,
        "patch"_a,
        "Checks if the current version of coal is at least the version "
        "provided by the input arguments.");

  m.def("checkVersionAtMost", &checkVersionAtMost, "major"_a, "minor"_a,
        "patch"_a,
        "Checks if the current version of coal is at most the version provided "
        "by the input arguments.");
}

NB_MODULE(COAL_PYTHON_LIBNAME, m) { exposeVersion(m); }
