#!/bin/bash
mkisofs -R -J -G ../../pc/utils/eltorito-e.hdr -no-emul-boot -b swiss-lz.dol -o 'swiss(pal).gcm' dist
mkisofs -R -J -G ../../pc/utils/eltorito-u.hdr -no-emul-boot -b swiss-lz.dol -o 'swiss(ntsc).gcm' dist
mkisofs -R -J -G ../../pc/utils/eltorito-j.hdr -no-emul-boot -b swiss-lz.dol -o 'swiss(ntsc-j).gcm' dist
