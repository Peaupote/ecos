#!/usr/bin/python3
# -*-coding:Utf-8 -*

# Usage: font_from_bmp.py src.bmp dst.data
#   génère un fichier font.data pour l'affichage à partir d'une bitmap
#   constitué d'une ligne des caractères de la police à importer allant
#   de 0x21('!') à 0x7e('~')

from PIL import Image
import sys

srcf = sys.argv[1]
dstf = sys.argv[2]

nb_c = 0x7f - 0x21
src  = Image.open(srcf)
w    = src.width // nb_c
h    = src.height
print('{} x {}'.format(w, h))

dst = open(dstf, 'w+b')

for c in range(nb_c):
    data = []
    for y in range(h):
        for x in range(w):
            data.append(src.getpixel((c * w + x, y))[0])
    dst.write(bytearray(data))

dst.close()
