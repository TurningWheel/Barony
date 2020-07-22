# - Try to find the Rapid JSON header library.
#
# Once done, this will define:
#
#  RAPID_JSON_INCLUDE_DIR - the Rapid JSON include directory

if (NOT RAPID_JSON_INCLUDE_DIR OR NOT RAPID_JSON_LIBRARIES)
	set(LIB_SEARCH_PATHS
		$ENV{RAPID_JSONROOT}/SDK
		$ENV{RAPID_JSON_ROOT}/SDK
		$ENV{RAPID_JSON_DIR}/SDK
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

	MARK_AS_ADVANCED(RAPID_JSON_INCLUDE_DIR)
endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RAPID_JSON DEFAULT_MSG RAPID_JSON_INCLUDE_DIR )