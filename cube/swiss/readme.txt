Swiss 0.1 (svn revision 33)

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

Current Features:
- Load DOL from SD/SDHC/DVD-R/HDD
- Load GCM/ISO from Original Disc/SD/SDHC/DVD-R/HDD
- Multi-Game DVD-R support (Cobra/GCOS)
- Multi-Disc support Original Disc/SD/SDHC/DVD-R/HDD
- Disc Ripping to SD/SDHC/HDD (WIP)
- Region free / Video forcing (WIP)
- Full cheat engine for all games (update it via SD/SDHC)
*HDD features require a IDE-EXI or homemade adapter
**SD features require a SDGecko or homemade adapter

Using cheats:
Cheats are stored in a Qoob based file format, they may be edited/updated
using the included cheats editor. You may convert action replay codes using
GCNCrypt and then add those too.
Swiss comes with a embedded cheats database but if you have an updated one,
place it as device:\cheats.qch and it will picked up and loaded instead of 
the internal cheats database.

IMPORTANT:
- 480P video mode forcing is Work in Progress - it may not work at all yet!
- Disc dumping is known to corrupt some HDD's - still work in progress!