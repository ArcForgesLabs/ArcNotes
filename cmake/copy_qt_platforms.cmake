if(NOT WIN32)
  return()
endif()

if(NOT DEFINED NOTES_WINDEPLOYQT OR NOTES_WINDEPLOYQT STREQUAL "")
  message(FATAL_ERROR "NOTES_WINDEPLOYQT was not provided.")
endif()

if(NOT EXISTS "${NOTES_WINDEPLOYQT}")
  message(FATAL_ERROR "windeployqt.exe was not found: ${NOTES_WINDEPLOYQT}")
endif()

set(_notes_deploy_args
    --dir
    "${NOTES_QT_RUNTIME_DEST}"
    --qmldir
    "${NOTES_QML_SOURCE_DIR}"
    --no-translations
    --force)

if(NOTES_DEPLOY_CONFIGURATION STREQUAL "Debug")
  list(APPEND _notes_deploy_args --debug)
else()
  list(APPEND _notes_deploy_args --release)
endif()

execute_process(
  COMMAND "${NOTES_WINDEPLOYQT}" ${_notes_deploy_args} "${NOTES_TARGET_FILE}"
  RESULT_VARIABLE _notes_windeployqt_result
  OUTPUT_VARIABLE _notes_windeployqt_stdout
  ERROR_VARIABLE _notes_windeployqt_stderr)

if(_notes_windeployqt_stdout)
  string(STRIP "${_notes_windeployqt_stdout}" _notes_windeployqt_stdout)
  message(STATUS "${_notes_windeployqt_stdout}")
endif()

if(_notes_windeployqt_stderr)
  string(STRIP "${_notes_windeployqt_stderr}" _notes_windeployqt_stderr)
  message(STATUS "${_notes_windeployqt_stderr}")
endif()

if(NOT _notes_windeployqt_result EQUAL 0)
  message(
    FATAL_ERROR
      "windeployqt failed with exit code ${_notes_windeployqt_result}.")
endif()

file(WRITE "${NOTES_QT_CONF_PATH}" "[Paths]\nPlugins = .\nQmlImports = qml\n")
message(STATUS "Wrote Qt runtime config to ${NOTES_QT_CONF_PATH}")
