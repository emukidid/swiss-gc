..\..\pc\utils\mkisofs.exe -R -J -G ..\..\pc\utils\eltorito-e.hdr -no-emul-boot -b swiss-lz.dol -o swiss(pal).gcm dist
..\..\pc\utils\mkisofs.exe -R -J -G ..\..\pc\utils\eltorito-u.hdr -no-emul-boot -b swiss-lz.dol -o swiss(ntsc).gcm dist
..\..\pc\utils\mkisofs.exe -R -J -G ..\..\pc\utils\eltorito-j.hdr -no-emul-boot -b swiss-lz.dol -o swiss(ntsc-j).gcm dist
