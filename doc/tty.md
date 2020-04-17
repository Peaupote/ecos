Le tty possède un flux de lecture (`tty0`) et deux flux d'écriture (`tty1` et `tty2`).

- `tty1` est utilisé pour la sortie standard des applications.
  Les applications peuvent utiliser cette sortie pour contrôler le tty
  via des séquences précédés du caractère `\033` (1B):
  - `\033d`: passe le tty en mode défaut
  - `\033p__prompt_msg__\033;`: passe le tty en mode prompt

- `tty2` est utilisé pour afficher les erreurs
