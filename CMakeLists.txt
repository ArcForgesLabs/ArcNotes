cmake_minimum_required(VERSION 4.2 FATAL_ERROR)

set(APP_VERSION 2.3.1)
set(APP_ID "io.github.nuttyartist.notes")
set(QML_MODULE_URI "nuttyartist.notes")
set(APP_TARGET "arcnotes-app")
set(APP_AUTHOR "Ruby Mamistvalove")
set(APP_AUTHOR_EMAIL "ruby.mamistvalove@gmail.com")
set(APP_WEBSITE "https://www.notes-foss.com/")
set(APP_REPOSITORY "https://github.com/nuttyartist/notes")
string(TIMESTAMP CURRENT_YEAR "%Y")

# Any of the options below can be changed by passing -D<option name>=<value> to CMake.
set(CMAKE_OSX_DEPLOYMENT_TARGET
    "10.15"
    CACHE STRING "Minimum macOS version to target for deployment")
option(GIT_REVISION
       "Append the current git revision to the app's version string" OFF)
option(UPDATE_CHECKER
       "Enable or disable both the update checker and auto-updater" ON)
option(PRO_VERSION "Enable or disable Notes Pro features" ON)
option(ENABLE_ASAN "Enable address sanitizer" OFF)
option(USE_WERROR "Error on compile warning" OFF)

project(
  Notes
  VERSION ${APP_VERSION}
  DESCRIPTION "Note-taking app. Write down your thoughts"
  LANGUAGES CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Embed MSVC/UCRT/SDK system include paths into compile_commands.json so that
# clangd can resolve system headers without the INCLUDE environment variable.
if(MSVC AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND DEFINED ENV{INCLUDE})
  set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES $ENV{INCLUDE})
  set(CMAKE_C_STANDARD_INCLUDE_DIRECTORIES $ENV{INCLUDE})
endif()

# Keep a stable compilation database at <source>/build for clangd and hooks.
if(CMAKE_EXPORT_COMPILE_COMMANDS)
  add_custom_target(
    sync-compile-commands ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/build"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_BINARY_DIR}/compile_commands.json"
            "${CMAKE_SOURCE_DIR}/build/compile_commands.json"
    COMMENT "Syncing compile_commands.json -> build/"
    VERBATIM)
endif()

# address sanitizer can be enabled by running cmake with -DENABLE_ASAN=ON
if(ENABLE_ASAN AND (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
                    OR CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
  set(ADDRESS_SANITIZER_FLAGS "-fsanitize=address -fno-omit-frame-pointer")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ADDRESS_SANITIZER_FLAGS}")
endif()

if(POLICY CMP0083)
  cmake_policy(SET CMP0083 NEW)
  include(CheckPIESupported)
  check_pie_supported()
endif()

if(PRO_VERSION)
  add_definitions(-DPRO_VERSION)
endif()

# List of Qt components required to build the app.
set(QT_COMPONENTS
    Core
    Concurrent
    Gui
    Network
    Qml
    Sql
    Test
    Quick
    QuickControls2
    Widgets
    QuickWidgets)

message(STATUS "Trying to find Qt 6...")
if(NOT DEFINED Qt6_DIR AND DEFINED ENV{Qt6_DIR})
  set(
    Qt6_DIR
    "$ENV{Qt6_DIR}"
    CACHE PATH "Path to the Qt6 CMake package directory")
endif()

if(NOT DEFINED Qt6_DIR OR Qt6_DIR STREQUAL "")
  message(
    FATAL_ERROR
      "Qt6_DIR is not set. Point it to the directory containing Qt6Config.cmake, for example C:/Qt/6.10.2/msvc2022_64/lib/cmake/Qt6."
  )
endif()

# https://doc.qt.io/qt-6/whatsnew610.html#build-system-changes
find_package(Qt6 REQUIRED COMPONENTS ${QT_COMPONENTS})

set(QT_VERSION_MAJOR 6)
set(QT_VERSION ${Qt6_VERSION})

message(STATUS "Success! Configuration details:")
message(STATUS "App name: ${PROJECT_NAME}")
message(STATUS "App version: ${PROJECT_VERSION}")
message(STATUS "Qt version: ${QT_VERSION}")
message(STATUS "Qt6_DIR: ${Qt6_DIR}")
message(STATUS "Update checker: ${UPDATE_CHECKER}")
message(STATUS "Pro Version: ${PRO_VERSION}")
if(CMAKE_BUILD_TYPE STREQUAL "")
  message(STATUS "Build type: (not set)")
else()
  message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
endif()

if(ENABLE_ASAN)
  message(STATUS "Enabling AddressSanitizer")
endif()

set(GIT_REV "")
if(GIT_REVISION)
  if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
    execute_process(
      COMMAND git rev-parse --short HEAD
      WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
      OUTPUT_VARIABLE GIT_REV
      ERROR_VARIABLE GIT_REV_ERR
      OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_STRIP_TRAILING_WHITESPACE)
    if(GIT_REV)
      message(STATUS "Git revision: ${GIT_REV}")
      string(PREPEND GIT_REV "+g")
    else()
      message(FATAL_ERROR "Failed to retrieve git revision: ${GIT_REV_ERR}")
    endif()
  else()
    message(
      WARNING
        "The GIT_REVISION option was set, but this is not a git repository.")
  endif()
endif()

set(QRC_SOURCES ${PROJECT_SOURCE_DIR}/src/fonts.qrc
                ${PROJECT_SOURCE_DIR}/src/images.qrc
                ${PROJECT_SOURCE_DIR}/src/styles.qrc)

qt_add_resources(QRC_GENERATED_SOURCES ${QRC_SOURCES})

set(SOURCES
    ${PROJECT_SOURCE_DIR}/src/dbmanager.cpp
    ${PROJECT_SOURCE_DIR}/src/dbmanager.h
    ${PROJECT_SOURCE_DIR}/src/lqtutils_enum.h
    ${PROJECT_SOURCE_DIR}/src/main.cpp
    ${PROJECT_SOURCE_DIR}/src/nodedata.cpp
    ${PROJECT_SOURCE_DIR}/src/nodedata.h
    ${PROJECT_SOURCE_DIR}/src/nodepath.cpp
    ${PROJECT_SOURCE_DIR}/src/nodepath.h
    ${PROJECT_SOURCE_DIR}/src/nodetreemodel.cpp
    ${PROJECT_SOURCE_DIR}/src/nodetreemodel.h
    ${PROJECT_SOURCE_DIR}/src/notelistmodel.cpp
    ${PROJECT_SOURCE_DIR}/src/notelistmodel.h
    ${PROJECT_SOURCE_DIR}/src/singleinstance.cpp
    ${PROJECT_SOURCE_DIR}/src/singleinstance.h
    ${PROJECT_SOURCE_DIR}/src/tagdata.cpp
    ${PROJECT_SOURCE_DIR}/src/tagdata.h
    ${PROJECT_SOURCE_DIR}/src/taglistmodel.cpp
    ${PROJECT_SOURCE_DIR}/src/taglistmodel.h
    ${PROJECT_SOURCE_DIR}/src/tagpool.cpp
    ${PROJECT_SOURCE_DIR}/src/tagpool.h
    ${PROJECT_SOURCE_DIR}/src/utils.h
    ${QRC_GENERATED_SOURCES})

set(SOURCES_3RD_PARTY)
set(QML_CXX_SOURCES
    ${PROJECT_SOURCE_DIR}/src/appbackend.cpp
    ${PROJECT_SOURCE_DIR}/src/appbackend.h
    ${PROJECT_SOURCE_DIR}/src/editorsettingsoptions.cpp
    ${PROJECT_SOURCE_DIR}/src/editorsettingsoptions.h
    ${PROJECT_SOURCE_DIR}/src/noteeditorbackend.cpp
    ${PROJECT_SOURCE_DIR}/src/noteeditorbackend.h)

if(APPLE)
  list(APPEND SOURCES ${PROJECT_SOURCE_DIR}/src/images/notes_icon.icns)
elseif(WIN32)
  list(APPEND SOURCES ${PROJECT_SOURCE_DIR}/src/images/notes.rc)
endif()

add_executable(${APP_TARGET} WIN32 MACOSX_BUNDLE ${SOURCES}
                                                 ${SOURCES_3RD_PARTY})

target_include_directories(${APP_TARGET} PUBLIC ${PROJECT_SOURCE_DIR}/src)

target_compile_definitions(
  ${APP_TARGET}
  PUBLIC APP_VERSION="${PROJECT_VERSION}${GIT_REV}" APP_ID="${APP_ID}"
         QT_DISABLE_DEPRECATED_BEFORE=0x050900)

# Enable useful compiler warnings (except on third-party code).
if(MSVC)
  set(NOTES_COMPILE_OPTIONS
      /W4
      /EHsc
      "$<$<COMPILE_LANGUAGE:CXX>:/MP>"
      "$<$<COMPILE_LANGUAGE:CXX>:/std:c++23preview>"
      "$<$<CONFIG:RELEASE>:/O2>")
  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    list(APPEND NOTES_COMPILE_OPTIONS -Wno-unused-command-line-argument
         -Wno-unused-parameter -Wno-microsoft-include)
  else()
    list(APPEND NOTES_COMPILE_OPTIONS /external:anglebrackets /external:W0
         /wd4100)
  endif()
  if(USE_WERROR)
    list(APPEND NOTES_COMPILE_OPTIONS /WX)
  endif()
  target_compile_options(${APP_TARGET} PUBLIC ${NOTES_COMPILE_OPTIONS})
  set_source_files_properties(${SOURCES_3RD_PARTY} PROPERTIES COMPILE_FLAGS /w)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES
                                                 "Clang")
  set(NOTES_COMPILE_OPTIONS -Wuninitialized -Wall -Wextra
                            -Wno-unused-parameter -Wshadow)
  if(USE_WERROR)
    list(APPEND NOTES_COMPILE_OPTIONS -Werror)
  endif()
  target_compile_options(${APP_TARGET} PUBLIC ${NOTES_COMPILE_OPTIONS})
  set_source_files_properties(${SOURCES_3RD_PARTY} PROPERTIES COMPILE_FLAGS -w)
  set_source_files_properties(${QRC_GENERATED_SOURCES} PROPERTIES COMPILE_FLAGS
                                                                 -w)
  set_source_files_properties(
    ${CMAKE_CURRENT_BINARY_DIR}/${APP_TARGET}_autogen/mocs_compilation.cpp
    PROPERTIES COMPILE_FLAGS -w)
  set_source_files_properties(
    ${CMAKE_CURRENT_BINARY_DIR}/${APP_TARGET}_qmltyperegistrations.cpp
    PROPERTIES COMPILE_FLAGS -w)
endif()

if(QT_VERSION VERSION_GREATER_EQUAL 6.2)
  set(QML_FILES
      src/qml/CheckBoxMaterial.qml
      src/qml/CustomHorizontalScrollBar.qml
      src/qml/CustomTextArea.qml
      src/qml/CustomTextField.qml
      src/qml/CustomVerticalScrollBar.qml
      src/qml/EditorSettings.qml
      src/qml/KanbanBoard.qml
      src/qml/Main.qml
      src/qml/FontChooserButton.qml
      src/qml/FontIconLoader.qml
      src/qml/FontIconsCodes.qml
      src/qml/IconButton.qml
      src/qml/OptionItemButton.qml
      src/qml/SubscriptionStatus.qml
      src/qml/SubscriptionWindow.qml
      src/qml/SwitchButton.qml
      src/qml/TextButton.qml
      src/qml/ThemeChooserButton.qml
      src/qml/TodoColumnDelegate.qml
      src/qml/TodoTaskDelegate.qml
      src/qml/Utilities.js
      src/qml/ViewChooserButton.qml
      src/qml/kanbanMain.qml)

  set_source_files_properties(src/qml/SubscriptionStatus.qml
                              PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

  foreach(file IN LISTS QML_FILES)
    get_filename_component(fileName ${file} NAME)
    set_source_files_properties(${file} PROPERTIES QT_RESOURCE_ALIAS
                                                   ${fileName})

    # This whole shenanigans is required to silence compiler warnings on files auto-generated by rcc.
    # It isn't pretty, but unfortunately there's no nicer way to do this yet:
    # https://discourse.cmake.org/t/qt-autogen-source-files-custom-compilation-flags/3002
    string(REPLACE "." "_" file_qml_cpp ${fileName})

    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES
                                               "Clang")
      set_source_files_properties(
        ${CMAKE_CURRENT_BINARY_DIR}/.rcc/qmlcache/${APP_TARGET}_src/qml/${file_qml_cpp}.cpp
        PROPERTIES COMPILE_FLAGS -w)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND MSVC_VERSION GREATER_EQUAL
                                                     1920)
      set_source_files_properties(
        ${CMAKE_CURRENT_BINARY_DIR}/.rcc/qmlcache/${APP_TARGET}_src/qml/${file_qml_cpp}.cpp
        PROPERTIES COMPILE_FLAGS /w)
    endif()
  endforeach()

  # Silence an extra warning given by Clang:
  # build/.rcc/qmlcache/Notes_qmlcache_loader.cpp#L46
  # must specify at least one argument for '...' parameter of variadic macro [-Wgnu-zero-variadic-macro-arguments]
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set_source_files_properties(
      ${CMAKE_CURRENT_BINARY_DIR}/.rcc/qmlcache/${APP_TARGET}_qmlcache_loader.cpp
      PROPERTIES COMPILE_FLAGS -w)
  endif()

  qt_add_qml_module(
    ${APP_TARGET}
    URI
    ${QML_MODULE_URI}
    VERSION
    1.0
    NO_PLUGIN
    RESOURCE_PREFIX
    "/qt/qml"
    SOURCES
    ${QML_CXX_SOURCES}
    QML_FILES
    ${QML_FILES}
    NO_RESOURCE_TARGET_PATH)
endif()

target_link_libraries(
  ${APP_TARGET}
  PUBLIC Qt${QT_VERSION_MAJOR}::Concurrent
         Qt${QT_VERSION_MAJOR}::Core
         Qt${QT_VERSION_MAJOR}::Gui
         Qt${QT_VERSION_MAJOR}::Network
         Qt${QT_VERSION_MAJOR}::Qml
         Qt${QT_VERSION_MAJOR}::Sql
         Qt${QT_VERSION_MAJOR}::Quick
         Qt${QT_VERSION_MAJOR}::QuickControls2
         Qt${QT_VERSION_MAJOR}::Widgets)

if(APPLE)
  set(COPYRIGHT_TEXT
      "Copyright (c) 2015-${CURRENT_YEAR} ${APP_AUTHOR} and contributors.")

  set_target_properties(
    ${APP_TARGET}
    PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME}
               MACOSX_BUNDLE_GUI_IDENTIFIER ${APP_ID}
               MACOSX_BUNDLE_ICON_FILE "notes_icon.icns"
               MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
               MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
               MACOSX_BUNDLE_LONG_VERSION_STRING ${PROJECT_VERSION}
               MACOSX_BUNDLE_COPYRIGHT ${COPYRIGHT_TEXT}
               MACOSX_BUNDLE_INFO_STRING ${COPYRIGHT_TEXT})

  set_source_files_properties(${PROJECT_SOURCE_DIR}/src/images/notes_icon.icns
                              PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

  target_link_libraries(${APP_TARGET} PUBLIC "-framework Carbon"
                                             "-framework Cocoa")
elseif(UNIX)
  # On Linux, it's conventional for executables to be all lowercase.
  string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWERCASE)
  set_target_properties(${APP_TARGET} PROPERTIES OUTPUT_NAME
                                                 ${PROJECT_NAME_LOWERCASE})

  find_package(X11 REQUIRED)
  target_link_libraries(${APP_TARGET} PUBLIC ${X11_LIBRARIES})

  include(GNUInstallDirs)

  configure_file(${PROJECT_SOURCE_DIR}/packaging/linux/common/notes.desktop.in
                 ${CMAKE_BINARY_DIR}/${APP_ID}.desktop)
  install(FILES ${CMAKE_BINARY_DIR}/${APP_ID}.desktop
          DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)

  configure_file(
    ${PROJECT_SOURCE_DIR}/packaging/linux/common/notes.appdata.xml.in
    ${CMAKE_BINARY_DIR}/${APP_ID}.metainfo.xml)
  install(FILES ${CMAKE_BINARY_DIR}/${APP_ID}.metainfo.xml
          DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/metainfo)

  foreach(ICON_SIZE 128 256 512)
    install(
      FILES
        ${PROJECT_SOURCE_DIR}/packaging/linux/common/icons/${ICON_SIZE}x${ICON_SIZE}/notes.png
      DESTINATION
        ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/${ICON_SIZE}x${ICON_SIZE}/apps
      RENAME ${APP_ID}.png)
  endforeach()

  install(
    FILES ${PROJECT_SOURCE_DIR}/LICENSE
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/licenses/${PROJECT_NAME_LOWERCASE})

  # On Debian-based distros, compress the changelog file and install the copyright file.
  # FIXME: Find out if it's possible to only do this when creating a deb package with CPack.
  if(EXISTS /etc/debian_version)
    string(TIMESTAMP DATE_NOW_UTC "%a, %d %b %Y %H:%M:%S +0000")

    configure_file(${PROJECT_SOURCE_DIR}/packaging/linux/debian/changelog.in
                   ${CMAKE_BINARY_DIR}/changelog.Debian)

    add_custom_command(
      COMMAND gzip ARGS -9 -c -f -k -n ${CMAKE_BINARY_DIR}/changelog.Debian >
              ${CMAKE_BINARY_DIR}/changelog.gz
      OUTPUT ${CMAKE_BINARY_DIR}/changelog.gz)

    add_custom_target(changelog ALL DEPENDS ${CMAKE_BINARY_DIR}/changelog.gz)

    install(
      FILES ${CMAKE_BINARY_DIR}/changelog.gz
      DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/doc/${PROJECT_NAME_LOWERCASE})

    configure_file(${PROJECT_SOURCE_DIR}/packaging/linux/debian/copyright.in
                   ${CMAKE_BINARY_DIR}/copyright)

    install(
      FILES ${CMAKE_BINARY_DIR}/copyright
      DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/doc/${PROJECT_NAME_LOWERCASE})
  endif()

  set(CPACK_PACKAGE_NAME ${PROJECT_NAME_LOWERCASE})
  set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION}${GIT_REV})
  set(CPACK_PACKAGE_VENDOR ${APP_AUTHOR})
  set(CPACK_PACKAGE_CONTACT "${APP_AUTHOR} <${APP_AUTHOR_EMAIL}>")
  set(CPACK_STRIP_FILES TRUE)

  # Package description.
  # Please keep the character count for every line at 80 (max) - it's a lintian rule.
  string(CONCAT CPACK_PACKAGE_DESCRIPTION
                "Note-taking app that is both beautiful and powerful.\n"
                " It is written in C++ using the Qt framework.")

  # Workaround for older CMake versions, where CPACK_PACKAGE_DESCRIPTION didn't exist.
  if(CMAKE_VERSION VERSION_LESS 3.12)
    string(CONCAT CPACK_DEBIAN_PACKAGE_DESCRIPTION "${PROJECT_DESCRIPTION}\n"
                  " ${CPACK_PACKAGE_DESCRIPTION}")
  endif()

  # deb-specific
  set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
  set(CPACK_DEBIAN_PACKAGE_DEPENDS "qt6-qpa-plugins")
  if(QT_VERSION_MINOR VERSION_GREATER_EQUAL 2)
    set(CPACK_DEBIAN_PACKAGE_DEPENDS
        "${CPACK_DEBIAN_PACKAGE_DEPENDS}, qml6-module-qtqml-workerscript, qml6-module-qtquick-controls, qml6-module-qtquick-layouts, qml6-module-qtquick-particles, qml6-module-qtquick-templates, qml6-module-qtquick-window"
    )
  endif()
  set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
  set(CPACK_DEBIAN_PACKAGE_HOMEPAGE ${APP_WEBSITE})
  set(CPACK_DEBIAN_PACKAGE_SECTION "utils")

  # rpm-specific
  set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
  if(UNIX
     AND NOT APPLE
     AND EXISTS /etc/os-release)
    file(READ /etc/os-release DISTRO_INFO)
    if(DISTRO_INFO MATCHES "SUSE")
      # Note: Different from Fedora 36, the SQLite library we need is not a dependency
      # of the base Qt 6 package on openSUSE Leap 15.4, so we have to manually include it here.
      set(CPACK_RPM_PACKAGE_REQUIRES "qt6-sql-sqlite")
    endif()
  endif()
  set(CPACK_RPM_PACKAGE_LICENSE MPL-2.0)
  set(CPACK_RPM_PACKAGE_URL ${APP_WEBSITE})
  set(CPACK_RPM_PACKAGE_GROUP Applications/Text)
  set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION /usr/share/icons)
  # Remove any leading whitespace character from the description text we added for the deb package.
  string(REGEX REPLACE "\n +" "\n" CPACK_RPM_PACKAGE_DESCRIPTION
                       ${CPACK_PACKAGE_DESCRIPTION})
  set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION /usr/share/icons)

  include(CPack)
elseif(WIN32)
  set_target_properties(${APP_TARGET} PROPERTIES OUTPUT_NAME ${PROJECT_NAME})
  target_compile_definitions(${APP_TARGET} PUBLIC QXT_STATIC)

  target_link_libraries(${APP_TARGET} PUBLIC user32)

  cmake_path(GET Qt6_DIR PARENT_PATH NOTES_QT_CMAKE_ROOT)
  cmake_path(GET NOTES_QT_CMAKE_ROOT PARENT_PATH NOTES_QT_LIB_DIR)
  set(NOTES_WINDEPLOYQT "${NOTES_QT_LIB_DIR}/../bin/windeployqt.exe")
  cmake_path(NORMAL_PATH NOTES_WINDEPLOYQT OUTPUT_VARIABLE NOTES_WINDEPLOYQT)

  if(NOT EXISTS "${NOTES_WINDEPLOYQT}")
    message(
      FATAL_ERROR
        "windeployqt.exe was not found next to the configured Qt installation: ${NOTES_WINDEPLOYQT}"
    )
  endif()

  add_custom_command(
    TARGET ${APP_TARGET}
    POST_BUILD
    COMMAND
      ${CMAKE_COMMAND}
      -DNOTES_WINDEPLOYQT=${NOTES_WINDEPLOYQT}
      -DNOTES_DEPLOY_CONFIGURATION=$<CONFIG>
      -DNOTES_QML_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/src/qml
      -DNOTES_TARGET_FILE=$<TARGET_FILE:${APP_TARGET}>
      -DNOTES_QT_RUNTIME_DEST=$<TARGET_FILE_DIR:${APP_TARGET}>
      -DNOTES_QT_CONF_PATH=$<TARGET_FILE_DIR:${APP_TARGET}>/qt.conf
      -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/copy_qt_platforms.cmake
    VERBATIM)
endif()

add_custom_command(
  TARGET ${APP_TARGET}
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory
          $<TARGET_FILE_DIR:${APP_TARGET}>/nuttyartist/notes
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_CURRENT_BINARY_DIR}/nuttyartist/notes
          $<TARGET_FILE_DIR:${APP_TARGET}>/nuttyartist/notes
  VERBATIM)

add_custom_command(
  TARGET ${APP_TARGET}
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND}
    -DNOTES_QMLLS_DEST=${CMAKE_SOURCE_DIR}/.qmlls.ini
    -DNOTES_QMLLS_BUILD_DIR=${CMAKE_BINARY_DIR}
    -DNOTES_QMLLS_IMPORT_PATHS=${CMAKE_BINARY_DIR}\;$<TARGET_FILE_DIR:${APP_TARGET}>\;$<TARGET_FILE_DIR:${APP_TARGET}>/qml
    -DNOTES_QMLLS_DOC_DIR=${QT_INSTALL_DOCS}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/sync_qmlls_ini.cmake
  VERBATIM)

install(
  TARGETS ${APP_TARGET}
  RUNTIME DESTINATION bin
  BUNDLE DESTINATION bin
  LIBRARY DESTINATION lib)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
