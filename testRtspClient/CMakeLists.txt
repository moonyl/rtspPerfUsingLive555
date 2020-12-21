# CMakeList.txt: rtspPerfUsingLive555에 대한 CMake 프로젝트, 여기에 소스를 포함하고
# 프로젝트 특정 논리를 정의합니다.
#
cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 17)

find_package(OpenSSL REQUIRED SSL Crypto)

find_library(liveMediaLib liveMedia REQUIRED)
find_library(groupsockLib groupsock REQUIRED)
find_library(UsageEnvironmentLib UsageEnvironment REQUIRED)
find_library(BasicUsageEnvironmentLib BasicUsageEnvironment REQUIRED)
find_path(live555Header UsageEnvironment.hh REQUIRED)

# 이 프로젝트의 실행 파일에 소스를 추가합니다.
add_executable (testRTSPClient testRTSPClient.cpp)
add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

set(testRTSPClientLib
	${liveMediaLib} 
	${BasicUsageEnvironmentLib}
	${UsageEnvironmentLib}
	${groupsockLib}
    OpenSSL::SSL 
	OpenSSL::Crypto
	)

if (WIN32)
	list(APPEND testRTSPClientLib ws2_32)
endif (WIN32)

target_link_libraries(testRTSPClient PRIVATE
	${testRTSPClientLib}
)
target_include_directories(testRTSPClient
	PRIVATE
	${live555Header}	
)

# TODO: 필요한 경우 테스트를 추가하고 대상을 설치합니다.
#install(TARGETS RtspMediaServer OpenSSL::SSL OpenSSL::Crypto
#		RUNTIME
#		DESTINATION bin)


#find_file(openssl_ssl bin/libssl-1_1-x64.dll)
#find_file(openssl_crypto bin/libcrypto-1_1-x64.dll)
# TODO: 필요한 경우 테스트를 추가하고 대상을 설치합니다.

