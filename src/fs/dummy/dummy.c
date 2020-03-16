#include "dummy.h"
#include "../../util/string.h"
#include "../../kernel/kutil.h"

#define INO_NB 128
#define SZ (sizeof(block_t) * 1024)
static char device[SZ];
static uint64_t offset = 0;
static const int nb_dir = BUFSIZE / sizeof(struct dirent);

void *dummy_mount() {
    char *ret = device + offset;
    if (ret > device + SZ) return 0;

    struct super_block *sp = (struct super_block*)ret;
    sp->block_size = sizeof(struct block);
    sp->inode_nb = INO_NB;

    struct block *root = (struct block*)(ret + sizeof(block_t));
    root->blk_ino.ino_kind = KD_DIR;
    root->blk_ino.ino_size = 0;

    // allocate 128 blocks ny mount
    offset += sizeof(struct super_block) + sizeof(block_t) * INO_NB;

    // set all remaining blocks to 0
    for (char *ptr = (char*)(root + (sizeof(struct block)));
         ptr < (char*)offset; *ptr++ = 0);

    return ret;
}

struct dirent *dummy_readdir(struct dirent *dir) {
    klogf(Log_info, "dumb", "readdir");
    dir++;
    if (dir->ino == 0) return 0;
    return dir;
}

int dummy_load(void *super, char *fname, struct stat *st, char **end) {
    block_t *start_sector = (block_t*)super;
    block_t *curr = start_sector + 1;
    ino_t p = 1; // last existing parent
    struct dirent *dir = (struct dirent*)(&curr->blk_content);
    char *ptr;

    klogf(Log_info, "dumb", "load");

    if (!end || !*end) return -1;

    // assert fname start with /
    while (dir && *fname++) {
        if (curr->blk_ino.ino_kind != KD_DIR) {
            return -1;
        }

        ptr = uindex(fname, '/');
        while((dir = dummy_readdir(dir))) {
            if (!ustrncmp(dir->fname, fname, *end - fname)) {
                p = dir->ino;
                curr = start_sector + dir->ino;
                *end = fname;
                fname = ptr;
                break;
            }
        }
    }

    if (!dir) {
        (*end)++;
        st->st_ino = p;
        return -1;
    }


    st->st_ino = dir->ino;
    // TODO : fill structure

    return 0;
}

int dummy_create(void *super, ino_t parent, char *fname) {
    block_t *start_sector = (block_t*)super;
    block_t *p = start_sector + parent;
    struct super_block *sp = (struct super_block*)super;

    if (p->blk_ino.ino_kind != KD_DIR) {
        klogf(Log_error, "dumb", "not directory");
        return -1;
    }

    struct dirent *dir = (struct dirent*)&p->blk_content;
    size_t c;
    for (c = 0; c < nb_dir && dir->ino; c++, dir++);

    if (c == nb_dir) {
        klogf(Log_error, "dumb", "no free spot in directory");
        return -1;
    }

    ino_t ino;
    for (ino = 2; ino < sp->inode_nb; ino++) {
        p = start_sector + ino;
        if (p->blk_ino.ino_kind == KD_FREE) break;
    }

    if (ino == sp->inode_nb) {
        klogf(Log_error, "dumb", "no free inode");
        return -1;
    }

    dir->ino = ino;
    ustrncpy(dir->fname, fname, 255);

    p->blk_ino.ino_kind = KD_USED;
    p->blk_ino.ino_size = 0;
    return ino;
}

int dummy_read(void *super, ino_t ino, void *buf, size_t len) {
    block_t *start_sector = (block_t*)super;
    block_t *b = start_sector + ino;
    if (b->blk_ino.ino_kind != KD_USED) return -1;

    char *dst = (char*)buf;
    size_t c;
    for (c = 0; c < len && b->blk_pos < BUFSIZE; c++)
        *dst++ = b->blk_content[b->blk_pos++];
    return c;
}

int dummy_write(void *super, ino_t ino, void *buf, size_t len) {
    block_t *start_sector = (block_t*)super;
    block_t *b = start_sector + ino;
    if (b->blk_ino.ino_kind != KD_USED) return -1;

    char *src = (char*)buf;
    size_t c;
    for (c = 0; c < len && b->blk_pos < BUFSIZE; c++)
        b->blk_content[b->blk_pos++] = *src++;
    return c;
}

int dummy_seek(void *super, ino_t ino, off_t pos) {
    block_t *start_sector = (block_t*)super;
    block_t *b = start_sector + ino;
    if (b->blk_ino.ino_kind != KD_USED) return -1;

    b->blk_pos = pos;
    return 0;
}
