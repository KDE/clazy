set(CLAZY_LIB_SRC
  ${CMAKE_CURRENT_LIST_DIR}/src/AccessSpecifierManager.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/checkbase.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/checkmanager.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/SuppressionManager.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/ContextUtils.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/FixItUtils.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/FixItExporter.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/LoopUtils.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/PreProcessorVisitor.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/QtUtils.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/StringUtils.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/TemplateUtils.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/TypeUtils.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/Utils.cpp
)

set(CLAZY_CHECKS_SRCS
  ${CMAKE_CURRENT_LIST_DIR}/src/checks/detachingbase.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/checks/inefficientqlistbase.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/checks/ruleofbase.cpp
)
include(CheckSources.cmake)

set(CLAZY_SHARED_SRCS # sources shared between clazy-standalone and clazy plugin
  ${CLAZY_CHECKS_SRCS}
  ${CMAKE_CURRENT_LIST_DIR}/src/ClazyContext.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/Clazy.cpp
  ${CLAZY_LIB_SRC}
)

set(CLAZY_PLUGIN_SRCS # Sources for the plugin
  ${CLAZY_SHARED_SRCS}
)

if (MSVC)
  set(CLAZY_STANDALONE_SRCS
    ${CLAZY_SHARED_SRCS}
    ${CMAKE_CURRENT_LIST_DIR}/src/ClazyStandaloneMain.cpp
  )
else()
  set(CLAZY_STANDALONE_SRCS
    ${CMAKE_CURRENT_LIST_DIR}/src/ClazyStandaloneMain.cpp
  )
endif()
