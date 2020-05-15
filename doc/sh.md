# SH

### Syntaxe

```
cmd_0 = <sous-shell>
      | vars=values... cmd args...

cmd_1 = <cmd_0> <redirections>

cmd_2 = sep_list(<cmd_1>, |)

cmd_3a = <cmd_2>
       | { <cmd_3c> ; } <redirections>
       | while <cmd_3c> ; do <cmd_3c> ; done <redirections>
	   | if <cmd_3c> ; then <cmd_3c> ;
	     ( elif <cmd_3c> ; then <cmd_3c> ; )*
		 ( else <cmd_3c> ; )?
		 fi <redirections>
	   | for var in words ; do <cmd_3c> ; done <redirections>


cmd_3b = <cmd_3a>
       | <cmd_3b> && <cmd_3a>
       | <cmd_3b> || <cmd_3a>

cmd_3c = sep_list(<cmd_3c>?, [;\n&])

sous_shell = \( <cmd_3c> \)
            
// pour cmd_3a: 0 <= fd < 3
redirections = fd? <redir_op> <redir_src>
redir_op     = < | > | >>
redir_src    = & fd
             | filename

```

Priorité des opérateurs (décroissante):

 - `|`
 - `&&` `||` associativité gauche
 - `&` 
 - `;`

### Builtins

Les builtins suivantes peuvent être utilisées n'importe où:

- `exit [status]`
- `cd [dir]`
- `export var[=val]...`
- `read var...`
- `jobs`
- `echo -ne words...`
- `kill -signum pids...`

La builtin `fg cmd_num` ne peut être utilisée qu'isolée, sans redirections.
Les commandes `echo` et `kill` sont implémentées comme des builtins de *sh*
pour des raison d'efficacité en réduisant leur lancement à un éventuel
*fork*.

### Fonctionnement

Après le parsing, les commandes sont représentées sous la forme d'un arbre
qui est la transcription directe de la syntaxe.
Pour exécuter une commande, on descend dans l'arbre jusqu'à atteindre
une `cmd_2`.
On lance alors les différentes `cmd_0` avec les redirections nécessaires
et on attend qu'elles terminent toutes ou soient stoppées.
C'est à ce stade qu'est effectuée l'expansion des variables.
Une fois qu'elles ont toutes terminé on déduit le statut de terminaison
de la `cmd_2` et on l'utilise pour remonter dans l'arbre.
Cette méthode purement itérative d'exécution permet l'interruption et la
reprise des commandes: il suffit de stocker la `cmd_2` en cours d'exécution
ainsi que quelques informations placées sur une pile propre à la commande
(redirections de `cmd_3`, état des boucles `for`...),
ce qui est réalisé par la structure `ecmd_2`.
