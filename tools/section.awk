#!/usr/bin/awk -f

#Usage: gawk -f section.awk -v sname=SECTION_NAME file
#extrait les parties du fichier entre des lignes:
#   #define SECTION_NAME
# et
#   #undef SECTION_NAME

BEGIN{in_sct=0}
/^\#undef( |\t)/{if($2 == sname) in_sct = 0;}
{if(in_sct==1) print $0}
/^\#define( |\t)/{if($2 == sname) in_sct = 1;}
