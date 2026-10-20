# Consumer packaging for the published engine, which has no browser library target.
# Compile upstream renderer sources without copying or editing engine implementations.
function(racing_add_webengine)
  set(BUILD_TESTING OFF)
  add_subdirectory("${WEBENGINE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/engine" EXCLUDE_FROM_ALL)
  include(FetchContent)
  FetchContent_GetProperties(jolt SOURCE_DIR jolt_source)
  set(RACING_JOLT_SOURCE_DIR "${jolt_source}" PARENT_SCOPE)
  if(EMSCRIPTEN)
    add_library(racing_engine_browser STATIC
      "${WEBENGINE_SOURCE_DIR}/src/renderer.cpp"
      "${WEBENGINE_SOURCE_DIR}/src/webgpu_renderer.cpp"
      "${WEBENGINE_SOURCE_DIR}/src/webgpu_water.cpp"
      "${WEBENGINE_SOURCE_DIR}/src/webgl_renderer.cpp"
      "${WEBENGINE_SOURCE_DIR}/src/webgl_water.cpp"
      "${WEBENGINE_SOURCE_DIR}/src/gui_renderer.cpp")
    target_include_directories(racing_engine_browser PRIVATE "${CMAKE_BINARY_DIR}/engine/generated")
    target_link_libraries(racing_engine_browser PUBLIC engine_core engine_gui)
    target_compile_options(racing_engine_browser PUBLIC --use-port=emdawnwebgpu -fexceptions)
    target_link_options(racing_engine_browser INTERFACE --use-port=emdawnwebgpu
      -fexceptions -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2)
  endif()
endfunction()
