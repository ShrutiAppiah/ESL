CMAKE_MINIMUM_REQUIRED(VERSION 3.12)

################################################################################
# Read and set the library version
################################################################################
FILE(READ "esl/version" version_header)

STRING(REGEX MATCH "[0-9a-zA-Z_]*ESL_VERSION_MAJOR[ /t]*=[ /t]*([0-9]+)"
        _ ${version_header})
SET(version_major ${CMAKE_MATCH_1})

STRING(REGEX MATCH "[0-9a-zA-Z_]*ESL_VERSION_MINOR[ /t]*=[ /t]*([0-9]+)"
        _ ${version_header})
SET(version_minor ${CMAKE_MATCH_1})

STRING(REGEX MATCH "[0-9a-zA-Z_]*ESL_VERSION_REVISION[ /t]*=[ /t]*([0-9]+)"
        _ ${version_header})
SET(version_revision ${CMAKE_MATCH_1})

# Enable new behaviour on this policy so we can set the version automatically.
# enables compatibility with older versions of cmake
CMAKE_POLICY(SET CMP0048 NEW)

IF(APPLE)
    # old version 10.9 for compatibility
    SET(CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE STRING "Minimum OS X deployment version")
    SET(CMAKE_MACOSX_RPATH 0)
    # If environment settings override this, you can enable force:
    # ADD_COMPILE_OPTIONS(-mmacosx-version-min=10.9)
ENDIF()

PROJECT(ESL VERSION ${version_major}.${version_minor}.${version_revision}
        DESCRIPTION "Economic Simulation Library"
        LANGUAGES CXX)

MESSAGE("ESL Version ${PROJECT_VERSION}")
MESSAGE("Building binary in ${CMAKE_BINARY_DIR}")

################################################################################
#   Project Configuration
################################################################################
SET(CMAKE_MODULE_PATH              "${CMAKE_SOURCE_DIR}/cmake")

SET(CMAKE_CXX_STANDARD              17)
SET(CMAKE_CXX_STANDARD_REQUIRED     ON)
SET(CMAKE_CXX_EXTENSIONS            OFF)

# build as a shared library, as opposed to the Python bindings or a single experiment only
OPTION(CONFIGURATION_SHARED "Build shared library" OFF  )

OPTION(WITH_TESTS "Build test cases" ON)

IF(NOT ESL_TARGET_NAME)
    SET(ESL_TARGET_NAME "esl")
ENDIF()

IF(NOT CMAKE_BUILD_TYPE)
    MESSAGE("CMAKE_BUILD_TYPE not set, defaulting to Release build.")
    SET(CMAKE_BUILD_TYPE    Release)
ENDIF()

IF(MSVC OR MINGW)
    # this prevents windows from defining IN and OUT macros globally
    ADD_DEFINITIONS(-D_NO_W32_PSEUDO_MODIFIERS)
ENDIF()

IF(APPLE)
    # ignore warnings about unavailable functions in standard library on older MacOS
    ADD_DEFINITIONS(-D_LIBCPP_DISABLE_AVAILABILITY)
ENDIF()

################################################################################
# Compile time improvements
################################################################################

FIND_PROGRAM(CCACHE_FOUND ccache)
IF(CCACHE_FOUND)
    SET_PROPERTY(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    SET_PROPERTY(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
ENDIF(CCACHE_FOUND)

################################################################################
# Find automatic differentiation library
################################################################################

# it is not optional thus it can fail the build if not found
FIND_PATH(ADEPT_INCLUDE_DIRECTORY "adept.h")

################################################################################
# Library optional add-ons
################################################################################
OPTION(WITH_MPI "Enable MPI using Boost.MPI" OFF)
IF(WITH_MPI)
    FIND_PACKAGE(MPI REQUIRED)      # looks for MPI implementation
    ADD_DEFINITIONS(-DWITH_MPI)     # enables ESL WITH_MPI code
ENDIF()

OPTION(WITH_PYTHON "Enables export of Python bindings using Boost.Python" ON)
IF(WITH_PYTHON)
    ADD_DEFINITIONS(-DWITH_PYTHON)  # enables ESL WITH_PYTHON code

    IF(SKBUILD)
        # No static linking to python on manylinux, macos

        if(MSVC)
            ADD_DEFINITIONS(-DBoost_USE_STATIC_LIBS)
            ADD_DEFINITIONS(-DBOOST_PYTHON_STATIC_LIB)
        ENDIF()
    ELSE()
        # TODO: confirm static linking is necessary if we are not creating a python package
        ADD_DEFINITIONS(-DBOOST_PYTHON_STATIC_LIB)
    ENDIF()
ENDIF()

OPTION(WITH_QL  "Enables QuantLib" OFF)
IF(WITH_QL)
    ADD_DEFINITIONS(-DWITH_QL)
ENDIF()

################################################################################
# Python bindings configuration
################################################################################
IF(WITH_PYTHON)
    FIND_PACKAGE(PythonInterp 3)            # for running tests on build machine, not required
    FIND_PACKAGE(PythonLibs 3 REQUIRED)     # for building python bindings

    IF(SKBUILD)
        MESSAGE(STATUS "The project is built using scikit-build")
    ELSE()
        MESSAGE(STATUS "The project is not built using scikit-build")
    ENDIF()

    GET_FILENAME_COMPONENT(ESL_PYTHON_LINK_DIRECTORY ${PYTHON_LIBRARY} DIRECTORY)

    # This reads the Python version that was found, and sets variables
    STRING(REPLACE "." ";" VERSION_LIST ${PYTHONLIBS_VERSION_STRING})
    LIST(GET VERSION_LIST 0 ESL_PYTHON_BINDINGS_VERSION_MAJOR)
    LIST(GET VERSION_LIST 1 ESL_PYTHON_BINDINGS_VERSION_MINOR)
    LIST(GET VERSION_LIST 2 ESL_PYTHON_BINDINGS_VERSION_PATCH)
ENDIF()

# TODO:
#INCLUDE_DIRECTORIES(SYSTEM ${PYTHON_INCLUDE_DIRS})

IF(NOT MSVC)
    # Gets the GNU standard insllation directories on this machine
    INCLUDE(GNUInstallDirs)
ENDIF()
################################################################################
# In this section we set up the Boost libraries,
# and inter-dependencies between Boost MPI and Python
################################################################################
LIST(APPEND Boost_LIBRARIES_DEPENDENCIES
        program_options
        date_time
        serialization
        unit_test_framework
        thread
        ) # NOTE: don't forget to match these to the build and deployment requirements

# If Python is enabled, we add Boost.Python of the appropriate version
IF(WITH_PYTHON)
    MESSAGE("################################################################################")
    MESSAGE("Compiling with Python ${ESL_PYTHON_BINDINGS_VERSION_MAJOR}.${ESL_PYTHON_BINDINGS_VERSION_MINOR}:")

    # TODO: confirm that this is actually how its meant to be; it appears that for Python 3.8
    #       windows has `python38` as a Boost dependency, and other platforms just have 'python'
    IF(MSVC) #ESL_PYTHON_BINDINGS_VERSION_MINOR LESS 8)
        LIST(APPEND Boost_LIBRARIES_DEPENDENCIES
                python${ESL_PYTHON_BINDINGS_VERSION_MAJOR}${ESL_PYTHON_BINDINGS_VERSION_MINOR})
    ELSE()
        LIST(APPEND Boost_LIBRARIES_DEPENDENCIES python)
    ENDIF()

    # On non-release builds, inform the users about the chosen Python and its paths
    IF(NOT CMAKE_BUILD_TYPE EQUAL "Release")
        MESSAGE("include: /t" ${PYTHON_INCLUDE_DIRS})
        MESSAGE("library: /t" ${PYTHON_LIBRARY})
        MESSAGE("libraries: /t" ${PYTHON_LIBRARIES})
    ENDIF()
ENDIF()

# If using the Message Passing Interface, enable MPI
IF(WITH_MPI)
    LIST(APPEND Boost_LIBRARIES_DEPENDENCIES mpi)

    # We can expose the python bindings for MPI as well, to permit direct access to MPI
    IF(WITH_PYTHON)
        LIST(APPEND Boost_LIBRARIES_DEPENDENCIES mpi_python)
    ENDIF()
ENDIF()

################################################################################
#   Collect native code source files
################################################################################
file(GLOB_RECURSE SOURCE_FILES "esl/**.cpp" "esl/**.hpp")

################################################################################
#
################################################################################
LIST(APPEND ALL_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR} SYSTEM ${ADEPT_INCLUDE_DIRECTORY})

IF(NOT MSVC)
    # TODO: test if we can do without, would be faster
    SET(CMAKE_POSITION_INDEPENDENT_CODE ON)
ENDIF()

SET(Boost_USE_MULTITHREADED         ON)

# 1.53 is the lowest version we tested in 2020
FIND_PACKAGE(Boost 1.53 COMPONENTS ${Boost_LIBRARIES_DEPENDENCIES} REQUIRED)
LIST(APPEND ALL_INCLUDE_DIRECTORIES ${Boost_INCLUDE_DIRS})

########################################################################################################################
# Optimized linear algebra and scientific computing routines
MESSAGE("################################################################################")

IF(MSVC)
    SET(BLA_STATIC ON)      # On windows, we will statically link a free blas package
ENDIF()

FIND_PACKAGE(BLAS)          # not required, can do with a fallback library
IF(BLAS_FOUND)
    FIND_PACKAGE(LAPACK)    # also not required but nice to have
ENDIF()

# GSL is required for optimisation routines
# 1.15 is the lowest working version we found to work (CentOS 7)

FIND_PACKAGE(GSL 1.15 REQUIRED)

################################################################################
MESSAGE("----------------------------------------")
MESSAGE("profile  " ${CMAKE_BUILD_TYPE})
MESSAGE("compiler " ${CMAKE_CXX_COMPILER_ID} " " ${CMAKE_CXX_COMPILER_VERSION} " (" ${CMAKE_CXX_COMPILER} ")")
MESSAGE("----------------------------------------")

IF(MSVC)
    # TODO: test if this works
    #TARGET_COMPILE_OPTIONS(${ESL_TARGET_NAME} PRIVATE "/MP")

    # multi-processor compilation on MSVC
    ADD_DEFINITIONS(/MP)
ELSE()
    SET(CMAKE_CXX_FLAGS "-Wall -Wextra")

    SET(CMAKE_C_FLAGS_DEBUG "-g")#   --coverage -fprofile-arcs -ftest-coverage") #  -fanalyzer
    SET(CMAKE_CXX_FLAGS_DEBUG "-g")#    --coverage -fprofile-arcs -ftest-coverage")

    SET(CMAKE_C_FLAGS_RELEASE "-O3")
    SET(CMAKE_CXX_FLAGS_RELEASE "-O3")

    IF(CMAKE_BUILD_TYPE MATCHES RELEASE)
        INCLUDE(CheckCXXCompilerFlag)
        CHECK_CXX_COMPILER_FLAG("-march=native" COMPILER_SUPPORTS_MARCH_NATIVE)
        ADD_DEFINITIONS(-DESL_RELEASE)
        IF(COMPILER_SUPPORTS_MARCH_NATIVE)
            SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
        ENDIF()
    ELSE()

    ENDIF(CMAKE_BUILD_TYPE MATCHES RELEASE)
ENDIF()


ADD_DEFINITIONS(-DADEPT_RECORDING_PAUSABLE)
ADD_DEFINITIONS(-DADEPT_STORAGE_THREAD_SAFE)
# Used to benchmark improvements using automatic differentiation
#ADD_DEFINITIONS(-DADEPT_NO_AUTOMATIC_DIFFERENTIATION)

################################################################################
# Enables ADD_TEST
################################################################################

ENABLE_TESTING()

################################################################################
# Add include directories
################################################################################

IF(WITH_MPI)
    list(APPEND ALL_INCLUDE_DIRECTORIES ${MPI_INCLUDE_PATH})
ENDIF()

IF(WITH_PYTHON)
    list(APPEND ALL_INCLUDE_DIRECTORIES ${PYTHON_INCLUDE_DIRS})
ENDIF()

INCLUDE_DIRECTORIES(SYSTEM ${ALL_INCLUDE_DIRECTORIES})
################################################################################
# Include directories
################################################################################

IF(MSVC)
    LIST(APPEND ALL_LINKED_LIBRARIES GSL::gsl)
ELSEIF(MINGW)
    # TODO: check whether we actually have blas, even though most distributions have it
    LIST(APPEND ALL_LINKED_LIBRARIES -static pthread -lgsl -lgslcblas -lm )
ELSE()
    LIST(APPEND ALL_LINKED_LIBRARIES pthread GSL::gsl)
ENDIF()

IF(MSVC OR MINGW)
    LIST(APPEND ALL_LINKED_LIBRARIES)
ELSE()
    LIST(APPEND ALL_LINKED_LIBRARIES dl util)
ENDIF()

IF(WITH_PYTHON)

    # pep-0513/#libpythonx-y-so-1
    IF(SKBUILD AND NOT APPLE)
        # No linking to python
    ELSE()
        LIST(APPEND ALL_LINKED_LIBRARIES ${PYTHON_LIBRARIES})
    ENDIF()
ENDIF()

IF(WITH_MPI)
    LIST(APPEND ALL_LINKED_LIBRARIES ${MPI_CXX_LIBRARIES} )
ENDIF()

IF(WITH_QL)
    LIST(APPEND ALL_LINKED_LIBRARIES QuantLib ${QuantLib_LIBRARIES})
ENDIF()

IF(BLAS_FOUND)
    LIST(APPEND ALL_LINKED_LIBRARIES ${BLAS_LIBRARIES} GSL::gslcblas)
    IF(LAPACK_FOUND)
        LIST(APPEND ALL_LINKED_LIBRARIES ${LAPACK_LIBRARIES})
    ENDIF()
ENDIF()

IF(MSVC)
#    MESSAGE("LINKING WITH")
#    MESSAGE(${ALL_LINKED_LIBRARIES})
    LINK_LIBRARIES(${ALL_LINKED_LIBRARIES})
ELSEIF(MINGW)
    LINK_LIBRARIES(${ALL_LINKED_LIBRARIES} -static -static-libgcc -static-libstdc++)
ELSE()
    LINK_LIBRARIES(${ALL_LINKED_LIBRARIES} dl)
ENDIF()

################################################################################
# This needs to be at this position, because for MSVC we need to issue a LINK_DIRECTORIES
# call before this to get it right

IF(CONFIGURATION_SHARED)
    IF(MSVC)
        # ADD_LIBRARY(${ESL_TARGET_NAME} STATIC ${SOURCE_FILES} ) # 11-14 15:40

        ADD_LIBRARY(${ESL_TARGET_NAME} SHARED ${SOURCE_FILES} )
    ELSE()
        ADD_LIBRARY(${ESL_TARGET_NAME} SHARED ${SOURCE_FILES} )
    ENDIF()

    SET_TARGET_PROPERTIES(${ESL_TARGET_NAME} PROPERTIES PREFIX "lib")

    IF(MSVC)
        # multi-processor compilation
        TARGET_COMPILE_OPTIONS(${ESL_TARGET_NAME} PRIVATE "/MP")
    ENDIF()

    IF(CMAKE_BUILD_TYPE MATCHES Debug)
        # TARGET_COMPILE_DEFINITIONS(${ESL_TARGET_NAME} PUBLIC ESL_RELEASE=0)
    elseif(CMAKE_BUILD_TYPE MATCHES Release)
        TARGET_COMPILE_DEFINITIONS(${ESL_TARGET_NAME} PUBLIC ESL_RELEASE=1)
    endif()

    IF(MSVC)
        # this one is for the shared library only. For python, this needs to be called again separately

        LINK_DIRECTORIES(${ESL_PYTHON_LINK_DIRECTORY} ${PYTHON_LIBRARY} ${Boost_LIBRARY_DIRS} ${GSL_LIBRARY} )
        TARGET_LINK_LIBRARIES(${ESL_TARGET_NAME} ${Boost_LIBRARIES} ${ALL_LINKED_LIBRARIES} )
    ELSEIF(MINGW)
        LINK_DIRECTORIES(${ESL_PYTHON_LINK_DIRECTORY} ${Boost_LIBRARY_DIRS} ${GSL_LIBRARY} )
        TARGET_LINK_LIBRARIES(${ESL_TARGET_NAME} ${Boost_LIBRARIES} ${ALL_LINKED_LIBRARIES} -static -static-libgcc -static-libstdc++)
    ELSE()
        TARGET_LINK_LIBRARIES(${ESL_TARGET_NAME} ${Boost_LIBRARIES} ${ALL_LINKED_LIBRARIES} dl)
    ENDIF()


    ################################################################################
    # Copy library into testing folder, so it can be linked against by Python tests
    ################################################################################
    #ADD_CUSTOM_COMMAND(TARGET ${ESL_TARGET_NAME} POST_BUILD
    #        COMMAND ${CMAKE_COMMAND} -E copy_if_different
    #        $<TARGET_FILE:${ESL_TARGET_NAME}>
    #        ${CMAKE_BINARY_DIR}/test/esl/$<TARGET_FILE_NAME:${ESL_TARGET_NAME}>)
ENDIF()

################################################################################
#   Add tests
################################################################################
IF(WITH_TESTS AND CONFIGURATION_SHARED)
    FILE(GLOB TEST_SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} test/test_*.cpp)
    FOREACH(test_src ${TEST_SRCS})
        GET_FILENAME_COMPONENT(test_name ${test_src} NAME_WE)

        ADD_EXECUTABLE(${test_name} ${test_src} ${SOURCE_FILES})
        TARGET_LINK_LIBRARIES(${test_name} ${Boost_LIBRARIES} ${ESL_TARGET_NAME})
        SET_TARGET_PROPERTIES(${test_name} PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY  ${CMAKE_BINARY_DIR}/test/)

        # test_mpi_* and test_mpi_python_* need to be run using a MPI job runner
        IF(${test_name} MATCHES "^(test_mpi_)")
            IF(WITH_MPI)
                IF(WITH_PYTHON AND ${test_name} MATCHES "^(test_mpi_python_)")
                    MESSAGE("\t MPI PYTHON TEST" ${test_name})
                    ADD_TEST(NAME ${test_name}
                            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/python/test/
                            COMMAND ${CMAKE_BINARY_DIR}/python/test/${test_name})
                ELSE()
                    MESSAGE("\t MPI TEST" ${test_name})
                    ADD_TEST(NAME ${test_name}
                            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/test/
                            COMMAND mpirun -np 2 ${CMAKE_BINARY_DIR}/test/${test_name})
                ENDIF()
            ENDIF()
        ELSEIF(${test_name} MATCHES "^(test_python_)")
            MESSAGE("\t PYTHON TEST" ${test_name})
            ADD_TEST(NAME ${test_name}
                    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/python/test/
                    COMMAND ${CMAKE_BINARY_DIR}/python/test/${test_name})
        ELSE()
            MESSAGE("\t C++ TEST " ${test_name})
            ADD_TEST(NAME ${test_name}
                    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/test/
                    COMMAND ${CMAKE_BINARY_DIR}/test/${test_name})
        ENDIF()

    ENDFOREACH(test_src)
ENDIF()


IF(WITH_PYTHON)
    ####################################################################################################################
    # create python modules
    # the requirement a file to be parsed as a python module is that it is a uniquely named .cpp file
    # with python_module as prefix of the filename
    FILE(GLOB_RECURSE ESL_PYTHON_SOURCE RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} esl/**python_module**.cpp)
    FOREACH (source ${ESL_PYTHON_SOURCE})
        GET_FILENAME_COMPONENT(module_name ${source} NAME_WE)
        GET_FILENAME_COMPONENT(module_path ${source} DIRECTORY)

        GET_FILENAME_COMPONENT(python_module_name ${module_path} NAME)
        LINK_DIRECTORIES(${ESL_PYTHON_LINK_DIRECTORY} ${PYTHON_LIBRARY} ${Boost_LIBRARY_DIRS} ${GSL_LIBRARY} )
        LINK_LIBRARIES(${Boost_LIBRARIES} ${ALL_LINKED_LIBRARIES})

        PYTHON_ADD_MODULE(_${python_module_name} ${source} ${SOURCE_FILES})

        IF(MSVC)
            # multi-processor compilation
            #TARGET_COMPILE_OPTIONS(_${python_module_name} PRIVATE "/MP")
            #SET(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
            #SET_PROPERTY(TARGET _${python_module_name} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
        ENDIF()

        # We have a special case for the main module residing in the root
        IF(python_module_name STREQUAL "esl")
            SET(module_path_modified ".")
        ELSE()
            # This replaces `esl/path1/path2/submodule` with `path1/path2/submodule
            STRING(SUBSTRING  ${module_path} 4 -1 module_path_modified)
        ENDIF()

        # Let user know we have detected the python module and processed it
        MESSAGE("PYTHON MODULE " _${python_module_name} ":/t/t " ${module_path_modified} "/t/t" ${source})
        # We install it in the right submodule path
        INSTALL(TARGETS _${python_module_name} DESTINATION ${module_path_modified})

        SET(ignoreMe "${SKBUILD}")
    ENDFOREACH ()

    ####################################################################################################################
    # create python tests
    FILE(GLOB TEST_SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} test/test_*.py)
    FOREACH(test_src ${TEST_SRCS})
        GET_FILENAME_COMPONENT(pytest_name ${test_src} NAME_WE)
        SET(pytest_name "${pytest_name}_python")
        MESSAGE("PYTEST " ${pytest_name})
        FILE(COPY ${test_src} DESTINATION ${CMAKE_BINARY_DIR}/python/test/)
    ENDFOREACH(test_src)

    ADD_CUSTOM_TARGET(pytest ALL COMMENT "pytest")

    #SET_TARGET_PROPERTIES(pytest PROPERTIES
    #    RUNTIME_OUTPUT_DIRECTORY  ${CMAKE_BINARY_DIR}/python/test/)

    #IF(PYTHONINTERP_FOUND)
    #    ADD_TEST(NAME pytest
    #            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/python/test/
    #            COMMAND ${PYTHON_EXECUTABLE} -m pytest -r a -v ${CMAKE_BINARY_DIR}/python/test/)
    #ENDIF()
ENDIF()