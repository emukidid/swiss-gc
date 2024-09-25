# Swiss

[![Build Status](https://github.com/emukidid/swiss-gc/actions/workflows/continuous-integration-workflow.yml/badge.svg)](https://github.com/emukidid/swiss-gc/actions/workflows/continuous-integration-workflow.yml)

## Table of Contents
 - [Purpose](#purpose)
 - [Main Features](#main-features)
 - [Requirements](#requirements)
 - [Usage](#usage)

## Purpose
Swiss aims to be an all-in-one homebrew utility for the Nintendo GameCube.

### Main Features
**Can browse the following devices**
- SDSC/SDHC/SDXC Card via [SD Gecko](https://www.gc-forever.com/wiki/index.php?title=SDGecko) or [SD2SP2](https://github.com/Extrems/SD2SP2)
- DVD (-/+R) or original GameCube discs via Disc Drive
- [Qoob Pro](https://www.gc-forever.com/wiki/index.php?title=Qoob) flash memory
- [USB Gecko](https://www.gc-forever.com/wiki/index.php?title=USBGecko) remote file storage
- [WASP](https://www.gc-forever.com/wiki/index.php?title=WASP_Fusion) / [Wiikey Fusion](https://www.gc-forever.com/wiki/index.php?title=Wiikey_Fusion)
- SMB, FTP, FSP via Broadband Adapter, ENC28J60 or W5500
- [WODE Jukebox](https://www.gc-forever.com/wiki/index.php?title=Wii_Optical_Drive_Emulator)
- [IDE-EXI](https://www.gc-forever.com/wiki/index.php?title=IDE-EXI) or M.2 Loader
- Memory Cards
- [GC Loader](https://www.gc-forever.com/wiki/index.php?title=GCLoader)
- [FlippyDrive](https://www.gc-forever.com/wiki/index.php?title=FlippyDrive)

### Requirements
- GameCube with controller
- A [way to boot homebrew](https://www.gc-forever.com/wiki/index.php?title=Booting_homebrew)

### Usage
1. [Download latest Swiss release](https://github.com/emukidid/swiss-gc/releases/latest) and extract its contents.
2. Copy the compressed Swiss DOL file found in the DOL folder to the device/medium you are using to boot homebrew.
3. Launch Swiss, browse your device and load a DOL or GCM!

Note: Specific devices will have specific locations/executable file variants that need to be used, please check the documentation with those devices on where Swiss will need to be placed.

## Navigating Swiss
### Controls
| Control                       | Action                  |
| ----------------------------- | ----------------------- |
| Control Stick or +Control Pad | Navigate through the UI |
| A Button                      | Select                  |
| B Button                      | Enter/Exit bottom pane  |
| X Button                      | Move back up a folder   |
| Z Button                      | Manage file or folder   |
| L Button                      | Move up a page          |
| R Button                      | Move down a page        |
| Start/Pause                   | Access recent list      |

### Swiss UI
- The top heading shows the version number, commit hash, and revision number of Swiss.
- The left panes show what device you are using.
- The largest portion is the Swiss file browser, through which you can navigate files and folders. The top of every folder includes a `..` option, and selecting this moves you back up a folder.
- The bottom pane, from the left:
    - Device Selection
    - Global Settings, Network Settings, Global Game Settings, Default Game Settings, and Current Game Settings
    - System Info, Device Info, Version Info, and Greetings
    - Return to top of file system
    - Restart GameCube
