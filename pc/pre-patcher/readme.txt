Pre-patcher for Swiss 0.1 - revision 17

Usage example:
                sd-boot-RC4-prepatch.exe file.gcm

Purpose:
Some backups contain multiple .dol or .elf files inside the game, even .dol files inside smaller .gcm(.tgc) files within the iso itself (demo discs - EA games). 
Since Swiss can only patch what the apploader loads, 
there will be a point in the game where SD-Boot will simply stop working if
a game contains more than one executable file which will be loaded later.

This pre-patcher should clear up those issues for most of those titles.

Examples that require this pre-patcher:
Need for Speed HP2
Mario Soccer
007 Agent under fire
Zelda Collectors editions etc
Many demos

and even then some more..

Note:
BEFORE USING THIS, MAKE A COPY OF THE FILE YOU'RE ABOUT TO PATCH, 
IT WILL OVERWRITE IT!

This will only work with Swiss 0.1 - revision 17 on wards

New in revision 17:
- Relative branching removed - fixes 007 Everything or Nothing
- Fix for Kirby's Air ride
New in revision 7:
- Full support for Zelda CE