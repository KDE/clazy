include(${CMAKE_CURRENT_LIST_DIR}/ClazySources.cmake)
include_directories(${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR}/src)

# dummy so it compiles
file(WRITE ${CMAKE_CURRENT_LIST_DIR}/clazylib_export.h "#define CLAZYLIB_EXPORT")

# regexp works fine on msvc2015
set(CLAZY_SRCS ${CLAZY_SRCS}  ${CMAKE_CURRENT_LIST_DIR}/src/checks/level2/oldstyleconnect.cpp)
