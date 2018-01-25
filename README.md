# Swiss

## Table of Contents
- [Purpose](#purpose)
 - [Main Features](#main-features)
 - [Requirements](#requirements)
 - [Usage](#usage)

## Purpose
Swiss aims to be an all-in-one homebrew utility for the Nintendo GameCube.

### Main Features
**Can browse the following devices**
- SD/SDHC/SDXC Card via SDGecko
- DVD (-/+R) via Disc Drive
- Qoob Pro flash memory
- USB Gecko remote file storage
- SD/SDHC via Wasp/WKF
- Samba via BBA
- Wode Jukebox
- IDE-EXI
- Memory cards

### Requirements
- GameCube with controller
- A [way to boot homebrew](https://gc-forever.com/wiki/index.php?title=Booting_Homebrew)

### Usage
1. [Download latest Swiss release](https://github.com/emukidid/swiss-gc/releases) and extract its contents.
2. Copy the compressed Swiss DOL file found in the DOL folder to the device/medium you are using to boot homebrew.
3. Launch Swiss, browse your device and load a DOL or GCM!

If the above steps do not work, try using the non-compressed Swiss DOL file.

## Navigating Swiss
### Controls
<table>
    <tr style="font-weight:bold">
        <td>Control</td>
        <td>Action</td>
    </tr>
    <tr>
        <td>Left Joysitck or D-Pad</td>
        <td>Navigate through the UI</td>
    </tr>
    <tr>
        <td>A</td>
        <td>Select</td>
    </tr>
    <tr>
        <td>B</td>
        <td>Enter/Exit Bottom Menu</td>
    </tr>
</table>

### Swiss UI
- The top heading shows the version number, commit number, and revision number of Swiss.
- The left panes show what device you are using.
- The largest portion is the Swiss file browser, through which you can navigate files and folders. The top of every folder includes a `..` option, and selecting this moves you back up a folder.
- The bottom pane, from the left:
    - Device Selection
    - Global Settings, Advanced Settings, and Current Game Settings
    - System Information, Device Info, and Credits
    - Return to top of file system
    - Restart GameCube
