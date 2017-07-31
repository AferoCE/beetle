beetle
======

Beetle is a small daemon that provides simplified access to a bluetooth interface. It listens on a local port (usually 6969) and uses a line-oriented text protocol described in detail in the [commands list](docs/commands.md).


## Code layout

There are two fundamental "modes" for beetle:

  - central mode (`mod cen`) -- as a hub that relays peripheral advertisements and may open connections to one or more peripherals at once [`central.c`](pkg/src/central.c)

  - peripheral mode (`mod per`) -- as a peripheral with characteristics (attributes) that can be read or written [`peripheral.c`](pkg/src/peripheral.c)

It starts in central mode, and switches to peripheral mode by request. Hubby (the hub software) uses peripheral mode for configuring wifi.

Both modes use a simple [event loop](pkg/src/evloop.c) and [command parser](pkg/src/command.c). The bluetooth interface is abstracted as much as possible into the [HCI layer](pkg/src/hci_beetle.c).
