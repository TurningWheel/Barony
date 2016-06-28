INTRODUCTION
	
	Barony is a first-person action role-playing game with several roguelike elements.
	You can play it in singleplayer or multiplayer modes.

ARGUMENTS
	
	-windowed        -  Forces the game to start in a window.
	-size=???x???    -  Sets the display resolution.
	-map=???         -  Chooses a map to run on startup as opposed to reading the first line from levels.txt. Filetype can be included/excluded at will.
	-gen=???         -  Generates a dungeon to run on startup as opposed to reading the first line from levels.txt.
	-config=???      -  Chooses a config file to execute on startup as opposed to simply running 'default.cfg'.
	-quickstart=???  -  Bypasses the menu/character creation process and simply starts the game with the player as the specified class.

	Ex:
		game -windowed -size=960x600 -map=test -quickstart=barbarian

	The above command starts the game in a window at 960x600 and quickstarts the map "test.lmp" as a barbarian.

CONSOLE COMMANDS

	The following is an inexhaustive list of commands which can be entered during the game to achieve certain effects:

	/ping
	Pings the server and returns the roundtrip time in milliseconds.

	/kick ???
	Kicks the given player from the game (server only)

	/spawnitem ???
	Spawns an item with the name given in '???' at the player's feet. If the full name is not entered, the first closest match will be spawned.

	/spawnbook ???
	Spawns a readable book with the title given in '???'

	/savemap ???
	saves the current map data as a file whose name is specified in '???'.

	/nextlevel
	Moves all players to the next level (server only)

	/pos
	Returns the current camera position and orientation.

	/exit
	Quits the whole game without confirmation.

	/showfps
	Prints the current frames-per-second (fps) number.

	/noclip
	Toggles noclipping mode. Only works on server and singleplayer mode.

	/dowse
	Returns the locations of all exit ladders on the level.

	/thirdperson
	Toggles thirdperson mode (detaches camera from player).

	/res ???x???
	Sets the screen resolution to the given number (eg 1280x720)

	/rscale ???
	Sets the rscale variable to the number given in '???' (unused)

	/smoothlighting
	Toggles smooth lighting.

	/fullscreen
	Toggles fullscreen (not applied immediately)

	/shaking
	Toggles camera shaking.

	/bobbing
	Toggles camera bobbing.

	/sfxvolume ???
	Sets the sound effects volume to the number given in '???'

	/musvolume ???
	Sets the music volume to the number given in '???'

	/bind X Y
	Binds a key given in X to an action given in Y. For examples, see the included default.cfg file.
	You can refer to this list for SDL's decimal value for a given key: https://wiki.libsdl.org/SDLScancodeLookup

	/mousespeed ???
	Sets the mouse speed to the number given in '???'

	/reversemouse
	Toggles the inversion of all vertical mouse movement.

	/smoothmouse
	Toggles the mouse smoothing feature.

	/mana
	Fills magic to the maximum (singleplayer only)

	/heal
	Fills health to the maximum (singleplayer only)

	/ip ???
	Sets the last ip to the address given in '???'

	/port ???
	Sets the last port number to the one given in '???'

	/noblood
	Toggles the blood option.

	/gamma ???
	Sets the gamma to the number given in '???'

	/capturemouse
	Toggles the capture of mouse input.

	/levelup
	Levels up the player immediately (singleplayer only)

	/hunger ???
	Sets the player's hunger level to the number given in '???' (singleplayer only)

	/testsound ???
	Plays the sound stored at index '???'

	/skipintro
	Toggles intro skipping functionality.

	/levelmagic
	Increases the player's magic skills (singleplayer only)

	/numentities
	Reports the number of entities in the level

	/killmonsters
	Kills all monsters on the level (singleplayer only)

MULTIPLAYER

	These instructions apply only to the non-Steam version of Barony:

	In order to play Barony online or over a network, the port specified by the server must be open on both the clients and server for both the UDP and TCP protocols for the game to work.
	
	You may find tools like Hamachi useful for setting up internet games if you can't figure out how to forward all your ports correctly. Additionally you can try playing the game on a LAN to bypass all that nonsense.

	Remember: when creating a game, the server only needs to enter a port number. The CLIENTS however must enter both the server's address and the port together, separated by a colon. So if the server's address is 192.168.1.1 and the port is 12345, you would enter:

	192.168.1.1:12345

	into the connection box, and press "Join" to connect.

	You wouldn't believe how many emails we've received about this... :)

CONTACT

	All suggestions, comments, and questions should be sent to: contact@baronygame.com

	Barony's official website can also be found at http://www.baronygame.com/
