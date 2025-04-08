# Copyright 2025 INRIA

if(CMAKE_VERSION VERSION_LESS 3.18)
  set(DEV_MODULE Development)
else()
  set(DEV_MODULE Development.Module)
endif()

# Python
find_package(Python COMPONENTS Interpreter ${DEV_MODULE} REQUIRED)

# Detect the installed nanobind package and import it into CMake
execute_process(
  COMMAND "${Python_EXECUTABLE}" -m nanobind --cmake_dir
  OUTPUT_STRIP_TRAILING_WHITESPACE
  OUTPUT_VARIABLE nanobind_ROOT
)
# Honor nanobind_ROOT
cmake_policy(PUSH)
cmake_policy(SET CMP0074 NEW)
find_package(nanobind CONFIG REQUIRED)
cmake_policy(POP)
find_package(Boost REQUIRED COMPONENTS system)

# Nanoeigenpy
find_package(nanoeigenpy CONFIG REQUIRED)

set(PYTHON_LIB_NAME_V2 ${PROJECT_NAME}_pywrap_nb)

# Headers
set(
  PYTHON_LIB_HEADERS
  broadphase/broadphase_callbacks_collision_manager.hh
  fwd.h
  pickle.hh
  serializable.hh
)

# Sources
set(
  PYTHON_LIB_SOURCES
  broadphase/broadphase.cc
  bvh.cc
  collision-geometries.cc
  collision.cc
  contact_patch.cc
  distance.cc
  math.cc
  gjk.cc
  height_field.cc
  memory-footprint.cc
  shapes.cc
  module.cc
)
if(COAL_HAS_OCTOMAP)
  list(APPEND PYTHON_LIB_SOURCES octree.cc)
endif(COAL_HAS_OCTOMAP)

nanobind_add_module(${PYTHON_LIB_NAME_V2} NB_STATIC LTO NB_SUPPRESS_WARNINGS ${PYTHON_LIB_SOURCES})
target_link_libraries(
  ${PYTHON_LIB_NAME_V2}
  PRIVATE ${PROJECT_NAME} Boost::system
)
target_compile_definitions(
  ${PYTHON_LIB_NAME_V2}
  PRIVATE COAL_PYTHON_LIBNAME=${PYTHON_LIB_NAME_V2}
)

set(MODULE_DIR ${PROJECT_NAME})

set_target_properties(
  ${PYTHON_LIB_NAME_V2}
  PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${MODULE_DIR}"
)

set(PYTHON_FILES ${MODULE_DIR}/__init__.py)
set(PYTHON_LIB_STUB_FILE ${MODULE_DIR}/${PYTHON_LIB_NAME_V2}.pyi)

nanobind_add_stub(
  ${PYTHON_LIB_NAME_V2}_stub
  VERBOSE
  MODULE ${PYTHON_LIB_NAME_V2}
  OUTPUT ${PYTHON_LIB_STUB_FILE}
  DEPENDS ${PYTHON_LIB_NAME_V2}
  PYTHON_PATH ${CMAKE_CURRENT_BINARY_DIR}/${MODULE_DIR}
)
add_dependencies(${PROJECT_NAME}_python ${PYTHON_LIB_NAME_V2})
ADD_SOURCE_GROUP(PYTHON_LIB_SOURCES)

foreach(pyfile ${PYTHON_FILES})
  PYTHON_BUILD(${PROJECT_NAME} ${pyfile})
  install(FILES ${pyfile} DESTINATION ${Python_SITELIB}/${MODULE_DIR})
endforeach(pyfile)

install(
  TARGETS ${PYTHON_LIB_NAME_V2}
  EXPORT ${TARGETS_EXPORT_NAME}
  LIBRARY DESTINATION ${Python_SITELIB}/${MODULE_DIR}
)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/${PYTHON_LIB_STUB_FILE}
  DESTINATION ${Python_SITELIB}/${MODULE_DIR}
)
