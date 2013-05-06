#!/bin/sh

# Ver http://stackoverflow.com/questions/96882/how-do-i-create-a-nice-looking-dmg-for-mac-os-x-using-command-line-tools

mv Gargoyle.app/Contents/MacOS/Gargoyle Gargoyle.app/Contents/MacOS/Gargoyle.exe
cp script_arranque_osx.sh Gargoyle.app/Contents/MacOS/Gargoyle
cp ~/Downloads/The.Ring.La.Señal.DVDRip.Spanish.by.jurimu.avi Gargoyle.app/Contents/Resources/pepe.avi
cp ~/Downloads/mplayer2.app/Contents/MacOS/mplayer2 Gargoyle.app/Contents/Resources/
cp -r ~/Downloads/mplayer2.app/Contents/MacOS/lib Gargoyle.app/Contents/Resources/
cp ~/Dropbox/Public/elcirculo.* Gargoyle.app/Contents/Resources/

