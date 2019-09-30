beetle
======

Beetle is a small daemon that provides simplified access to a bluetooth interface. It listens on a local port (usually 6969) and uses a line-oriented text protocol described in detail in the [commands list](docs/commands.md).

Fixed in Release 1.3 of	BBGW and in Release 1.2	of SAMA5

Fixed in this release:
After profile OTA, subsequent OTA's were failing.
Whitelist contained stale entries that needed to be removed.
Factory Reset event added to afEdge.
Factory	Reset event added to afLib4.
Hub logs filled up /etc	which caused profile OTA to fail.

## Building

There are three ways to build beetle:

1. yocto (cross-compiled for raspi)

    $ cd ../rpi-build; bitbake beetle

2. cmake (local Linux)

    $ ./build-linux-x86.sh

3. autoconf (local Linux)

    $ ./build-local.sh

If you've built using autoconf recently, you may need to `cd pkg; make distclean` before doing another yocto build.

The local builds deposit a `beetle` in `pkg/src/`. The yocto build deposits it in your yocto build folder.


## Code layout

There are two fundamental "modes" for beetle:

  - central mode (`mod cen`) -- as a hub that relays peripheral advertisements and may open connections to one or more peripherals at once [`central.c`](pkg/src/central.c)

  - peripheral mode (`mod per`) -- as a peripheral with characteristics (attributes) that can be read or written [`peripheral.c`](pkg/src/peripheral.c)

It starts in central mode, and switches to peripheral mode by request. Hubby (the hub software) uses peripheral mode for configuring wifi.

Both modes use a simple [event loop](pkg/src/evloop.c) and [command parser](pkg/src/command.c). The bluetooth interface is abstracted as much as possible into the [HCI layer](pkg/src/hci_beetle.c).
