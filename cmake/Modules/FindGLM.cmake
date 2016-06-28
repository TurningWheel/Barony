FIND_PATH(GLM_INCLUDE_DIR glm/glm.hpp
   /usr/include
   /usr/local/include
   $ENV{GLMROOT}
   $ENV{GLM_ROOT}
   $ENV{GLM_DIR}
   [HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\8.0\\Setup\\VC]/PlatformSDK/Include
)

set(GLM_LIBRARY_DIR )

if(GLM_INCLUDE_DIR)
	set(GLM_FOUND "YES")
    #message(STATUS "Found GLM!")
endif(GLM_INCLUDE_DIR)