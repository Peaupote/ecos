Pour ajouter une système de fichier, il faut implémenter l'ensemble des fonctions de la structure `fs` définie dans [kernel/file.h](../include/kernel/file.h).

```
struct fs {
    char                 fs_name[8];
    fs_mnt_t            *fs_mnt;
    fs_lookup_t         *fs_lookup;
    fs_stat_t           *fs_stat;
    fs_rdwr_t           *fs_read;
    fs_rdwr_t           *fs_write;
    fs_create_t         *fs_touch;
    fs_create_t         *fs_mkdir;
    fs_truncate_t       *fs_truncate;
    fs_getdents_t       *fs_getdents;
    fs_opench_t         *fs_opench;
    fs_open_t           *fs_open;
    fs_close_t          *fs_close;
    fs_rm_t             *fs_rm;
    fs_destroy_dirent_t *fs_destroy_dirent;
    fs_link_t           *fs_link;
    fs_symlink_t        *fs_symlink;
    fs_readlink_t       *fs_readlink;
};
```

Ces fonctions sont ensuite appellées par le système de fichier virtuel. C'est l'ensemble des opérations que le système supporte pour le moment.

On ne permet pas de monter dynamiquement un système de fichier. Les différents points de montages sont indiqués en dur dans le code (fonction `vfs_init`). On ajoute alors une entrée dans le tableau des devices

```
struct device {
    dev_t             dev_id;

    dev_t             dev_parent_dev;
    ino_t             dev_parent_red; // ino du dossier remplacé par le dev
    ino_t             dev_parent_ino;

    // liste chainée des dev montés comme enfants
    dev_t             dev_childs;
    dev_t             dev_sib;

    uint8_t           dev_free;
    uint8_t           dev_fs;
    struct mount_info dev_info;
};
```

qui permettra de faire l'association entre le nom du fichier pour l'utilisateur, la position du fichier dans la mémoire et le système de fichier à utiliser.

## ext2fs

Notre implémentation de ext2 est loin d'être complète ou optimale. En particulier, elle ne gère pas la supression de fichiers. Plus d'informations sur les détails de ext2fs [ici](https://www.nongnu.org/ext2-doc/ext2.html).

## procfs

Notre implémentation de procfs réuni en réalité les idées de `procfs` et de `devfs`. En effet, à travers ce système de fichier nous pouvous accéder à des informations sur l'état noyau ; mais procfs est en plus un pont entre le hardware et le système de fichier virtuel. Les entrées clavier sont directement redirigées sur le fichier `/proc/tty/tty0` et l'affichage suit ce qui est écrit sur le fichier `/proc/tty/tty1`. De plus, le système de fichier virtuel accède aux pipes grâce à leur numéro d'inoeud dans `procfs`.

Les actions en écriture sur ce système de fichier sont sans effet.
