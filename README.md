# bspatch for esp32

This project adds support for bspatch to the esp32 with some changes: no compression (bz2), no header and changed the interfaces to allow streaming all inputs and outputs.

The functionality in particular can be useful to perform OTA firmware upgrades using compressed diffs between the running firmware and the target firwmare.

At the moment on the esp32 only applying the patch is supported (with the patch generation happening on a computer or anyhow a device with the adequate resources).

Important: `esp32_bsdiff` is incompatible with the version usually available. Build `esp32_bsdiff` as described below.


## Usage

To use with an ESP-IDF project you include this repo in the components directory.

```
$ cd components
$ git submodule add git@github.com:blockstream/esp32_bsdiff.git esp32_bsdiff
```

To build bsdiff and bspatch for your computer:
```
gcc -O2 -DBSDIFF_EXECUTABLE -o esp32_bsdiff components/esp32_bsdiff/bsdiff.c
gcc -O2 -DBSDIFF_EXECUTABLE -o esp32_bspatch components/esp32_bsdiff/bspatch.c
```

Usage of the command line tools are unchanged from bsdiff/bspatch.

To compress the resulting patch one may use the following (and use miniz.h to decompress on the esp32):

```
python -c "import zlib; import sys; open(sys.argv[2], 'wb').write(zlib.compress(open(sys.argv[1], 'rb').read(), 9))" uncompress_patch_file.bin compressed_patch_file.bin
```
