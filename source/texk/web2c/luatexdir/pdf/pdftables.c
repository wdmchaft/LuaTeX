/* pdftables.c

   Copyright 2009 Taco Hoekwater <taco@luatex.org>

   This file is part of LuaTeX.

   LuaTeX is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 2 of the License, or (at your
   option) any later version.

   LuaTeX is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
   License for more details.

   You should have received a copy of the GNU General Public License along
   with LuaTeX; if not, see <http://www.gnu.org/licenses/>. */

static const char __svn_version[] =
    "$Id$"
    "$URL$";

#include "ptexlib.h"

/**********************************************************************/
/* One AVL tree each for a few obj_type out of 0...PDF_OBJ_TYPE_MAX */

/* mark which objects should be searchable through AVL tree */

static int obj_in_tree[PDF_OBJ_TYPE_MAX + 1] = {
    0,                          /* obj_type_font = 0,       */
    0,                          /* obj_type_outline = 1,    */
    1,                          /* obj_type_dest = 2,       */
    0,                          /* obj_type_obj = 3,        */
    0,                          /* obj_type_xform = 4,      */
    0,                          /* obj_type_ximage = 5,     */
    1,                          /* obj_type_thread = 6,     */
    /* the ones below don't go into a linked list */
    0,                          /* obj_type_pagestream = 7, */
    1,                          /* obj_type_page = 8,       */
    0,                          /* obj_type_pages = 9,      */
    0,                          /* obj_type_link = 10,      */
    0,                          /* obj_type_bead = 11,      */
    0,                          /* obj_type_annot = 12,     */
    0,                          /* obj_type_objstm = 13,    */
    0                           /* obj_type_others = 14     */
};

/* AVL sort oentry into avl_table[] */

static int compare_info(const void *pa, const void *pb, void *param)
{
    const oentry *a, *b;
    (void) param;
    a = (const oentry *) pa;
    b = (const oentry *) pb;
    if (a->u_type == b->u_type) {
        if (a->u_type == union_type_int)
            return ((a->u.int0 <
                     b->u.int0 ? -1 : (a->u.int0 > b->u.int0 ? 1 : 0)));
        else                    /* string type */
            return strcmp(a->u.str0, b->u.str0);
    } else if (a->u_type == union_type_int)
        return -1;
    else
        return 1;
}

static void avl_put_obj(PDF pdf, integer t, oentry * oe)
{
    void **pp;
    assert(t >= 0 || t <= PDF_OBJ_TYPE_MAX || obj_in_tree[t] == 1);
    if (pdf->obj_tree[t] == NULL) {
        pdf->obj_tree[t] = avl_create(compare_info, NULL, &avl_xallocator);
        if (pdf->obj_tree[t] == NULL)
            pdftex_fail("avlstuff.c: avl_create() pdf->obj_tree failed");
    }
    pp = avl_probe(pdf->obj_tree[t], oe);
    if (pp == NULL)
        pdftex_fail("avlstuff.c: avl_probe() out of memory in insertion");
}

static void avl_put_int_obj(PDF pdf, integer int0, integer objptr, integer t)
{
    oentry *oe;
    oe = xtalloc(1, oentry);
    oe->u.int0 = int0;
    oe->u_type = union_type_int;
    oe->objptr = objptr;
    avl_put_obj(pdf, t, oe);
}

static void avl_put_str_obj(PDF pdf, char *str0, integer objptr, integer t)
{
    oentry *oe;
    oe = xtalloc(1, oentry);
    oe->u.str0 = str0;          /* no xstrdup() here */
    oe->u_type = union_type_cstring;
    oe->objptr = objptr;
    avl_put_obj(pdf, t, oe);
}

static integer avl_find_int_obj(PDF pdf, integer t, integer i)
{
    oentry *p;
    oentry tmp;
    assert(t >= 0 || t <= PDF_OBJ_TYPE_MAX || obj_in_tree[t] == 1);
    tmp.u.int0 = i;
    tmp.u_type = union_type_int;
    if (pdf->obj_tree[t] == NULL)
        return 0;
    p = (oentry *) avl_find(pdf->obj_tree[t], &tmp);
    if (p == NULL)
        return 0;
    return p->objptr;
}

static integer avl_find_str_obj(PDF pdf, integer t, char *s)
{
    oentry *p;
    oentry tmp;
    assert(t >= 0 || t <= PDF_OBJ_TYPE_MAX || obj_in_tree[t] == 1);
    tmp.u.str0 = s;
    tmp.u_type = union_type_cstring;
    if (pdf->obj_tree[t] == NULL)
        return 0;
    p = (oentry *) avl_find(pdf->obj_tree[t], &tmp);
    if (p == NULL)
        return 0;
    return p->objptr;
}

/**********************************************************************/
/* create an object with type |t| and identifier |i| */

void pdf_create_obj(PDF pdf, integer t, integer i)
{
    integer a, p, q;
    char *ss = NULL;
    if (pdf->sys_obj_ptr == sup_obj_tab_size)
        overflow("indirect objects table size", pdf->obj_tab_size);
    if (pdf->sys_obj_ptr == pdf->obj_tab_size) {
        a = 0.2 * pdf->obj_tab_size;
        if (pdf->obj_tab_size < sup_obj_tab_size - a)
            pdf->obj_tab_size = pdf->obj_tab_size + a;
        else
            pdf->obj_tab_size = sup_obj_tab_size;
        pdf->obj_tab =
            xreallocarray(pdf->obj_tab, obj_entry, pdf->obj_tab_size);
    }
    incr(pdf->sys_obj_ptr);
    pdf->obj_ptr = pdf->sys_obj_ptr;
    obj_info(pdf, pdf->obj_ptr) = i;
    obj_type(pdf, pdf->obj_ptr) = t;
    set_obj_fresh(pdf, pdf->obj_ptr);
    obj_aux(pdf, pdf->obj_ptr) = 0;
    if (i < 0) {
        ss = makecstring(-i);
        avl_put_str_obj(pdf, ss, pdf->obj_ptr, t);
    } else
        avl_put_int_obj(pdf, i, pdf->obj_ptr, t);
    if (t <= HEAD_TAB_MAX) {
        obj_link(pdf, pdf->obj_ptr) = pdf->head_tab[t];
        pdf->head_tab[t] = pdf->obj_ptr;
        if ((t == obj_type_dest) && (i < 0))
            append_dest_name(pdf, makecstring(-obj_info(pdf, pdf->obj_ptr)),
                             pdf->obj_ptr);
    }
}

integer find_obj(PDF pdf, integer t, integer i, boolean byname)
{
    char *ss = NULL;
    integer ret;
    assert(i >= 0);             /* no tricks */
    if (byname) {
        ss = makecstring(i);
        ret = avl_find_str_obj(pdf, t, ss);
        free(ss);
    } else {
        ret = avl_find_int_obj(pdf, t, i);
    }
    return ret;
}

/* The following function finds an object with identifier |i| and type |t|.
   Identifier |i| is either an integer or a token list index. If no
   such object exists then it will be created. This function is used mainly to
   find destination for link annotations and outlines; however it is also used
   in |ship_out| (to check whether a Page object already exists) so we need
   to declare it together with subroutines needed in |hlist_out| and
   |vlist_out|.
*/

integer get_obj(PDF pdf, integer t, integer i, boolean byname)
{
    integer r;
    str_number s;
    assert(i >= 0);
    if (byname > 0) {
        s = tokens_to_string(i);
        r = find_obj(pdf, t, s, true);
    } else {
        s = 0;
        r = find_obj(pdf, t, i, false);
    }
    if (r == 0) {
        if (byname > 0) {
            pdf_create_obj(pdf, t, -s);
            s = 0;
        } else {
            pdf_create_obj(pdf, t, i);
        }
        r = pdf->obj_ptr;
        if (t == obj_type_dest)
            set_obj_dest_ptr(pdf, r, null);
    }
    if (s != 0)
        flush_str(s);
    return r;
}

/* create a new object and return its number */
integer pdf_new_objnum(PDF pdf)
{
    pdf_create_obj(pdf, obj_type_others, 0);
    return pdf->obj_ptr;
}

void check_obj_exists(PDF pdf, integer t, integer objnum)
{
    if (objnum < 0 || objnum > pdf->obj_ptr)
        pdf_error("ext1", "cannot find referenced object");
    if (t != obj_type(pdf, objnum))
        pdf_error("ext1", "referenced object has wrong type");
}

void set_rect_dimens(PDF pdf, halfword p, halfword parent_box, scaledpos cur,
                     scaled_whd alt_rule, scaled margin)
{
    scaledpos ll, ur;           /* positions relative to cur */
    scaledpos pos_ll, pos_ur, tmp;
    posstructure localpos;
    localpos.dir = pdf->posstruct->dir;
    ll.h = 0;                   /* pdf contains current point on page */
    if (is_running(alt_rule.dp))
        ll.v = depth(parent_box) - cur.v;
    else
        ll.v = alt_rule.dp;
    if (is_running(alt_rule.wd))
        ur.h = width(parent_box) - cur.h;
    else
        ur.h = alt_rule.wd;
    if (is_running(alt_rule.ht))
        ur.v = -height(parent_box) - cur.v;
    else
        ur.v = -alt_rule.ht;

    synch_pos_with_cur(&localpos, pdf->posstruct, ll);
    pos_ll = localpos.pos;
    synch_pos_with_cur(&localpos, pdf->posstruct, ur);
    pos_ur = localpos.pos;

    if (pos_ll.h > pos_ur.h) {
        tmp.h = pos_ll.h;
        pos_ll.h = pos_ur.h;
        pos_ur.h = tmp.h;
    }
    if (pos_ll.v > pos_ur.v) {
        tmp.v = pos_ll.v;
        pos_ll.v = pos_ur.v;
        pos_ur.v = tmp.v;
    }
    if (is_shipping_page && matrixused()) {
        matrixtransformrect(pos_ll.h, pos_ll.v, pos_ur.h, pos_ur.v);
        pos_ll.h = getllx();
        pos_ll.v = getlly();
        pos_ur.h = geturx();
        pos_ur.v = getury();
    }
    pdf_ann_left(p) = pos_ll.h - margin;
    pdf_ann_bottom(p) = pos_ll.v - margin;
    pdf_ann_right(p) = pos_ur.h + margin;
    pdf_ann_top(p) = pos_ur.v + margin;
}

void libpdffinish(PDF pdf)
{
    fb_free(pdf);
    xfree(pdf->job_id_string);
    fm_free();
    t1_free();
    enc_free();
    epdf_free();
    ttf_free();
    sfd_free();
    glyph_unicode_free();
    zip_free(pdf);
}

/*
Store some of the pdftex data structures in the format. The idea here is
to ensure that any data structures referenced from pdftex-specific whatsit
nodes are retained. For the sake of simplicity and speed, all the filled parts
of |pdf->mem| and |obj_tab| are retained, in the present implementation. We also
retain three of the linked lists that start from |head_tab|, so that it is
possible to, say, load an image in the \.{INITEX} run and then reference it in a
\.{VIRTEX} run that uses the dumped format.
*/

void dump_pdftex_data(PDF pdf)
{
    integer k, x;
    pdf_object_list *l;
    dumpimagemeta();            /* the image information array */
    dump_int(pdf->mem_size);
    dump_int(pdf->mem_ptr);
    for (k = 1; k <= pdf->mem_ptr - 1; k++) {
        x = pdf->mem[k];
        dump_int(x);
    }
    print_ln();
    print_int(pdf->mem_ptr - 1);
    tprint(" words of pdf memory");
    x = pdf->obj_tab_size;
    dump_int(x);
    x = pdf->obj_ptr;
    dump_int(x);
    x = pdf->sys_obj_ptr;
    dump_int(x);
    for (k = 1; k <= pdf->sys_obj_ptr; k++) {
        x = obj_info(pdf, k);
        dump_int(x);
        x = obj_link(pdf, k);
        dump_int(x);
        x = obj_os_idx(pdf, k);
        dump_int(x);
        x = obj_aux(pdf, k);
        dump_int(x);
        x = obj_type(pdf, k);
        dump_int(x);
    }
    print_ln();
    print_int(pdf->sys_obj_ptr);
    tprint(" indirect objects");
    dump_int(pdf->obj_count);
    dump_int(pdf->xform_count);
    dump_int(pdf->ximage_count);
    if (pdf->resources != NULL && (l = pdf->resources->obj_list) != NULL) {
        while (l != NULL) {
            dump_int(l->info);
            l = l->link;
        }
    }
    dump_int(0);                /* signal end of obj_list */
    if (pdf->resources != NULL && (l = pdf->resources->xform_list) != NULL) {
        while (l != NULL) {
            dump_int(l->info);
            l = l->link;
        }
    }
    dump_int(0);                /* signal end of xform_list */
    if (pdf->resources != NULL && (l = pdf->resources->ximage_list) != NULL) {
        while (l != NULL) {
            dump_int(l->info);
            l = l->link;
        }
    }
    dump_int(0);                /* signal end of ximage_list */

    x = pdf->head_tab[obj_type_obj];
    dump_int(x);
    x = pdf->head_tab[obj_type_xform];
    dump_int(x);
    x = pdf->head_tab[obj_type_ximage];
    dump_int(x);
    dump_int(pdf_last_obj);
    dump_int(pdf_last_xform);
    dump_int(pdf_last_ximage);
}

/*
And restoring the pdftex data structures from the format. The
two function arguments to |undumpimagemeta| have been restored
already in an earlier module.
*/

void undump_pdftex_data(PDF pdf)
{
    integer k, x;
    undumpimagemeta(pdf, pdf_minor_version, pdf_inclusion_errorlevel);  /* the image information array */
    undump_int(pdf->mem_size);
    pdf->mem = xreallocarray(pdf->mem, int, pdf->mem_size);
    undump_int(pdf->mem_ptr);
    for (k = 1; k <= pdf->mem_ptr - 1; k++) {
        undump_int(x);
        pdf->mem[k] = (int) x;
    }
    undump_int(x);
    pdf->obj_tab_size = x;
    undump_int(x);
    pdf->obj_ptr = x;
    undump_int(x);
    pdf->sys_obj_ptr = x;
    for (k = 1; k <= pdf->sys_obj_ptr; k++) {
        undump_int(x);
        obj_info(pdf, k) = x;
        undump_int(x);
        obj_link(pdf, k) = x;
        set_obj_offset(pdf, k, -1);
        undump_int(x);
        obj_os_idx(pdf, k) = x;
        undump_int(x);
        obj_aux(pdf, k) = x;
        undump_int(x);
        obj_type(pdf, k) = x;
    }

    undump_int(x);
    pdf->obj_count = x;
    undump_int(x);
    pdf->xform_count = x;
    undump_int(x);
    pdf->ximage_count = x;

    /* todo : the next 3 loops can be done much more efficiently */
    undump_int(x);
    while (x != 0) {
        append_object_list(pdf, obj_type_obj, x);
        undump_int(x);
    }
    undump_int(x);
    while (x != 0) {
        append_object_list(pdf, obj_type_xform, x);
        undump_int(x);
    }
    undump_int(x);
    while (x != 0) {
        append_object_list(pdf, obj_type_ximage, x);
        undump_int(x);
    }

    undump_int(x);
    pdf->head_tab[obj_type_obj] = x;
    undump_int(x);
    pdf->head_tab[obj_type_xform] = x;
    undump_int(x);
    pdf->head_tab[obj_type_ximage] = x;
    undump_int(pdf_last_obj);
    undump_int(pdf_last_xform);
    undump_int(pdf_last_ximage);
}
