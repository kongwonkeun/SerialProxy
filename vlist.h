#ifndef VLIST_H
#define VLIST_H

#include <stdio.h>

struct vlist_i 
{
    void *data;
    struct vlist_i *prev, *next;
};
typedef struct vlist_i vlist_i;

struct vlist_s
{
    vlist_i *head, *tail;
    int  count;
    int  owner;
    void (*destroyitem)(void *);
};
typedef struct vlist_s vlist_s;
typedef void vlist_destroy_func_t(void *);

int   vlist_init(vlist_s *st, vlist_destroy_func_t destroyitem);
void  vlist_cleanup(vlist_s *st);
void  vlist_clear(vlist_s *st);
void  vlist_delete(vlist_s *st, vlist_i *it);
void *vlist_get(vlist_i *it);

vlist_i *vlist_add(vlist_s *st, vlist_i *it, void *data);
vlist_i *vlist_insert(vlist_s *st, vlist_i *it, void *data);
vlist_i *vlist_findfwd(vlist_i *it, void *data);
vlist_i *vlist_findrev(vlist_i *it, void *data);

#endif	/* VLIST_H */
