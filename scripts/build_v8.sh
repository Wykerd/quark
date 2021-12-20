#!/bin/bash
echo "Building V8"

if ! command -v git >/dev/null &> /dev/null; then
  echo "Error: git is not installed."
  exit 1
fi

ROOT_DIR=$(git rev-parse --show-toplevel)
ROOT_DIR="${ROOT_DIR}"

# Check that command was successful.
if [ "$ROOT_DIR" == "" ]; then
  echo "Error: Could not find the top-level directory."
  exit 1
else
    echo "Found the top-level directory: $ROOT_DIR"
fi

if ! command -v gclient &> /dev/null
then
    if [ -d "$ROOT_DIR/deps/depot_tools" ]
    then 
        echo "Found depot_tools locally."
        export PATH="$ROOT_DIR/deps/depot_tools:$PATH"
    else
        echo "depot_tools not found. Installing..."
        git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git "$ROOT_DIR/deps/depot_tools"
        export PATH=$ROOT_DIR/deps/depot_tools:$PATH
    fi
else
    echo "depot_tools found in PATH."
fi

ORIGINAL_DIR="$PWD"

cd "$ROOT_DIR/deps/v8"

# get cpu architecture
ARCH=`uname -m`

TARGET_ARCH="$ARCH"

if [[ $ARCH == x86_64* ]]
then
    TARGET_ARCH="x64"
elif [[ $ARCH == i*86 ]]
then
    TARGET_ARCH="x86"
elif [[ $ARCH == arm64 ]] || [[ $ARCH == aarch64 ]]
then
    TARGET_ARCH="arm64"
elif [[ $ARCH == arm* ]]
then
    TARGET_ARCH="arm"
fi

echo "Target architecture: $TARGET_ARCH"

gn gen out/release --args="dcheck_always_on=false is_component_build=false is_debug=false use_custom_libcxx=false v8_monolithic=true v8_use_external_startup_data=false target_cpu=\"$TARGET_ARCH\""

ninja -C out/release v8_monolith

cd "$ORIGINAL_DIR"
