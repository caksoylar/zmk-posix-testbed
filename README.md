# ZMK testbed using native posix board for local development

This repo contains some settings for doing ZMK development using the native posix board definition that can run on your computer.
So far the following are working:
- Display emulation through SDL support
- Bluetooth emulation
  - NOTE: Missing settings subsytem support which means pairing keys cannot be saved

![image](https://user-images.githubusercontent.com/7876996/186974209-2498232c-ff5e-4391-8ea1-482b07649f02.png)

I am intending to use this for developing custom status screens with widgets to save time on testing.

## Setup
Following are instructions for Ubuntu 22.04 that I used to get things working.

### Display
Install libsdl with `sudo apt install libsdl2-dev`.

### Bluetooth
Prerequisites if you are using WSL2:
- Install `usbipd` from https://github.com/dorssel/usbipd-win (I installed 2.3.0 release)
- Recompile your WSL2 kernel with BT support, following the instructions at https://github.com/Diegorro98/usbipd-win-wiki/blob/master/WSL-support.md#bluetooth-on-wsl2
  - See discussion in https://github.com/dorssel/usbipd-win/discussions/310 for reference
  - I used the current release kernel version as a base, tagged at https://github.com/microsoft/WSL2-Linux-Kernel/releases/tag/linux-msft-wsl-5.10.102.1
  - I also had to install `pahole` package in addition to the packages listed in order to get the kernel compiling successfully
  - You need to specify the new kernel image in `.wslconfig` (applies to all distros) rather than `wsl.conf` (distro-specific settings)
- In an admin Powershell, figure out your BT device through `usbipd list` then attach it to your new distribution (I called it `Ubuntu-BT`): `usbipd wsl attach --busid 1-14 -d Ubuntu-BT`
  - Note that this will disable the BT adapter on Windows, so make sure you have a USB keyboard handy

General instructions:
- Install a bluetooth manager like `sudo apt install bluez`
- In your distro, make sure you have `dbus` running `sudo service dbus start` (might not be necessary) but `bluetoothd` disabled `sudo service bluetooth stop`
- Find out your `hci` device ID through `hciconfig`
- You can try to see if your BT adapter works through `bluetoothctl` (probably requires `bluetoothd` running and `sudo hciconfig hci0 up`), where you can `scan` for devices

## Build and test

Build the `native_posix_64` board with config from this repo:
```
west build -p -d build/sdl -b native_posix_64 -- -DZMK_CONFIG=/abs/path/to/this/repo
```

> **Note**
> If you are using the built-in status screen, you might have to fix some string sizes in the display widgets so a buffer overflow doesn't happen, see the diff below.

```diff
diff --git a/app/src/display/widgets/battery_status.c b/app/src/display/widgets/battery_status.c
index 3dfcdb47..cbdb41a8 100644
--- a/app/src/display/widgets/battery_status.c
+++ b/app/src/display/widgets/battery_status.c
@@ -27,7 +27,7 @@ struct battery_status_state {
 };

 static void set_battery_symbol(lv_obj_t *label, struct battery_status_state state) {
-    char text[2] = "  ";
+    char text[7] = "  ";

     uint8_t level = state.level;
```

You can provide key event inputs to the executable in two ways:
1. Automated inputs: Use the `ZMK_MOCK_PRESS` and `ZMK_MOCK_RELEASE` macros in the `events` property of the `kscan` node in the keymap, similar to ZMK tests
2. Use the interactive shell with ZMK-defined `key` commands

### Running with automated input events

Run the produced executable, passing the BT device using its `hciN` ID from above if you have it enabled:
```
sudo build/sdl/zephyr/zmk.exe --bt-dev=hci0
```
`sudo` is required to utilize the BT device.

With the configuration in the keymap, it should bring up a screen with default widgets, change layers for 5 seconds, wait 5 seconds, then send a keystroke for 1 second. If bluetooth initializes without error you can try to pair to a different device in the first 10 seconds and see if you can observe the sent keystroke.


### Running with interactive shell

For this you need the PR zmkfirmware/zmk#1318 to add the `key` shell commands and enable mock kscan driver to run without events. Run the executable then attach to the created pty manually, or automatically by passing the `--attach_uart` flag:
```
sudo build/sdl/zephyr/zmk.exe --bt-dev=hci0 --attach_uart --attach_uart_cmd='tmux new-window screen %s'
```

You can replace the `attach_uart_cmd` value to use a different terminal emulator.

In the shell, you can send key events with `key press/release/tap POS` commands where `POS` is the linearized key position (between 0 to 3 for this keymap).


## References
- ZMK docs on posix board: https://zmk.dev/docs/development/posix-board
- Board DTS files (to look up nodes to modify):
  - https://github.com/zephyrproject-rtos/zephyr/blob/main/boards/posix/native_posix/native_posix.dts
  - https://github.com/zmkfirmware/zmk/blob/main/app/boards/native_posix_64.overlay
- Zephyr shell documentation: https://docs.zephyrproject.org/3.0.0/reference/shell/index.html
