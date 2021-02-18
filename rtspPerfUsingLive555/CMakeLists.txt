# CMakeList.txt: rtspPerfUsingLive555에 대한 CMake 프로젝트, 여기에 소스를 포함하고
# 프로젝트 특정 논리를 정의합니다.
#

cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 17)

find_package(Boost REQUIRED)
find_package(OpenSSL REQUIRED SSL Crypto)
find_package(nlohmann_json CONFIG REQUIRED)

find_library(liveMediaLib liveMedia REQUIRED)
find_library(groupsockLib groupsock REQUIRED)
find_library(UsageEnvironmentLib UsageEnvironment REQUIRED)
find_library(BasicUsageEnvironmentLib BasicUsageEnvironment REQUIRED)
find_path(live555Header UsageEnvironment.hh REQUIRED)
#get_filename_component(live555HeaderPath ${live555Header} DIRECTORY )
#message("header:" ${live555Header} ${live555HeaderPath})
#message(${liveMediaLib} ${groupsockLib} ${UsageEnvironmentLib} ${BasicUsageEnvironmentLib})

# 이 프로젝트의 실행 파일에 소스를 추가합니다.
add_executable (rtspPerfUsingLive555
		rtspPerfUsingLive555.cpp
		DynamicTaskScheduler.h
		DynamicTaskScheduler.cpp
		TaskCommand.h
		PerfCheckRtspClient.cpp
		PerfCheckRtspClient.h
		RawStreamPerfCheckSink.cpp
		RawStreamPerfCheckSink.h
		OnConnectRequested.cpp
		OnConnectRequested.h
		OnDisconnectRequested.cpp
		OnDisconnectRequested.h
		OnRemoveOneRequested.cpp
		OnRemoveOneRequested.h
		Statistics.cpp
		Statistics.h
		RemoteCommandInterpreter.cpp
		RemoteCommandInterpreter.h
		LocalCommandInterpreter.cpp
		LocalCommandInterpreter.h
		PerformanceEnvironment.cpp
		PerformanceEnvironment.h
		)
if (WIN32)
add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif(WIN32)

#get_target_property(OpenSSL_LOCATION OpenSSL LOCATION)
message(STATUS ${LIB_EAY_RELEASE})
set(rtspPerfUsingLive555 	
	${liveMediaLib} 
	${BasicUsageEnvironmentLib}
	${UsageEnvironmentLib}
	${groupsockLib}
    OpenSSL::SSL 
	OpenSSL::Crypto
	Boost::boost
	nlohmann_json nlohmann_json::nlohmann_json
	)

if (WIN32)
	list(APPEND rtspPerfUsingLive555 ws2_32)
endif (WIN32)

target_link_libraries(rtspPerfUsingLive555 PRIVATE	
	${rtspPerfUsingLive555}
)
target_include_directories(rtspPerfUsingLive555
	PRIVATE
	${live555Header}	
)

if(${CMAKE_BUILD_TYPE} STREQUAL Debug)
	set (SEARCH_PATH ${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/Debug/bin/)
else()
	set (SEARCH_PATH ${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin/)
endif()

if (WIN32)
find_file(openssl_ssl
		libssl-1_1-x64.dll
		PATHS ${SEARCH_PATH}
		)
find_file(openssl_crypto
		libcrypto-1_1-x64.dll
		PATHS ${SEARCH_PATH}
		)
message(${SEARCH_PATH})
message("${openssl_ssl} ${openssl_crypto}")
endif(WIN32)

# TODO: 필요한 경우 테스트를 추가하고 대상을 설치합니다.
install(TARGETS rtspPerfUsingLive555
		RUNTIME
		DESTINATION bin)
install(FILES ${openssl_ssl} ${openssl_crypto}
		DESTINATION bin)

set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
if(${CMAKE_BUILD_TYPE} STREQUAL Debug)
	message("Debug")
	set(CMAKE_INSTALL_DEBUG_LIBRARIES TRUE)
endif()


include(InstallRequiredSystemLibraries)
install(PROGRAMS ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}
		DESTINATION bin)

set(CPACK_PACKAGE_NAME "rtspPerfUsingLive555Installer")
set(CPACK_PACKAGE_VENDOR "team2")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "rtspPerfUsingLive555")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "rtspPerfUsingLive555")

include(CPack)