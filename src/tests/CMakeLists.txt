project(XrayTests LANGUAGES C CXX)

cpmaddpackage(
    NAME
    boost-ut
    GITHUB_REPOSITORY
    boost-ext/ut
    GIT_TAG
    v2.3.0
    OPTIONS
    "BOOST_UT_DISABLE_MODULE ON")

# cpmaddpackage("gh:boost-ext/ut@2.3.0")

set(XRAY_MATH_TEST_SOURCES
    ${PROJECT_SOURCE_DIR}/xray.math.tests/bounding.box.test.cc)

# add the executable
add_executable(MathTests ${XRAY_MATH_TEST_SOURCES})
target_link_libraries(MathTests PRIVATE xray-base xray-math
                                        xray-set-compile-options-lib Boost::ut)
add_test(NAME MathTests COMMAND MathTests)
