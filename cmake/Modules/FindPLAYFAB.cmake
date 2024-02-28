# - Try to find the PlayFab C++ SDK
#
# Once done, this will define:
#
#  PLAYFAB_INCLUDE_DIR - the PlayFab include directory
#  PLAYFAB_LIBRARIES - The libraries needed to use PlayFab

if (NOT PLAYFAB_INCLUDE_DIR OR NOT PLAYFAB_LIBRARIES)
	set(LIB_SEARCH_PATHS
		~/Library/Frameworks
		/Library/Frameworks
		/usr/lib
		/usr/lib64
		/usr/local/lib
		/usr/local/lib64
		$ENV{PLAYFAB_ROOT}/SDK
		$ENV{PLAYFAB_DIR}/SDK
	)
	FIND_PATH(PLAYFAB_INCLUDE_DIR PlayFabAdminApi.h
		/usr/include
		/usr/local/include
		$ENV{PLAYFAB_ROOT}/include/playfab
		$ENV{PLAYFAB_DIR}/include/playfab
		DOC "Include path for PlayFab"
	)

	if (Windows)
		FIND_LIBRARY(PLAYFAB_LIBRARY NAMES playfab.lib
			PATHS
			$ENV{PLAYFAB_ROOT}/lib
			$ENV{PLAYFAB_DIR}/lib
			DOC "PlayFab library name"
		)
	elseif (APPLE)
		FIND_LIBRARY(PLAYFAB_LIBRARY NAMES playfab.a
			PATHS
			$ENV{PLAYFAB_ROOT}/lib
			$ENV{PLAYFAB_DIR}/lib
			DOC "PlayFab library name"
		)
	else ()
		FIND_LIBRARY(PLAYFAB_LIBRARY NAMES playfab.a
			PATHS
			$ENV{PLAYFAB_ROOT}/lib
			$ENV{PLAYFAB_DIR}/lib
			DOC "PlayFab library name"
		)
	endif ()
	if (PLAYFAB_LIBRARY)
		set(PLAYFAB_LIBRARIES ${PLAYFAB_LIBRARY})
	endif ()
	MARK_AS_ADVANCED(PLAYFAB_INCLUDE_DIR PLAYFAB_LIBRARIES)
endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PLAYFAB DEFAULT_MSG PLAYFAB_INCLUDE_DIR PLAYFAB_LIBRARIES)
