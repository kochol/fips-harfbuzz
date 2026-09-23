# Integration check using either plain CMake or the documented fips macros.
cmake_minimum_required(VERSION 3.21)
project(fips_text_smoke LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
get_filename_component(repositories "../../.." ABSOLUTE)

option(FIPS_TEXT_USE_FIPS "Verify fips compiler settings and target macros" OFF)
if(FIPS_TEXT_USE_FIPS)
    set(FIPS_ROOT_DIR "${repositories}/fips" CACHE PATH "Path to fips")
    include("${FIPS_ROOT_DIR}/cmake/fips.cmake")
    fips_setup()
endif()

add_subdirectory("${repositories}/fips-freetype/fips-files/freetype" freetype)
add_subdirectory("../harfbuzz" harfbuzz)
add_subdirectory("${repositories}/fips-icu/fips-files/icu" icu)
if(FIPS_TEXT_USE_FIPS)
    fips_begin_app(text_smoke cmdline)
        fips_files(main.cpp)
        fips_deps(harfbuzz freetype icu)
    fips_end_app()
else()
    add_executable(text_smoke main.cpp)
    target_link_libraries(text_smoke PRIVATE harfbuzz freetype icu)
endif()

enable_testing()
add_test(NAME text_smoke COMMAND text_smoke
    "${CMAKE_CURRENT_SOURCE_DIR}/../../test/subset/data/fonts/Amiri-Regular.ttf")
