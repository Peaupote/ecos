### Syntaxe

```
cmd_0 = <sous-shell>
      | cmd args...

cmd_1 = <cmd_0> <redirections>

cmd_2 = sep_list(<cmd_1>, |)

cmd_3a = <cmd_2>
       | { <cmd_3c> ; } <redirections>
       | while <cmd_3c> ; do <cmd_3c> ; done <redirections>


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
- `jobs`
La builtin `fg cmd_num` ne peut être utilisée qu'isolée, sans redirections.
