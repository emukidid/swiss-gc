# Swiss

## Table of Contents
- [Purpose](#purpose)
- [Launching Swiss](#launching-swiss)
    - [Using SDGecko](#using-sdgecko)
        - [Requirements](#requirements)
        - [Formatting your SD card](#formatting-your-sd-card)
            - [Windows](#windows)
            - [Linux and Mac](#linux-and-mac)
        - [Launch](#launch)
- [Navigating Swiss](#navigating-swiss)

## Purpose

Swiss aims to be an all-in-one homebrew utility for the Nintendo GameCube.

## Launching Swiss

### Using SDGecko

#### Requirements
- GameCube with controller
- Action Replay (Preferably the newest version you can find)
- SD card/Memory Card adaptor (Commonly referred to as SDGecko, but any generic adapter should work)
- SD, SDHC, or SDXC card with <4GB of maximum storage
- A Computer with an SD card slot, a USB SD card adaptor, or some other means of accessing the SD card from your computer
- Software on the computer which can extract 7z compressed archive files, such as [7-Zip](http://www.7-zip.org/).

#### Formatting your SD card

##### Windows
1. Connect your SD card to your computer.
2. Right click the SD card and select Format.
3. Ensure Capacity is <4GB
4. For File system, select FAT (Default).
5. For Allocation unit size, select 32 kilobytes.
6. Volume Label can be left as is or changed to your liking. I personally set it as GAMECUBE_SD.
7. Leave Quick Format checked.
8. Press Start.

##### Linux and Mac

1. Connect your SD card to your computer.
2. Open a terminal, run `sudo fdisk -l`
3. Take note of the Disk Name and the Device Name, which are fairly similar. Should be something like `/dev/mmcblk0` and `/dev/mmcblk0pl`, respectively.
4. Unmount the SD card by running `sudo umount <device-name>`.
5. Run `sudo fdisk <disk-name>` to begin formatting the SD card. The fdisk utility will now be running.
6. Type `d` and hit Enter/Return. This will delete the existing partition.
7. Type `n` and hit Enter/Return. This will begin the process for creating the new partition.
8. For Partition type, Partition number, First Sector, and Last Sector, hit Enter/Return to use the default values.
9. Type `t` and hit Enter/Return.
10. Type `e` and hit Enter/Return.
11. Type `w` and hit Enter/Return to write to the SD Card and quit.
12. Run `sudo mkfs -t vfat <device-name>` to change the file system type.
13. Run `sudo mlabel -i <device-name> ::<new-label>` to set the Volume Label. I personally use `sudo mlabel -i <device-name> ::GAMECUBE_SD`.
14. Remove and reinsert the SD card. It should remount and you should be able to move files to it.

#### Launch

1. [Download latest Swiss release](https://github.com/emukidid/swiss-gc/releases) and extract its contents.
2. Copy the compressed Swiss DOL file found in the DOL folder to the root of the SD card. It's name will follow the format swiss_r###-compressed.dol
3. If you want Swiss to boot automatically, rename the Swiss DOL file to AUTOEXEC.dol.
4. If you are using an older version of Action Replay, copy the SDLOADER.BIN file from the ActionReplay folder to the root of your SD card.
6. Safely ejected the SD card.
5. Place Action Replay disc into GameCube.
6. Place SD card into the SD Card/Memory Card Adaptor, and place the Adapter into Memory Card a Memory Card slot of the GameCube. It should not matter which slot is used.
7. Power on the GameCube, and you will see the Action Replay boot screen. If you renamed the Swiss DOL file to AUTOEXEC.dol, then from there it should boot directly into the Swiss menu. Otherwise, select the Swiss DOL from the list of files.

If the above steps do not work, try using the non-compressed Swiss DOL file.

## Navigating Swiss

### Controls

<table>
    <tr>
        <td>Up or Down on the Left Joysitck or D-Pad</td>
        <td>Navigate up or down through the file syste</td>
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
