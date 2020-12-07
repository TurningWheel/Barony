# - Try to find the EOS SDK
#
# Once done, this will define:
#
#  EOS_INCLUDE_DIR - the EOS include directory
#  EOS_LIBRARIES - The libraries needed to use EOS

if (NOT EOS_INCLUDE_DIR OR NOT EOS_LIBRARIES)
	set(LIB_SEARCH_PATHS
		~/Library/Frameworks
		/Library/Frameworks
		/usr/lib
		/usr/lib64
		/usr/local/lib
		/usr/local/lib64
		$ENV{EOSROOT}/SDK
		$ENV{EOS_ROOT}/SDK
		$ENV{EOS_DIR}/SDK
	)
	FIND_PATH(EOS_INCLUDE_DIR eos_sdk.h
		/usr/include
		/usr/local/include
		$ENV{EOSROOT}/SDK/Include
		$ENV{EOS_ROOT}/SDK/Include
		$ENV{EOS_DIR}/SDK/Include
		DOC "Include path for EOS"
	)

	if (Windows)
		if (BIT_32) #Need a better way of determining bits needed. Maybe have (LIBRARIES_32_BIT and LIBRARIES_64_BIT)?
			FIND_LIBRARY(EOS_LIBRARY NAMES EOSSDK-Win32-Shipping.lib
				PATHS
				$ENV{EOS_ROOT}/SDK/Lib
				DOC "EOS library name"
			)
			MESSAGE("32 bit EOS")
		else ()
			FIND_LIBRARY(EOS_LIBRARY NAMES EOSSDK-Win64-Shipping.lib
				PATHS
				$ENV{EOS_ROOT}/SDK/Lib
				DOC "EOS library name"
			)
			MESSAGE("64 bit EOS")
		endif()
	elseif (APPLE)
		FIND_LIBRARY(EOS_LIBRARY NAMES libEOSSDK-Mac-Shipping.dylib
			PATHS
			$ENV{EOS_ROOT}/SDK/Bin
			$ENV{EOS_DIR}/SDK/Bin
			DOC "EOS library name"
		)
	else () # TODO: Technically, Linux portion. I don't know what the EOS Linux SDK looks like yet, since the launcher doesn't even run on Linux, but we'll probably get to this eventually?
		FIND_LIBRARY(EOS_LIBRARY NAMES libEOSSDK-Linux-Shipping.so
			PATHS
			$ENV{EOS_ROOT}/SDK/Bin
			$ENV{EOS_DIR}/SDK/Bin
			DOC "EOS library name"
		)
	endif ()
	if (EOS_LIBRARY)
		set(EOS_LIBRARIES ${EOS_LIBRARY})
	endif ()
	MARK_AS_ADVANCED(EOS_INCLUDE_DIR EOS_LIBRARIES)
endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(EOS DEFAULT_MSG EOS_INCLUDE_DIR EOS_LIBRARIES)