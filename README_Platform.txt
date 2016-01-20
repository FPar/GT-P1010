How to build platform

1. Get android open source.
    : version info - Android gingerbread 2.3.6
    ( Download site : http://source.android.com )

2. Overwrite modules that you want to build.
- \external\alsa-lib : Write "libasound \" into "build\core\user_tags.mk" so that add this module.
- \external\ip : Write "ip \" into "build\core\user_tags.mk" so that add this module.

3. Copy the files to original Gingerbread source tree (overwrite) and then make

4. Add the following lines at the end of build/target/board/generic/BoardConfig.mk

BOARD_USES_ALSA_AUDIO := true

5. ./build.sh user