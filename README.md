# flipsyfat

This is a [MiSoC](https://github.com/m-labs/misoc)-based system on chip which integrates a basic SD card emulation
peripheral based on the [Project Vault ORP](https://github.com/ProjectVault/orp).

The emulated SD card has its block reads backed by software, which may choose to return a different filename each
time the file allocation table is scanned. The hardware peripheral can then generate configurable triggers precisely
when specific blocks are returned.

By analyzing side-channel emanations in sync with these triggers, we can evaluate the progress of a firmware routine's
filename matching code. By interacting with the SoC application to flip through filenames letter by letter, any names
of interest to the firmware can be eventually determined.
