# pwswd++

A modern C++ rewrite of the [OpenDingux pwswd daemon](https://github.com/tonyjih/RG350_pwswd).

## Currently supported features

- Power button shortcuts
  - Kill foreground application (Select)
  - Increase screen sharpness (DpadRight)
  - Decrease screen sharpness (DpadLeft)
  - Increase brightness (DpadUp)
  - Decrease brightness (DpadDown)
  - Toggle aspect ratio and integer scaling (VolumeUp) **\*(NEW)\***
  - Mute device (VolumeDown) **\*(NEW)\***
  - Mouse mode (L3 for left stick mouse, R3 for right stick mouse) **\*(IMPROVED)\***
  - Access emulator settigns (Start) **\*(NEW)\***
- General shortcuts
  - Device standby mode! (Power button) **\*(NEW)\***
  - Increase volume (VolumeUp)
  - Decrease volume (VolumeDown)
  
## Missing but planed features

- Screenshots
- Additional features that improve the software UI and UX of OpenDingux devices
- Additional features
  - Overlays (Volume/Brightness/Sharpness slider, shortcut feedback "notifications")

## Currently supported devices

- RG350
- RG350M
- Possibly more

## Installation

- Make a backup of your system sd card using `dd`
- Connect to your OpenDingux device over FTP and extract the release zip into the `/usr/local/sbin` directory
- Give permissions to `frontend_start` and `pwswdpp` by running `chmod 766 frontend_start pwswdpp`
- Restart your device

If you're using a custom launcher instead of gmenu2x, you probably already have a frontend_start file. In this case the two of them need to be merged.

## Building

Build is done using `meson` and my OpenDingux toolchain. A complete setup and usage guide can be found here: https://werwolv.net/projects/opendingux_toolchain

- `meson build --cross-file=dingux`
- `meson compile -C build`
- Output file can be found in `build/pwswdpp`