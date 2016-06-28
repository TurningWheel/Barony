list(APPEND GAME_SOURCES
	"${CMAKE_CURRENT_SOURCE_DIR}/interface.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/screenshot.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/clickdescription.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/consolecommand.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/drawminimap.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/drawstatus.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/magicgui.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/updatecharactersheet.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/updaterightsidebar.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/updatechestinventory.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/identify_and_appraise.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/playerinventory.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/bookgui.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/shopgui.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/removecurse.cpp"
)
list(APPEND EDITOR_SOURCES
)

set(GAME_SOURCES ${GAME_SOURCES} PARENT_SCOPE)
set(EDITOR_SOURCES ${EDITOR_SOURCES} PARENT_SCOPE)
