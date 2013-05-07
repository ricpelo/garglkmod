#!/bin/sh

# INSTRUCCIONES:
# 1. Renombramos Gargoyle a Gargoyle.exe en Gargoyle.app/Contents/MacOS/
# 2. Copiamos este script en Gargoyle.app/Contents/MacOS/ y lo
#    renombramos a Gargoyle
# 3. Borramos el archivo garglk.ini de Gargoyle.app/Contents/Resources/
# 4. Copiamos elcirculo.blb y elcirculo.ini a Gargoyle.app/Contents/Resources/
# 5. Copiamos mplayer2 y su lib/ en Gargoyle.app/Contents/Resources/
# 6. Copiamos los vídeos también en Gargoyle.app/Contents/Resources/
# EL ARCHIVO .app NO PUEDE TENER ACENTOS

GARGOYLEROOT=$(cd "${0%/*}" && echo $PWD)
cd "$GARGOYLEROOT"
"$GARGOYLEROOT/Gargoyle.exe" "$GARGOYLEROOT/../Resources/elcirculo.blb"
kill -9 `ps ax | grep Gargoyle.app | grep git | cut -d" " -f1` > /dev/null 2>&1

