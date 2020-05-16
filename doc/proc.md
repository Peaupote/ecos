# Processus

Les processus peuvent être dans les états suivants:

 - FREE  `f`
 - SLEEP `S`
 - BLOCK `B`
 - BLOCR `L`
 - RUN   `R`
 - ZOMB  `Z`
 - STOP  `T`

Ils sont généralement exécutés en `ring 3`.

Afin de pouvoir répondre efficacement aux appels systèmes `wait`,
on chaîne les enfants d'un même processus. On fait de plus en sorte de
placer les éventuels `ZOMB` en début de liste via la structure suivante:
```
            ----------            ------              
p_fchd --->|   ZOMB   |p_nxsb -->| non  |p_nxsb --> ... non
-1 = p_prsb|          |<-- p_prsb| ZOMB |<-- p_prsb ... ZOMB
            ----------            ------
            p_nxzb  ^    
               |    |          ------
               |     -- p_prsb| ZOMB |p_nxzb --> ... ZOMB
                ------------->|      |<-- p_prsb ...
                               ------
```

### Appels systèmes

Les appels systèmes sont effectués via l'interruption `0x80`.
Les processus lancés en `ring 1` ont aussi accès à des appels pour exécuter
des parties en `ring 0` ou se synchroniser (int `0x7e` et `0x7f`).

Si l'appel système entraîne un blocage (*wait*, *read*...), le processus est
passé dans l'état BLOCK et n'est pas replacé dans le scheduler.
Les valeurs des registres au moment de l'appel système sont sauvegardées.
Lorsqu'un évènement est susceptible de débloquer le processus, il est passé
en mode BLOCR et ajouté dans le scheduler (pour *wait* il est exécuté
directement lors de la mort de l'enfant).
Lorsqu'il est choisi pour être exécuté, on relance à nouveau l'appel système
avec les paramètres sauvegardés.
