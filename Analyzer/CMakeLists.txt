################################################################################
# CMake 基本设置
################################################################################

################################################################################
# 导入要使用的模块配置
################################################################################

################################################################################
# 变量初始化
################################################################################

################################################################################
# 全局函数配置
################################################################################


################################################################################
# 项目设置
################################################################################

# 添加可执行文件
add_executable(StreamMediaAnalysisServerExecutable 
"main.cpp"
"src/config.cpp"
"src/log.cpp"
"src/executor.cpp"
"src/scheduler.cpp"
"src/server.cpp"
"src/stream.cpp"
)

################################################################################
# 注入宏定义
################################################################################

# add_definitions(-DNPY_NO_DEPRECATED_API=NPY_1_7_API_VERSION) # 禁止使用 Numpy 废弃的 API


################################################################################
# 子项目设置
################################################################################

# add_dependencies(Analyzer
#     AVSAlgorithm
#     AVSAlarmManage
# )

find_package(Python3 COMPONENTS Development REQUIRED)
find_package(FFMPEG REQUIRED)
find_package(CURL CONFIG REQUIRED)
find_package(libjpeg-turbo CONFIG REQUIRED)
find_package(jsoncpp CONFIG REQUIRED)
find_package(Libevent CONFIG REQUIRED)
find_package(OpenCV REQUIRED)
find_package(ZLIB REQUIRED)

################################################################################
# 头文件库设置
################################################################################

target_include_directories(StreamMediaAnalysisServerExecutable
          INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>$<INSTALL_INTERFACE:include>) # 构建时使用使用当前源码路径作为头文件搜索路径, 安装时使用 include 作为头文件搜索路径

# 设置头文件搜索路径
set(PUBLIC_INCLUDE_DIRS
"${PROJECT_BINARY_DIR}"
"${CMAKE_CURRENT_SOURCE_DIR}/include"
# "${CMAKE_SOURCE_DIR}/3rdparty/numpy/release/x86_64/lib/site-packages/numpy-1.24.3-py3.9-win-amd64.egg/numpy/core/include" # Numpy_INCLUDE_DIRS
"D:/vcpkg/installed/x64-windows/include"
)

set(PRIVATE_INCLUDE_DIRS
"${CMAKE_SOURCE_DIR}/AlgorithmManage/include"
"${CMAKE_SOURCE_DIR}/ResultManage/include"
"${CMAKE_SOURCE_DIR}/StreamManage/include"
"${CMAKE_SOURCE_DIR}/TaskManage/include"
)


target_include_directories(StreamMediaAnalysisServerExecutable 
  PUBLIC ${PUBLIC_INCLUDE_DIRS}
  PRIVATE ${PRIVATE_INCLUDE_DIRS}
  )



################################################################################
# 依赖库设置
################################################################################

# 链接子项目的依赖库
# target_link_libraries(Analyzer PRIVATE AVSAlgorithm)
# target_link_libraries(Analyzer PRIVATE AVSAlarmManage)

set(PUBLIC_LINK_LIBRARIES 

"$<$<CONFIG:Debug>:"
""
">"
"$<$<CONFIG:Release>:"
""
">"
# "${CMAKE_SOURCE_DIR}/3rdparty/numpy/release/x86_64/lib/site-packages/numpy-1.24.3-py3.9-win-amd64.egg/numpy/core/lib/npymath.lib"
)


set(PUBLIC_LINK_LIBRARIES "${PUBLIC_LINK_LIBRARIES};ws2_32.lib")


set(PRIVATE_LINK_LIBRARIES "")

target_link_libraries(StreamMediaAnalysisServerExecutable 
  PUBLIC ${PUBLIC_LINK_LIBRARIES}
  PRIVATE ${PRIVATE_LINK_LIBRARIES})

target_link_directories(StreamMediaAnalysisServerExecutable PRIVATE ${FFMPEG_LIBRARY_DIRS})
target_link_libraries(StreamMediaAnalysisServerExecutable PRIVATE ${FFMPEG_LIBRARIES})
# target_link_libraries(StreamMediaAnalysisServerExecutable PRIVATE CURL::libcurl)
target_link_libraries(StreamMediaAnalysisServerExecutable PRIVATE libjpeg-turbo::turbojpeg)
target_link_libraries(StreamMediaAnalysisServerExecutable PRIVATE JsonCpp::JsonCpp)
# target_link_libraries(StreamMediaAnalysisServerExecutable PRIVATE Python3::Python)
target_link_libraries(StreamMediaAnalysisServerExecutable PRIVATE libevent::core libevent::extra) 
target_link_libraries(StreamMediaAnalysisServerExecutable PRIVATE ZLIB::ZLIB)


################################################################################
# 文件预处理
################################################################################

set(DLL_FILES 
# "$<TARGET_FILE_DIR:jsoncpp_lib>/jsoncpp.dll"
# "$<TARGET_FILE_DIR:opencv_world>/opencv_world4.dll"
# "$<TARGET_FILE_DIR:CURL::libcurl>/libcurl.dll"
# "$<TARGET_FILE_DIR:libjpeg-turbo::turbojpeg>/turbojpeg.dll"

# "$<TARGET_FILE_DIR:libevent::core>/event_core.dll"
# "$<TARGET_FILE_DIR:libevent::core>/event_extra.dll"

# "D:/vcpkg/installed/x64-windows/bin/avcodec-59.dll"
# "D:/vcpkg/installed/x64-windows/bin/avdevice-59.dll"
# "D:/vcpkg/installed/x64-windows/bin/avfilter-8.dll"
# "D:/vcpkg/installed/x64-windows/bin/avformat-59.dll"
# "D:/vcpkg/installed/x64-windows/bin/avutil-57.dll"
# "D:/vcpkg/installed/x64-windows/bin/swresample-4.dll"
# "D:/vcpkg/installed/x64-windows/bin/swscale-6.dll"

"${CMAKE_CURRENT_SOURCE_DIR}/config.json"
)

message(STATUS "DLL_FILES: ${DLL_FILES}")

# 拷贝 dll 文件
add_custom_command(TARGET StreamMediaAnalysisServerExecutable POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${DLL_FILES}
        $<TARGET_FILE_DIR:StreamMediaAnalysisServerExecutable>
)
add_custom_target(copy_dlls ALL DEPENDS StreamMediaAnalysisServerExecutable)

################################################################################
# 打包规则
################################################################################


# ################################################################################
# # 安装规则
# ################################################################################

