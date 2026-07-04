ARCH=""
DIST=""
ENABLE_GL=0

while [[ $# -gt 0 ]]; do
    case "$1" in
        --arch) ARCH="$2"; shift 2;;
        --dist) DIST="$2"; shift 2;;
        --gl) ENABLE_GL=1; shift;;
        *) echo "Error: Unknown option: $1"; exit 1;;
    esac
done

if [ "$ARCH" = "arm" ]; then	
	SUFFIX="armel"
elif [ "$ARCH" = "x86" ]; then
	SUFFIX="i386"
else
	echo "Error: Unknown architecture '$ARCH'. Use 'arm' or 'x86'."
	exit 1
fi												

if [ ! -d "$DIST" ]; then
	echo "Error: DIST directory '$DIST' does not exist."
	exit 1
fi

DIST=$(realpath $DIST)


TMP_DIR="/tmp/local_apt"
rm -rf "$TMP_DIR"
mkdir -p "$TMP_DIR"

APT_DIR="$TMP_DIR/apt"
mkdir -p "$APT_DIR/lists/partial"
mkdir -p "$APT_DIR/cache/archives/partial"
mkdir -p "$APT_DIR/sources.list.d"
touch "$APT_DIR/status"

SOURCES_LIST="$APT_DIR/sources.list"
echo "deb [trusted=yes arch=$SUFFIX] http://deb.debian.org/debian trixie main" > "$SOURCES_LIST"

echo "[*] Using sources list: $SOURCES_LIST"

APT_OPT=(
	-o Dir::State="$APT_DIR/lists"
	-o Dir::State::status="$APT_DIR/status"
	-o Dir::Cache="$APT_DIR/cache"
	-o Dir::Etc::sourcelist="$SOURCES_LIST"
	-o Dir::Etc::sourceparts="$APT_DIR/sources.list.d"
	-o APT::Architecture="$SUFFIX"
	-o APT::Architectures="$SUFFIX"
)

echo "[*] apt-get update"
apt-get "${APT_OPT[@]}" update

if [[ "$ENABLE_GL" -eq 1 ]]; then
	ROOT_PACKAGES="libc6:$SUFFIX libstdc++6:$SUFFIX libgles2:$SUFFIX libegl1:$SUFFIX"
else
	ROOT_PACKAGES="libc6:$SUFFIX libstdc++6:$SUFFIX"
fi

PACKAGES=$(
	apt-cache "${APT_OPT[@]}" depends --recurse --no-recommends --no-suggests --no-conflicts --no-breaks --no-replaces --no-enhances \
	$ROOT_PACKAGES \
	| grep "^\w"| tr '\n' ' '
)

DEB_DIR="$TMP_DIR/debs"
mkdir -p "$DEB_DIR"
cd "$DEB_DIR"

echo "apt-get "${APT_OPT[@]}" download $PACKAGES"
apt-get "${APT_OPT[@]}" download $PACKAGES

echo "[*] Extracting packages..."
EXTRACT_DIR="$TMP_DIR/extracted"
mkdir -p "$EXTRACT_DIR"
for deb in *.deb; do
	dpkg -x "$deb" "$EXTRACT_DIR"
done

echo "[*] Copying shared libraries to $DIST/lib..."
mkdir -p "$DIST/lib"
# Pass 1: Copy regular files
find "$EXTRACT_DIR" -name "*.so*" -type f -exec cp -a -t "$DIST/lib/" {} +
# Pass 2: Copy symlinks without overwriting existing target files
find "$EXTRACT_DIR" -name "*.so*" -type l -exec cp -a --update=none -t "$DIST/lib/" {} +


echo "[*] Cleaning up temporary files..."
rm -rf "$TMP_DIR"
echo "[*] Done!"