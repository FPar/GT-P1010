HOW TO BUILD KERNEL 2.6.35 FOR GT-P1010
 
1. How to Build
        - get Toolchain
        Visit http://www.codesourcery.com/, download and install Sourcery G++ Lite 2009q3-68 toolchain for ARM EABI.
        Extract kernel source and move into the top directory.
        $ toolchain\arm-eabi-4.4.3
        $ cd kernel/
        $ make p1fwifi_defconfig
        $ make
 
2. Output files
        - Kernel : kernel/arch/arm/boot/zImage
 
