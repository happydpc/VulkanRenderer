target_sources(VulkanEngine PRIVATE

${CMAKE_CURRENT_SOURCE_DIR}/FrameGraph.cpp
${CMAKE_CURRENT_SOURCE_DIR}/Renderer.cpp
${CMAKE_CURRENT_SOURCE_DIR}/ViewCamera.cpp
)

add_subdirectory(backend)
add_subdirectory(renderers)