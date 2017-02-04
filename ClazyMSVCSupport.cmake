include(${CMAKE_CURRENT_LIST_DIR}/ClazySources.cmake)
include_directories(${CMAKE_CURRENT_LIST_DIR})

# dummy so it compiles
file(WRITE ${CMAKE_CURRENT_LIST_DIR}/clazylib_export.h "#define CLAZYLIB_EXPORT")
