if(UNIX)
  find_program(POD2MAN_EXECUTABLE pod2man)
  set_package_properties(pod2man PROPERTIES
    TYPE RECOMMENDED
    DESCRIPTION "Man page generator"
    PURPOSE "Generate clazy man pages."
  )

  if(POD2MAN_EXECUTABLE)
    add_custom_command(
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/clazy.1
      COMMAND ${POD2MAN_EXECUTABLE} -c "KDAB" -r "\"${CLAZY_VERSION}\"" -s 1 ${CMAKE_CURRENT_SOURCE_DIR}/clazy.pod ${CMAKE_CURRENT_BINARY_DIR}/clazy.1
      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/clazy.pod
    )
    add_custom_target(man ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/clazy.1)

    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/clazy.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
  endif()
endif()
