ARCH=$1
LIBG_PATH=$2

SCRIPTDIR=$(dirname "${BASH_SOURCE[0]}")

#python $SCRIPTDIR/../loader/patches/apply.py --arch $ARCH $LIBG_PATH

#if [ $? -ne 0 ]; then
#    echo "[!] Failed to apply patches. Aborting."
#    exit 1
#fi

patchelf --replace-needed libc.so libc.so.6 $LIBG_PATH
patchelf --replace-needed libm.so libm.so.6 $LIBG_PATH
patchelf --replace-needed libdl.so libdl.so.2 $LIBG_PATH
patchelf --replace-needed libstdc++.so libstdc++.so.6 $LIBG_PATH
#patchelf --remove-needed libfmod.so $LIBG_PATH
patchelf --replace-needed libGLESv2.so libGLESv2.so.2 $LIBG_PATH

python3 $SCRIPTDIR/strip_verneed.py $LIBG_PATH

if [ $? -ne 0 ]; then
    echo "[!] Failed to strip_verneed. Aborting."
    exit 1
fi

python3 $SCRIPTDIR/patch_init_array.py $LIBG_PATH

if [ $? -ne 0 ]; then
    echo "[!] Failed to patch init_array. Aborting."
    exit 1
fi
