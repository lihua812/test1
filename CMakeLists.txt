# CMakeList.txt: PnP1 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.10)

# 如果支持，请为 MSVC 编译器启用热重载。
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("PnP1")

# 将源代码添加到此项目的可执行文件。
# main.cpp 只保留 main() 入口，其余功能函数的实现放在 PnP1.cpp、声明放在 PnP1.h。
add_executable (PnP1 "main.cpp" "PnP1.cpp" "PnP1.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET PnP1 PROPERTY CXX_STANDARD 20)
endif()

# ---------------------------------------------------------------------------
# OpenCV 依赖
# ---------------------------------------------------------------------------
# 本机使用官方预编译包 OpenCV 4.1.4，它只提供 x64/vc16 的库。
# 因此仅在 64 位构建中接入 OpenCV；32 位预设直接跳过，避免因找不到库而 configure 失败。
if (CMAKE_SIZEOF_VOID_P EQUAL 8)
  # 允许通过命令行 -DOpenCV_DIR=... 覆盖；未指定时按候选顺序自动定位。
  # 预编译包含有两份 cmake 配置：build 顶层那份需要按 MSVC 版本推断架构（VS2022 下可能映射失败），
  # 而 x64/vc16/lib 下的配置直接可用，所以优先探测后者。
  if (NOT OpenCV_DIR)
    foreach(_candidate "D:/opencv/opencv/build/x64/vc16/lib" "D:/opencv/opencv/build")
      if (EXISTS "${_candidate}/OpenCVConfig.cmake")
        set(OpenCV_DIR "${_candidate}" CACHE PATH "OpenCV 的 cmake 配置目录（含 OpenCVConfig.cmake）")
        break()
      endif()
    endforeach()
  endif()
  if (NOT OpenCV_DIR)
    message(FATAL_ERROR "未找到 OpenCV：请用 -DOpenCV_DIR=<含 OpenCVConfig.cmake 的目录> 指定")
  endif()
  find_package(OpenCV REQUIRED CONFIG)

  message(STATUS "OpenCV ${OpenCV_VERSION} -> ${OpenCV_DIR}")
  # 头文件与目标库由 OpenCVConfig.cmake 提供，直接用其导出的变量即可，无需手写路径。
  target_include_directories(PnP1 PRIVATE ${OpenCV_INCLUDE_DIRS})
  target_link_libraries(PnP1 PRIVATE ${OpenCV_LIBS})
  # 供源码中的 #ifdef HAVE_OPENCV 做条件编译（32 位构建时不存在 OpenCV）。
  target_compile_definitions(PnP1 PRIVATE HAVE_OPENCV)

  # 预编译包里 bin 相对 lib 的位置并不固定，逐个候选探测真实的 DLL 目录。
  foreach(_candidate "${OpenCV_DIR}/x64/vc16/bin" "${OpenCV_DIR}/../bin" "${OpenCV_DIR}/bin")
    if (EXISTS "${_candidate}")
      get_filename_component(OpenCV_BIN_DIR "${_candidate}" ABSOLUTE)
      break()
    endif()
  endforeach()

  # 把 DLL 目录注入调试运行环境的 PATH，避免在 VS 中 F5 时提示缺少 opencv_world4xx.dll。
  if (OpenCV_BIN_DIR)
    set_target_properties(PnP1 PROPERTIES
      VS_DEBUGGER_ENVIRONMENT "PATH=${OpenCV_BIN_DIR};%PATH%")
  endif()
else()
  message(STATUS "32 位构建：本机 OpenCV 预编译包仅为 x64，已跳过 OpenCV 链接")
endif()

# TODO: 如有需要，请添加测试并安装目标。
