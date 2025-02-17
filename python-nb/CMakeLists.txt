find_package(Python REQUIRED COMPONENTS Interpreter Development)
find_package(nanobind CONFIG REQUIRED)

set(PYTHON_LIB_NAME_V2 coal_pywrap_v2)
set(coal_pywrap_v2_SOURCES coal.cc)

nanobind_add_module(${PYTHON_LIB_NAME_V2} NB_STATIC LTO NB_SUPPRESS_WARNINGS ${coal_pywrap_v2_SOURCES})
target_link_libraries(
  ${PYTHON_LIB_NAME_V2}
  PRIVATE ${PROJECT_NAME} Boost::system
)
target_compile_definitions(
  ${PYTHON_LIB_NAME_V2}
  PRIVATE COAL_PYTHON_LIBNAME=${PYTHON_LIB_NAME_V2}
)

set(MODULE_DIR ${PROJECT_NAME}/v2)
string(REPLACE "/" "." MODULE_NAME ${MODULE_DIR})

set_target_properties(
  ${PYTHON_LIB_NAME_V2}
  PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${MODULE_DIR}"
)

set(PYTHON_FILES __init__.py v2/__init__.py)

foreach(pyfile ${PYTHON_FILES})
  PYTHON_BUILD(${PROJECT_NAME} ${pyfile})
endforeach(pyfile)

nanobind_add_stub(
  coal_pywrap_v2_stub
  MODULE ${MODULE_NAME}
  OUTPUT ${MODULE_NAME}.pyi
  DEPENDS ${PYTHON_LIB_NAME_V2}
)
