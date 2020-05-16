# ECOS

__ecos__ est un système d'exploitation pour l'architecture *x86_64*.
Il implémente une partie de la norme *POSIX*.

### Organisation du dépôt

 - `data`
 - `doc`: fichiers détaillant certains aspects du fonctionnement
   du système
 - `include`: fichiers d'en-tête
   - `fs`: systèmes de fichier
   - `headers`: définitions utilisées à la fois par le kernel et l'userspace
   - `kernel`: définitions utilisées en interne par le kernel
   - `libc`: utilisée à la fois par l'userspace et le kernel sous le nom de
     `libk`
   - `util`: définitions de constantes liées au hardware et fonctions utilitaires,
     utilisées dans tout le projet
 - `src`:
   - `boot`: partie *32 bits* servant à lancer le kernel
   - `fs`
   - `kernel`: contient le code qui sera placé dans le noyau ce qui inclut
     `init` et `idle`
   - `libc`
   - `sys`: contient les codes sources des programmes pouvant être exécutés sur
     le système, notament `init1`, `sh` et `edit`, placés dans `/bin`
   - `util`: code pouvant être compilé en 32 ou 64 bits ainsi que pour le
     système compilant le projet ("hôte")
 - `tests`
   - `host`: contient des tests pouvant être effectué automatiquement sur
     l'hôte via `make tests`
     - `ext2`: propose une interface en ligne de commande pour tester les
       opérations
     - `libc`
     - `unit`: extractions de parties pouvant provoquer des difficultées
   - `run`: programmes de tests à exécuter sur le système, placés dans
     `/home/test`
 - `tools`: contient des scripts et programmes utilitaires utilisés pour la
   compilation du projet

### Compilation

La compilation du projet requiert l'accès à un cross-compiler gcc pour 32 et
64 bits: `i686-elf-gcc` et `x86_64-elf-gcc`, le second devant disposer de
l'option `-mcmodel=large`.
Il est aussi nécessaire de disposer de `GRUB` et de l'ensemble d'utilitaires
`e2tools` permettant de générer les images *ext2*.

`make` produit une image `ecos.iso`.

Voici la liste des combinaisons gcc/émulateurs que nous avons pu tester:

 - `GCC 5.4.0` `QEMU 2.5.0`
 - `GCC 5.4.0` `Bochs 2.6.11`
 - `GCC 9.3.0` `QEMU 4.2`

### Documentation

Informations sur le fonctionnement et l'interface de certaines sections du
projet :

 - [processus](./doc/proc.md)
 - [appels systèmes](./doc/syscall.md)
 - [paging et mémoire](./doc/paging.md)
 - [signaux](./doc/signal.md)
 - [tty](./doc/tty.md)
 - [shell](./doc/sh.md)
 - [système de fichier](./doc/fs.md)

### Bugs

 - __Difficulté à lire une ligne sur un pipe:__ `lseek` ne pouvant être
   utilisé sur un pipe, un processus ne peut y lire une unique ligne à moins
   d'effectuer un appel à *read* par caractère.
   Ceci pose problème s'il lit des caractères destinés à un autre processus.
   Par exemple, la builtin `read` se comporte mal sur les pipes,
   c'est la raison pour laquelle on passe par un fichier intermédiaire
   dans `sh/colors.sh` pour la partie utilisant des boucles *while*
   (même si dans ce cas simple l'utilisation d'un buffer global au shell,
   comme avec *fread*, résoudrait le problème).

 - __Dépendance lors de la compilation:__ afin d'éviter d'avoir à recompiler
   tous le projet en cas de modifications, on utilise `gcc -M` pour générer
   les dépendances entre les différents fichiers.
   La compilation effectue tout de même de nombreuses opérations même en
   l'absence de modifications à cause des appels entre les différents
   Makefiles.
   Il semble de plus que certaines dépendances ne soient pas correctement
   prises en compte et `make re` est parfois nécessaire.
