Swiss 0.3 Readme file

-- Usage and features list:
http://www.gc-forever.com/wiki/index.php?title=Swiss


-- What's New:
New in Swiss 0.3:
- Added 16:9 widescreen forcing for games (thx Extrems!)
- Added WiiRD Support for Games from all devices
- Added 576p video mode support to Swiss/Games (thx Extrems!)
- Added memory card emulation from SDGecko in slot A (WIP)
- Added Ocarina Cheat engine support (.gct files or .qch archive still supported)
- Added USBGecko PC browsing and game launch support (swissserver.exe)
- Added automatic device selection priority (wiikey fusion, sd gecko (if "default device"), dvd if medium up)
- Added Wiikey Fusion flashing support
- Added IPL and DVD disc dumping via BBA HTTP (point browser at GC IP addr)
- Added audio muting during reads if enabled (disabled by default)
- Added boot.dol launching from an SD Card if detected at startup
- Added GUI user friendly enhancements
- Fix IDE-EXI support
- Fix wiikey fusion and 32GB SD cards
- Fix input scanning blocking threads (BBA now works a lot better)
- Fix large files trying to get patched (Fifa 2004)
- Fix Linux compiling issue
- Removed GCARS cheat engine (in favour of Ocarina)

New in svn revision 123:
- High level type patch support significantly improved (driveless support too)
- Disc swap code re-written (untested in anything but Tales of Symphonia - requires LOW patch type and disc in drive)
- Re-wrote patches to use a common assembly code base
- GX conversion
- Fixed Luigis Mansion NTSC (from SD, DVD, etc, haven't tested Wiikey Fusion/Wode)
- Fixed Phantasy Star Online Ep 3 (from DVD so far)
- USB Gecko debug printing made safer
- IDE-EXI v1 support partially broken for the time being whilst I fix it again..
- Too much else to mention, check the SVN log :)

New in svn revision 62:
- New high level patch (Fixes Super Mario Sunshine and a few other games)
- Basic Wode support added
- Pre-Patcher integrated into Swiss
- Consolidated SD patches to one per slot (SD & SDHC in one)
- Use RequestError (0xE0) instead of Seek to sector 0 for Low Patch
- Stop DVD motor after drive init for Qoob users
- MP3 Playback (no point really)

New in svn revision 47:
- Fix pre-patcher
- Update pre-patcher version
- Fix bugs in patch code
- Added fast scrolling via analog stick in file selection
- Bring up the info screen when booting a game

New in svn revision 39:
- No need to navigate to the "Boot" button anymore, just press A on a file
- Cheat file loading from browser on any device (.QCH format)
- Reset SD Gecko on mount
- Fix the pre-patcher detection
- Motor Stopping is not allowed on pre-patched games
- Added Qoob Flash file browsing support (DOL loading from it)
- Changed the device selection screen to allow for more devices to come
- Attempt to fix black/green screen for users who have Swiss in 480p and then try booting a game

New in svn revision 33:
- Zelda WW on Wii support
- Add "Stop Motor" support now
- DVD Seek instead of small Read now

New in svn revision 28:
- Fixed a pretty big bug in SD/HDD code
- Pokemon Box support

New in svn revision 24:
- Extreme WIP version 
- Fragmented files detection (no support for them yet)
- Disabled disc dumper until I tidy up the code/fix it
- Disabled Qoob Cheats loading until I fix that too
- Removed ELMchan FATFS in favour of libFAT
- Simplified IDE-EXI code to use a 32bit sector addr (2TB Max)

New in svn revision 21:
- Re-added device speed selection
- Simplify Video forcing/patching (less green screen boot)
- Fixed pre-patched image check
- Fixed cheats scrolling being too fast
- Cheats file now should be placed in device root:\cheats.qch

New in svn revision 17:
- Fixed Zelda CE Windwaker demo (pre-patcher issue)
- Fixed Zelda Master Quest
- Changed relative branching to direct branching (mtctr,bctrl) (fixed 007 Everything or nothing)
- Possibly more titles fixed - please test :)
- Fixed broken ide-exi code
- Swiss now checks the version a game was pre-patched with

New in svn revision 7:
- Fixed Invalidation and Flushing
- Fixed Zelda CE and possibly a few other games (use pre-patcher for it)
- Fixed Audio Streaming original media on Wii
- Fixed Other PAL titles (German/etc)