find_package(Python REQUIRED COMPONENTS Interpreter Development)
find_package(nanobind CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)

set(PYTHON_LIB_NAME_V2 coal_pywrap_nb)
set(PYTHON_LIB_SOURCES bvh.cc collision-geometries.cc math.cc module.cc)

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
string(REPLACE "/" "." MODULE_NAME ${MODULE_DIR})

set_target_properties(
  ${PYTHON_LIB_NAME_V2}
  PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${MODULE_DIR}"
)

set(PYTHON_FILES __init__.py)

foreach(pyfile ${PYTHON_FILES})
  PYTHON_BUILD(${PROJECT_NAME} ${pyfile})
endforeach(pyfile)

nanobind_add_stub(
  ${PYTHON_LIB_NAME_V2}_stub
  MODULE ${MODULE_NAME}.${PYTHON_LIB_NAME_V2}
  OUTPUT ${MODULE_DIR}/__init__.pyi
  DEPENDS ${PYTHON_LIB_NAME_V2}
)
