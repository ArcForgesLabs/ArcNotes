if(NOT WIN32)
  return()
endif()

if(NOT NOTES_DEPLOY_QT_PLATFORMS)
  return()
endif()

set(_notes_qt_platform_candidates
    "${NOTES_QT_BIN_DIR}/../Qt6/plugins/platforms"
    "${NOTES_QT_BIN_DIR}/../plugins/platforms")
set(_notes_qt_qml_candidates
    "${NOTES_QT_BIN_DIR}/../Qt6/qml"
    "${NOTES_QT_BIN_DIR}/../qml")
set(_notes_qt_runtime_bin_candidates
    "${NOTES_QT_BIN_DIR}"
    "${NOTES_QT_BIN_DIR}/../bin")

set(NOTES_QT_PLATFORMS_SOURCE "")
foreach(_notes_candidate IN LISTS _notes_qt_platform_candidates)
  cmake_path(NORMAL_PATH _notes_candidate OUTPUT_VARIABLE _notes_candidate)
  if(EXISTS "${_notes_candidate}")
    set(NOTES_QT_PLATFORMS_SOURCE "${_notes_candidate}")
    break()
  endif()
endforeach()

if(NOT NOTES_QT_PLATFORMS_SOURCE)
  message(WARNING "Qt platforms directory was not found from ${NOTES_QT_BIN_DIR}")
  return()
endif()

set(NOTES_QT_QML_SOURCE "")
foreach(_notes_candidate IN LISTS _notes_qt_qml_candidates)
  cmake_path(NORMAL_PATH _notes_candidate OUTPUT_VARIABLE _notes_candidate)
  if(EXISTS "${_notes_candidate}")
    set(NOTES_QT_QML_SOURCE "${_notes_candidate}")
    break()
  endif()
endforeach()

file(MAKE_DIRECTORY "${NOTES_QT_PLATFORMS_DEST}")
file(COPY "${NOTES_QT_PLATFORMS_SOURCE}/"
     DESTINATION "${NOTES_QT_PLATFORMS_DEST}")

message(STATUS "Copied Qt platforms to ${NOTES_QT_PLATFORMS_DEST}")

if(NOTES_QT_QML_SOURCE)
  file(MAKE_DIRECTORY "${NOTES_QT_QML_DEST}")
  file(COPY "${NOTES_QT_QML_SOURCE}/" DESTINATION "${NOTES_QT_QML_DEST}")
  message(STATUS "Copied Qt qml modules to ${NOTES_QT_QML_DEST}")
else()
  message(WARNING "Qt qml directory was not found from ${NOTES_QT_BIN_DIR}")
endif()

set(NOTES_QT_RUNTIME_BIN_SOURCE "")
foreach(_notes_candidate IN LISTS _notes_qt_runtime_bin_candidates)
  cmake_path(NORMAL_PATH _notes_candidate OUTPUT_VARIABLE _notes_candidate)
  if(EXISTS "${_notes_candidate}")
    set(NOTES_QT_RUNTIME_BIN_SOURCE "${_notes_candidate}")
    break()
  endif()
endforeach()

if(NOTES_QT_RUNTIME_BIN_SOURCE)
  file(GLOB _notes_qt_runtime_dlls "${NOTES_QT_RUNTIME_BIN_SOURCE}/Qt6*.dll")
  if(_notes_qt_runtime_dlls)
    file(COPY ${_notes_qt_runtime_dlls} DESTINATION "${NOTES_QT_RUNTIME_DEST}")
    message(STATUS "Copied Qt runtime dlls to ${NOTES_QT_RUNTIME_DEST}")
  else()
    message(WARNING "No Qt runtime dlls found in ${NOTES_QT_RUNTIME_BIN_SOURCE}")
  endif()
else()
  message(WARNING "Qt runtime bin directory was not found from ${NOTES_QT_BIN_DIR}")
endif()

file(WRITE "${NOTES_QT_CONF_PATH}" "[Paths]\nPlugins = platforms\nQmlImports = qml\n")
message(STATUS "Wrote Qt runtime config to ${NOTES_QT_CONF_PATH}")
