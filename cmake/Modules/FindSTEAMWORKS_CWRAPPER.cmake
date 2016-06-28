# - Try to find the Steamworks C Wrapper library
# Once done this will define
#
#  STEAMWORKS_CWRAPPER_FOUND - system has the Steamworks C Wrapper
#  STEAMWORKS_CWRAPPER_INCLUDE_DIR - the Steamworks C Wrapper include directory
#  STEAMWORKS_CWRAPPER_LIBRARIES - The libraries needed to use the Steamworks C Wrapper

if (NOT STEAMWORKS_CWRAPPER_INCLUDE_DIR OR NOT STEAMWORKS_CWRAPPER_LIBRARIES)
	set(LIB_SEARCH_PATHS
		~/Library/Frameworks
		/Library/Frameworks
		/usr/lib
		/usr/lib64
		/usr/local/lib
		/usr/local/lib64
		$ENV{STEAMWORKS_CWRAPPERROOT}/lib
		$ENV{STEAMWORKS_CWRAPPER_ROOT}/lib
		$ENV{STEAMWORKS_CWRAPPER_DIR}/lib
	)
	FIND_PATH(STEAMWORKS_CWRAPPER_INCLUDE_DIR steamworks_cwrapper/steam_wrapper.h
		/usr/include
		/usr/local/include
		$ENV{STEAMWORKS_CWRAPPERROOT}/include
		$ENV{STEAMWORKS_CWRAPPER_ROOT}/include
		$ENV{STEAMWORKS_CWRAPPER_DIR}/include
		$ENV{STEAMWORKS_CWRAPPER_DIR}/inc
		[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\8.0\\Setup\\VC]/PlatformSDK/Include
		DOC "Include path for the Steamworks C Wrapper"
	)

	FIND_LIBRARY(STEAMWORKS_CWRAPPER_LIBRARY NAMES steamworks_cwrapper
		PATHS ${LIB_SEARCH_PATHS}
		DOC "Steamworks C Wrapper library name"
	)

	if (STEAMWORKS_CWRAPPER_LIBRARY)
		set(STEAMWORKS_CWRAPPER_LIBRARIES ${STEAMWORKS_CWRAPPER_LIBRARY})
	endif ()
	MARK_AS_ADVANCED(STEAMWORKS_CWRAPPER_INCLUDE_DIR STEAMWORKS_CWRAPPER_LIBRARIES)
endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(STEAMWORKS_CWRAPPER DEFAULT_MSG STEAMWORKS_CWRAPPER_INCLUDE_DIR STEAMWORKS_CWRAPPER_LIBRARIES)
