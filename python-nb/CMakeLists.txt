if(CMAKE_VERSION VERSION_LESS 3.18)
  set(DEV_MODULE Development)
else()
  set(DEV_MODULE Development.Module)
endif()

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

set(PYTHON_LIB_NAME_V2 ${PROJECT_NAME}_pywrap_nb)
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

set_target_properties(
  ${PYTHON_LIB_NAME_V2}
  PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${MODULE_DIR}"
)

set(PYTHON_FILES __init__.py)

foreach(pyfile ${PYTHON_FILES})
  PYTHON_BUILD(${PROJECT_NAME} ${pyfile})
  install(
    FILES ${PROJECT_NAME}/${pyfile}
    DESTINATION ${Python_SITELIB}/${PROJECT_NAME}
  )
endforeach(pyfile)

nanobind_add_stub(
  ${PYTHON_LIB_NAME_V2}_stub
  MODULE ${PROJECT_NAME}.${PYTHON_LIB_NAME_V2}
  OUTPUT ${MODULE_DIR}/__init__.pyi
  DEPENDS ${PYTHON_LIB_NAME_V2}
)
add_dependencies(${PROJECT_NAME}_python ${PYTHON_LIB_NAME_V2})
ADD_SOURCE_GROUP(PYTHON_LIB_SOURCES)
