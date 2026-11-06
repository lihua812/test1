cmake_minimum_required(VERSION 3.15)
project(medium_term LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(OpenCV_DIR "D:/opencv/opencv/build")

set(OpenCV_VERSION_SUFFIX "4140")


if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(OpenCV_ARCH "x64")
else()
    set(OpenCV_ARCH "x86")
endif()
set(OpenCV_LIB_DIR "${OpenCV_DIR}/${OpenCV_ARCH}/vc16/lib")
set(OpenCV_BIN_DIR "${OpenCV_DIR}/${OpenCV_ARCH}/vc16/bin")


if(NOT EXISTS "${OpenCV_LIB_DIR}")
    message(FATAL_ERROR "未找到 OpenCV 库目录：${OpenCV_LIB_DIR}。\n"
        "若本机只安装了 x64 版 OpenCV，请改用 x64-debug / x64-release 预设，或安装对应架构的 OpenCV。")
endif()

add_executable(run_medium_term
    main.cpp)


target_include_directories(run_medium_term PRIVATE "${OpenCV_DIR}/include")
target_link_directories(run_medium_term PRIVATE "${OpenCV_LIB_DIR}")

target_compile_options(run_medium_term PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

target_link_libraries(run_medium_term PRIVATE
    $<$<CONFIG:Debug>:opencv_world${OpenCV_VERSION_SUFFIX}d.lib>
    $<$<CONFIG:Release>:opencv_world${OpenCV_VERSION_SUFFIX}.lib>
)


set(OpenCV_DLL_NAME "$<IF:$<CONFIG:Debug>,opencv_world${OpenCV_VERSION_SUFFIX}d.dll,opencv_world${OpenCV_VERSION_SUFFIX}.dll>")
if(EXISTS "${OpenCV_BIN_DIR}/opencv_world${OpenCV_VERSION_SUFFIX}d.dll"
   AND EXISTS "${OpenCV_BIN_DIR}/opencv_world${OpenCV_VERSION_SUFFIX}.dll")
    add_custom_command(TARGET run_medium_term POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${OpenCV_BIN_DIR}/${OpenCV_DLL_NAME}"
            "$<TARGET_FILE_DIR:run_medium_term>"
        COMMENT "拷贝 OpenCV 运行库到输出目录")
else()
    message(WARNING "未在 ${OpenCV_BIN_DIR} 找到完整 OpenCV DLL，已跳过自动拷贝；运行时请把该目录加入 PATH。")
endif()
