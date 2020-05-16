# Paging

On met en place un paging de 4 niveaux (*PML4*, *PDPT*, *PD* et *PT*).
On n'utilise pas les entrées du PML4 au delà de `0x100` afin que le bit 47 soit
nul et que l'adresse soit complétée par des zéros en forme canonique.
Chaque processus possède un *PML4*. Celui-ci est divisé en deux parties:

 - une première spécifique à chaque processus (adresses basses)

 - une commune à tout les processus et référençant le code et les données du
   kernel.

 - on ajoute de plus 3 entrées spéciales:
    
	- `LOOP` qui pointe vers le pml4 considéré et permet d'accéder aux
	  structures de paging

	- `PSKD` qui permet de stocker des données spécifique au processus lors de
	  l'exécution de `execve`

	- `COPY_RES` est utilisé ponctuellement pour accéder à des pages
	  n'appartenant pas au paging actuel

Dans le but d'éviter des écritures accidentelles sur les parties du kernel en
lecture seule (notamment le code) et détecter un appel système entraînant
l'écriture sur une zone de l'userspace en lecture seule,
on active la fonction `Write Protect` dans `CR0` qui lève une *Page Fault* si
le kernel essaie d'écrire à une adresse dont l'un des niveau de translation ne
contient pas le drapeau `W` (écriture).
Afin de pouvoir accéder aux structures de paging, on fait en sorte que le
drapeau `W` ne puisse être absent que dans le dernier niveau de paging
(*PT*).

### Allocation des pages physiques

Pour allouer les pages physiques on découpe la mémoire disponible en blocs de
2MB (la surface couverte par une *page table*, même si on n'en profite pas).
Chaque bloc est géré par une structure `MemBlock`.
Entre ces différents blocs, on utilise des structures `MemBlockTree` pour
indexer les blocs entièrement ou partiellement libres.

### Partie userspace

Afin de limiter les copies lors de *fork* et l'allocation de pages lors du
lancement des processus on utilise les drapeaux des structures de paging
réservés au système (*Y1*-*Y3*, bits 9-11) pour partager des pages ou
allouer des pages lors d'un accès détecté par l'interruption *page fault*.

| Type    | Y2:  SV | Y1:  SP |    P    |
| ------- |:-------:|:-------:|:-------:|
| Present |    0    |    0    |    1    |
| Shared  |    1    |    0    |    1    |
| Alloc   |    0    |    1    |    0    |
| Alloc0  |    1    |    0    |    0    |
| Value   |    1    |    1    |    0    |

Les parties *Shared* sont des parties partagées entre plusieurs processus.
Elles sont utilisés pour les parties en lecture seule de la *libc* et ne sont
pas dupliquées lors d'un *fork* ni libérés lors de la mort du processus.
Les parties *Alloc* sont alloués lors de l'accès.
Les parties *Alloc0* sont de plus remplies par des zéros.
Les parties *Value* sont partagés entre plusieurs processus via un
pointeur partagé dont l'identifiant est stocké dans l'entrée du paging.
La page référencé est placée lors d'un accès (en étant éventuellement copiée).

### Partie kernel

L'entrée `0x80` des PML4 est utilisée pour accéder aux structures et données
utiles au kernel, on y référence:

 - Les sections statiques chargées lors du boot
 - Des emplacements dynamiques permettant d'accéder temporairement à certaines
   adresses physiques
 - Un *tas* pour allouer des structures dont la taille est déterminée lors
   de l'exécution. Il ne s'agit pas d'une pile mais d'une structure
   `MemBlockTree` réutilisée ici pour allouer des pages virtuelles.
 - Un emplacement pour allouer les structures nécessaires aux pointeurs
   partagés.
 - Le buffer utilisé pour accéder à l'affichage (texte ou linéaire)
