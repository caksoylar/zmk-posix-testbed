After requisites are installed in the readme, build and run as such, e.g. from ZMK app/ folder:
```sh
west build -d build/sdl -b native_posix_64 -- -DZMK_CONFIG=/path/to/zmk-posix-testbed -DSHIELD=jyojoh && build/sdl/zephyr/zmk.exe --bt-dev=hci0
```

Currently this is set to peripheral, to test central remove `ZMK_SPLIT` in Kconfig.defconfig, or set `ZMK_SPLIT_ROLE_CENTRAL` in addition.
