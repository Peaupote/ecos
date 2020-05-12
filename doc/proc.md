# Processus

Les processus peuvent être dans les états suivants:

 - FREE  `f`
 - SLEEP `S`
 - BLOCK `B`
 - BLOCR `L`
 - RUN   `R`
 - ZOMB  `Z`
 - STOP  `T`

Afin de pouvoir répondre efficacement aux appels systèmes `wait`,
on chaîne les enfants d'un même processus. On fait de plus en sorte de
placer les `ZOMB` en début via la structure suivante:
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
