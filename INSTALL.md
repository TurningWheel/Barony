# Dependencies
You will need the following libraries to build Barony:

 * SDL2 (https://www.libsdl.org/download-2.0.php)
 * SDL2_image (https://www.libsdl.org/projects/SDL_image/)
 * SDL2_net (https://www.libsdl.org/projects/SDL_net/)
 * SDL2_ttf (https://www.libsdl.org/projects/SDL_ttf/)
 * libpng (http://www.libpng.org/pub/png/libpng.html)
 * libz (https://zlib.net/) used by libpng
 * PhysFS
 * RapidJSON
 * dirent.h (Comes with Linux/POSIX systems; need to acquire on Windows)
 * OpenGL
 * CMake (on Windows, use versions at least as new as 3.8.0)

OPTIONAL dependencies:
 * One of FMOD Core API 2.00.08 or OpenAL for audio support.

FMOD Studio API can be downloaded at https://www.fmod.com/download (you do need an account to download it).
You can disable FMOD by running cmake with -DFMOD_ENABLED=OFF (it's also disabled if not found).

OpenAL can be used with -DOPENAL_ENABLED
 
You will also need the following tools:

 * A working C++ compiler (Visual Studio, MinGW _(not officially supported, CMakeLists.txt may require modification)_, GCC, Clang, or xtools)
 * CMake
 * Linux users will also need Make, or whatever alternate you may generate build files for.

# Windows Instructions

## Acquire Dependencies

PhysFS:
 * Download from https://icculus.org/physfs/downloads/physfs-3.0.1.tar.bz2
 * Open with .7zip or similar, open up docs/INSTALL.txt inside the archive for instructions to read along with this short guide.
 * Download/install cmake-gui for Windows (or use a command line version if you have it -- you may select "Add CMake to the system PATH for all users/current user" during installation for ease of use)
 * Open up the physfs-3.0.1 directory in cmake-gui and select configure, then generate
 * You will now get Visual Studio files to open up and build
 * Open up files in Visual Studio, in Solution Explorer right click the 'physfs' solution and 'build'
 * You will now see a physfs.lib file in physfs-3.0.1/Release/ to use when building Barony. Put this in one of the VCC++ Library Directories folder for the Barony project.
 * Similarly you can find physfs-3.0.1/src/physfs.h to put in one of the VC++ Include Directories from the Barony project.

dirent.h is a POSIX header, you will need to obtain a Windows port. For example, from https://github.com/tronkko/dirent

//TODO: Where get OpenGL? Did you need to install the "Windows SDK" when setting up Visual Studio?

Download everything else. You may need to build things. More explicit instructions here would be nice.

### TODO: vcpkg something or other

## Building Barony

Visual Studio Instructions:
* Rather than individually setting up environment variables for every dependency, you can simply create an environment variable named `BARONY_WIN32_LIBRARIES` and point it to a combined dependencies folder. Said folder should have a subdirectory named `include` with all libraries' header files in there, and a `lib` subdirectory for the library archives themselves.
* After that, create a directory named `build` (or somesuch) in the root Barony directory, and either run `cmake ..` inside it from a command prompt, or use cmake-gui.
** There are some additional options you can specify, such as -DFMOD_ENABLED, -DSTEAMWORKS_ENABLED, -DEOS_ENABLED, -DOPENAL_ENABLED.
** If you do not specify one of FMOD or OpenAL enabled, the game will build without sound support.
* Open barony.sln and the standard Visual Studio experience is now all yours.
** Build the whole solution to generate the .exe files. (Make sure that the appropriate Platform Toolset is installed)
** If the environment variable `BARONY_DATADIR` is defined, it will be used as the current working directory. Set this to wherever you have all of the game's assets and just mash the "play" button in Visual Studio :)
*** If that variable doesn't exist, no worries, CMake will not modify the current working directory property. You'll either have to copy the executables to whever you have the assets yourself, or you can copy the assets over to the debugger's directory.
* MinGW probably not supported right now in the CMakeList.txt...PRs welcome :)
TODO: MinGW support, one master SLN with Steam, EOS, FMOD, OpenAL, etc variants all in one place.

If you're using MinGW or GCC, you'll need to run CMake first, then make: cmake . && make *(NOTE: Not supported, CMakeLists.txt is probably broken for MinGW support)*

If you are missing GL header files like glext.h they are available from https://www.khronos.org/registry/OpenGL/api/GL/.

### Running Barony in Visual Studio.

By default, the "barony" project is set as the startup project. (The startup project is the project Visual Studio will launch when you hit the green "play" (debug) button)

If you've defined the `BARONY_DATADIR` environment variable to point to wherever you have the game's assets, this will...mostly work. The missing piece is it won't copy over the lang/en.txt included in the source repo.

To do that, you must right click on the "INSTALL" project and hit "Build". This will build the source code and then copy over lang/en.txt, barony.exe, and editor.exe to BARONY_DATADIR/

After that, all you have to do is hit the big green play button, and the game should run.

If you'd rather debug the editor, instead of hitting the green play button up top, right click on the "editor" project, mouse over "debug", and then click "start new instance"

# Linux Instructions

## Acquire Dependencies

For Debian/Ubuntu, you should be able to install most of these dependencies with: //TODO: Add OpenAL to the list.
sudo apt-get install libsdl2-dev libsdl2-image-dev libsdl2-net-dev libsdl2-ttf-dev libpng-dev libz-dev libphysfs-dev rapidjson-dev

You will also need PhysFS v3.0.1 for Barony v3.1.5+ if not available in your distro's package repository.
Linux Install (Navigate to somewhere to drop install files first):
 * wget https://icculus.org/physfs/downloads/physfs-3.0.1.tar.bz2
 * bzip2 -d physfs-3.0.1.tar.bz2
 * tar -xvf physfs-3.0.1.tar
 * cd physfs-3.0.1
 * cmake ./
 * make install
You can then remove the installation files.

## Building Barony

You can do something along the following lines:
```
mkdir build
cd build
cmake ..
make -j
```

# Build Flags

TODO: Document all of them...

For Audio support, you may pass in one of the following to your cmake invocation, depending on which you installed:
`-DFMOD_ENABLED` or `-DOPENAL_ENABLED`

Steamworks or EOS support:
`-DSTEAMWORKS_ENABLED` and `-DEOS_ENABLED`

Example cmake invocation, for a build using Steamworks, EOS, and FMOD:
```
cmake -DCMAKE_BUILD_TYPE=Release -DFMOD_ENABLED=ON -DSTEAMWORKS_ENABLED=1 -DEOS_ENABLED=1 # TODO: Standardize ON and 1...
```
