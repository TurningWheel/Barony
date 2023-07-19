#!/bin/bash

# 1) Fetch the dependencies

# But first, check if we should even fetch the dependencies at all.
download_zip=true
if [ $# -ne 0 ]; then
	if [ $1 = "--dontDownloadZip" ]; then
		download_zip=false
	fi
fi

if [ $download_zip = true ]; then
	if [ -z $DEPENDENCIES_ZIP_KEY ]; then
		echo "Error! No key provided for encrypted dependencies zip. Aborting."
		exit 1
	fi
	if [ -z $DEPENDENCIES_ZIP_IV ]; then
		echo "Error! No IV provided for encrypted dependencies zip. Aborting."
		exit 1
	fi

	# 1.a) Create dependencies directory.
	mkdir ../dependencies
	cd ..
	export DEPENDENCIES_DIR=$(pwd)/dependencies/
	cd dependencies

	# 1.b) Fetch the encrypted dependencies zip.
	wget https://github.com/TurningWheel/Barony/releases/download/ci_deps_1.6/dependencies_linux.zip.enc -O dependencies.zip.enc
	RESULT=$?
	if [ $RESULT -ne 0 ]; then
		echo "Fetching encrypted dependencies zip failed. Aborting."
		exit $RESULT
	fi

	# 1.c) Provision encrypted dependencies zip. (password protected, pass in key & IV through an environment variable!!)
	openssl aes-256-ctr -d -in dependencies.zip.enc -out dependencies.zip -K $DEPENDENCIES_ZIP_KEY -iv $DEPENDENCIES_ZIP_IV
	RESULT=$?
	if [ $RESULT -ne 0 ]; then
		echo "Decrypting dependencies zip failed. Aborting."
		exit $RESULT
	fi
	unzip dependencies.zip
	RESULT=$?
	if [ $RESULT -ne 0 ]; then
		echo "Unzipping dependencies zip failed. Aborting."
		exit $RESULT
	fi
	cd ..
else
	echo "Skipping zip download..."
	cd ..
	export DEPENDENCIES_DIR=$(pwd)/dependencies/
fi

# 1.d) Set dependencies search paths.
export STEAMWORKS_ROOT="${DEPENDENCIES_DIR}/steamworks_sdk/"
export STEAMWORKS_ENABLED=1
export FMOD_DIR="${DEPENDENCIES_DIR}/fmod/"
export NFD_DIR="${DEPENDENCIES_DIR}/nfd/"
export RAPID_JSON_DIR="${DEPENDENCIES_DIR}/"
export PHYSFSDIR="${DEPENDENCIES_DIR}/physfs/"
export EDITOR_ENABLED=1
export GAME_ENABLED=0

# 2) Build from source

mkdir -p build/release
cd build/release

export OPTIMIZATION_LEVEL="-O2"
export CXX=g++
export CC=gcc
cmake -DCMAKE_BUILD_TYPE=Release -DFMOD_ENABLED=ON -G "Unix Makefiles" ../..
RESULT=$?
if [ $RESULT -ne 0 ]; then
  echo "CMAKE generation failed. Aborting."
  exit $RESULT
fi

make
RESULT=$?
if [ $RESULT -ne 0 ]; then
  echo "Compilation failed. Aborting."
  exit $RESULT
fi
