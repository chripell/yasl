# Puremodules 

This directory contains a minimal set of file to build the firmware
for the [Puremodules core
module](https://www.kickstarter.com/projects/pureengineering/puremodules-for-dreamers-tinkerers-hackers-and-des/description). It
was written because the [demo
software](https://github.com/PureEngineering/PUREmodules) is
frustrating to use and fix, being just a demo.

The goal is to build **only** the App, always use the **Application
only** option while flashing the product. It works with the pre-loaded
bootlader. If you changed it and changed the private key, you need to
modify the file `private.pem`.

## Prerequisites

You need:

* `pip install nrfutil` for the Nordic tool which creates the signed
  zip file.
  
* `pacman -S arm-none-eabi-gcc` or equivalent on other distributions
  (or you need to download the toolchain from
  [ARM](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads)).
  
## Building

Simply run `make` or `make VERBOSE=1`.

## Flashing

1. Put the Puremodule nRF52 core in DFU mode. Keep the USR button
   pressed while inserting the battery. Please note that using reset
   will *not* do the trick.
   
1. Use the app [nRF Toolbox for
   BLE](https://play.google.com/store/apps/details?id=no.nordicsemi.android.nrftoolbox)
   to download `app_dfu_package.zip`. Select **Application only** when
   prompted because this zip file does **not** contain the Bootloader.


