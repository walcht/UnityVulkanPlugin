cmake_minimum_required(VERSION 3.21 FATAL_ERROR)

project(TextureSubPlugin)

message(STATUS "building for target ${CMAKE_SYSTEM_NAME}")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message("Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "build type" FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
endif()

message(STATUS "build type is ${CMAKE_BUILD_TYPE}")

# Unity's plugin API (PluginAPI folder) path
if(NOT IS_DIRECTORY ${UNITY_PLUGIN_API})
  message(FATAL_ERROR "target Unity's PluginAPI folder path has to provided by setting UNITY_PLUGIN_API")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(UNITY_LINUX 1)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Android" AND CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
  message(FATAL_ERROR "building native plugin for Android using NDK is not supported on Windows")
endif()

set(SOURCES
    src/TextureSubPlugin.cpp
    src/TextureSubPluginAPI.cpp
)

if (SUPPORT_VULKAN)
  list(APPEND SOURCES src/TextureSubPluginAPI_Vulkan.cpp)
endif()

if (SUPPORT_OPENGL_CORE)
  list(APPEND SOURCES src/TextureSubPluginAPI_OpenGLCoreES.cpp)
  if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # glad path for OpenGL build on Windows
    if(NOT IS_DIRECTORY ${GLAD_PATH})
      message(FATAL_ERROR "glad (OpenGL loader) is not a valid directory - set it by assigning GLAD_PATH")
    endif()
    list(APPEND SOURCES ${GLAD_PATH}/src/glad.c)
  endif()
endif()

if (SUPPORT_D3D11)
  list(APPEND SOURCES src/TextureSubPluginAPI_D3D11.cpp)
endif()

# POSITION_INDEPENDENT_CODE property is True by default for SHARED targets
add_library(TextureSubPlugin SHARED ${SOURCES})

set_property(TARGET TextureSubPlugin PROPERTY CXX_STANDARD 17)

target_include_directories(TextureSubPlugin
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/
        ${UNITY_PLUGIN_API}
)

if(SUPPORT_OPENGL_CORE AND CMAKE_SYSTEM_NAME STREQUAL "Windows")
  target_include_directories(TextureSubPlugin
      PRIVATE
          ${GLAD_PATH}/include/
  )
endif()

if(SUPPORT_VULKAN)
  find_package(Vulkan)
  if(NOT Vulkan_FOUND)
    message(WARNING "could not find Vulkan SDK - unsetting SUPPORT_VULKAN")
    unset(SUPPORT_VULKAN)
  endif()
  target_link_libraries(TextureSubPlugin Vulkan::Vulkan)
endif()

if(SUPPORT_OPENGL_CORE)
  find_package(OpenGL)
  if(NOT OpenGL_FOUND)
    message(WARNING "could not find OpenGL - unsetting SUPPORT_OPENGL_CORE")
    unset(SUPPORT_OPENGL_CORE)
  endif()
  target_link_libraries(TextureSubPlugin OpenGL::GL)
endif()


if(SUPPORT_OPENGL_ES)
  # TODO: add OpenGLES support
endif()

# pass preprocessor definitions
target_compile_definitions(TextureSubPlugin
    PRIVATE
        -DSUPPORT_VULKAN=${SUPPORT_VULKAN}
        -DSUPPORT_D3D11=${SUPPORT_D3D11}
        -DSUPPORT_OPENGL_CORE=${SUPPORT_OPENGL_CORE}
        -DSUPPORT_OPENGL_ES=${SUPPORT_OPENGL_ES}
        -DUNITY_LINUX=${UNITY_LINUX}
)

install(TARGETS TextureSubPlugin DESTINATION .)
install(FILES ${PROJECT_SOURCE_DIR}/TextureSubPlugin.cs DESTINATION .)
