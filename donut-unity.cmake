file(GLOB donut_unity_src
    LIST_DIRECTORIES false
    include/donut/unity/*.h
    src/unity/*.cpp
)

add_library(donut_unity STATIC EXCLUDE_FROM_ALL ${donut_unity_src})
target_include_directories(donut_unity PUBLIC include)
target_include_directories(donut_unity PRIVATE nvrhi/thirdparty/Vulkan-Headers/include)
target_include_directories(donut_unity PRIVATE thirdparty/glfw/include)
target_include_directories(donut_unity PRIVATE "${CMAKE_SOURCE_DIR}/thirdparty/readerwriterqueue")

target_link_libraries(donut_unity donut_engine)
set_target_properties(donut_unity PROPERTIES FOLDER Donut)
