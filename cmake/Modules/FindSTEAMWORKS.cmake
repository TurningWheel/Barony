# - Try to find the steamworks library
#
# Once done, this will define:
#
#  STEAMWORKS_INCLUDE_DIR - the Steamworks include directory
#  STEAMWORKS_LIBRARIES - The libraries needed to use Steamworks

if (NOT STEAMWORKS_INCLUDE_DIR OR NOT STEAMWORKS_LIBRARIES)
	set(LIB_SEARCH_PATHS
		~/Library/Frameworks
		/Library/Frameworks
		/usr/lib
		/usr/lib64
		/usr/local/lib
		/usr/local/lib64
		$ENV{STEAMWORKSROOT}/sdk/redistributable_bin/linux64 #I don't like this. TODO: Make it determine 64/32 bit automatically.
		$ENV{STEAMWORKS_ROOT}/sdk/redistributable_bin/linux64
		$ENV{STEAMWORKS_DIR}/sdk/redistributable_bin/linux64
		#$ENV{STEAMWORKSROOT}/sdk/redistributable_bin/linux32
		#$ENV{STEAMWORKS_ROOT}/sdk/redistributable_bin/linux32
		#$ENV{STEAMWORKS_DIR}/sdk/redistributable_bin/linux32
	)
	FIND_PATH(STEAMWORKS_INCLUDE_DIR steam/steam_api.h
		/usr/include
		/usr/local/include
		$ENV{STEAMWORKSROOT}/sdk/public/
		$ENV{STEAMWORKS_ROOT}/sdk/public/
		$ENV{STEAMWORKS_DIR}/sdk/public/
		DOC "Include path for Steamworks"
	)

	if (Windows)
		if (BIT_32) #Need a better way of determining bits needed. Maybe have (LIBRARIES_32_BIT and LIBRARIES_64_BIT)?
			FIND_LIBRARY(STEAMWORKS_LIBRARY NAMES steam_api
				PATHS
				$ENV{STEAMWORKS_ROOT}/sdk/redistributable_bin
				DOC "Steamworks library name"
			)
			MESSAGE("32 bit steam")
		else ()
			FIND_LIBRARY(STEAMWORKS_LIBRARY NAMES steam_api64
				PATHS
				$ENV{STEAMWORKS_ROOT}/sdk/redistributable_bin/win64
				DOC "Steamworks library name"
			)
			MESSAGE("64 bit steam")
		endif()
	elseif (APPLE)
		FIND_LIBRARY(STEAMWORKS_LIBRARY NAMES steam_api
			PATHS
			$ENV{STEAMWORKS_ROOT}/sdk/redistributable_bin
			$ENV{STEAMWORKS_ROOT}/sdk/redistributable_bin/osx32
			DOC "Steamworks library name"
		)
	else ()
		FIND_LIBRARY(STEAMWORKS_LIBRARY NAMES steam_api
			PATHS ${LIB_SEARCH_PATHS}
			DOC "Steamworks library name"
		)
	endif ()
	if (STEAMWORKS_LIBRARY)
		set(STEAMWORKS_LIBRARIES ${STEAMWORKS_LIBRARY})
	endif ()
	MARK_AS_ADVANCED(STEAMWORKS_INCLUDE_DIR STEAMWORKS_LIBRARIES)
endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(STEAMWORKS DEFAULT_MSG STEAMWORKS_INCLUDE_DIR STEAMWORKS_LIBRARIES)