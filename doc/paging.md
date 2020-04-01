# Paging

On met en place un paging de 4 niveaux (*PML4*, *PDPT*, *PD* et *PT*).
Chaque processus possède un *PML4*. Celui-ci est divisé en deux parties:

 - une première spécifique à chaque processus (adresses basses)

 - une commune à tout les processus et référençant le code et les données du kernel.

 - on ajoute de plus 3 entrées spéciales:
    
	- `LOOP` qui pointe vers le pml4 considéré et permet d'accéder aux structures de paging

	- `PSKD` qui permet de stocker des données spécifique au processus lors de l'exécution `execve`
	- `COPY_RES` est utilisé ponctuellement pour accéder à des pages n'appartenant pas au paging actuel

Dans le but d'éviter des écritures accidentelles sur les parties du kernel en lecture seule (notamment le code), on active la fonction `Write Protect` dans `CR0` qui lève une *Page Fault* si le kernel essaie d'écrire à une adresse dont l'un des niveau de translation ne contient pas le drapeau `W` (écriture).
Afin de pouvoir accéder aux structures de paging, on fait en sorte que le drapeau `W` ne puisse être absent que dans le dernier niveau de paging (*PT*).
