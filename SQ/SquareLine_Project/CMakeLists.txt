#CMake file for different scenarios: Native build on Linux or Windows (automatic), and cross-build from Unix to Windows (by defining build-host as Unix and build-target as Windows)

cmake_minimum_required( VERSION 3.10 )

project( SquareLine_Project C )

set( BUILD_CASE__UNIX_NATIVE 0 )
set( BUILD_CASE__UNIX_TO_WINDOWS 0 )
set( BUILD_CASE__WINDOWS_NATIVE 0 )
if     ( (UNIX OR (DEFINED SQUARELINE_BUILD_HOST__UNIX)) AND (NOT DEFINED SQUARELINE_BUILD_TARGET__WINDOWS) )  #AND (DEFINED SQUARELINE_BUILD_TARGET__UNIX) (VSCode wouldn't work with this)
    set( BUILD_CASE__UNIX_NATIVE 1 )
elseif ( (WIN32 OR (DEFINED SQUARELINE_BUILD_HOST__WINDOWS)) AND NOT (DEFINED SQUARELINE_BUILD_TARGET__UNIX) )  #AND (DEFINED SQUARELINE_BUILD_TARGET__WINDOWS) (VSCode wouldn't work with this)
    set( BUILD_CASE__WINDOWS_NATIVE 1 )
elseif ( (UNIX OR (DEFINED SQUARELINE_BUILD_HOST__UNIX)) AND (DEFINED SQUARELINE_BUILD_TARGET__WINDOWS) )
    set( BUILD_CASE__UNIX_TO_WINDOWS 1 )
else() #native desktop build as fallback if no external define is provided (e.g. when building from VSCode without providing the SQUARELINE_BUILD_TARGET__xx defines, no need to run build.sh/build.bat for 1st build)
    if ( UNIX OR (CMAKE_SYSTEM_NAME STREQUAL "Linux") OR (CMAKE_SYSTEM_NAME STREQUAL "Darwin") )
        set( BUILD_CASE__UNIX_NATIVE 1 )
    elseif (WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set( BUILD_CASE__WINDOWS_NATIVE 1 )
    else () #if somehow the system-name couldn't be detected, assume Unix
        set( BUILD_CASE__UNIX_NATIVE 1 )
    endif()
endif()

set( CMAKE_C_STANDARD 11 )
set( CMAKE_CXX_STANDARD 11 )
set( CMAKE_C_FLAGS "-Wall -Wextra -Wno-unused -g" )
set( CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-unused -g" )


include_directories (
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/lvgl
    ${PROJECT_SOURCE_DIR}/ui
)

FILE( GLOB_RECURSE LVGL_Sources CONFIGURE_DEPENDS lvgl/*.c )
FILE( GLOB_RECURSE UI_Sources CONFIGURE_DEPENDS ui/*.c ui/*.cpp )


if (BUILD_CASE__UNIX_NATIVE) #Linux or MacOS host native compilation

    find_package( SDL2 REQUIRED SDL2 )

    include_directories ( ${SDL2_INCLUDE_DIRS} ${SDL2_INCLUDE_DIRS}/../ )

    add_executable( ${PROJECT_NAME} main.c ${LVGL_Sources} ${UI_Sources} )

    #string( STRIP ${SDL2_LIBRARIES} SDL2_LIBRARIES )

    target_link_libraries( ${PROJECT_NAME} PRIVATE ${SDL2_LIBRARIES} )


elseif (BUILD_CASE__WINDOWS_NATIVE) #Native Windows 32bit or 64bit host compilation


elseif (BUILD_CASE__UNIX_TO_WINDOWS) #Unix to Windows cross-compilation

    set( CMAKE_SYSTEM_NAME Generic )

    set( CMAKE_CROSSCOMPILING true )
    set( CMAKE_SYSTEM_PROCESSOR x86_64 )
    set( CMAKE_LIBRARY_ARCHITECTURE x86_64-w64-mingw32 )
    set( CMAKE_C_COMPILER ${CMAKE_LIBRARY_ARCHITECTURE}-gcc )
    set( CMAKE_CXX_COMPILER ${CMAKE_LIBRARY_ARCHITECTURE}-g++ )
    set( CMAKE_LINKER ${CMAKE_LIBRARY_ARCHITECTURE}-ld )


endif()


if (BUILD_CASE__WINDOWS_NATIVE OR BUILD_CASE__UNIX_TO_WINDOWS)

    set( CMAKE_GENERATOR "MinGW Makefiles" )

    set( SDL_FOLDER sdl2-dev-x86_64 )
    set( CMAKE_FIND_ROOT_PATH ${SDL_FOLDER} )
    set( CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER )
    set( CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY )
    set( CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY )

    set( CMAKE_BUILD_RPATH "${SDL_FOLDER}/include" )
    set( CMAKE_INSTALL_RPATH ${CMAKE_BUILD_RPATH} )

    include_directories( ${PROJECT_SOURCE_DIR}/${SDL_FOLDER}/include )

    FILE( GLOB_RECURSE SDL_Sources CONFIGURE_DEPENDS ${SDL_FOLDER} )

    add_executable( ${PROJECT_NAME} main.c ${LVGL_Sources} ${SDL_Sources} ${UI_Sources} )

    add_custom_command (
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/SDL2.dll
            ${CMAKE_BINARY_DIR}/SDL2.dll
        COMMENT "Copying SDL2.dll to the build directory"
    )

    target_link_libraries( ${PROJECT_NAME} -L"${PROJECT_SOURCE_DIR}/${SDL_FOLDER}/lib" SDL2 )

endif()

