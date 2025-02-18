#include <nanobind/nanobind.h>

namespace nb = nanobind;

#define DEF_CLASS_FUNC(cls_name, name) def(#name, &cls_name::name)
