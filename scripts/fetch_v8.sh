#!/bin/bash
echo "Fetching V8"

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

gclient &> /dev/null

ORIGINAL_DIR="$PWD"

cd "$ROOT_DIR/deps"

if [ -d "v8" ]
then
    echo "Found v8 locally."
else
    echo "v8 not found. Installing..."
    fetch v8
fi

cd ./v8
gclient sync

V8_VERSION=`curl -s https://omahaproxy.appspot.com/v8.json | grep -o '"v8_version": "9.[0-9]*' | grep -o '9.[0-9]*'`
V8_VERSION=${V8_VERSION}
echo "Checking out V8 version: $V8_VERSION"

git checkout branch-heads/$V8_VERSION
cd "$ORIGINAL_DIR"
