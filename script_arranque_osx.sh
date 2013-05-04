#!/bin/bash

# INSTRUCCIONES:
# 1. Renombramos Gargoyle a Gargoyle.exe en Gargoyle.app/Contents/MacOS/
# 2. Copiamos este script en Gargoyle.app/Contents/MacOS/ y lo
#    renombramos a Gargoyle
# 3. Borramos el archivo garglk.ini de Gargoyle.app/Contents/Resource/
# 4. Copiamos elcirculo.blb y elcirculo.ini a Gargoyle.app/Contents/Resource/
# 5. Copiamos mplayer2 y su lib/ en Gargoyle.app/Contents/MacOS/
# EL ARCHIVO .app NO PUEDE TENER ACENTOS

GARGOYLEROOT=$(cd "${0%/*}" && echo $PWD)
cd "$GARGOYLEROOT"
"$GARGOYLEROOT/mplayer2" -fs "$GARGOYLEROOT/../Resources/The_Ring.avi"
"$GARGOYLEROOT/Gargoyle.exe" "$GARGOYLEROOT/../Resources/elcirculo.blb"

