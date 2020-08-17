# pwswd++
A modern C++ rewrite of the [OpenDingux pwswd daemon](https://github.com/tonyjih/RG350_pwswd).

## Currently supported features
- Power button shortcuts
  - Device standby mode! (Power button) **\*(NEW)\***
  - Return to main menu (Select)
  - Increase screen sharpness (DpadRight)
  - Decrease screen sharpness (DpadLeft)
  - Increase brightness (DpadUp)
  - Decrease brightness (DpadDown)
  - Toggle aspect ratio and integer scaling (VolumeUp) **\*(NEW)\***
  - Mute device (VolumeDown) **\*(NEW)\***
  - Mouse mode (L3 for left stick mouse, R3 for right stick mouse) **\*(IMPROVED)\***
- General shortcuts
  - Increase volume (VolumeUp)
  - Decrease volume (VolumeDown)
- Additional features
  - Overlays (Volume/Brightness/Sharpness slider, shortcut feedback "notifications") **\*(NEW)\***
  
## Missing but planed features
- Screenshots
- Additional features that improve the software UI and UX of OpenDingux devices

## Currently supported devices
- RG350
- RG350M

## Installation
- Make a backup of your system sd card using `dd`
- Connect to your OpenDingux device over FTP and extract the release zip into the `/usr/local/sbin` directory
- Give permissions to `frontend_start` and `pwswdpp` by running `chmod 766 frontend_start pwswdpp`
- Restart your device