set(ARCNOTES_TEST_TARGET arcnotes-tests)

set(ARCNOTES_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tst_notedata.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tst_notedata.h
    ${CMAKE_CURRENT_SOURCE_DIR}/tst_notemodel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tst_notemodel.h)

add_executable(${ARCNOTES_TEST_TARGET} ${ARCNOTES_TEST_SOURCES})

target_include_directories(
  ${ARCNOTES_TEST_TARGET}
  PRIVATE ${PROJECT_SOURCE_DIR}/src)

target_compile_definitions(
  ${ARCNOTES_TEST_TARGET}
  PRIVATE APP_VERSION="${PROJECT_VERSION}${GIT_REV}" APP_ID="${APP_ID}"
          QT_DISABLE_DEPRECATED_BEFORE=0x050900 QSU_INCLUDE_MOC=1)

if(DEFINED NOTES_COMPILE_OPTIONS)
  target_compile_options(${ARCNOTES_TEST_TARGET} PRIVATE ${NOTES_COMPILE_OPTIONS})
endif()

target_link_libraries(
  ${ARCNOTES_TEST_TARGET}
  PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Test)

add_test(NAME ${ARCNOTES_TEST_TARGET} COMMAND ${ARCNOTES_TEST_TARGET})
set_tests_properties(${ARCNOTES_TEST_TARGET} PROPERTIES ENVIRONMENT
                                                       "QT_QPA_PLATFORM=offscreen")
