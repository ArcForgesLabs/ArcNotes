if(NOT DEFINED NOTES_QMLLS_DEST OR NOT DEFINED NOTES_QMLLS_BUILD_DIR)
  message(
    FATAL_ERROR
      "NOTES_QMLLS_DEST and NOTES_QMLLS_BUILD_DIR must be set.")
endif()

cmake_path(CONVERT "${NOTES_QMLLS_BUILD_DIR}" TO_CMAKE_PATH_LIST _notes_qmlls_build_dir)
set(_notes_qmlls_contents "[General]\n")
string(APPEND _notes_qmlls_contents "buildDir=${_notes_qmlls_build_dir}\n")
string(APPEND _notes_qmlls_contents "no-cmake-calls=true\n")

if(DEFINED NOTES_QMLLS_DOC_DIR AND NOT NOTES_QMLLS_DOC_DIR STREQUAL "")
  cmake_path(CONVERT "${NOTES_QMLLS_DOC_DIR}" TO_CMAKE_PATH_LIST _notes_qmlls_doc_dir)
  string(APPEND _notes_qmlls_contents "docDir=${_notes_qmlls_doc_dir}\n")
endif()

if(DEFINED NOTES_QMLLS_IMPORT_PATHS AND NOT NOTES_QMLLS_IMPORT_PATHS STREQUAL "")
  cmake_path(CONVERT "${NOTES_QMLLS_IMPORT_PATHS}" TO_CMAKE_PATH_LIST _notes_qmlls_import_paths)
  string(APPEND _notes_qmlls_contents "importPaths=${_notes_qmlls_import_paths}\n")
endif()

get_filename_component(_notes_qmlls_dest_dir "${NOTES_QMLLS_DEST}" DIRECTORY)
file(MAKE_DIRECTORY "${_notes_qmlls_dest_dir}")
file(WRITE "${NOTES_QMLLS_DEST}" "${_notes_qmlls_contents}")
message(STATUS "Wrote qmlls config to ${NOTES_QMLLS_DEST}")
