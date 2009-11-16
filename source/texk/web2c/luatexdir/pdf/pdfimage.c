/* pdfimage.c
   
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

static void place_img(PDF pdf, image_dict * idict, scaled_whd dim,
                      int transform)
{
    float a[6];                 /* transformation matrix */
    float xoff, yoff, tmp;
    pdfstructure *p = pdf->pstruct;
    scaledpos pos = pdf->posstruct->pos;
    int r;                      /* number of digits after the decimal point */
    int k;
    scaledpos tmppos;
    pdffloat cm[6];
    integer groupref;           /* added from web for 1.40.8 */
    assert(idict != 0);
    assert(p != NULL);
    a[0] = a[3] = 1.0e6;
    a[1] = a[2] = 0;
    if (img_type(idict) == IMG_TYPE_PDF
        || img_type(idict) == IMG_TYPE_PDFSTREAM) {
        a[0] /= img_xsize(idict);
        a[3] /= img_ysize(idict);
        xoff = (float) img_xorig(idict) / img_xsize(idict);
        yoff = (float) img_yorig(idict) / img_ysize(idict);
        r = 6;
    } else {
        /* added from web for 1.40.8 */
        if (img_type(idict) == IMG_TYPE_PNG) {
            groupref = img_group_ref(idict);
            if ((groupref > 0) && (pdf->img_page_group_val == 0))
                pdf->img_page_group_val = groupref;
        }
        /* /added from web */
        a[0] /= one_hundred_bp;
        a[3] = a[0];
        xoff = yoff = 0;
        r = 4;
    }
    if ((transform & 7) > 3) {  // mirror cases
        a[0] *= -1;
        xoff *= -1;
    }
    switch ((transform + img_rotation(idict)) & 3) {
    case 0:                    /* no transform */
        break;
    case 1:                    /* rot. 90 deg. (counterclockwise) */
        a[1] = a[0];
        a[2] = -a[3];
        a[3] = a[0] = 0;
        tmp = yoff;
        yoff = xoff;
        xoff = -tmp;
        break;
    case 2:                    /* rot. 180 deg. (counterclockwise) */
        a[0] *= -1;
        a[3] *= -1;
        xoff *= -1;
        yoff *= -1;
        break;
    case 3:                    /* rot. 270 deg. (counterclockwise) */
        a[1] = -a[0];
        a[2] = a[3];
        a[3] = a[0] = 0;
        tmp = yoff;
        yoff = -xoff;
        xoff = tmp;
        break;
    default:;
    }
    xoff *= dim.wd;
    yoff *= dim.ht + dim.dp;
    a[0] *= dim.wd;
    a[1] *= dim.ht + dim.dp;
    a[2] *= dim.wd;
    a[3] *= dim.ht + dim.dp;
    a[4] = pos.h - xoff;
    a[5] = pos.v - yoff;
    k = transform + img_rotation(idict);
    if ((transform & 7) > 3)
        k++;
    switch (k & 3) {
    case 0:                    /* no transform */
        break;
    case 1:                    /* rot. 90 deg. (counterclockwise) */
        a[4] += dim.wd;
        break;
    case 2:                    /* rot. 180 deg. */
        a[4] += dim.wd;
        a[5] += dim.ht + dim.dp;
        break;
    case 3:                    /* rot. 270 deg. */
        a[5] += dim.ht + dim.dp;
        break;
    default:;
    }
    /* the following is a kludge, TODO: use pdfpage.c functions */
    setpdffloat(cm[0], round(a[0]), r);
    setpdffloat(cm[1], round(a[1]), r);
    setpdffloat(cm[2], round(a[2]), r);
    setpdffloat(cm[3], round(a[3]), r);
    tmppos.h = round(a[4]);
    tmppos.v = round(a[5]);
    pdf_goto_pagemode(pdf);
    (void) calc_pdfpos(p, tmppos);
    cm[4] = p->cm[4];
    cm[5] = p->cm[5];
    if (pdf->img_page_group_val == 0)
        pdf->img_page_group_val = img_group_ref(idict); /* added from web for 1.40.8 */
    pdf_printf(pdf, "q\n");
    pdf_print_cm(pdf, cm);
    pdf_printf(pdf, "/Im");
    pdf_print_int(pdf, img_index(idict));
    pdf_print_resname_prefix(pdf);
    pdf_printf(pdf, " Do\nQ\n");
    if (lookup_object_list(pdf, obj_type_ximage, img_objnum(idict)) == NULL)
        append_object_list(pdf, obj_type_ximage, img_objnum(idict));
    if (img_state(idict) < DICT_OUTIMG)
        img_state(idict) = DICT_OUTIMG;
}

/* for normal output, see pdflistout.c */

void pdf_place_image(PDF pdf, halfword p)
{
    scaled_whd dim;
    image_dict *idict = idict_array[pdf_ximage_index(p)];
    assert(idict != NULL);
    dim.wd = width(p);
    dim.ht = height(p);
    dim.dp = depth(p);
    place_img(pdf, idict, dim, pdf_ximage_transform(p));
}

/* for images in virtual fonts through Lua, see vf_out_image() in limglib.c */

void pdf_place_vf_img(PDF pdf, image * img)
{
    image_dict *idict;
    assert(img != NULL);
    idict = img_dict(img);
    assert(idict != NULL);
    place_img(pdf, idict, img_dimen(img), img_transform(img));
}
