# TTY

Le tty possède un flux de lecture (`tty0`) et deux flux d'écriture (`tty1` et `tty2`).

- `tty1` est utilisé pour la sortie standard des applications.
  Les applications peuvent utiliser cette sortie pour contrôler le tty
  via des séquences précédés du caractère `\033` (`\e`, `\x1B`).
  Ces séquences ne doivent pas êtres séparées en plusieurs apppels à `write`.
  - `\033d`: passe le tty en mode défaut
  - `\033p__color____prompt_msg__\033;`: passe le tty en mode prompt où
    `__color__` est un numéro de couleur
  - `\033l`: passe le tty en mode `live`
  - `\033\n`: nouvelle ligne si la ligne actuelle n'est pas vide
  - `\033[__colors__m`: change la couleur, `__colors__` est une liste de codes
    séparés par `;`:
  
     - `0`: restaure les valeurs par défaut
     - `1`: utilise des couleurs plus claires (affecte les prochaines couleurs)
     - `3x`: change la couleur du texte
     - `4x`: change la couleur du fond
  
  - `\033i`: infos, le tty envoie la séquence suivante sur `tty0` via des
    `tty_live_t` de `key` nulles:
    ```
    i__width__;__height__;
    ```

- `tty2` est utilisé pour afficher les erreurs

### Numéro des couleurs

| `x` | couleur |
|:---:|:------- |
| 0   | noir    |
| 1   | rouge   |
| 2   | vert    |
| 3   | marron  |
| 4   | bleu    |
| 5   | magenta |
| 6   | cyan    |
| 7   | blanc   |

### Mode live

Lorsque le tty est en mode `live`, les entrés claviers sont envoyées
directement sur tty0 au format binaire de la structure `tty_live_t`.
L'affichage se fait en utilisant un curseur d'écriture.
Les séquences suivantes sont alors disponibles:

  - `\033c__x__;__y__;`: déplace le curseur d'écriture à la position `x`, `y`
  - `\033Cc__x__;__y__;`: déplace le curseur affiché, avec `c` comme caractère en dessous
  - `\033x`: efface l'écran en utilisant la couleur de fond par défaut

### Mode debug

En effectuant `C-tab`, le tty passe en mode debug et fournit des commandes
permettant d'inspecter l'état du système, les mêmes commandes peuvent
être accessibles en cas de `panic` (dépend de la gravité et de la source
du problème).

 - `memat addr`: affiche l'octet à l'adresse demandée
 - `pg2 addr`: affiche les différentes entrées de paging jusqu'à l'adresse
 - `kprint`: affiche les entrées claviers
 - `kill pid signum`
 - `ps`
 - `ls path`
 - `log -lvl hd`:
   
   `lvl` peut être:
   - `e`: erreur
   - `w`: warn
   - `i`: info
   - `v`: verbose
   - `vv`: very verbose
   
   `hd` est l'en-tête auquel on veut se limiter (ou `*` pour tous),
   par exemple `mem`, `sig`, `vfs`...
 - `info err|mem|dis`

