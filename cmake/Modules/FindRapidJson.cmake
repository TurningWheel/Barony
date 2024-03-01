# - Try to find the Rapid JSON header library.
#
# Once done, this will define:
#
#  RAPID_JSON_INCLUDE_DIR - the Rapid JSON include directory

if (NOT RAPID_JSON_INCLUDE_DIR OR NOT RAPID_JSON_LIBRARIES)
	set(LIB_SEARCH_PATHS
		$ENV{RAPID_JSONROOT}/
		$ENV{RAPID_JSON_ROOT}/
		$ENV{RAPID_JSON_DIR}/
		~/Library/Frameworks
		/Library/Frameworks
		/usr/lib
		/usr/lib64
		/usr/local/lib
		/usr/local/lib64
	)
	FIND_PATH(RAPID_JSON_INCLUDE_DIR rapidjson/rapidjson.h
		HINTS
		$ENV{LIB_SEARCH_PATHS}
		PATH_SUFFIXES include
		PATHS ${LIB_SEARCH_PATHS}
		DOC "Include path for Rapid JSON"
	)
endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RapidJson DEFAULT_MSG RAPID_JSON_INCLUDE_DIR )
