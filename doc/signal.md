# Signaux

Les signaux sont numérotés au sein de l'interface des processus par leur
`signum` compris entre `0` et `32`. Au sein du kernel et de la libc
on utilise aussi `sigid = signum - 1`.
Les signaux peuvent être contrôlés via la fonction `signal`. Il s'agit
d'une fonction de la libc qui se charge de lancer l'appel système
correspondant (`_signal`). En effet, dans le cas de l'installation
d'un gestionnaire de signal, c'est la libc qui se charge d'appeler
la bonne fonction lors de la réception d'un signal:
la fonction `libc_sighandler` est appelée par le kernel, elle appelle le 
gestionnaire correspondant au `sigid` puis lance l'appel système
`sigreturn`.

### Signaux avec actions par défaut

| SIGNUM | Nom     | Action                        |
|:------:|:-------:|:----------------------------- |
|   1    | SIGHUP  | terminaison                   |
|   2    | SIGINT  | terminaison                   |
|   9    | SIGKILL | terminaison, non contrôlable  |
|   17   | SIGSTOP | arrêt, non contrôlable        |
|   18   | SIGTSTP | arrêt                         |
|   19   | SIGCONT | reprise, même si gestionnaire |
