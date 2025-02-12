find_package(Python REQUIRED COMPONENTS Interpreter Development)
find_package(nanobind CONFIG REQUIRED)

set(PYTHON_LIB_NAME_V2 coal_pywrap_v2)
set(coal_pywrap_v2_SOURCES coal.cc)

nanobind_add_module(${PYTHON_LIB_NAME_V2} NB_STATIC NB_SUPPRESS_WARNINGS ${coal_pywrap_v2_SOURCES})
target_link_libraries(
  ${PYTHON_LIB_NAME_V2}
  PRIVATE ${PROJECT_NAME} Boost::system
)
target_compile_definitions(
  ${PYTHON_LIB_NAME_V2}
  PRIVATE COAL_PYTHON_LIBNAME=${PYTHON_LIB_NAME_V2}
)
