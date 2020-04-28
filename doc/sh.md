### Syntaxe

```
cmd_0 	= <sous-shell>
		| cmd args...

cmd_1	= <cmd_0> <redirections>

cmd_2	= sep_list(<cmd_1>, |)

cmd_3a	= <cmd_2>
		| \( <cmd_3c> \)

cmd_3b  = <cmd_3a>
		| <cmd_3b> && <cmd_3a>
		| <cmd_3b> || <cmd_3a>

cmd_3c	= sep_list(<cmd_3c>?, [;\n&])

sous_shell	= $\( <cmd_3c> \)
```

Priorité des opérateurs (décroissante):
 - `|`
 - `&&` `||` associativité gauche
 - `&` 
 - `;`
