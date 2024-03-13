# Building in the CLion IDE
This document is a brief summary of what was required to build the project from within the CLion IDE on Windows,
however sections may be added for other environments as reliable build processes are determined. I love JetBrains IDEs, 
so I was excited to start modding Barony in CLion. Ultimately setting up the build process wasn't too difficult, but
it took some head scratching, so I am documenting what I learned so that others may learn, too.

<!-- TOC -->
* [Building in the CLion IDE](#building-in-the-clion-ide)
  * [Building for Windows x64 on Windows 10 (No DRM)](#building-for-windows-x64-on-windows-10-no-drm)
    * [Fork and Checkout](#fork-and-checkout)
    * [Visual Studio Dev Tools](#visual-studio-dev-tools)
    * [Dependencies](#dependencies)
      * [FMOD](#fmod)
    * [Toolchains](#toolchains)
      * [Visual Studio Toolchain](#visual-studio-toolchain)
    * [CMake Profiles](#cmake-profiles)
    * [CMake](#cmake)
      * [Issues](#issues)
    * [Build](#build)
      * [Build Configuration](#build-configuration)
      * [Building](#building)
      * [Issues](#issues-1)
    * [Playing the modded game](#playing-the-modded-game)
      * [Issues](#issues-2)
  * [Moving forward](#moving-forward)
<!-- TOC -->

## Building for Windows x64 on Windows 10 (No DRM)
The following procedure has produced a working build system for a Windows machine, creating `barony.exe` from source
without errors. It might work for other versions of windows, or other build targets, but don't bet your life on it.

If you are building for Steam or Epic Games, there might be some other little snags you'll have to figure out. If so, 
best of luck!

### Fork and Checkout
If you are here, hopefully you know how to use Git, or at least GitHub. It's a good idea to fork the Barony project
and then clone it to your local machine. Opening the resulting folder with CLion should get you started. Nothing special
is needed here.

### Visual Studio Dev Tools
In order to compile the `barony.exe` and run `cmake` without too many bumps we are going to need some of the build tools
from Microsoft Visual Studio.

Running the following command in Windows PowerShell will start a download of a couple GB including build tools for 
C / C++. It may not be the minimal installation, but it's the smallest one I have found that works so far.
```shell
winget install Microsoft.VisualStudio.2022.BuildTools --force --override "--wait --passive --add Microsoft.VisualStudio.Component.VC.Tools.x86.x64 --add Microsoft.VisualStudio.Component.Windows11SDK.22621 --add Microsoft.VisualStudio.ComponentGroup.NativeDesktop.Core --add Microsoft.Component.MSBuild --add Microsoft.VisualStudio.Component.Windows10SDK --add Microsoft.VisualStudio.Component.VC.Tools.arm --add Microsoft.VisualStudio.Component.VC.Tools.arm64 --add Microsoft.VisualStudio.ComponentGroup.UWP.VC"
```

### Dependencies
In order to build the project, you will need the various libraries it depends on to function. There are quite a few ways
to get these, but the simplest way is to Download the latest dependency package from Turning Wheel - which can be found 
as a pinned message in the `#-opensource-chat` channel in the [Barony Discord Server](https://discord.gg/QybDAHZz)

Once you have this zip file of dependencies, you will need to extract the contents and place them in a folder somewhere.
The specific location and the name of the folder doesn't matter, but we will refer to this location as 
`BARONY_WIN32_LIBRARIES`. 

This folder should contain two sub-folders which contain library (.lib) and header (.h) files. We want to rename the 
folder containing the `****.h` files to `include`. The folder containing the `****.lib` files should be renamed to 
`lib`.

There's one last step we need to go through for dependencies to be all good to go. Inside the lib folder 
(we just renamed) there should be a few different `png.lib, libpng.lib, libpng16.lib etc`. I found that only one of
these was needed, and in fact, the others could screw up the build if they didn't match the architecture you're building 
for. This is because the CMake configuration just uses the first one it finds.

Since I was building for an x64 system, I deleted all of these png related libraries except `libpng16.lib`. I left the 
png related `.dll` files alone. If you are building for a different architecture, you might have to guess and check.

#### FMOD
You will need to download and install FMOD separately from the other dependencies. In order to get FMOD, go to the
[FMOD website](https://www.fmod.com/) create an account, and download the FMOD Engine. I got version 2.02.16 but other
readmes should be able to tell you what versions are supported if not that one. Wherever you download and install it, 
remember that location! We'll need to reference it later!

### Toolchains
The toolchain is what you - uh... See, a toolchain is like... Uh... Okay just trust me, you need a toolchain.

I have had luck using both MinGW and VisualStudio toolchains but the setup for
each is pretty similar. I am going to focus on setting up the Visual Studio toolchain since we downloaded the
prerequisites for it earlier, and it will be good to create a toolchain other than the default one.

#### Visual Studio Toolchain
In CLion, go to `Settings` -> `Build, execution, deployment` -> `Toolchains`. Once there, you may have existing 
toolchains in the list. Ignore them and hit the little `+` icon to create a new one. From the dropdown, select 
"Visual Studio" and, if you successfully installed the visual studio components from earlier, CLion should just 
auto-populate the info it needs.

Now that you have a Visual Studio toolchain created, use the little arrow buttons to move it to the top of the list, 
making it the default.

### CMake Profiles
In CLion, go to `Settings` -> `Build, execution, deployment` -> `CMAKE`. A default profile may already exist, and if it 
does, feel free to edit it. Otherwise, create a new profile. You can name this profile whatever you like, and you should
be able to select a `Build Type` `Toolchain` and `Generator`.

The `Build Type` is just the sort of barony executable you want to make, I have only tested the debug and release types,
but it should work the same for all of them. `Release` is presumably the normal one!

The `Toolchain` for this CMake Profile will probably automatically have been set to default, which very well could be 
the Visual Studio toolchain we just created. Make sure that it is the one we created.

The `Generator` can be set to `Let CMake decide` and that should work fine. I have also had success with `Ninja`.

Next we'll want to set a `Build Directory`. This is up to you, but I decided to set it to a separate folder from the
project so that I didn't have to worry about my CMake files showing up in version control.

Finally, we'll want to set some `Environment Variables` which can be conveniently set right here on our CMake profile.
We only need to define two. The text box should have a nice convenient button on the right side to enter these in as 
separate items.
1. BARONY_WIN32_LIBRARIES = `Set this to your BARONY_WIN32_LIBRARIES folder we talked about earlier`
2. FMOD_DIR = `Set this to your FMOD installation folder - the one containing bin, api, doc, plugins, etc.`

### CMake
Now you're basically all set up. You should have a little CMake tab on the left side of CLion (little triangle thing)
which should have a tab within it showing your default CMake profile. It may have already built, but just in case, 
you can click on the little gear icon in there and select `reset cache and reload project`. This puts all your build
files together for making the `barony.exe`.

#### Issues
I ran into issues here occasionally (not when following the above instructions). If you got the wrong dependency zip or
if dependencies you need were missing, you'd see errors here about not finding libraries. Try to figure out what you're
missing and maybe double-check the source you got your libraries from.

### Build
Hopefully CMake went well, and now it's time to build. In the upper right third of CLion you should have a CMake profile
dropdown, and next to that, some build configurations.

#### Build Configuration
I didn't actually have to set up build configurations to get this to work, but in case yours aren't there or aren't set
up by default (or maybe CMake populated them) here's what I have:
1. Barony: 
   1. Target: barony (with a little cmd prompt icon)
   2. Executable: barony (with a little cmd prompt icon)
   3. Working directory: Blank, but I actually set this to a specific folder where I wanted my Exes to end up.
2. Editor:
   1. Same stuff as Barony but with the word "editor".

#### Building
All that was left at that point was to select the `barony` build config and hit the little hammer icon.

#### Issues
I ran into a few transient issues here (not after following the above process), so I'll just list some symptoms and 
what I identified as the cause.
1. Got a lot of warning output and interspersed with those warnings were a couple errors saying that `std::clamp` wasn't
   a real thing. The root cause was that I wasn't building with c++ version 17. The fix was making sure my visual studio
   toolchain was detecting version 17 in the little space below the path to the visual studio build tools. Also, the
   [CMakeLists.txt] file should be specifying that version 17 be used, like so: `set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -std=c++17")`
2. Got a lot of errors at the end of the build saying that linked libraries didn't exist. This was caused by a couple 
   little issues with CMakeLists.txt that I am fixing in this commit. Also, when I had selected the wrong version of the
   png library (the x86 one instead of x64) it showed a lot of png related functions not being found.

### Playing the modded game
Once you're done, you have to go and put your EXE with all its friends - the game resources. You will find these in your
game install directory either under Steam, Epic Games, or whatever. Place the EXE with those resources (or vice versa)
and launch that new `barony.exe`

#### Issues
The only issues I hit at this point were popup errors saying DLLs couldn't be found. This just happens when you try to
run the .exe without being in the same folder as all the game resources. 

## Moving forward
Please contribute to this document if you learn anything more about building Barony with CLion! 