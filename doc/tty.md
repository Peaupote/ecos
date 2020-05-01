Le tty possède un flux de lecture (`tty0`) et deux flux d'écriture (`tty1` et `tty2`).

- `tty1` est utilisé pour la sortie standard des applications.
  Les applications peuvent utiliser cette sortie pour contrôler le tty
  via des séquences précédés du caractère `\033` (`\e`, `\x1B`):
  - `\033d`: passe le tty en mode défaut
  - `\033p__prompt_msg__\033;`: passe le tty en mode prompt
  - `\033\n`: nouvelle ligne si la ligne actuelle n'est pas vide
  - `\033[__colors__m`: change la couleur, `__colors__` est une liste de codes
    séparés par `;`:
	 - `0`: restaure les valeurs par défaut
	 - `1`: utilise des couleurs plus claires (affecte les prochaines couleurs)
	 - `3x`: change la couleur du texte
	 - `4x`: change la couleur du fond
	
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

- `tty2` est utilisé pour afficher les erreurs
