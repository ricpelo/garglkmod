#!/bin/bash

# INSTRUCCIONES:
# 1. Renombramos Gargoyle a Gargoyle.exe en Gargoyle.app/Contents/MacOS/
# 2. Copiamos este script en Gargoyle.app/Contents/MacOS/ y lo
#    renombramos a Gargoyle
# 3. Borramos el archivo garglk.ini de Gargoyle.app/Contents/Resource/
# 4. Copiamos elcirculo.blb y elcirculo.ini a Gargoyle.app/Contents/Resource/

GARGOYLEROOT=$(cd "${0%/*}" && echo $PWD)

cd $GARGOYLEROOT
$GARGOYLEROOT/Gargoyle.exe $GARGOYLEROOT/../Resources/elcirculo.blb

