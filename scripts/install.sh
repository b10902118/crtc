#!/bin/bash

# Initialize variables with defaults
APK="./1.9.2.apk"
DEST="./env/build_loader"
NO_BUILD=0
DOCKER_RUN_SCRIPT=0
PORTABLE_RUNTIME=0
GRAPHICS=0
DEBUG=0

# ensure it is run from the project root
PROJECT_ROOT=$(realpath "$(dirname "${BASH_SOURCE[0]}")/..")

if [[ "$(realpath .)" != "$PROJECT_ROOT" ]]; then
    echo "Error: CWD is '$(realpath .)', please run this script from the project root '$PROJECT_ROOT'"
    exit 1
fi

# determine arch by uname -m, if 86 in the string classify x86, else arm
if [[ $(uname -m) == *86* ]]; then
    ARCH="x86"
elif [[ $(uname -m) == *arm* || $(uname -m) == *aarch64* ]]; then
    ARCH="arm"
else
    ARCH=""
fi

function print_usage {
    echo "Usage: $0 [--apk <apk_file>] [--arch <arm|x86>] [--dest <dest_dir>]"
}

# Parse long flags
while [[ $# -gt 0 ]]; do
  case "$1" in
    --apk)
      APK="$2"
      shift 2
      ;;
    --arch)
      ARCH="$2"
      shift 2
      ;;
    --no-build)
      NO_BUILD=1
      shift 1
      ;;
    --docker-run-script)
      DOCKER_RUN_SCRIPT=1
      shift 1
      ;;
    --portable-runtime)
      PORTABLE_RUNTIME=1
      DOCKER_RUN_SCRIPT=0 # if really want to run portable by docker, add --docker-run-script after
      shift 1
      ;;
    --graphics)
      GRAPHICS=1
      shift 1
      ;;
    --debug)
      DEBUG=1
      shift 1
      ;;
    -*)
      echo "Error: Unknown option: $1"
      print_usage
      exit 1
      ;;
  esac
done

if [[ "$ARCH" != "arm" && "$ARCH" != "x86" ]]; then
    echo "Error: Unsupported architecture '$ARCH'. Use '--arch arm' or '--arch x86'."
    exit 1
fi

# Check if the APK file actually exists (either the default or the flagged one)
if [ ! -f "$APK" ]; then
    echo "Error: APK file '$APK' not found."
    exit 1
fi

CHECKSUM=6a87c0f2ff47b886c099a195aec242e88cdca40bf6993095db0826bca320f0fa
if [ "$(sha256sum "$APK" | cut -d' ' -f1)" != $CHECKSUM ]; then
    echo "Error: APK file '$APK' sha256 not $CHECKSUM."
    exit 1
fi

# Determine the correct library path based on architecture
if [ "$ARCH" = "arm" ]; then
    LIBG_ZIP_PATH="lib/armeabi-v7a/libg.so"
elif [ "$ARCH" = "x86" ]; then
    LIBG_ZIP_PATH="lib/x86/libg.so"
else
    echo "Error: Unsupported architecture '$ARCH'. Use '--arch arm' or '--arch x86'."
    exit 1
fi

# Ensure the destination directory exists
echo $DEST
mkdir -p "$DEST"

# Perform the extractions
echo "Using APK: $APK"
echo "Extracting assets to $DEST..."
unzip -o -q "$APK" "assets/*" -d "$DEST"

echo "Extracting $LIBG_ZIP_PATH directly to $DEST..."
unzip -o -q -j "$APK" "$LIBG_ZIP_PATH" -d "$DEST"

LIBG_PATH="$DEST/libg.so"

# patch_libg.sh is at the same dir as install.sh, get the dir first
./scripts/patch_libg.sh "$ARCH" "$LIBG_PATH"

if [[ "$NO_BUILD" -eq 1 ]]; then
    echo "Done!"
    exit 0
fi

# build
TOOLCHAIN_FILE=""
if [[ "$ARCH" == "arm" ]]; then
    TOOLCHAIN_FILE="toolchain-clang-armel.cmake"
elif [[ "$ARCH" == "x86" ]]; then
    TOOLCHAIN_FILE="toolchain-clang-i386.cmake"
fi

CMAKE_FLAGS=""
if [[ $GRAPHICS -eq 1 ]]; then
    CMAKE_FLAGS="$CMAKE_FLAGS -DENABLE_GRAPHICS=ON"
fi
if [[ $DEBUG -eq 1 ]]; then
    CMAKE_FLAGS="$CMAKE_FLAGS -DCMAKE_BUILD_TYPE=Debug"
fi

cmake -B "$DEST" $CMAKE_FLAGS -DCMAKE_TOOLCHAIN_FILE="$(realpath ./loader/toolchains/$TOOLCHAIN_FILE)" ./loader
cd "$DEST"
make

if [[ $? -ne 0 ]]; then
    exit 1
fi

cd -

if [[ $PORTABLE_RUNTIME -eq 1 ]]; then
  if [[ $GRAPHICS -eq 1 ]]; then
    ./scripts/apt_get_dep.sh --gl --arch $ARCH --dist $DEST
  else
    ./scripts/apt_get_dep.sh --arch $ARCH --dist $DEST
  fi

  if [[ $? -ne 0 ]]; then
    exit 1
  fi
fi


# write run.sh for python module's default command
DEST_RELATIVE=$(realpath -s --relative-to=. $DEST)
if [[ $DOCKER_RUN_SCRIPT -eq 1 ]]; then
  if [[ "$ARCH" == "arm" ]]; then
    DOCKER_PLATFORM="linux/arm/v5"
    DOCKER_IMAGE="cr-loader:armel"
  else
    DOCKER_PLATFORM="linux/386"
    DOCKER_IMAGE="cr-loader:i386"
  fi

  cat > $DEST/run.sh << EOF
#!/usr/bin/env bash
BUILD_DIR=\$(dirname "\${BASH_SOURCE[0]}")

docker run --platform $DOCKER_PLATFORM -i --rm \\
  -v "\$BUILD_DIR":/workspace \\
  --device /dev/dri:/dev/dri \\
  -w /workspace \\
  $DOCKER_IMAGE \\
  ./cr_loader "\$@"
EOF

elif [[ $PORTABLE_RUNTIME -eq 1 ]]; then
  if [[ $ARCH == "arm" ]]; then
    LD_NAME="ld-linux.so.3"
  else
    LD_NAME="ld-linux.so.2"
  fi
  cat > $DEST/run.sh << EOF
#!/usr/bin/env bash
BUILD_DIR=\$(dirname "\${BASH_SOURCE[0]}")

\$BUILD_DIR/lib/$LD_NAME \$BUILD_DIR/cr_loader "\$@"
EOF

else
  cat > $DEST/run.sh << EOF
#!/usr/bin/env bash
BUILD_DIR=\$(dirname "\${BASH_SOURCE[0]}")

\$BUILD_DIR/cr_loader "\$@"
EOF
fi

chmod +x $DEST/run.sh
echo "run.sh created at $DEST/run.sh"

if [ $? -eq 0 ]; then
    echo "Done!"
fi