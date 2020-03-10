#include "file.h"
#include "proc.h"

/**
 * Red-black tree implementation for file system
 */

enum color_t { BLACK, RED };

typedef struct node {
    struct file f;

    // red-black tree structure
    struct node *parent, *left, *right;
    enum color_t color;
} node_t;

node_t *grandparent(node_t *node) {
    node_t *parent = node ? node : node->parent;
    return parent ? parent : parent->parent;
}

node_t *sibling(node_t *node) {
    // no parent means no sibling
    if (!node || !node->parent) return 0;

    node_t *p = node->parent;
    return p->left == node ? p->right : p->left;
}

node_t *uncle(node_t *node) {
    if (!node || !node->parent) return 0;
    return sibling(node->parent);
}

void rot_left(node_t *n) {
    node_t *nnew = n->right;
    node_t *p = n->parent;

    n->right = nnew->left;
    nnew->left = n;
    n->parent = nnew;

    if (n->right) n->right->parent = n;

    if (p) {
        if (n == p->left) p->left = nnew;
        else p->right = nnew;
    }

    nnew->parent = p;
}

void rot_right(node_t* n) {
    node_t* nnew = n->left;
    node_t* p = n->parent;

    n->left = nnew->right;
    nnew->right = n;
    n->parent = nnew;

    if (n->left) n->left->parent = n;

    if (p) {
        if (n == p->left) p->left = nnew;
        else p->right = nnew;
    }

    nnew->parent = p;
}

void insert_rec(node_t* root, node_t* n) {
    if (root) {
        if (n->f.f_inode < root->f.f_inode) {
            if (root->left) {
                insert_rec(root->left, n);
                return;
            }

            root->left = n;
        } else {
            if (root->right) {
                insert_rec(root->right, n);
                return;
            }

            root->right = n;
        }
    }

    n->parent = root;
    n->left = 0;
    n->right = 0;
    n->color = RED;
}

void insert_fix_tree(node_t* n) {
    if (!n) return;
    if (!n->parent) n->color = BLACK;
    else if (n->parent->color == BLACK) return;
    else if (!uncle(n) && uncle(n)->color == RED) {
        n->parent->color = BLACK;
        uncle(n)->color = BLACK;

        node_t *gparent = grandparent(n);
        gparent->color = RED;
        insert_fix_tree(gparent);
    } else {
        node_t* p = n->parent;
        node_t* g = p->parent;

        if (n == p->right && p == g->left) {
            rot_left(p);
            n = n->left;
        } else if (n == p->left && p == g->right) {
            rot_right(p);
            n = n->right;
        }

        p = n->parent;
        g = p->parent;

        if (n == p->left) rot_right(g);
        else rot_left(g);

        p->color = BLACK;
        g->color = RED;
    }
}


node_t* insert(node_t* root, node_t* n) {
    insert_rec(root, n);
    insert_fix_tree(n);

    for (root = n; root->parent; root = root->parent);
    return root;
}


buf_t *load_buffer(ino_t file, pos_t offset) {
    bid_t id;
    for (id = 0; id < NBUF && state.st_buf[id].buf_size > 0; id++);
    if (id == NBUF) return 0;

    buf_t *buf = &state.st_buf[id];

    // for now fill buffer with 'a'
    for (size_t i = 0; i < BUFSIZE; i++)
        buf->buf_content[i] = 'a';

    buf->buf_inode = file;
    buf->buf_offset = offset;
    buf->buf_pv = 0;
    buf->buf_nx = 0;
    buf->buf_size = BUFSIZE;
    return buf;
}
