/* Copyright (C) 2000-2007 by George Williams */
/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.

 * The name of the author may not be used to endorse or promote products
 * derived from this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "pfaeditui.h"
#include <utype.h>
#include <ustring.h>

int coverageformatsallowed=3;

#include "ttf.h"

#ifdef LUA_FF_LIB
extern void gwwv_post_error(char *a, ...);
extern int unic_isrighttoleft (int);
#define isrighttoleft  unic_isrighttoleft 
#endif

/* This file contains routines to create the otf gpos and gsub tables and their */
/*  attendant subtables */

/* Undocumented fact: ATM (which does kerning for otf fonts in Word) can't handle features with multiple lookups */

/* Undocumented fact: Only one feature with a given tag allowed per script/lang */
/*  So if we have multiple lookups with the same tag they must be merged into */
/*  one feature with many lookups */

/* scripts (for opentype) that I understand */
    /* see also list in lookups.c mapping script tags to friendly names */

static uint32 scripts[][15] = {
/* Arabic */	{ CHR('a','r','a','b'), 0x0600, 0x06ff, 0xfb50, 0xfdff, 0xfe70, 0xfeff },
/* Aramaic */	{ CHR('a','r','a','m'), 0x820, 0x83f },
/* Armenian */	{ CHR('a','r','m','n'), 0x0530, 0x058f, 0xfb13, 0xfb17 },
/* Balinese */	{ CHR('b','a','l','i'), 0x1b00, 0x1b7f },
/* Bengali */	{ CHR('b','e','n','g'), 0x0980, 0x09ff },
/* Bliss symb */{ CHR('b','l','i','s'), 0x12200, 0x124ff },
/* Bopomofo */	{ CHR('b','o','p','o'), 0x3100, 0x312f, 0x31a0, 0x31bf },
/* Braille */	{ CHR('b','r','a','i'), 0x2800, 0x28ff },
/* Buginese */	{ CHR('b','u','g','i'), 0x1a00, 0x1a1f },
/* Buhid */	{ CHR('b','u','h','d'), 0x1740, 0x1753 },
/* Byzantine M*/{ CHR('b','y','z','m'), 0x1d000, 0x1d0ff },
/* Canadian Syl*/{CHR('c','a','n','s'), 0x1400, 0x167f },
/* Cherokee */	{ CHR('c','h','e','r'), 0x13a0, 0x13ff },
/* Cirth */	{ CHR('c','i','r','t'), 0x12080, 0x120ff },
/* CJKIdeogra */{ CHR('h','a','n','i'), 0x3300, 0x9fff, 0xf900, 0xfaff, 0x020000, 0x02ffff },
/* Coptic */	{ CHR('c','o','p','t'), 0x2c80, 0x2cff },
/* Cypriot */	{ CHR('c','p','m','n'), 0x10800, 0x1083f },
/* Cyrillic */	{ CHR('c','y','r','l'), 0x0400, 0x052f },
/* Deseret */	{ CHR('d','s','r','t'), 0x10400, 0x1044f },
/* Devanagari */{ CHR('d','e','v','a'), 0x0900, 0x097f },
/* Ethiopic */	{ CHR('e','t','h','i'), 0x1200, 0x139f },
/* Georgian */	{ CHR('g','e','o','r'), 0x1080, 0x10ff },
/* Glagolitic */{ CHR('g','l','a','g'), 0x1080, 0x10ff },
/* Gothic */	{ CHR('g','o','t','h'), 0x10330, 0x1034a },
/* Greek */	{ CHR('g','r','e','k'), 0x0370, 0x03ff, 0x1f00, 0x1fff },
/* Gujarati */	{ CHR('g','u','j','r'), 0x0a80, 0x0aff },
/* Gurmukhi */	{ CHR('g','u','r','u'), 0x0a00, 0x0a7f },
/* Hangul */	{ CHR('h','a','n','g'), 0xac00, 0xd7af, 0x3130, 0x319f, 0xffa0, 0xff9f },
/* Hanunoo */	{ CHR('h','a','n','o'), 0x1720, 0x1734 },
 /* I'm not sure what the difference is between the 'hang' tag and the 'jamo' */
 /*  tag. 'Jamo' is said to be the precomposed forms, but what's 'hang'? */
/* Hebrew */	{ CHR('h','e','b','r'), 0x0590, 0x05ff, 0xfb1e, 0xfb4f },
#if 0	/* Hiragana used to have its own tag, but has since been merged with katakana */
/* Hiragana */	{ CHR('h','i','r','a'), 0x3040, 0x309f },
#endif
/* Hangul Jamo*/{ CHR('j','a','m','o'), 0x1100, 0x11ff, 0x3130, 0x319f, 0xffa0, 0xffdf },
/* Katakana */	{ CHR('k','a','n','a'), 0x3040, 0x30ff, 0xff60, 0xff9f },
/* Javanese */	{ CHR('j','a','v','a'), 0 },	/* MS has a tag, but there is no unicode range */
/* Khmer */	{ CHR('k','h','m','r'), 0x1780, 0x17ff },
/* Kannada */	{ CHR('k','n','d','a'), 0x0c80, 0x0cff },
/* Kharosthi */	{ CHR('k','h','a','r'), 0x10a00, 0x10a5f },
/* Latin */	{ CHR('l','a','t','n'), 0x0041, 0x005a, 0x0061, 0x007a,
	0x00c0, 0x02af, 0x1d00, 0x1eff, 0xfb00, 0xfb0f, 0xff00, 0xff5f, 0, 0 },
/* Lao */	{ CHR('l','a','o',' '), 0x0e80, 0x0eff },
/* Limbu */	{ CHR('l','i','m','b'), 0x1900, 0x194f },
/* Linear A */	{ CHR('l','i','n','a'), 0x10180, 0x102cf },
/* Linear B */	{ CHR('l','i','n','b'), 0x10000, 0x100fa },
/* Malayalam */	{ CHR('m','l','y','m'), 0x0d00, 0x0d7f },
/* Mathematical Alphanumeric Symbols */
		{ CHR('m','a','t','h'), 0x1d400, 0x1d7ff },
/* Mongolian */	{ CHR('m','o','n','g'), 0x1800, 0x18af },
/* Musical */	{ CHR('m','u','s','i'), 0x1d100, 0x1d1ff },
/* Myanmar */	{ CHR('m','y','m','r'), 0x1000, 0x107f },
/* N'Ko */	{ CHR('n','k','o',' '), 0x07c0, 0x07fa },
/* Ogham */	{ CHR('o','g','a','m'), 0x1680, 0x169f },
/* Old Italic */{ CHR('i','t','a','l'), 0x10300, 0x1031e },
/* Old Permic */{ CHR('p','e','r','m'), 0x10350, 0x1037f },
/* Old Persian cuneiform */
		{ CHR('x','p','e','o'), 0x103a0, 0x103df },
/* Oriya */	{ CHR('o','r','y','a'), 0x0b00, 0x0b7f },
/* Osmanya */	{ CHR('o','s','m','a'), 0x10480, 0x104a9 },
/* Phags-pa */	{ CHR('p','h','a','g'), 0xa840, 0xa87f },
/* Phoenician */{ CHR('p','h','n','x'), 0x10900, 0x1091f },
/* Pollard */	{ CHR('p','l','r','d'), 0x104b0, 0x104d9 },
/* Runic */	{ CHR('r','u','n','r'), 0x16a0, 0x16ff },
/* Shavian */	{ CHR('s','h','a','w'), 0x10450, 0x1047f },
/* Sinhala */	{ CHR('s','i','n','h'), 0x0d80, 0x0dff },
/* Sumero-Akkadian Cuneiform */
		{ CHR('x','s','u','x'), 0x12000, 0x1236e },
/* Syloti Nagri */
		{ CHR('s','y','l','o'), 0xa800, 0xa82f },
/* Syriac */	{ CHR('s','y','r','c'), 0x0700, 0x074f },
/* Tagalog */	{ CHR('t','a','g','l'), 0x1700, 0x1714 },
/* Tagbanwa */	{ CHR('t','a','g','b'), 0x1760, 0x1773 },
/* Tai Le */	{ CHR('t','a','l','e'), 0x1950, 0x1974 },
/* Tai Lu */	{ CHR('t','a','l','u'), 0x1980, 0x19df },
/* Tamil */	{ CHR('t','a','m','l'), 0x0b80, 0x0bff },
/* Telugu */	{ CHR('t','e','l','u'), 0x0c00, 0x0c7f },
/* Tengwar */	{ CHR('t','e','n','g'), 0x12000, 0x1207f },
/* Thaana */	{ CHR('t','h','a','a'), 0x0780, 0x07bf },
/* Thai */	{ CHR('t','h','a','i'), 0x0e00, 0x0e7f },
/* Tibetan */	{ CHR('t','i','b','t'), 0x0f00, 0x0fff },
/* Tifinagh */	{ CHR('t','f','n','g'), 0x2d30, 0x2d7f },
/* Ugaritic */	{ CHR('u','g','a','r'), 0x10380, 0x1039d },
/* Yi */	{ CHR('y','i',' ',' '), 0xa000, 0xa4c6 },
		{ 0 }
};

int ScriptIsRightToLeft(uint32 script) {
    if ( script==CHR('a','r','a','b') || script==CHR('h','e','b','r') ||
	    script==CHR('c','p','m','n') || script==CHR('k','h','a','r') ||
	    script==CHR('s','y','r','c') || script==CHR('t','h','a','a') ||
	    script==CHR('n','k','o',' '))
return( true );

return( false );
}

uint32 ScriptFromUnicode(int u,SplineFont *sf) {
    int s, k;

    if ( u!=-1 ) {
	for ( s=0; scripts[s][0]!=0; ++s ) {
	    for ( k=1; scripts[s][k+1]!=0; k += 2 )
		if ( u>=scripts[s][k] && u<=scripts[s][k+1] )
	    break;
	    if ( scripts[s][k+1]!=0 )
	break;
	}
	if ( scripts[s][0]!=0 ) {
	    extern int use_second_indic_scripts;
	    uint32 script = scripts[s][0];
	    if ( use_second_indic_scripts ) {
		/* MS has a parallel set of script tags for their new */
		/*  Indic font shaper */
		if ( script == CHR('b','e','n','g' )) script = CHR('b','n','g','2');
		else if ( script == CHR('d','e','v','a' )) script = CHR('d','e','v','2');
		else if ( script == CHR('g','u','j','r' )) script = CHR('g','j','r','2');
		else if ( script == CHR('g','u','r','u' )) script = CHR('g','u','r','2');
		else if ( script == CHR('k','n','d','a' )) script = CHR('k','n','d','2');
		else if ( script == CHR('m','l','y','m' )) script = CHR('m','l','y','2');
		else if ( script == CHR('o','r','y','a' )) script = CHR('o','r','y','2');
		else if ( script == CHR('t','a','m','l' )) script = CHR('t','m','l','2');
		else if ( script == CHR('t','e','l','u' )) script = CHR('t','e','l','2');
	    }
return( script );
	}
    } else if ( sf!=NULL ) {
	if ( sf->cidmaster!=NULL || sf->subfontcnt!=0 ) {
	    if ( sf->cidmaster!=NULL ) sf = sf->cidmaster;
	    if ( strmatch(sf->ordering,"Identity")==0 )
return( DEFAULT_SCRIPT );
	    else if ( strmatch(sf->ordering,"Korean")==0 )
return( CHR('h','a','n','g'));
	    else
return( CHR('h','a','n','i') );
	}
    }

return( DEFAULT_SCRIPT );
}

uint32 SCScriptFromUnicode(SplineChar *sc) {
    char *pt;
    PST *pst;
    SplineFont *sf;
    int i; unsigned uni;
    FeatureScriptLangList *features;

    if ( sc==NULL )
return( DEFAULT_SCRIPT );

    sf = sc->parent;
    if ( sc->unicodeenc!=-1 )
return( ScriptFromUnicode( sc->unicodeenc,sf ));

    pt = sc->name;
    if ( *pt ) for ( ++pt; *pt!='\0' && *pt!='_' && *pt!='.'; ++pt );
    if ( *pt!='\0' ) {
	char *str = copyn(sc->name,pt-sc->name);
	int uni = sf==NULL || sf->fv==NULL ? UniFromName(str,ui_none,&custom) :
			    UniFromName(str,sf->uni_interp,sf->fv->map->enc);
	free(str);
	if ( uni!=-1 )
return( ScriptFromUnicode( uni,sf ));
    }
    /* Adobe ligature uniXXXXXXXX */
    if ( strncmp(sc->name,"uni",3)==0 && sscanf(sc->name+3,"%4x", &uni)==1 )
return( ScriptFromUnicode( uni,sf ));

    if ( sf==NULL )
return( DEFAULT_SCRIPT );

    if ( sf->cidmaster ) sf=sf->cidmaster;
    else if ( sf->mm!=NULL ) sf=sf->mm->normal;
    for ( i=0; i<2; ++i ) {
	for ( pst=sc->possub; pst!=NULL; pst=pst->next ) {
	    if ( pst->type == pst_lcaret )
	continue;
	    for ( features = pst->subtable->lookup->features; features!=NULL; features=features->next ) {
		if ( features->scripts!=NULL )
return( features->scripts->script );
	    }
	}
    }
return( ScriptFromUnicode( sc->unicodeenc,sf ));
}

int SCRightToLeft(SplineChar *sc) {

    if ( sc->unicodeenc>=0x10800 && sc->unicodeenc<=0x10fff )
return( true );		/* Supplemental Multilingual Plane, RTL scripts */
    if ( sc->unicodeenc!=-1 && sc->unicodeenc<0x10000 )
return( isrighttoleft(sc->unicodeenc ));

return( ScriptIsRightToLeft(SCScriptFromUnicode(sc)));
}

static void GlyphMapFree(SplineChar ***map) {
    int i;

    if ( map==NULL )
return;
    for ( i=0; map[i]!=NULL; ++i )
	free(map[i]);
    free(map);
}

static SplineChar **FindSubs(SplineChar *sc,struct lookup_subtable *sub) {
    SplineChar *spc[30], **space = spc;
    int max = sizeof(spc)/sizeof(spc[0]);
    int cnt=0;
    char *pt, *start;
    SplineChar *subssc, **ret;
    PST *pst;

    for ( pst=sc->possub; pst!=NULL; pst=pst->next ) {
	if ( pst->subtable==sub ) {
	    pt = pst->u.subs.variant;
	    while ( 1 ) {
		while ( *pt==' ' ) ++pt;
		start = pt;
		pt = strchr(start,' ');
		if ( pt!=NULL )
		    *pt = '\0';
		subssc = SFGetChar(sc->parent,-1,start);
		if ( subssc!=NULL && subssc->ttf_glyph!=-1 ) {
		    if ( cnt>=max ) {
			if ( spc==space ) {
			    space = galloc((max+=30)*sizeof(SplineChar *));
			    memcpy(space,spc,(max-30)*sizeof(SplineChar *));
			} else
			    space = grealloc(space,(max+=30)*sizeof(SplineChar *));
		    }
		    space[cnt++] = subssc;
		}
		if ( pt==NULL )
	    break;
		*pt=' ';
	    }
	}
    }
    if ( cnt==0 )	/* Might happen if the substitution names weren't in the font */
return( NULL );
    ret = galloc((cnt+1)*sizeof(SplineChar *));
    memcpy(ret,space,cnt*sizeof(SplineChar *));
    ret[cnt] = NULL;
    if ( space!=spc )
	free(space);
return( ret );
}

static SplineChar ***generateMapList(SplineChar **glyphs, struct lookup_subtable *sub) {
    int cnt;
    SplineChar *sc;
    int i;
    SplineChar ***maps=NULL;

    for ( cnt=0; glyphs[cnt]!=NULL; ++cnt );
    maps = galloc((cnt+1)*sizeof(SplineChar **));
    for ( i=0; i<cnt; ++i ) {
	sc = glyphs[i];
	maps[i] = FindSubs(sc,sub);
    }
    maps[cnt] = NULL;
return( maps );
}

void AnchorClassDecompose(SplineFont *sf,AnchorClass *_ac, int classcnt, int *subcnts,
	SplineChar ***marks,SplineChar ***base,
	SplineChar ***lig,SplineChar ***mkmk,
	struct glyphinfo *gi) {
    /* Run through the font finding all characters with this anchor class */
    /*  (and the cnt-1 classes after it) */
    /*  and distributing in the four possible anchor types */
    int i,j,k,gid, gmax;
    struct sclist { int cnt; SplineChar **glyphs; } heads[at_max];
    AnchorPoint *test;
    AnchorClass *ac;

    memset(heads,0,sizeof(heads));
    memset(subcnts,0,classcnt*sizeof(int));
    memset(marks,0,classcnt*sizeof(SplineChar **));
    gmax = gi==NULL ? sf->glyphcnt : gi->gcnt;
    for ( j=0; j<2; ++j ) {
	for ( i=0; i<gmax; ++i ) if ( (gid = gi==NULL ? i : gi->bygid[i])!=-1 && sf->glyphs[gid]!=NULL ) {
	    for ( ac = _ac, k=0; k<classcnt; ac=ac->next ) if ( ac->matches ) {
		for ( test=sf->glyphs[gid]->anchor; test!=NULL ; test=test->next ) {
		    if ( test->anchor==ac ) {
			if ( test->type==at_mark ) {
			    if ( j )
				marks[k][subcnts[k]] = sf->glyphs[gid];
			    ++subcnts[k];
			    if ( ac->type!=act_mkmk )
		break;
			} else if ( test->type!=at_centry && test->type!=at_cexit ) {
			    if ( heads[test->type].glyphs!=NULL ) {
			    /* If we have multiple mark classes, we may use the same base glyph */
			    /* with more than one mark class. But it should only appear once in */
			    /* the output */
				if ( heads[test->type].cnt==0 ||
					 heads[test->type].glyphs[heads[test->type].cnt-1]!=sf->glyphs[gid] ) {
				    heads[test->type].glyphs[heads[test->type].cnt] = sf->glyphs[gid];
				    ++heads[test->type].cnt;
				}
			    } else
				++heads[test->type].cnt;
			    if ( ac->type!=act_mkmk )
		break;
			}
		    }
		}
		++k;
	    }
	}
	if ( j==1 )
    break;
	for ( i=0; i<4; ++i )
	    if ( heads[i].cnt!=0 ) {
		heads[i].glyphs = galloc((heads[i].cnt+1)*sizeof(SplineChar *));
		/* I used to set glyphs[cnt] to NULL here. But it turns out */
		/*  cnt may be an overestimate on the first pass. So we can */
		/*  only set it at the end of the second pass */
		heads[i].cnt = 0;
	    }
	for ( k=0; k<classcnt; ++k ) if ( subcnts[k]!=0 ) {
	    marks[k] = galloc((subcnts[k]+1)*sizeof(SplineChar *));
	    marks[k][subcnts[k]] = NULL;
	    subcnts[k] = 0;
	}
    }
    for ( i=0; i<4; ++i )
	if ( heads[i].glyphs!=NULL )
	    heads[i].glyphs[heads[i].cnt] = NULL;

    *base = heads[at_basechar].glyphs;
    *lig  = heads[at_baselig].glyphs;
    *mkmk = heads[at_basemark].glyphs;
}

SplineChar **EntryExitDecompose(SplineFont *sf,AnchorClass *ac,struct glyphinfo *gi) {
    /* Run through the font finding all characters with this anchor class */
    int i,j, cnt, gmax, gid;
    SplineChar **array;
    AnchorPoint *test;

    array=NULL;
    gmax = gi==NULL ? sf->glyphcnt : gi->gcnt;
    for ( j=0; j<2; ++j ) {
	cnt = 0;
	for ( i=0; i<gmax; ++i ) if ( (gid = gi==NULL ? i : gi->bygid[i])!=-1 && sf->glyphs[gid]!=NULL ) {
	    for ( test=sf->glyphs[gid]->anchor; test!=NULL && test->anchor!=ac; test=test->next );
	    if ( test!=NULL && (test->type==at_centry || test->type==at_cexit )) {
		if ( array!=NULL )
		    array[cnt] = sf->glyphs[gid];
		++cnt;
	    }
	}
	if ( cnt==0 )
return( NULL );
	if ( j==1 )
    break;
	array = galloc((cnt+1)*sizeof(SplineChar *));
	array[cnt] = NULL;
    }
return( array );
}

static void AnchorGuessContext(SplineFont *sf,struct alltabs *at) {
    int i;
    int maxbase=0, maxmark=0, basec, markc;
    AnchorPoint *ap;
    int hascursive = 0;

    /* the order in which we examine the glyphs does not matter here, so */
    /*  we needn't add the complexity running though in gid order */
    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i] ) {
	basec = markc = 0;
	for ( ap = sf->glyphs[i]->anchor; ap!=NULL; ap=ap->next )
	    if ( ap->type==at_basemark )
		++markc;
	    else if ( ap->type==at_basechar || ap->type==at_baselig )
		++basec;
	    else if ( ap->type==at_centry )
		hascursive = true;
	if ( basec>maxbase ) maxbase = basec;
	if ( markc>maxmark ) maxmark = markc;
    }

    if ( maxbase*(maxmark+1)>at->os2.maxContext )
	at->os2.maxContext = maxbase*(maxmark+1);
    if ( hascursive && at->os2.maxContext<2 )
	at->os2.maxContext=2;
}

static void dumpcoveragetable(FILE *gpos,SplineChar **glyphs) {
    int i, last = -2, range_cnt=0, start;
    /* the glyph list should already be sorted */
    /* figure out whether it is better (smaller) to use an array of glyph ids */
    /*  or a set of glyph id ranges */

    for ( i=0; glyphs[i]!=NULL; ++i ) {
	if ( glyphs[i]->ttf_glyph<=last )
	    IError("Glyphs must be ordered when creating coverage table");
	if ( glyphs[i]->ttf_glyph!=last+1 )
	    ++range_cnt;
	last = glyphs[i]->ttf_glyph;
    }
    /* I think Windows will only accept format 2 coverage tables? */
    if ( !(coverageformatsallowed&2) || ((coverageformatsallowed&1) && i<=3*range_cnt )) {
	/* We use less space with a list of glyphs than with a set of ranges */
	putshort(gpos,1);		/* Coverage format=1 => glyph list */
	putshort(gpos,i);		/* count of glyphs */
	for ( i=0; glyphs[i]!=NULL; ++i )
	    putshort(gpos,glyphs[i]->ttf_glyph);	/* array of glyph IDs */
    } else {
	putshort(gpos,2);		/* Coverage format=2 => range list */
	putshort(gpos,range_cnt);	/* count of ranges */
	last = -2; start = -2;		/* start is a index in our glyph array, last is ttf_glyph */
	for ( i=0; glyphs[i]!=NULL; ++i ) {
	    if ( glyphs[i]->ttf_glyph!=last+1 ) {
		if ( last!=-2 ) {
		    putshort(gpos,glyphs[start]->ttf_glyph);	/* start glyph ID */
		    putshort(gpos,last);			/* end glyph ID */
		    putshort(gpos,start);			/* coverage index of start glyph */
		}
		start = i;
	    }
	    last = glyphs[i]->ttf_glyph;
	}
	if ( last!=-2 ) {
	    putshort(gpos,glyphs[start]->ttf_glyph);	/* start glyph ID */
	    putshort(gpos,last);			/* end glyph ID */
	    putshort(gpos,start);			/* coverage index of start glyph */
	}
    }
}

static int sc_ttf_order( const void *_sc1, const void *_sc2) {
    const SplineChar *sc1 = *(const SplineChar **) _sc1, *sc2 = *(const SplineChar **) _sc2;
return( sc1->ttf_glyph - sc2->ttf_glyph );
}

static SplineChar **SFOrderedGlyphsWithPSTinSubtable(SplineFont *sf,struct lookup_subtable *sub) {
    SplineChar **glyphs = SFGlyphsWithPSTinSubtable(sf,sub);
    int cnt, i, k;
    if ( glyphs==NULL )
return( NULL );
    for ( cnt=0; glyphs[cnt]!=NULL; ++cnt);
    qsort(glyphs,cnt,sizeof(SplineChar *),sc_ttf_order);
    if ( glyphs[0]->ttf_glyph==-1 ) {
	/* Not sure if this can happen, but it's easy to fix */
	for ( k=0; k<cnt && glyphs[k]->ttf_glyph==-1; ++k);
	for ( i=0; i<=cnt-k; ++i )
	    glyphs[i] = glyphs[i+k];
    }
return( glyphs );
}

SplineChar **SFGlyphsFromNames(SplineFont *sf,char *names) {
    int cnt, ch;
    char *pt, *end;
    SplineChar *sc, **glyphs;

    cnt = 0;
    for ( pt = names; *pt; pt = end+1 ) {
	++cnt;
	end = strchr(pt,' ');
	if ( end==NULL )
    break;
    }

    glyphs = galloc((cnt+1)*sizeof(SplineChar *));
    cnt = 0;
    for ( pt = names; *pt; pt = end+1 ) {
	end = strchr(pt,' ');
	if ( end==NULL )
	    end = pt+strlen(pt);
	ch = *end;
	*end = '\0';
	sc = SFGetChar(sf,-1,pt);
	if ( sc!=NULL && sc->ttf_glyph!=-1 )
	    glyphs[cnt++] = sc;
	*end = ch;
	if ( ch=='\0' )
    break;
    }
    glyphs[cnt] = NULL;
return( glyphs );
}

static SplineChar **OrderedGlyphsFromNames(SplineFont *sf,char *names) {
    SplineChar **glyphs = SFGlyphsFromNames(sf,names);
    int i,j;

    if ( glyphs==NULL || glyphs[0]==NULL )
return( glyphs );

    for ( i=0; glyphs[i+1]!=NULL; ++i ) for ( j=i+1; glyphs[j]!=NULL; ++j ) {
	if ( glyphs[i]->ttf_glyph > glyphs[j]->ttf_glyph ) {
	    SplineChar *sc = glyphs[i];
	    glyphs[i] = glyphs[j];
	    glyphs[j] = sc;
	}
    }
    if ( glyphs[0]!=NULL ) {		/* Glyphs should not appear twice in the name list, just just in case they do... */
	for ( i=0; glyphs[i+1]!=NULL; ++i ) {
	    if ( glyphs[i]==glyphs[i+1] ) {
		for ( j=i+1; glyphs[j]!=NULL; ++j )
		    glyphs[j] = glyphs[j+1];
	    }
	}
    }
return( glyphs );
}

static void gposvrmaskeddump(FILE *gpos,int vf1,int mask,int offset) {
    if ( vf1&1 ) putshort(gpos,mask&1 ? offset : 0 );
    if ( vf1&2 ) putshort(gpos,mask&2 ? offset : 0 );
    if ( vf1&4 ) putshort(gpos,mask&4 ? offset : 0 );
    if ( vf1&8 ) putshort(gpos,mask&8 ? offset : 0 );
}

#ifdef FONTFORGE_CONFIG_DEVICETABLES
static int devtaboffsetsize(DeviceTable *dt) {
    int type = 1, i;

    for ( i=dt->last_pixel_size-dt->first_pixel_size; i>=0; --i ) {
	if ( dt->corrections[i]>=8 || dt->corrections[i]<-8 )
return( 3 );
	else if ( dt->corrections[i]>=2 || dt->corrections[i]<-2 )
	    type = 2;
    }
return( type );
}

static void dumpgposdevicetable(FILE *gpos,DeviceTable *dt) {
    int type;
    int i,cnt,b;

    if ( dt==NULL || dt->corrections==NULL )
return;
    type = devtaboffsetsize(dt);
    putshort(gpos,dt->first_pixel_size);
    putshort(gpos,dt->last_pixel_size );
    putshort(gpos,type);
    cnt = dt->last_pixel_size - dt->first_pixel_size + 1;
    if ( type==3 ) {
	for ( i=0; i<cnt; ++i )
	    putc(dt->corrections[i],gpos);
	if ( cnt&1 )
	    putc(0,gpos);
    } else if ( type==2 ) {
	for ( i=0; i<cnt; i+=4 ) {
	    int val = 0;
	    for ( b=0; b<4 && i+b<cnt; ++b )
		val |= (dt->corrections[i+b]&0x000f)<<(12-b*4);
	    putshort(gpos,val);
	}
    } else {
	for ( i=0; i<cnt; i+=8 ) {
	    int val = 0;
	    for ( b=0; b<8 && i+b<cnt; ++b )
		val |= (dt->corrections[i+b]&0x0003)<<(14-b*2);
	    putshort(gpos,val);
	}
    }
}

static int DevTabLen(DeviceTable *dt) {
    int type;
    int cnt;

    if ( dt==NULL || dt->corrections==NULL )
return( 0 );
    cnt = dt->last_pixel_size - dt->first_pixel_size + 1;
    type = devtaboffsetsize(dt);
    if ( type==3 )
	cnt = (cnt+1)/2;
    else if ( type==2 )
	cnt = (cnt+3)/4;
    else
	cnt = (cnt+7)/8;
    cnt += 3;		/* first, last, type */
return( sizeof(uint16)*cnt );
}

static int ValDevTabLen(ValDevTab *vdt) {

    if ( vdt==NULL )
return( 0 );

return( DevTabLen(&vdt->xadjust) + DevTabLen(&vdt->yadjust) +
	DevTabLen(&vdt->xadv) + DevTabLen(&vdt->yadv) );
}

static int gposdumpvaldevtab(FILE *gpos,ValDevTab *vdt,int bits,int next_dev_tab ) {

    if ( bits&0x10 ) {
	if ( vdt==NULL || vdt->xadjust.corrections==NULL )
	    putshort(gpos,0);
	else {
	    putshort(gpos,next_dev_tab);
	    next_dev_tab += DevTabLen(&vdt->xadjust);
	}
    }
    if ( bits&0x20 ) {
	if ( vdt==NULL || vdt->yadjust.corrections==NULL )
	    putshort(gpos,0);
	else {
	    putshort(gpos,next_dev_tab);
	    next_dev_tab += DevTabLen(&vdt->yadjust);
	}
    }
    if ( bits&0x40 ) {
	if ( vdt==NULL || vdt->xadv.corrections==NULL )
	    putshort(gpos,0);
	else {
	    putshort(gpos,next_dev_tab);
	    next_dev_tab += DevTabLen(&vdt->xadv);
	}
    }
    if ( bits&0x80 ) {
	if ( vdt==NULL || vdt->yadv.corrections==NULL )
	    putshort(gpos,0);
	else {
	    putshort(gpos,next_dev_tab);
	    next_dev_tab += DevTabLen(&vdt->yadv);
	}
    }
return( next_dev_tab );
}

static int gposmaskeddumpdevtab(FILE *gpos,DeviceTable *dt,int bits,int mask,
	int next_dev_tab ) {

    if ( bits&0x10 ) {
	if ( !(mask&0x10) || dt==NULL )
	    putshort(gpos,0);
	else {
	    putshort(gpos,next_dev_tab);
	    next_dev_tab += DevTabLen(dt);
	}
    }
    if ( bits&0x20 ) {
	if ( !(mask&0x20) || dt==NULL )
	    putshort(gpos,0);
	else {
	    putshort(gpos,next_dev_tab);
	    next_dev_tab += DevTabLen(dt);
	}
    }
    if ( bits&0x40 ) {
	if ( !(mask&0x40) || dt==NULL )
	    putshort(gpos,0);
	else {
	    putshort(gpos,next_dev_tab);
	    next_dev_tab += DevTabLen(dt);
	}
    }
    if ( bits&0x80 ) {
	if ( !(mask&0x80) || dt==NULL )
	    putshort(gpos,0);
	else {
	    putshort(gpos,next_dev_tab);
	    next_dev_tab += DevTabLen(dt);
	}
    }
return( next_dev_tab );
}

static int DevTabsSame(DeviceTable *dt1, DeviceTable *dt2) {
    DeviceTable _dt;
    int i;

    if ( dt1==NULL && dt2==NULL )
return( true );
    if ( dt1==NULL ) {
	memset(&_dt,0,sizeof(_dt));
	dt1 = &_dt;
    }
    if ( dt2==NULL ) {
	memset(&_dt,0,sizeof(_dt));
	dt2 = &_dt;
    }
    if ( dt1->corrections==NULL && dt2->corrections==NULL )
return( true );
    if ( dt1->corrections==NULL || dt2->corrections==NULL )
return( false );
    if ( dt1->first_pixel_size!=dt2->first_pixel_size ||
	    dt1->last_pixel_size!=dt2->last_pixel_size )
return( false );
    for ( i=dt2->last_pixel_size-dt1->first_pixel_size; i>=0; --i )
	if ( dt1->corrections[i]!=dt2->corrections[i] )
return( false );

return( true );
}

static int ValDevTabsSame(ValDevTab *vdt1, ValDevTab *vdt2) {
    ValDevTab _vdt;

    if ( vdt1==NULL && vdt2==NULL )
return( true );
    if ( vdt1==NULL ) {
	memset(&_vdt,0,sizeof(_vdt));
	vdt1 = &_vdt;
    }
    if ( vdt2==NULL ) {
	memset(&_vdt,0,sizeof(_vdt));
	vdt2 = &_vdt;
    }
return( DevTabsSame(&vdt1->xadjust,&vdt2->xadjust) &&
	DevTabsSame(&vdt1->yadjust,&vdt2->yadjust) &&
	DevTabsSame(&vdt1->xadv,&vdt2->xadv) &&
	DevTabsSame(&vdt1->yadv,&vdt2->yadv) );
}
#endif

static void dumpGPOSsimplepos(FILE *gpos,SplineFont *sf,struct lookup_subtable *sub ) {
    int cnt, cnt2;
    int32 coverage_pos, end;
    PST *pst, *first=NULL;
    int bits = 0, same=true;
    SplineChar **glyphs;

    glyphs = SFOrderedGlyphsWithPSTinSubtable(sf,sub);
    for ( cnt=cnt2=0; glyphs[cnt]!=NULL; ++cnt) {
	for ( pst=glyphs[cnt]->possub; pst!=NULL; pst=pst->next ) {
	    if ( pst->subtable==sub && pst->type==pst_position ) {
		if ( first==NULL ) first = pst;
		else if ( same ) {
		    if ( first->u.pos.xoff!=pst->u.pos.xoff ||
			    first->u.pos.yoff!=pst->u.pos.yoff ||
			    first->u.pos.h_adv_off!=pst->u.pos.h_adv_off ||
			    first->u.pos.v_adv_off!=pst->u.pos.v_adv_off )
			same = false;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
		    if ( !ValDevTabsSame(pst->u.pos.adjust,first->u.pos.adjust))
			same = false;
#endif
		}
		if ( pst->u.pos.xoff!=0 ) bits |= 1;
		if ( pst->u.pos.yoff!=0 ) bits |= 2;
		if ( pst->u.pos.h_adv_off!=0 ) bits |= 4;
		if ( pst->u.pos.v_adv_off!=0 ) bits |= 8;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
		if ( pst->u.pos.adjust!=NULL ) {
		    if ( pst->u.pos.adjust->xadjust.corrections!=NULL ) bits |= 0x10;
		    if ( pst->u.pos.adjust->yadjust.corrections!=NULL ) bits |= 0x20;
		    if ( pst->u.pos.adjust->xadv.corrections!=NULL ) bits |= 0x40;
		    if ( pst->u.pos.adjust->yadv.corrections!=NULL ) bits |= 0x80;
		}
#endif
		++cnt2;
	break;
	    }
	}
    }
    if ( bits==0 ) bits=1;
    if ( cnt!=cnt2 )
	IError( "Count mismatch in dumpGPOSsimplepos#1 %d vs %d\n", cnt, cnt2 );

    putshort(gpos,same?1:2);	/* 1 means all value records same */
    coverage_pos = ftell(gpos);
    putshort(gpos,0);		/* offset to coverage table */
    putshort(gpos,bits);
    if ( same ) {
	if ( bits&1 ) putshort(gpos,first->u.pos.xoff);
	if ( bits&2 ) putshort(gpos,first->u.pos.yoff);
	if ( bits&4 ) putshort(gpos,first->u.pos.h_adv_off);
	if ( bits&8 ) putshort(gpos,first->u.pos.v_adv_off);
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	if ( bits&0xf0 ) {
	    int next_dev_tab = ftell(gpos)-coverage_pos+2+
		sizeof(int16)*((bits&0x10?1:0) + (bits&0x20?1:0) + (bits&0x40?1:0) + (bits&0x80?1:0));
	    if ( bits&0x10 ) { putshort(gpos,next_dev_tab); next_dev_tab += DevTabLen(&first->u.pos.adjust->xadjust); }
	    if ( bits&0x20 ) { putshort(gpos,next_dev_tab); next_dev_tab += DevTabLen(&first->u.pos.adjust->yadjust); }
	    if ( bits&0x40 ) { putshort(gpos,next_dev_tab); next_dev_tab += DevTabLen(&first->u.pos.adjust->xadv); }
	    if ( bits&0x80 ) { putshort(gpos,next_dev_tab); next_dev_tab += DevTabLen(&first->u.pos.adjust->yadv); }
	    if ( bits&0x10 ) dumpgposdevicetable(gpos,&first->u.pos.adjust->xadjust);
	    if ( bits&0x20 ) dumpgposdevicetable(gpos,&first->u.pos.adjust->yadjust);
	    if ( bits&0x40 ) dumpgposdevicetable(gpos,&first->u.pos.adjust->xadv);
	    if ( bits&0x80 ) dumpgposdevicetable(gpos,&first->u.pos.adjust->yadv);
	    if ( next_dev_tab!=ftell(gpos)-coverage_pos+2 )
		IError( "Device Table offsets wrong in simple positioning 2");
	}
#endif
    } else {
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	int vr_size = 
		sizeof(int16)*((bits&0x1?1:0) + (bits&0x2?1:0) + (bits&0x4?1:0) + (bits&0x8?1:0) +
		    (bits&0x10?1:0) + (bits&0x20?1:0) + (bits&0x40?1:0) + (bits&0x80?1:0));
	int next_dev_tab = ftell(gpos)-coverage_pos+2+2+vr_size*cnt;
#endif
	putshort(gpos,cnt);
	for ( cnt2 = 0; glyphs[cnt2]!=NULL; ++cnt2 ) {
	    for ( pst=glyphs[cnt2]->possub; pst!=NULL; pst=pst->next ) {
		if ( pst->subtable==sub && pst->type==pst_position ) {
		    if ( bits&1 ) putshort(gpos,pst->u.pos.xoff);
		    if ( bits&2 ) putshort(gpos,pst->u.pos.yoff);
		    if ( bits&4 ) putshort(gpos,pst->u.pos.h_adv_off);
		    if ( bits&8 ) putshort(gpos,pst->u.pos.v_adv_off);
#ifdef FONTFORGE_CONFIG_DEVICETABLES
		    next_dev_tab = gposdumpvaldevtab(gpos,pst->u.pos.adjust,bits,
			    next_dev_tab);
#endif
	    break;
		}
	    }
	}
	if ( cnt!=cnt2 )
	    IError( "Count mismatch in dumpGPOSsimplepos#3 %d vs %d\n", cnt, cnt2 );
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	if ( bits&0xf0 ) {
	    for ( cnt2 = 0; glyphs[cnt2]!=NULL; ++cnt2 ) {
		for ( pst=glyphs[cnt2]->possub; pst!=NULL; pst=pst->next ) {
		    if ( pst->subtable==sub && pst->type==pst_position ) {
			if ( bits&0x10 ) dumpgposdevicetable(gpos,&first->u.pos.adjust->xadjust);
			if ( bits&0x20 ) dumpgposdevicetable(gpos,&first->u.pos.adjust->yadjust);
			if ( bits&0x40 ) dumpgposdevicetable(gpos,&first->u.pos.adjust->xadv);
			if ( bits&0x80 ) dumpgposdevicetable(gpos,&first->u.pos.adjust->yadv);
		    }
		}
	    }
	}
	if ( next_dev_tab!=ftell(gpos)-coverage_pos+2 )
	    IError( "Device Table offsets wrong in simple positioning 2");
#endif
    }
    end = ftell(gpos);
    fseek(gpos,coverage_pos,SEEK_SET);
    putshort(gpos,end-coverage_pos+2);
    fseek(gpos,end,SEEK_SET);
    dumpcoveragetable(gpos,glyphs);
    fseek(gpos,0,SEEK_END);
    free(glyphs);
}

struct sckppst {
    uint16 samewas;
    uint16 devtablen;
    uint16 tot;
    uint8 isv;
    uint8 subtable_too_big;
/* The first few fields are only meaningful in the first structure in the array*/
/*   and provide information about the entire rest of the array */
    uint16 other_gid;
    SplineChar *sc;
    KernPair *kp;
    PST *pst;
};

static int cmp_gid( const void *_s1, const void *_s2 ) {
    const struct sckppst *s1 = _s1, *s2 = _s2;
return( ((int) s1->other_gid) - ((int) s2->other_gid) );
}

static void dumpGPOSpairpos(FILE *gpos,SplineFont *sf,struct lookup_subtable *sub) {
    int cnt;
    int32 coverage_pos, offset_pos, end, start, pos;
    PST *pst;
    KernPair *kp;
    int vf1 = 0, vf2=0, i, j, k, tot, bit_cnt, v;
    int start_cnt, end_cnt;
    int chunk_cnt, chunk_max;
    SplineChar *sc, **glyphs, *gtemp;
    int isr2l = -1;
    struct sckppst **seconds;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
    int devtablen;
#endif

    /* Figure out all the data we need. First the glyphs with kerning info */
    /*  then the glyphs to which they kern, and by how much */
    glyphs = SFOrderedGlyphsWithPSTinSubtable(sf,sub);
    for ( cnt=0; glyphs[cnt]!=NULL; ++cnt);
    seconds = galloc(cnt*sizeof(struct sckppst));
    for ( cnt=0; glyphs[cnt]!=NULL; ++cnt) {
	for ( k=0; k<2; ++k ) {
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	    devtablen = 0;
#endif
	    tot = 0;
	    for ( pst=glyphs[cnt]->possub; pst!=NULL; pst=pst->next ) {
		if ( pst->subtable==sub && pst->type==pst_pair &&
			(sc = SFGetChar(sf,-1,pst->u.pair.paired))!=NULL &&
			sc->ttf_glyph!=-1 ) {
		    if ( k ) {
			seconds[cnt][tot].sc = sc;
			seconds[cnt][tot].other_gid = sc->ttf_glyph;
			seconds[cnt][tot].pst = pst;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
			devtablen += ValDevTabLen(pst->u.pair.vr[0].adjust) +
				 ValDevTabLen(pst->u.pair.vr[1].adjust);
#endif
		    }
		    ++tot;
		}
	    }
	    for ( v=0; v<2; ++v ) {
		for ( kp = v ? glyphs[cnt]->vkerns : glyphs[cnt]->kerns; kp!=NULL; kp=kp->next ) {
		    if ( kp->sc->ttf_glyph!=-1 ) {
			if ( k ) {
			    seconds[cnt][tot].other_gid = kp->sc->ttf_glyph;
			    seconds[cnt][tot].sc = kp->sc;
			    seconds[cnt][tot].kp = kp;
			    seconds[cnt][tot].isv = v;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
			    devtablen += DevTabLen(kp->adjust);
#endif
			}
			++tot;
		    }
		}
	    }
	    if ( k==0 ) {
		seconds[cnt] = gcalloc(tot+1,sizeof(struct sckppst));
	    } else {
		qsort(seconds[cnt],tot,sizeof(struct sckppst),cmp_gid);
		seconds[cnt][0].tot = tot;
		/* Devtablen is 0 unless we are configured for device tables */
#ifdef FONTFORGE_CONFIG_DEVICETABLES
		seconds[cnt][0].devtablen = devtablen;
#endif
		seconds[cnt][0].samewas = 0xffff;
	    }
	}
    }

    /* Some fonts do a primitive form of class based kerning, several glyphs */
    /*  can share the same list of second glyphs & offsets */
    for ( cnt=0; glyphs[cnt]!=NULL; ++cnt) {
	struct sckppst *test = seconds[cnt], *test2;
	for ( i=cnt-1; i>=0; --i ) {
	    test2 = seconds[i];
	    if ( test[0].tot != test2[0].tot )
	continue;
	    for ( j=test[0].tot-1; j>=0; --j ) {
		if ( test[j].other_gid != test2[j].other_gid )
	    break;
		if ( test[j].kp!=NULL && test2[j].kp!=NULL &&
			test[j].kp->off == test2[j].kp->off 
#ifdef FONTFORGE_CONFIG_DEVICETABLES
			&& DevTabsSame(test[j].kp->adjust,test2[j].kp->adjust)
#endif
			)
		    /* So far, so good */;
		else if ( test[j].pst!=NULL && test2[j].pst!=NULL &&
			test[j].pst->u.pair.vr[0].xoff == test2[j].pst->u.pair.vr[0].xoff &&
			test[j].pst->u.pair.vr[0].yoff == test2[j].pst->u.pair.vr[0].yoff &&
			test[j].pst->u.pair.vr[0].h_adv_off == test2[j].pst->u.pair.vr[0].h_adv_off &&
			test[j].pst->u.pair.vr[0].v_adv_off == test2[j].pst->u.pair.vr[0].v_adv_off &&
			test[j].pst->u.pair.vr[1].xoff == test2[j].pst->u.pair.vr[1].xoff &&
			test[j].pst->u.pair.vr[1].yoff == test2[j].pst->u.pair.vr[1].yoff &&
			test[j].pst->u.pair.vr[1].h_adv_off == test2[j].pst->u.pair.vr[1].h_adv_off &&
			test[j].pst->u.pair.vr[1].v_adv_off == test2[j].pst->u.pair.vr[1].v_adv_off
#ifdef FONTFORGE_CONFIG_DEVICETABLES
			&& ValDevTabsSame(test[j].pst->u.pair.vr[0].adjust,test2[j].pst->u.pair.vr[0].adjust)
			&& ValDevTabsSame(test[j].pst->u.pair.vr[1].adjust,test2[j].pst->u.pair.vr[1].adjust)
#endif
			)
		    /* That's ok too. */;
		else
	    break;
	    }
	    if ( j>=0 )
	continue;
	    test2[0].samewas = j;
	break;
	}
    }

    /* Ok, how many offsets must we output? Normall kerning will just use */
    /*  one offset (with perhaps a device table), but the standard allows */
    /*  us to adjust 8 different values (with 8 different device tables) */
    /* Find out which we need */
    for ( cnt=0; glyphs[cnt]!=NULL; ++cnt) {
	for ( tot=0 ; tot<seconds[cnt][0].tot; ++tot ) {
	    if ( (pst=seconds[cnt][tot].pst)!=NULL ) {
		if ( pst->subtable==sub && pst->type==pst_pair ) {
		    if ( pst->u.pair.vr[0].xoff!=0 ) vf1 |= 1;
		    if ( pst->u.pair.vr[0].yoff!=0 ) vf1 |= 2;
		    if ( pst->u.pair.vr[0].h_adv_off!=0 ) vf1 |= 4;
		    if ( pst->u.pair.vr[0].v_adv_off!=0 ) vf1 |= 8;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
		    if ( pst->u.pair.vr[0].adjust!=NULL ) {
			if ( pst->u.pair.vr[0].adjust->xadjust.corrections!=NULL ) vf1 |= 0x10;
			if ( pst->u.pair.vr[0].adjust->yadjust.corrections!=NULL ) vf1 |= 0x20;
			if ( pst->u.pair.vr[0].adjust->xadv.corrections!=NULL ) vf1 |= 0x40;
			if ( pst->u.pair.vr[0].adjust->yadv.corrections!=NULL ) vf1 |= 0x80;
		    }
#endif
		    if ( pst->u.pair.vr[1].xoff!=0 ) vf2 |= 1;
		    if ( pst->u.pair.vr[1].yoff!=0 ) vf2 |= 2;
		    if ( pst->u.pair.vr[1].h_adv_off!=0 ) vf2 |= 4;
		    if ( pst->u.pair.vr[1].v_adv_off!=0 ) vf2 |= 8;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
		    if ( pst->u.pair.vr[1].adjust!=NULL ) {
			if ( pst->u.pair.vr[1].adjust->xadjust.corrections!=NULL ) vf2 |= 0x10;
			if ( pst->u.pair.vr[1].adjust->yadjust.corrections!=NULL ) vf2 |= 0x20;
			if ( pst->u.pair.vr[1].adjust->xadv.corrections!=NULL ) vf2 |= 0x40;
			if ( pst->u.pair.vr[1].adjust->yadv.corrections!=NULL ) vf2 |= 0x80;
		    }
#endif
		}
	    }
	    if ( (kp = seconds[cnt][tot].kp)!=NULL ) {
		int mask = 0, mask2=0;
		if ( !seconds[cnt][tot].isv && isr2l==-1 ) {
		    SplineChar *sc = seconds[cnt][tot].sc;
		    int known_r2l = SCRightToLeft(sc);
		    if ( known_r2l )
			isr2l = true;
		    else if ( SCScriptFromUnicode(sc)== DEFAULT_SCRIPT )
	continue;
		    else
			isr2l = false;
		}
		if ( seconds[cnt][tot].isv )
		    mask = 0x0008;
		else if ( isr2l )
		    mask2 = 0x0004;
		else
		    mask = 0x0004;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
		if ( kp->adjust!=NULL ) {
		    mask  |= mask<<4;
		    mask2 |= mask2<<4;
		}
#endif
		vf1 |= mask;
		vf2 |= mask2;
	    }
	}
    }
    if ( isr2l==-1 ) isr2l = 0;
    if ( vf1==0 && vf2==0 ) vf1=1;
    bit_cnt = 0;
    for ( i=0; i<8; ++i ) {
	if ( vf1&(1<<i) ) ++bit_cnt;
	if ( vf2&(1<<i) ) ++bit_cnt;
    }

    chunk_max = chunk_cnt = 0;
    for ( start_cnt=0; start_cnt<cnt; start_cnt=end_cnt ) {
	int len = 5*2;
	for ( end_cnt=start_cnt; end_cnt<cnt; ++end_cnt ) {
	    int glyph_len = 2;		/* For the glyph's offset */
	    if ( seconds[end_cnt][0].samewas==0xffff || seconds[end_cnt][0].samewas<start_cnt )
		glyph_len += (bit_cnt*2+2)*seconds[end_cnt][0].tot +
				seconds[end_cnt][0].devtablen;
	    if ( glyph_len>65535 && end_cnt==start_cnt ) {
		LogError(_("Lookup subtable %s contains a glyph %s whose kerning information takes up more than 64k bytes\n"),
			sub->subtable_name, glyphs[start_cnt]->name );
		len += glyph_len;
	    } else if ( len+glyph_len>65535 ) {
		if( start_cnt==0 )
		    LogError(_("Lookup subtable %s had to be split into several subtables\nbecause it was too big.\n"),
			sub->subtable_name );
	break;
	    } else
		len += glyph_len;
	}
	if ( start_cnt!=0 || end_cnt!=cnt ) {
	    if ( chunk_cnt>=chunk_max )
		sub->extra_subtables = grealloc(sub->extra_subtables,((chunk_max+=10)+1)*sizeof(uint32));
	    sub->extra_subtables[chunk_cnt++] = ftell(gpos);
	    sub->extra_subtables[chunk_cnt] = -1;
	}

	start = ftell(gpos);
	putshort(gpos,1);		/* 1 means char pairs (ie. not classes) */
	coverage_pos = ftell(gpos);
	putshort(gpos,0);		/* offset to coverage table */
	putshort(gpos,vf1);
	putshort(gpos,vf2);
	putshort(gpos,end_cnt-start_cnt);
	offset_pos = ftell(gpos);
	for ( i=start_cnt; i<end_cnt; ++i )
	    putshort(gpos,0);	/* Fill in later */
	for ( i=start_cnt; i<end_cnt; ++i ) {
	    if ( seconds[i][0].samewas>= start_cnt && seconds[i][0].samewas!=0xffff ) {
		/* It's the same as the glyph at samewas, so just copy the */
		/*  offset from there. We don't need to do anything else */
		int offset;
		fseek(gpos,offset_pos+(seconds[i][0].samewas-start_cnt)*sizeof(uint16),SEEK_SET);
		offset = getushort(gpos);
		fseek(gpos,offset_pos+(i-start_cnt)*sizeof(uint16),SEEK_SET);
		putshort(gpos,offset);
		fseek(gpos,0,SEEK_END);
	continue;
	    }
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	    int next_dev_tab = ftell(gpos)-start;
	    if ( (vf1&0xf0) || (vf2&0xf0) ) {
		for ( tot=0 ; tot<seconds[i][0].tot; ++tot ) {
		    if ( (pst=seconds[i][tot].pst)!=NULL ) {
			if ( pst->u.pair.vr[0].adjust!=NULL ) {
			    dumpgposdevicetable(gpos,&pst->u.pair.vr[0].adjust->xadjust);
			    dumpgposdevicetable(gpos,&pst->u.pair.vr[0].adjust->yadjust);
			    dumpgposdevicetable(gpos,&pst->u.pair.vr[0].adjust->xadv);
			    dumpgposdevicetable(gpos,&pst->u.pair.vr[0].adjust->yadv);
			}
			if ( pst->u.pair.vr[1].adjust!=NULL ) {
			    dumpgposdevicetable(gpos,&pst->u.pair.vr[1].adjust->xadjust);
			    dumpgposdevicetable(gpos,&pst->u.pair.vr[1].adjust->yadjust);
			    dumpgposdevicetable(gpos,&pst->u.pair.vr[1].adjust->xadv);
			    dumpgposdevicetable(gpos,&pst->u.pair.vr[1].adjust->yadv);
			}
		    }
		    if ( (kp=seconds[i][tot].kp)!=NULL && kp->adjust!=NULL )
			dumpgposdevicetable(gpos,kp->adjust);
		}
	    }
#endif
	    pos = ftell(gpos);
	    fseek(gpos,offset_pos+(i-start_cnt)*sizeof(uint16),SEEK_SET);
	    putshort(gpos,pos-start);
	    fseek(gpos,pos,SEEK_SET);

	    putshort(gpos,seconds[i][0].tot);
	    for ( tot=0 ; tot<seconds[i][0].tot; ++tot ) {
		putshort(gpos,seconds[i][tot].other_gid);
		if ( (pst=seconds[i][tot].pst)!=NULL ) {
		    if ( vf1&1 ) putshort(gpos,pst->u.pair.vr[0].xoff);
		    if ( vf1&2 ) putshort(gpos,pst->u.pair.vr[0].yoff);
		    if ( vf1&4 ) putshort(gpos,pst->u.pair.vr[0].h_adv_off);
		    if ( vf1&8 ) putshort(gpos,pst->u.pair.vr[0].v_adv_off);
#ifdef FONTFORGE_CONFIG_DEVICETABLES
		    next_dev_tab = gposdumpvaldevtab(gpos,pst->u.pair.vr[0].adjust,vf1,
			    next_dev_tab);
#endif
		    if ( vf2&1 ) putshort(gpos,pst->u.pair.vr[1].xoff);
		    if ( vf2&2 ) putshort(gpos,pst->u.pair.vr[1].yoff);
		    if ( vf2&4 ) putshort(gpos,pst->u.pair.vr[1].h_adv_off);
		    if ( vf2&8 ) putshort(gpos,pst->u.pair.vr[1].v_adv_off);
#ifdef FONTFORGE_CONFIG_DEVICETABLES
		    next_dev_tab = gposdumpvaldevtab(gpos,pst->u.pair.vr[1].adjust,vf2,
			    next_dev_tab);
#endif
		} else if ( (kp=seconds[i][tot].kp)!=NULL ) {
		    int mask=0, mask2=0;
		    if ( seconds[i][tot].isv )
			mask = 0x8;
		    else if ( isr2l )
			mask2 = 0x4;
		    else
			mask = 0x4;
		    gposvrmaskeddump(gpos,vf1,mask,kp->off);
#ifdef FONTFORGE_CONFIG_DEVICETABLES
		    next_dev_tab = gposmaskeddumpdevtab(gpos,kp->adjust,vf1,mask<<4,
			    next_dev_tab);
#endif
		    gposvrmaskeddump(gpos,vf2,mask2,kp->off);
#ifdef FONTFORGE_CONFIG_DEVICETABLES
		    next_dev_tab = gposmaskeddumpdevtab(gpos,kp->adjust,vf2,mask2<<4,
			    next_dev_tab);
#endif
		}
	    }
	}
	end = ftell(gpos);
	fseek(gpos,coverage_pos,SEEK_SET);
	putshort(gpos,end-start);
	fseek(gpos,end,SEEK_SET);
	gtemp = glyphs[end_cnt]; glyphs[end_cnt] = NULL;
	dumpcoveragetable(gpos,glyphs+start_cnt);
	glyphs[end_cnt] = gtemp;
    }
    for ( i=0; i<cnt; ++i )
	free(seconds[i]);
    free(seconds);
    free(glyphs);
}

uint16 *ClassesFromNames(SplineFont *sf,char **classnames,int class_cnt,
	int numGlyphs, SplineChar ***glyphs, int apple_kc) {
    uint16 *class;
    int i;
    char *pt, *end, ch;
    SplineChar *sc, **gs=NULL;
    int offset = (apple_kc && classnames[0]!=NULL);

    class = gcalloc(numGlyphs,sizeof(uint16));
    if ( glyphs ) *glyphs = gs = gcalloc(numGlyphs,sizeof(SplineChar *));
    for ( i=0; i<class_cnt; ++i ) {
	if ( i==0 && classnames[0]==NULL )
    continue;
	for ( pt = classnames[i]; *pt; pt = end+1 ) {
	    while ( *pt==' ' ) ++pt;
	    if ( *pt=='\0' )
	break;
	    end = strchr(pt,' ');
	    if ( end==NULL )
		end = pt+strlen(pt);
	    ch = *end;
	    *end = '\0';
	    sc = SFGetChar(sf,-1,pt);
	    if ( sc!=NULL && sc->ttf_glyph!=-1 ) {
		class[sc->ttf_glyph] = i+offset;
		if ( gs!=NULL )
		    gs[sc->ttf_glyph] = sc;
	    }
	    *end = ch;
	    if ( ch=='\0' )
	break;
	}
    }
return( class );
}

static SplineChar **GlyphsFromClasses(SplineChar **gs, int numGlyphs) {
    int i, cnt;
    SplineChar **glyphs;

    for ( i=cnt=0; i<numGlyphs; ++i )
	if ( gs[i]!=NULL ) ++cnt;
    glyphs = galloc((cnt+1)*sizeof(SplineChar *));
    for ( i=cnt=0; i<numGlyphs; ++i )
	if ( gs[i]!=NULL )
	    glyphs[cnt++] = gs[i];
    glyphs[cnt++] = NULL;
    free(gs);
return( glyphs );
}

static SplineChar **GlyphsFromInitialClasses(SplineChar **gs, int numGlyphs, uint16 *classes, uint16 *initial) {
    int i, j, cnt;
    SplineChar **glyphs;

    for ( i=cnt=0; i<numGlyphs; ++i ) {
	for ( j=0; initial[j]!=0xffff; ++j )
	    if ( initial[j]==classes[i])
	break;
	if ( initial[j]!=0xffff && gs[i]!=NULL ) ++cnt;
    }
    glyphs = galloc((cnt+1)*sizeof(SplineChar *));
    for ( i=cnt=0; i<numGlyphs; ++i ) {
	for ( j=0; initial[j]!=0xffff; ++j )
	    if ( initial[j]==classes[i])
	break;
	if ( initial[j]!=0xffff && gs[i]!=NULL )
	    glyphs[cnt++] = gs[i];
    }
    glyphs[cnt++] = NULL;
return( glyphs );
}

static void DumpClass(FILE *gpos,uint16 *class,int numGlyphs) {
    int ranges, i, cur, first= -1, last=-1, istart;

    for ( i=ranges=0; i<numGlyphs; ) {
	istart = i;
	cur = class[i];
	while ( i<numGlyphs && class[i]==cur )
	    ++i;
	if ( cur!=0 ) {
	    ++ranges;
	    if ( first==-1 ) first = istart;
	    last = i-1;
	}
    }
    if ( ranges*3+1>last-first+1+2 || first==-1 ) {
	if ( first==-1 ) first = last = 0;
	putshort(gpos,1);		/* Format 1, list of all posibilities */
	putshort(gpos,first);
	putshort(gpos,last-first+1);
	for ( i=first; i<=last ; ++i )
	    putshort(gpos,class[i]);
    } else {
	putshort(gpos,2);		/* Format 2, series of ranges */
	putshort(gpos,ranges);
	for ( i=0; i<numGlyphs; ) {
	    istart = i;
	    cur = class[i];
	    while ( i<numGlyphs && class[i]==cur )
		++i;
	    if ( cur!=0 ) {
		putshort(gpos,istart);
		putshort(gpos,i-1);
		putshort(gpos,cur);
	    }
	}
    }
}

static int KernClassR2L(SplineFont *sf, KernClass *kc) {
    int i, ch, known_r2l ;
    char *pt, *start;
    SplineChar *sc;

    for ( i=1; i<kc->first_cnt; ++i ) {
	for ( start = kc->firsts[i]; *start ; ) {
	    for ( pt=start; *pt!=' ' && *pt!='\0'; ++pt );
	    ch = *pt; *pt = '\0';
	    sc = SFGetChar(sf,-1,start);
	    *pt = ch;
	    if ( sc!=NULL ) {
		known_r2l = SCRightToLeft(sc);
		if ( known_r2l )
return( true );
		else if ( SCScriptFromUnicode(sc)== DEFAULT_SCRIPT )
		    /* Do Nothing */;
		else
return( false );
	    }
	    while ( ch==' ' )
		ch = *++pt;
	    start = pt;
	}
    }

    /* Eh? All the data were non-commital? Well, try the second set of classes*/
    for ( i=1; i<kc->second_cnt; ++i ) {
	for ( start = kc->seconds[i]; *start ; ) {
	    for ( pt=start; *pt!=' ' && *pt!='\0'; ++pt );
	    ch = *pt; *pt = '\0';
	    sc = SFGetChar(sf,-1,start);
	    *pt = ch;
	    if ( sc!=NULL ) {
		known_r2l = SCRightToLeft(sc);
		if ( known_r2l )
return( true );
		else if ( SCScriptFromUnicode(sc)== DEFAULT_SCRIPT )
		    /* Do Nothing */;
		else
return( false );
	    }
	    while ( ch==' ' )
		ch = *++pt;
	    start = pt;
	}
    }

    /* We still don't have any idea? Well, lets just say it's l2r */
return( false );
}

static void dumpgposkernclass(FILE *gpos,SplineFont *sf,
	struct lookup_subtable *sub, struct alltabs *at) {
    uint32 begin_off = ftell(gpos), pos;
    uint16 *class1, *class2;
    KernClass *kc = sub->kc, *test;
    SplineChar **glyphs;
    int i, isv, isr2l;
    int anydevtab = false;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
    int next_devtab;
#endif

    putshort(gpos,2);		/* format 2 of the pair adjustment subtable */
    putshort(gpos,0);		/* offset to coverage table */
#ifdef FONTFORGE_CONFIG_DEVICETABLES
    for ( i=0; i<kc->first_cnt*kc->second_cnt; ++i ) {
	if ( kc->adjusts[i].corrections!=NULL ) {
	    anydevtab = true;
    break;
	}
    }
#endif

    for ( test=sf->vkerns; test!=NULL && test!=kc; test=test->next );
    isv = test==kc;
    isr2l = KernClassR2L(sf,kc);

    if ( isv ) {
	/* As far as I know there is no "bottom to top" writing direction */
	/*  Oh. There is. Ogham, Runic */
	putshort(gpos,anydevtab?0x0088:0x0008);	/* Alter YAdvance of first character */
	putshort(gpos,0x0000);			/* leave second char alone */
    } else if ( isr2l ) {
	/* Right to left kerns modify the second character's width */
	/*  this doesn't make sense to me, but who am I to argue */
	putshort(gpos,0x0000);			/* leave first char alone */
	putshort(gpos,anydevtab?0x0044:0x0004);	/* Alter XAdvance of second character */
    } else {
	putshort(gpos,anydevtab?0x0044:0x0004);	/* Alter XAdvance of first character */
	putshort(gpos,0x0000);			/* leave second char alone */
    }
    class1 = ClassesFromNames(sf,kc->firsts,kc->first_cnt,at->maxp.numGlyphs,&glyphs,false);
    glyphs = GlyphsFromClasses(glyphs,at->maxp.numGlyphs);
    class2 = ClassesFromNames(sf,kc->seconds,kc->second_cnt,at->maxp.numGlyphs,NULL,false);
    putshort(gpos,0);		/* offset to first glyph classes */
    putshort(gpos,0);		/* offset to second glyph classes */
    putshort(gpos,kc->first_cnt);
    putshort(gpos,kc->second_cnt);
#ifdef FONTFORGE_CONFIG_DEVICETABLES
    next_devtab = ftell(gpos)-begin_off + kc->first_cnt*kc->second_cnt*2*sizeof(uint16);
#endif
    for ( i=0; i<kc->first_cnt*kc->second_cnt; ++i ) {
	putshort(gpos,kc->offsets[i]);
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	if ( anydevtab && kc->adjusts[i].corrections!=NULL ) {
	    putshort(gpos,next_devtab);
	    next_devtab += DevTabLen(&kc->adjusts[i]);
	} else if ( anydevtab )
	    putshort(gpos,0);
#endif
    }
#ifdef FONTFORGE_CONFIG_DEVICETABLES
    if ( anydevtab ) {
	for ( i=0; i<kc->first_cnt*kc->second_cnt; ++i ) {
	    if ( kc->adjusts[i].corrections!=NULL )
		dumpgposdevicetable(gpos,&kc->adjusts[i]);
	}
	if ( next_devtab!=ftell(gpos)-begin_off )
	    IError("Device table offsets screwed up in kerning class");
    }
#endif
    pos = ftell(gpos);
    fseek(gpos,begin_off+4*sizeof(uint16),SEEK_SET);
    putshort(gpos,pos-begin_off);
    fseek(gpos,pos,SEEK_SET);
    DumpClass(gpos,class1,at->maxp.numGlyphs);

    pos = ftell(gpos);
    fseek(gpos,begin_off+5*sizeof(uint16),SEEK_SET);
    putshort(gpos,pos-begin_off);
    fseek(gpos,pos,SEEK_SET);
    DumpClass(gpos,class2,at->maxp.numGlyphs);

    pos = ftell(gpos);
    fseek(gpos,begin_off+sizeof(uint16),SEEK_SET);
    putshort(gpos,pos-begin_off);
    fseek(gpos,pos,SEEK_SET);
    dumpcoveragetable(gpos,glyphs);

    free(glyphs);
    free(class1);
    free(class2);
}

static void dumpanchor(FILE *gpos,AnchorPoint *ap, int is_ttf ) {
#ifdef FONTFORGE_CONFIG_DEVICETABLES
    int base = ftell(gpos);
#endif
#ifdef FONTFORGE_CONFIG_DEVICETABLES
    if ( ap->xadjust.corrections!=NULL || ap->yadjust.corrections!=NULL )
	putshort(gpos,3);	/* format 3 w/ device tables */
    else
#endif
    if ( ap->has_ttf_pt && is_ttf )
	putshort(gpos,2);	/* format 2 w/ a matching ttf point index */
    else
	putshort(gpos,1);	/* Anchor format 1 just location*/
    putshort(gpos,ap->me.x);	/* X coord of attachment */
    putshort(gpos,ap->me.y);	/* Y coord of attachment */
#ifdef FONTFORGE_CONFIG_DEVICETABLES
    if ( ap->xadjust.corrections!=NULL || ap->yadjust.corrections!=NULL ) {
	putshort(gpos,ap->xadjust.corrections==NULL?0:
		ftell(gpos)-base+4);
	putshort(gpos,ap->yadjust.corrections==NULL?0:
		ftell(gpos)-base+2+DevTabLen(&ap->xadjust));
	dumpgposdevicetable(gpos,&ap->xadjust);
	dumpgposdevicetable(gpos,&ap->yadjust);
    } else
#endif
    if ( ap->has_ttf_pt && is_ttf )
	putshort(gpos,ap->ttf_pt_index);
}

static void dumpgposCursiveAttach(FILE *gpos, SplineFont *sf,
	struct lookup_subtable *sub,struct glyphinfo *gi) {
    AnchorClass *ac, *testac;
    SplineChar **entryexit;
    int cnt, offset,j;
    AnchorPoint *ap, *entry, *exit;
    uint32 coverage_offset, start;

    ac = NULL;
    for ( testac=sf->anchor; testac!=NULL; testac = testac->next ) {
	if ( testac->subtable == sub ) {
	    if ( ac==NULL )
		ac = testac;
	    else {
		gwwv_post_error(_("Two cursive anchor classes"),_("Two cursive anchor classes in the same subtable, %s"),
			sub->subtable_name);
    break;
	    }
	}
    }
    if ( ac==NULL ) {
	IError( "Missing anchor class for %s", sub->subtable_name );
return;
    }
    entryexit = EntryExitDecompose(sf,ac,gi);
    if ( entryexit==NULL )
return;

    for ( cnt=0; entryexit[cnt]!=NULL; ++cnt );

    start = ftell(gpos);
    putshort(gpos,1);	/* format 1 for this subtable */
    putshort(gpos,0);	/* Fill in later, offset to coverage table */
    putshort(gpos,cnt);	/* number of glyphs */

    offset = 6+2*2*cnt;
    for ( j=0; j<cnt; ++j ) {
	entry = exit = NULL;
	for ( ap=entryexit[j]->anchor; ap!=NULL; ap=ap->next ) {
	    if ( ap->anchor==ac && ap->type==at_centry ) entry = ap;
	    if ( ap->anchor==ac && ap->type==at_cexit ) exit = ap;
	}
	if ( entry!=NULL ) {
	    putshort(gpos,offset);
	    offset += 6;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	    if ( entry->xadjust.corrections!=NULL || entry->yadjust.corrections!=NULL )
		offset += 4 + DevTabLen(&entry->xadjust) + DevTabLen(&entry->yadjust);
#endif
	} else
	    putshort(gpos,0);
	if ( exit!=NULL ) {
	    putshort(gpos,offset);
	    offset += 6;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	    if ( exit->xadjust.corrections!=NULL || exit->yadjust.corrections!=NULL )
		offset += 4 + DevTabLen(&exit->xadjust) + DevTabLen(&exit->yadjust);
	    else
#endif
	    if ( gi->is_ttf && ap->has_ttf_pt )
		offset += 2;
	} else
	    putshort(gpos,0);
    }
    for ( j=0; j<cnt; ++j ) {
	entry = exit = NULL;
	for ( ap=entryexit[j]->anchor; ap!=NULL; ap=ap->next ) {
	    if ( ap->anchor==ac && ap->type==at_centry ) entry = ap;
	    if ( ap->anchor==ac && ap->type==at_cexit ) exit = ap;
	}
	if ( entry!=NULL )
	    dumpanchor(gpos,entry,gi->is_ttf);
	if ( exit!=NULL )
	    dumpanchor(gpos,exit,gi->is_ttf);
    }
    coverage_offset = ftell(gpos);
    dumpcoveragetable(gpos,entryexit);
    fseek(gpos,start+2,SEEK_SET);
    putshort(gpos,coverage_offset-start);
    fseek(gpos,0,SEEK_END);

    free(entryexit);
}

static int orderglyph(const void *_sc1,const void *_sc2) {
    SplineChar * const *sc1 = _sc1, * const *sc2 = _sc2;

return( (*sc1)->ttf_glyph - (*sc2)->ttf_glyph );
}

static SplineChar **allmarkglyphs(SplineChar ***glyphlist, int classcnt) {
    SplineChar **glyphs;
    int i, tot, k;

    if ( classcnt==1 )
return( glyphlist[0]);

    for ( i=tot=0; i<classcnt; ++i ) {
	for ( k=0; glyphlist[i][k]!=NULL; ++k );
	tot += k;
    }
    glyphs = galloc((tot+1)*sizeof(SplineChar *));
    for ( i=tot=0; i<classcnt; ++i ) {
	for ( k=0; glyphlist[i][k]!=NULL; ++k )
	    glyphs[tot++] = glyphlist[i][k];
    }
    qsort(glyphs,tot,sizeof(SplineChar *),orderglyph);
    for ( i=k=0; i<tot; ++i ) {
	while ( i+1<tot && glyphs[i]==glyphs[i+1]) ++i;
	glyphs[k++] = glyphs[i];
    }
    glyphs[k] = NULL;
return( glyphs );
}

static void dumpgposAnchorData(FILE *gpos,AnchorClass *_ac,
	enum anchor_type at,
	SplineChar ***marks,SplineChar **base,
	int classcnt, struct glyphinfo *gi) {
    AnchorClass *ac=NULL;
    int j,cnt,k,l, pos, offset, tot, max;
    uint32 coverage_offset, markarray_offset, subtable_start;
    AnchorPoint *ap = NULL, **aps;
    SplineChar **markglyphs;

    for ( cnt=0; base[cnt]!=NULL; ++cnt );

    subtable_start = ftell(gpos);
    putshort(gpos,1);	/* format 1 for this subtable */
    putshort(gpos,0);	/* Fill in later, offset to mark coverage table */
    putshort(gpos,0);	/* Fill in later, offset to base coverage table */
    putshort(gpos,classcnt);
    putshort(gpos,0);	/* Fill in later, offset to mark array */
    putshort(gpos,12);	/* Offset to base array */
	/* Base array */
    putshort(gpos,cnt);	/* Number of entries in array */
    if ( at==at_basechar || at==at_basemark ) {
	offset = 2;
	for ( l=0; l<3; ++l ) {
	    for ( j=0; j<cnt; ++j ) {
		for ( k=0, ac=_ac; k<classcnt; ++k, ac=ac->next ) {
		    for ( ap=base[j]->anchor; ap!=NULL && (ap->anchor!=ac || ap->type!=at);
			    ap=ap->next );
		    switch ( l ) {
		      case 0:
			offset += 2;
		      break;
		      case 1:
			if ( ap==NULL )
			    putshort(gpos,0);
			else {
			    putshort(gpos,offset);
			    offset += 6;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
			    if ( ap->xadjust.corrections!=NULL || ap->yadjust.corrections!=NULL )
				offset += 4 + DevTabLen(&ap->xadjust) + DevTabLen(&ap->yadjust);
			    else
#endif
			    if ( gi->is_ttf && ap->has_ttf_pt )
				offset += 2;
			}
		      break;
		      case 2:
			if ( ap!=NULL )
			    dumpanchor(gpos,ap,gi->is_ttf);
		      break;
		    }
		}
	    }
	}
    } else {
	offset = 2+2*cnt;
	max = 0;
	for ( j=0; j<cnt; ++j ) {
	    putshort(gpos,offset);
	    pos = tot = 0;
	    for ( ap=base[j]->anchor; ap!=NULL ; ap=ap->next )
		for ( k=0, ac=_ac; k<classcnt; ++k, ac=ac->next ) {
		    if ( ap->anchor==ac ) {
			if ( ap->lig_index>pos ) pos = ap->lig_index;
			++tot;
		    }
		}
	    if ( pos>max ) max = pos;
	    offset += 2+(pos+1)*classcnt*2+tot*6;
		/* 2 for component count, for each component an offset to an offset to an anchor record */
	}
	++max;
	aps = galloc((classcnt*max)*sizeof(AnchorPoint *));
	for ( j=0; j<cnt; ++j ) {
	    memset(aps,0,(classcnt*max)*sizeof(AnchorPoint *));
	    pos = 0;
	    for ( ap=base[j]->anchor; ap!=NULL ; ap=ap->next )
		for ( k=0, ac=_ac; k<classcnt; ++k, ac=ac->next ) {
		    if ( ap->anchor==ac ) {
			if ( ap->lig_index>pos ) pos = ap->lig_index;
			aps[k*max+ap->lig_index] = ap;
		    }
		}
	    ++pos;
	    putshort(gpos,pos);
	    offset = 2+2*pos*classcnt;
	    for ( l=0; l<pos; ++l ) {
		for ( k=0; k<classcnt; ++k ) {
		    if ( aps[k*max+l]==NULL )
			putshort(gpos,0);
		    else {
			putshort(gpos,offset);
			offset += 6;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
			if ( aps[k*max+l]->xadjust.corrections!=NULL || aps[k*max+l]->yadjust.corrections!=NULL )
			    offset += 4 + DevTabLen(&aps[k*max+l]->xadjust) +
					DevTabLen(&aps[k*max+l]->yadjust);
			else
#endif
			if ( gi->is_ttf && aps[k*max+l]->has_ttf_pt )
			    offset += 2;
		    }
		}
	    }
	    for ( l=0; l<pos; ++l ) {
		for ( k=0; k<classcnt; ++k ) {
		    if ( aps[k*max+l]!=NULL ) {
			dumpanchor(gpos,aps[k*max+l],gi->is_ttf);
		    }
		}
	    }
	}
	free(aps);
    }
    coverage_offset = ftell(gpos);
    fseek(gpos,subtable_start+4,SEEK_SET);
    putshort(gpos,coverage_offset-subtable_start);
    fseek(gpos,0,SEEK_END);
    dumpcoveragetable(gpos,base);

    /* We tried sharing the mark table, (among all these sub-tables) but */
    /*  that doesn't work because we need to be able to reorder the sub-tables */
    markglyphs = allmarkglyphs(marks,classcnt);
    coverage_offset = ftell(gpos);
    dumpcoveragetable(gpos,markglyphs);
    markarray_offset = ftell(gpos);
    for ( cnt=0; markglyphs[cnt]!=NULL; ++cnt );
    putshort(gpos,cnt);
    offset = 2+4*cnt;
    for ( j=0; j<cnt; ++j ) {
	if ( classcnt==0 ) {
	    putshort(gpos,0);		/* Only one class */
	    ap = NULL;
	} else {
	    for ( k=0, ac=_ac; k<classcnt; ac=ac->next ) {
		if ( ac->matches ) {
		    for ( ap = markglyphs[j]->anchor; ap!=NULL && (ap->anchor!=ac || ap->type!=at_mark);
			    ap=ap->next );
		    if ( ap!=NULL )
	    break;
		    ++k;
		}
	    }
	    putshort(gpos,k);
	}
	putshort(gpos,offset);
	offset += 6;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	if ( ap!=NULL && (ap->xadjust.corrections!=NULL || ap->yadjust.corrections!=NULL ))
	    offset += 4 + DevTabLen(&ap->xadjust) + DevTabLen(&ap->yadjust);
	else
#endif
	if ( gi->is_ttf && ap->has_ttf_pt )
	    offset += 2;
    }
    for ( j=0; j<cnt; ++j ) {
	for ( k=0, ac=_ac; k<classcnt; ac=ac->next ) {
	    if ( ac->matches ) {
		for ( ap = markglyphs[j]->anchor; ap!=NULL && (ap->anchor!=ac || ap->type!=at_mark);
			ap=ap->next );
		if ( ap!=NULL )
	break;
		++k;
	    }
	}
	dumpanchor(gpos,ap,gi->is_ttf);
    }
    if ( markglyphs!=marks[0] )
	free(markglyphs);

    fseek(gpos,subtable_start+2,SEEK_SET);	/* mark coverage table offset */
    putshort(gpos,coverage_offset-subtable_start);
    fseek(gpos,4,SEEK_CUR);
    putshort(gpos,markarray_offset-subtable_start);

    fseek(gpos,0,SEEK_END);
}

static void dumpGSUBsimplesubs(FILE *gsub,SplineFont *sf,struct lookup_subtable *sub) {
    int cnt, diff, ok = true;
    int32 coverage_pos, end;
    SplineChar **glyphs, ***maps;

    glyphs = SFOrderedGlyphsWithPSTinSubtable(sf,sub);
    maps = generateMapList(glyphs,sub);

    diff = (*maps[0])->ttf_glyph - glyphs[0]->ttf_glyph;
    for ( cnt=0; glyphs[cnt]!=NULL; ++cnt)
	if ( diff!= maps[cnt][0]->ttf_glyph-glyphs[cnt]->ttf_glyph ) ok = false;

    if ( ok ) {
	putshort(gsub,1);	/* delta format */
	coverage_pos = ftell(gsub);
	putshort(gsub,0);		/* offset to coverage table */
	putshort(gsub,diff);
    } else {
	putshort(gsub,2);		/* glyph list format */
	coverage_pos = ftell(gsub);
	putshort(gsub,0);		/* offset to coverage table */
	putshort(gsub,cnt);
	for ( cnt = 0; glyphs[cnt]!=NULL; ++cnt )
	    putshort(gsub,(*maps[cnt])->ttf_glyph);
    }
    end = ftell(gsub);
    fseek(gsub,coverage_pos,SEEK_SET);
    putshort(gsub,end-coverage_pos+2);
    fseek(gsub,end,SEEK_SET);
    dumpcoveragetable(gsub,glyphs);

    free(glyphs);
    GlyphMapFree(maps);
}

static void dumpGSUBmultiplesubs(FILE *gsub,SplineFont *sf,struct lookup_subtable *sub) {
    int cnt, offset;
    int32 coverage_pos, end;
    int gc;
    SplineChar **glyphs, ***maps;

    glyphs = SFOrderedGlyphsWithPSTinSubtable(sf,sub);
    maps = generateMapList(glyphs,sub);
    for ( cnt=0; glyphs[cnt]!=NULL; ++cnt);

    putshort(gsub,1);		/* glyph list format */
    coverage_pos = ftell(gsub);
    putshort(gsub,0);		/* offset to coverage table */
    putshort(gsub,cnt);
    offset = 6+2*cnt;
    for ( cnt = 0; glyphs[cnt]!=NULL; ++cnt ) {
	putshort(gsub,offset);
	for ( gc=0; maps[cnt][gc]!=NULL; ++gc );
	offset += 2+2*gc;
    }
    for ( cnt = 0; glyphs[cnt]!=NULL; ++cnt ) {
	for ( gc=0; maps[cnt][gc]!=NULL; ++gc );
	putshort(gsub,gc);
	for ( gc=0; maps[cnt][gc]!=NULL; ++gc )
	    putshort(gsub,maps[cnt][gc]->ttf_glyph);
    }
    end = ftell(gsub);
    fseek(gsub,coverage_pos,SEEK_SET);
    putshort(gsub,end-coverage_pos+2);
    fseek(gsub,end,SEEK_SET);
    dumpcoveragetable(gsub,glyphs);

    free(glyphs);
    GlyphMapFree(maps);
}

static int AllToBeOutput(LigList *lig) {
    struct splinecharlist *cmp;

    if ( lig->lig->u.lig.lig->ttf_glyph==-1 ||
	    lig->first->ttf_glyph==-1 )
return( 0 );
    for ( cmp=lig->components; cmp!=NULL; cmp=cmp->next )
	if ( cmp->sc->ttf_glyph==-1 )
return( 0 );
return( true );
}

static void dumpGSUBligdata(FILE *gsub,SplineFont *sf,
	struct lookup_subtable *sub, struct alltabs *at) {
    int32 coverage_pos, next_val_pos, here, lig_list_start;
    int cnt, i, pcnt, lcnt, max=100, j;
    uint16 *offsets=NULL, *ligoffsets=galloc(max*sizeof(uint16));
    SplineChar **glyphs;
    LigList *ll;
    struct splinecharlist *scl;

    glyphs = SFOrderedGlyphsWithPSTinSubtable(sf,sub);
    cnt=0;
    if ( glyphs!=NULL ) for ( ; glyphs[cnt]!=NULL; ++cnt );

    putshort(gsub,1);		/* only one format for ligatures */
    coverage_pos = ftell(gsub);
    putshort(gsub,0);		/* offset to coverage table */
    putshort(gsub,cnt);
    next_val_pos = ftell(gsub);
    if ( glyphs!=NULL )
	offsets = galloc(cnt*sizeof(int16));
    for ( i=0; i<cnt; ++i )
	putshort(gsub,0);
    for ( i=0; i<cnt; ++i ) {
	offsets[i] = ftell(gsub)-coverage_pos+2;
	for ( pcnt = 0, ll = glyphs[i]->ligofme; ll!=NULL; ll=ll->next )
	    if ( ll->lig->subtable==sub && AllToBeOutput(ll))
		++pcnt;
	putshort(gsub,pcnt);
	if ( pcnt>=max ) {
	    max = pcnt+100;
	    ligoffsets = grealloc(ligoffsets,max*sizeof(int));
	}
	lig_list_start = ftell(gsub);
	for ( j=0; j<pcnt; ++j )
	    putshort(gsub,0);			/* Place holders */
	for ( pcnt=0, ll = glyphs[i]->ligofme; ll!=NULL; ll=ll->next ) {
	    if ( ll->lig->subtable==sub && AllToBeOutput(ll)) {
		ligoffsets[pcnt] = ftell(gsub)-lig_list_start+2;
		putshort(gsub,ll->lig->u.lig.lig->ttf_glyph);
		for ( lcnt=0, scl=ll->components; scl!=NULL; scl=scl->next ) ++lcnt;
		putshort(gsub, lcnt+1);
		if ( lcnt+1>at->os2.maxContext )
		    at->os2.maxContext = lcnt+1;
		for ( scl=ll->components; scl!=NULL; scl=scl->next )
		    putshort(gsub, scl->sc->ttf_glyph );
		++pcnt;
	    }
	}
	fseek(gsub,lig_list_start,SEEK_SET);
	for ( j=0; j<pcnt; ++j )
	    putshort(gsub,ligoffsets[j]);
	fseek(gsub,0,SEEK_END);
    }
    free(ligoffsets);
    if ( glyphs!=NULL ) {
	here = ftell(gsub);
	fseek(gsub,coverage_pos,SEEK_SET);
	putshort(gsub,here-coverage_pos+2);
	fseek(gsub,next_val_pos,SEEK_SET);
	for ( i=0; i<cnt; ++i )
	    putshort(gsub,offsets[i]);
	fseek(gsub,here,SEEK_SET);
	dumpcoveragetable(gsub,glyphs);
	free(glyphs);
	free(offsets);
    }
}

static int ui16cmp(const void *_i1, const void *_i2) {
    if ( *(const uint16 *) _i1 > *(const uint16 *) _i2 )
return( 1 );
    if ( *(const uint16 *) _i1 < *(const uint16 *) _i2 )
return( -1 );

return( 0 );
}

static uint16 *FigureInitialClasses(FPST *fpst) {
    uint16 *initial = galloc((fpst->nccnt+1)*sizeof(uint16));
    int i, cnt, j;

    for ( i=cnt=0; i<fpst->rule_cnt; ++i ) {
	for ( j=0; j<cnt ; ++j )
	    if ( initial[j] == fpst->rules[i].u.class.nclasses[0] )
	break;
	if ( j==cnt )
	    initial[cnt++] = fpst->rules[i].u.class.nclasses[0];
    }
    qsort(initial,cnt,sizeof(uint16),ui16cmp);
    initial[cnt] = 0xffff;
return( initial );
}

static SplineChar **OrderedInitialGlyphs(SplineFont *sf,FPST *fpst) {
    SplineChar **glyphs, *sc;
    int i, j, cnt, ch;
    char *pt, *names;

    glyphs = galloc((fpst->rule_cnt+1)*sizeof(SplineChar *));
    for ( i=cnt=0; i<fpst->rule_cnt; ++i ) {
	names = fpst->rules[i].u.glyph.names;
	pt = strchr(names,' ');
	if ( pt==NULL ) pt = names+strlen(names);
	ch = *pt; *pt = '\0';
	sc = SFGetChar(sf,-1,names);
	*pt = ch;
	for ( j=0; j<cnt; ++j )
	    if ( glyphs[j]==sc )
	break;
	if ( j==cnt && sc!=NULL )
	    glyphs[cnt++] = sc;
    }
    glyphs[cnt] = NULL;
    if ( cnt==0 )
return( glyphs );

    for ( i=0; glyphs[i+1]!=NULL; ++i ) for ( j=i+1; glyphs[j]!=NULL; ++j ) {
	if ( glyphs[i]->ttf_glyph > glyphs[j]->ttf_glyph ) {
	    sc = glyphs[i];
	    glyphs[i] = glyphs[j];
	    glyphs[j] = sc;
	}
    }
return( glyphs );
}

static int NamesStartWith(SplineChar *sc,char *names ) {
    char *pt;

    pt = strchr(names,' ');
    if ( pt==NULL ) pt = names+strlen(names);
    if ( pt-names!=strlen(sc->name))
return( false );

return( strncmp(sc->name,names,pt-names)==0 );
}

static int CntRulesStartingWith(FPST *fpst,SplineChar *sc) {
    int i, cnt;

    for ( i=cnt=0; i<fpst->rule_cnt; ++i ) {
	if ( NamesStartWith(sc,fpst->rules[i].u.glyph.names))
	    ++cnt;
    }
return( cnt );
}

static int CntRulesStartingWithClass(FPST *fpst,uint16 cval) {
    int i, cnt;

    for ( i=cnt=0; i<fpst->rule_cnt; ++i ) {
	if ( fpst->rules[i].u.class.nclasses[0]==cval )
	    ++cnt;
    }
return( cnt );
}

static void dumpg___ContextChainGlyphs(FILE *lfile,SplineFont *sf,
	struct lookup_subtable *sub, struct alltabs *at) {
    FPST *fpst = sub->fpst;
    int iscontext = fpst->type==pst_contextpos || fpst->type==pst_contextsub;
    uint32 base = ftell(lfile);
    int i,cnt, subcnt, j,k,l, maxcontext,curcontext;
    SplineChar **glyphs, **subglyphs;
    int lc;

    glyphs = OrderedInitialGlyphs(sf,fpst);
    for ( cnt=0; glyphs[cnt]!=NULL; ++cnt );

    putshort(lfile,1);		/* Sub format 1 => glyph lists */
    putshort(lfile,(3+cnt)*sizeof(short));	/* offset to coverage */
    putshort(lfile,cnt);
    for ( i=0; i<cnt; ++i )
	putshort(lfile,0);	/* Offset to rule */
    dumpcoveragetable(lfile,glyphs);

    maxcontext = 0;

    for ( i=0; i<cnt; ++i ) {
	uint32 pos = ftell(lfile);
	fseek(lfile,base+(3+i)*sizeof(short),SEEK_SET);
	putshort(lfile,pos-base);
	fseek(lfile,pos,SEEK_SET);
	subcnt = CntRulesStartingWith(fpst,glyphs[i]);
	putshort(lfile,subcnt);
	for ( j=0; j<subcnt; ++j )
	    putshort(lfile,0);
	for ( j=k=0; k<fpst->rule_cnt; ++k ) if ( NamesStartWith(glyphs[i],fpst->rules[k].u.glyph.names )) {
	    uint32 subpos = ftell(lfile);
	    fseek(lfile,pos+(1+j)*sizeof(short),SEEK_SET);
	    putshort(lfile,subpos-pos);
	    fseek(lfile,subpos,SEEK_SET);

	    for ( l=lc=0; l<fpst->rules[k].lookup_cnt; ++l )
		if ( fpst->rules[k].lookups[l].lookup->lookup_index!=-1 )
		    ++lc;
	    if ( iscontext ) {
		subglyphs = SFGlyphsFromNames(sf,fpst->rules[k].u.glyph.names);
		for ( l=0; subglyphs[l]!=NULL; ++l );
		putshort(lfile,l);
		curcontext = l;
		putshort(lfile,lc);
		for ( l=1; subglyphs[l]!=NULL; ++l )
		    putshort(lfile,subglyphs[l]->ttf_glyph);
		free(subglyphs);
	    } else {
		subglyphs = SFGlyphsFromNames(sf,fpst->rules[k].u.glyph.back);
		for ( l=0; subglyphs[l]!=NULL; ++l );
		putshort(lfile,l);
		curcontext = l;
		for ( l=0; subglyphs[l]!=NULL; ++l )
		    putshort(lfile,subglyphs[l]->ttf_glyph);
		free(subglyphs);
		subglyphs = SFGlyphsFromNames(sf,fpst->rules[k].u.glyph.names);
		for ( l=0; subglyphs[l]!=NULL; ++l );
		putshort(lfile,l);
		curcontext += l;
		for ( l=1; subglyphs[l]!=NULL; ++l )
		    putshort(lfile,subglyphs[l]->ttf_glyph);
		free(subglyphs);
		subglyphs = SFGlyphsFromNames(sf,fpst->rules[k].u.glyph.fore);
		for ( l=0; subglyphs[l]!=NULL; ++l );
		putshort(lfile,l);
		curcontext += l;
		for ( l=0; subglyphs[l]!=NULL; ++l )
		    putshort(lfile,subglyphs[l]->ttf_glyph);
		free(subglyphs);
		putshort(lfile,lc);
	    }
	    for ( l=0; l<fpst->rules[k].lookup_cnt; ++l )
		if ( fpst->rules[k].lookups[l].lookup->lookup_index!=-1 ) {
		    putshort(lfile,fpst->rules[k].lookups[l].seq);
		    putshort(lfile,fpst->rules[k].lookups[l].lookup->lookup_index);
		}
	    ++j;
	    if ( curcontext>maxcontext ) maxcontext = curcontext;
	}
    }
    free(glyphs);

    if ( maxcontext>at->os2.maxContext )
	at->os2.maxContext = maxcontext;
}

static void dumpg___ContextChainClass(FILE *lfile,SplineFont *sf,
	struct lookup_subtable *sub, struct alltabs *at) {
    FPST *fpst = sub->fpst;
    int iscontext = fpst->type==pst_contextpos || fpst->type==pst_contextsub;
    uint32 base = ftell(lfile), rulebase, pos, subpos, npos;
    uint16 *initialclasses, *iclass, *bclass, *lclass;
    SplineChar **iglyphs, **bglyphs, **lglyphs, **glyphs;
    int i,ii,cnt, subcnt, j,k,l , maxcontext,curcontext;
    int lc;

    putshort(lfile,2);		/* Sub format 2 => class */
    putshort(lfile,0);		/* offset to coverage table */
    if ( iscontext )
	putshort(lfile,0);	/* offset to input classdef */
    else {
	putshort(lfile,0);	/* offset to backtrack classdef */
	putshort(lfile,0);	/* offset to input classdef */
	putshort(lfile,0);	/* offset to lookahead classdef */
    }
    initialclasses = FigureInitialClasses(fpst);
    putshort(lfile,fpst->nccnt);
    rulebase = ftell(lfile);
    for ( cnt=0; cnt<fpst->nccnt; ++cnt )
	putshort(lfile,0);

    iclass = ClassesFromNames(sf,fpst->nclass,fpst->nccnt,at->maxp.numGlyphs,&iglyphs,false);
    lglyphs = bglyphs = NULL; bclass = lclass = NULL;
    if ( !iscontext ) {
	bclass = ClassesFromNames(sf,fpst->bclass,fpst->bccnt,at->maxp.numGlyphs,&bglyphs,false);
	lclass = ClassesFromNames(sf,fpst->fclass,fpst->fccnt,at->maxp.numGlyphs,&lglyphs,false);
    }
    pos = ftell(lfile);
    fseek(lfile,base+sizeof(uint16),SEEK_SET);
    putshort(lfile,pos-base);
    fseek(lfile,pos,SEEK_SET);
    glyphs = GlyphsFromInitialClasses(iglyphs,at->maxp.numGlyphs,iclass,initialclasses);
    dumpcoveragetable(lfile,glyphs);
    free(glyphs);
    free(iglyphs); free(bglyphs); free(lglyphs);

    if ( iscontext ) {
	pos = ftell(lfile);
	fseek(lfile,base+2*sizeof(uint16),SEEK_SET);
	putshort(lfile,pos-base);
	fseek(lfile,pos,SEEK_SET);
	DumpClass(lfile,iclass,at->maxp.numGlyphs);
	free(iclass);
    } else {
	pos = ftell(lfile);
	fseek(lfile,base+2*sizeof(uint16),SEEK_SET);
	putshort(lfile,pos-base);
	fseek(lfile,pos,SEEK_SET);
	DumpClass(lfile,bclass,at->maxp.numGlyphs);
	if ( ClassesMatch(fpst->bccnt,fpst->bclass,fpst->nccnt,fpst->nclass)) {
	    npos = pos;
	    fseek(lfile,base+3*sizeof(uint16),SEEK_SET);
	    putshort(lfile,npos-base);
	    fseek(lfile,0,SEEK_END);
	} else {
	    npos = ftell(lfile);
	    fseek(lfile,base+3*sizeof(uint16),SEEK_SET);
	    putshort(lfile,npos-base);
	    fseek(lfile,npos,SEEK_SET);
	    DumpClass(lfile,iclass,at->maxp.numGlyphs);
	}
	if ( ClassesMatch(fpst->fccnt,fpst->fclass,fpst->bccnt,fpst->bclass)) {
	    fseek(lfile,base+4*sizeof(uint16),SEEK_SET);
	    putshort(lfile,pos-base);
	    fseek(lfile,0,SEEK_END);
	} else if ( ClassesMatch(fpst->fccnt,fpst->fclass,fpst->nccnt,fpst->nclass)) {
	    fseek(lfile,base+4*sizeof(uint16),SEEK_SET);
	    putshort(lfile,npos-base);
	    fseek(lfile,0,SEEK_END);
	} else {
	    pos = ftell(lfile);
	    fseek(lfile,base+4*sizeof(uint16),SEEK_SET);
	    putshort(lfile,pos-base);
	    fseek(lfile,pos,SEEK_SET);
	    DumpClass(lfile,lclass,at->maxp.numGlyphs);
	}
	free(iclass); free(bclass); free(lclass);
    }

    ii=0;
    for ( i=0; i<fpst->nccnt; ++i ) {
	if ( initialclasses[ii]!=i ) {
	    /* This class isn't an initial one, so leave it's rule pointer NULL */
	} else {
	    ++ii;
	    pos = ftell(lfile);
	    fseek(lfile,rulebase+i*sizeof(short),SEEK_SET);
	    putshort(lfile,pos-base);
	    fseek(lfile,pos,SEEK_SET);
	    subcnt = CntRulesStartingWithClass(fpst,i);
	    putshort(lfile,subcnt);
	    for ( j=0; j<subcnt; ++j )
		putshort(lfile,0);
	    for ( j=k=0; k<fpst->rule_cnt; ++k ) if ( i==fpst->rules[k].u.class.nclasses[0] ) {
		subpos = ftell(lfile);
		fseek(lfile,pos+(1+j)*sizeof(short),SEEK_SET);
		putshort(lfile,subpos-pos);
		fseek(lfile,subpos,SEEK_SET);

		for ( l=lc=0; l<fpst->rules[k].lookup_cnt; ++l )
		    if ( fpst->rules[k].lookups[l].lookup->lookup_index!=-1 )
			++lc;
		if ( iscontext ) {
		    putshort(lfile,fpst->rules[k].u.class.ncnt);
		    putshort(lfile,lc);
		    for ( l=1; l<fpst->rules[k].u.class.ncnt; ++l )
			putshort(lfile,fpst->rules[k].u.class.nclasses[l]);
		} else {
		    putshort(lfile,fpst->rules[k].u.class.bcnt);
		    for ( l=0; l<fpst->rules[k].u.class.bcnt; ++l )
			putshort(lfile,fpst->rules[k].u.class.bclasses[l]);
		    putshort(lfile,fpst->rules[k].u.class.ncnt);
		    for ( l=1; l<fpst->rules[k].u.class.ncnt; ++l )
			putshort(lfile,fpst->rules[k].u.class.nclasses[l]);
		    putshort(lfile,fpst->rules[k].u.class.fcnt);
		    for ( l=0; l<fpst->rules[k].u.class.fcnt; ++l )
			putshort(lfile,fpst->rules[k].u.class.fclasses[l]);
		    putshort(lfile,lc);
		}
		for ( l=0; l<fpst->rules[k].lookup_cnt; ++l )
		    if ( fpst->rules[k].lookups[l].lookup->lookup_index!=-1 ) {
			putshort(lfile,fpst->rules[k].lookups[l].seq);
			putshort(lfile,fpst->rules[k].lookups[l].lookup->lookup_index);
		    }
		++j;
	    }
	}
    }
    free(initialclasses);

    maxcontext = 0;
    for ( i=0; i<fpst->rule_cnt; ++i ) {
	curcontext = fpst->rules[i].u.class.ncnt+fpst->rules[i].u.class.bcnt+fpst->rules[i].u.class.fcnt;
	if ( curcontext>maxcontext ) maxcontext = curcontext;
    }
    if ( maxcontext>at->os2.maxContext )
	at->os2.maxContext = maxcontext;
}

static void dumpg___ContextChainCoverage(FILE *lfile,SplineFont *sf,
	struct lookup_subtable *sub, struct alltabs *at) {
    FPST *fpst = sub->fpst;
    int iscontext = fpst->type==pst_contextpos || fpst->type==pst_contextsub;
    uint32 base = ftell(lfile), ibase=0, lbase, bbase;
    int i, l;
    SplineChar **glyphs;
    int curcontext;
    int lc;

    if ( fpst->rule_cnt!=1 )
	IError("Bad rule cnt in coverage context lookup");
    if ( fpst->format==pst_reversecoverage && fpst->rules[0].u.rcoverage.always1!=1 )
	IError("Bad input count in reverse coverage lookup" );

    putshort(lfile,3);		/* Sub format 3 => coverage */
    for ( l=lc=0; l<fpst->rules[0].lookup_cnt; ++l )
	if ( fpst->rules[0].lookups[l].lookup->lookup_index!=-1 )
	    ++lc;
    if ( iscontext ) {
	putshort(lfile,fpst->rules[0].u.coverage.ncnt);
	putshort(lfile,lc);
	for ( i=0; i<fpst->rules[0].u.coverage.ncnt; ++i )
	    putshort(lfile,0);
	for ( i=0; i<fpst->rules[0].lookup_cnt; ++i )
	    if ( fpst->rules[0].lookups[i].lookup->lookup_index!=-1 ) {
		putshort(lfile,fpst->rules[0].lookups[i].seq);
		putshort(lfile,fpst->rules[0].lookups[i].lookup->lookup_index);
	    }
	for ( i=0; i<fpst->rules[0].u.coverage.ncnt; ++i ) {
	    uint32 pos = ftell(lfile);
	    fseek(lfile,base+6+2*i,SEEK_SET);
	    putshort(lfile,pos-base);
	    fseek(lfile,pos,SEEK_SET);
	    glyphs = OrderedGlyphsFromNames(sf,fpst->rules[0].u.coverage.ncovers[i]);
	    dumpcoveragetable(lfile,glyphs);
	    free(glyphs);
	}
    } else {
	if ( fpst->format==pst_reversecoverage ) {
	    ibase = ftell(lfile);
	    putshort(lfile,0);
	}
	putshort(lfile,fpst->rules[0].u.coverage.bcnt);
	bbase = ftell(lfile);
	for ( i=0; i<fpst->rules[0].u.coverage.bcnt; ++i )
	    putshort(lfile,0);
	if ( fpst->format==pst_coverage ) {
	    putshort(lfile,fpst->rules[0].u.coverage.ncnt);
	    ibase = ftell(lfile);
	    for ( i=0; i<fpst->rules[0].u.coverage.ncnt; ++i )
		putshort(lfile,0);
	}
	putshort(lfile,fpst->rules[0].u.coverage.fcnt);
	lbase = ftell(lfile);
	for ( i=0; i<fpst->rules[0].u.coverage.fcnt; ++i )
	    putshort(lfile,0);
	if ( fpst->format==pst_coverage ) {
	    putshort(lfile,lc);
	    for ( i=0; i<fpst->rules[0].lookup_cnt; ++i )
		if ( fpst->rules[0].lookups[i].lookup->lookup_index!=-1 ) {
		    putshort(lfile,fpst->rules[0].lookups[i].seq);
		    putshort(lfile,fpst->rules[0].lookups[i].lookup->lookup_index);
		}
	} else {		/* reverse coverage */
	    glyphs = SFGlyphsFromNames(sf,fpst->rules[0].u.rcoverage.replacements);
	    for ( i=0; glyphs[i]!=0; ++i );
	    putshort(lfile,i);
	    for ( i=0; glyphs[i]!=0; ++i )
		putshort(lfile,glyphs[i]->ttf_glyph);
	}
	for ( i=0; i<fpst->rules[0].u.coverage.ncnt; ++i ) {
	    uint32 pos = ftell(lfile);
	    fseek(lfile,ibase+2*i,SEEK_SET);
	    putshort(lfile,pos-base);
	    fseek(lfile,pos,SEEK_SET);
	    glyphs = OrderedGlyphsFromNames(sf,fpst->rules[0].u.coverage.ncovers[i]);
	    dumpcoveragetable(lfile,glyphs);
	    free(glyphs);
	}
	for ( i=0; i<fpst->rules[0].u.coverage.bcnt; ++i ) {
	    uint32 pos = ftell(lfile);
	    fseek(lfile,bbase+2*i,SEEK_SET);
	    putshort(lfile,pos-base);
	    fseek(lfile,pos,SEEK_SET);
	    glyphs = OrderedGlyphsFromNames(sf,fpst->rules[0].u.coverage.bcovers[i]);
	    dumpcoveragetable(lfile,glyphs);
	    free(glyphs);
	}
	for ( i=0; i<fpst->rules[0].u.coverage.fcnt; ++i ) {
	    uint32 pos = ftell(lfile);
	    fseek(lfile,lbase+2*i,SEEK_SET);
	    putshort(lfile,pos-base);
	    fseek(lfile,pos,SEEK_SET);
	    glyphs = OrderedGlyphsFromNames(sf,fpst->rules[0].u.coverage.fcovers[i]);
	    dumpcoveragetable(lfile,glyphs);
	    free(glyphs);
	}
    }

    curcontext = fpst->rules[0].u.coverage.ncnt+fpst->rules[0].u.coverage.bcnt+fpst->rules[0].u.coverage.fcnt;
    if ( curcontext>at->os2.maxContext )
	at->os2.maxContext = curcontext;
}

static void dumpg___ContextChain(FILE *lfile,SplineFont *sf,
	struct lookup_subtable *sub, struct alltabs *at) {
    FPST *fpst = sub->fpst;

    switch ( fpst->format ) {
      case pst_glyphs:
	dumpg___ContextChainGlyphs(lfile,sf,sub,at);
      break;
      case pst_class:
	dumpg___ContextChainClass(lfile,sf,sub,at);
      break;
      case pst_coverage:
      case pst_reversecoverage:
	dumpg___ContextChainCoverage(lfile,sf,sub,at);
      break;
    }

    fseek(lfile,0,SEEK_END);

}

static void AnchorsAway(FILE *lfile,SplineFont *sf,
	struct lookup_subtable *sub, struct glyphinfo *gi ) {
    SplineChar **base, **lig, **mkmk;
    AnchorClass *ac, *acfirst;
    SplineChar ***marks;
    int *subcnts;
    int cmax, classcnt;
    int i;

    marks = galloc((cmax=20)*sizeof(SplineChar **));
    subcnts = galloc(cmax*sizeof(int));

    classcnt = 0;
    acfirst = NULL;
    for ( ac=sf->anchor; ac!=NULL; ac = ac->next ) {
	ac->matches = false;
	if ( ac->subtable==sub && !ac->processed ) {
	    if ( acfirst == NULL )
		acfirst = ac;
	    if ( ac->type==act_curs )
    continue;
	    else if ( ac->has_mark && ac->has_base ) {
		ac->matches = ac->processed = true;
		++classcnt;
	    }
	}
    }
    if ( classcnt>cmax ) {
	marks = grealloc(marks,(cmax=classcnt+10)*sizeof(SplineChar **));
	subcnts = grealloc(subcnts,cmax*sizeof(int));
    }
    AnchorClassDecompose(sf,acfirst,classcnt,subcnts,marks,&base,&lig,&mkmk,gi);
    switch ( sub->lookup->lookup_type ) {
      case gpos_mark2base:
	if ( marks[0]!=NULL && base!=NULL )
	    dumpgposAnchorData(lfile,acfirst,at_basechar,marks,base,classcnt,gi);
      break;
      case gpos_mark2ligature:
	if ( marks[0]!=NULL && lig!=NULL )
	    dumpgposAnchorData(lfile,acfirst,at_baselig,marks,lig,classcnt,gi);
      break;
      case gpos_mark2mark:
		if ( marks[0]!=NULL && mkmk!=NULL )
		  dumpgposAnchorData(lfile,acfirst,at_basemark,marks,mkmk,classcnt,gi);
		break;
	default:
	  break;
    }
    for ( i=0; i<classcnt; ++i )
	free(marks[i]);
    free(base);
    free(lig);
    free(mkmk);
    free(marks);
    free(subcnts);
}

static int lookup_size_cmp(const void *_l1, const void *_l2) {
    const OTLookup *l1 = _l1, *l2 = _l2;
return( l1->lookup_length-l2->lookup_length );
}

static int FPSTRefersToOTL(FPST *fpst,OTLookup *otl) {
    int i, j;

    if ( fpst==NULL || fpst->type == pst_reversesub )
return( false );
    for ( i=0; i<fpst->rule_cnt; ++i ) {
	for ( j=0; j< fpst->rules[i].lookup_cnt; ++j )
	    if ( fpst->rules[i].lookups[j].lookup == otl )
return( true );
    }
return( false );
}

static int OnlyMac(OTLookup *otl, OTLookup *all) {
    FeatureScriptLangList *features = otl->features;
    int anymac = 0;
    struct lookup_subtable *sub;

    switch ( otl->lookup_type ) {
    /* These two lookup types are mac only */
      case kern_statemachine: case morx_indic: case morx_context: case morx_insert:
return( true );
    /* These lookup types are OpenType only */
      case gsub_multiple: case gsub_alternate: case gsub_context:
      case gsub_contextchain: case gsub_reversecchain:
      case gpos_single: case gpos_cursive: case gpos_mark2base:
      case gpos_mark2ligature: case gpos_mark2mark:
      case gpos_context: case gpos_contextchain:
return( false );
    /* These two can be expressed in both, and might be either */
     case gpos_pair: case gsub_single: case gsub_ligature:
	for ( features = otl->features; features!=NULL; features = features->next ) {
	    if ( !features->ismac )
return( false );
	    else
		anymac = true;
	}
	/* Either it has no features at all (nested), or all its features */
	/*  are mac feature settings. Even if all are mac feature settings it */
	/*  might still be used as under control of a contextual feature */
	/*  so in both cases check for nested */
	while ( all!=NULL ) {
	    if ( all!=otl && !all->unused &&
		    (all->lookup_type==gpos_context ||
		     all->lookup_type==gpos_contextchain ||
		     all->lookup_type==gsub_context ||
		     all->lookup_type==gsub_contextchain /*||
		     all->lookup_type==gsub_reversecchain*/ )) {
		for ( sub=all->subtables; sub!=NULL; sub=sub->next ) if ( !sub->unused && sub->fpst!=NULL ) {
		    if ( FPSTRefersToOTL(sub->fpst,otl) )
return( false );
		}
	    }
	    all = all->next;
	default:
	  break;
	}
	if ( anymac )
return( true );
	/* As far as I can tell, this lookup isn't used at all */
	/*  Let's output it anyway, just in case we ever support some other */
	/*  table that uses GPOS/GSUB lookups (I think JUST) */
return( false );
    }
    /* Should never get here, but gcc probably thinks we might */
return( true );		    
}

static FILE *G___figureLookups(SplineFont *sf,int is_gpos,
	struct alltabs *at) {
    OTLookup *otl;
    struct lookup_subtable *sub;
    int index, i;
    FILE *final;
    FILE *lfile = tmpfile();
    OTLookup **sizeordered;
    OTLookup *all = is_gpos ? sf->gpos_lookups : sf->gsub_lookups;
    char *buffer;
    int len;

    index = 0;
    for ( otl=all; otl!=NULL; otl=otl->next ) {
	if ( otl->unused || OnlyMac(otl,all))
	    otl->lookup_index = -1;
	else
	    otl->lookup_index = index++;
    }
    for ( otl=all; otl!=NULL; otl=otl->next ) {
	if ( otl->lookup_index!=-1 ) {
	    otl->lookup_offset = ftell(lfile);
	    for ( sub = otl->subtables; sub!=NULL; sub=sub->next ) {
		sub->extra_subtables = NULL;
		if ( sub->unused )
		    sub->subtable_offset = -1;
		else {
		    sub->subtable_offset = ftell(lfile);
		    switch ( otl->lookup_type ) {
		    /* GPOS lookup types */
		      case gpos_single:
			dumpGPOSsimplepos(lfile,sf,sub);
		      break;

		      case gpos_pair:
			if ( at->os2.maxContext<2 )
			    at->os2.maxContext = 2;
			if ( sub->kc!=NULL )
			    dumpgposkernclass(lfile,sf,sub,at);
			else
			    dumpGPOSpairpos(lfile,sf,sub);
		      break;

		      case gpos_cursive:
			dumpgposCursiveAttach(lfile,sf,sub,&at->gi);
		      break;

		      case gpos_mark2base:
		      case gpos_mark2ligature:
		      case gpos_mark2mark:
			AnchorsAway(lfile,sf,sub,&at->gi);
		      break;

		      case gpos_contextchain:
		      case gpos_context:
			dumpg___ContextChain(lfile,sf,sub,at);
		      break;

		    /* GSUB lookup types */
		      case gsub_single:
			dumpGSUBsimplesubs(lfile,sf,sub);
		      break;

		      case gsub_multiple:
		      case gsub_alternate:
			dumpGSUBmultiplesubs(lfile,sf,sub);
		      break;

		      case gsub_ligature:
			dumpGSUBligdata(lfile,sf,sub,at);
		      break;

		      case gsub_contextchain:
		      case gsub_context:
		      case gsub_reversecchain:
			dumpg___ContextChain(lfile,sf,sub,at);
		      break;
			default:
			  break;
		    }
		    if ( ftell(lfile)-sub->subtable_offset==0 ) {
			IError( "Lookup sub table, %s in %s, contains no data.\n",
				sub->subtable_name, sub->lookup->lookup_name );
			sub->unused = true;
			sub->subtable_offset = -1;
		    } else if ( sub->extra_subtables==NULL &&
			    ftell(lfile)-sub->subtable_offset>65535 )
			IError( "Lookup sub table, %s in %s, is too big. Will not be useable.\n",
				sub->subtable_name, sub->lookup->lookup_name );
		}
	    }
	    otl->lookup_length = ftell(lfile)-otl->lookup_offset;
	}
    }
    if ( is_gpos )
	AnchorGuessContext(sf,at);

    /* We don't need to reorder short files */
    if ( ftell(lfile)<65536 )
return( lfile );

    /* Order the lookups so that the smallest ones come first */
    /*  thus we are less likely to need extension tables */
    /* I think it's better to order the entire lookup rather than ordering the*/
    /*  subtables -- since the extension subtable would be required for all */
    /*  subtables in the lookup, so might as well keep them all together */
    sizeordered = galloc(index*sizeof(OTLookup *));
    for ( otl=is_gpos ? sf->gpos_lookups : sf->gsub_lookups; otl!=NULL; otl=otl->next )
	if ( otl->lookup_index!=-1 )
	    sizeordered[ otl->lookup_index ] = otl;
    qsort(sizeordered,index,sizeof(OTLookup *),lookup_size_cmp);

    final = tmpfile();
    buffer = galloc(32768);
    for ( i=0; i<index; ++i ) {
	otl = sizeordered[i];
	fseek(lfile,otl->lookup_offset,SEEK_SET);
	otl->lookup_offset = ftell(final);
	len = otl->lookup_length;
	while ( len>=32768 ) {
	    int done = fread(buffer,1,32768,lfile);
	    if ( done==EOF )
	break;
	    fwrite(buffer,1,done,final);
	    len -= done;
	}
	if ( len>0 && len<=32768 ) {
	    int done = fread(buffer,1,len,lfile);
	    if ( done==EOF )
	break;
	    fwrite(buffer,1,done,final);
	}
    }
    free(buffer);
    free(sizeordered);
    fclose(lfile);
return( final );
}

struct ginfo {
    int fmax, fcnt;
    struct feat_lookups {
	uint32 tag;
	int lcnt;
	OTLookup **lookups;
	int feature_id;		/* Initially consecutive, but may be rearranged by sorting */
    } *feat_lookups;
    int sc;
    struct scriptset {
	uint32 script;
	int lc;
	struct langsys {
	    uint32 lang;
	    int fc;
	    int *feature_id;
	    int same_as;
	    int32 offset;
	} *langsys;
    } *scripts;
};

static int FindOrMakeNewFeatureLookup(struct ginfo *ginfo,OTLookup **lookups,
	uint32 tag) {
    int i, j;

    for ( i=0; i<ginfo->fcnt; ++i ) {
	if ( ginfo->feat_lookups[i].tag!= tag )
    continue;
	if ( lookups==NULL && ginfo->feat_lookups[i].lookups==NULL )	/* 'size' feature */
return( i );
	if ( lookups==NULL || ginfo->feat_lookups[i].lookups==NULL )
    continue;
	for ( j=0; lookups[j]!=NULL && ginfo->feat_lookups[i].lookups[j]!=NULL; ++j )
	    if ( ginfo->feat_lookups[i].lookups[j]!=lookups[j] )
	break;
	if ( ginfo->feat_lookups[i].lookups[j]==lookups[j] ) {
	    free(lookups);
return( i );
	}
    }
    if ( ginfo->fcnt>=ginfo->fmax )
	ginfo->feat_lookups = grealloc(ginfo->feat_lookups,(ginfo->fmax+=20)*sizeof(struct feat_lookups));
    ginfo->feat_lookups[i].feature_id = i;
    ginfo->feat_lookups[i].tag = tag;
    ginfo->feat_lookups[i].lookups = lookups;
    j=0;
    if ( lookups!=NULL ) for ( ; lookups[j]!=NULL; ++j );
    ginfo->feat_lookups[i].lcnt = j;
    ++ginfo->fcnt;
return( i );
}

static int feat_alphabetize(const void *_fl1, const void *_fl2) {
    const struct feat_lookups *fl1 = _fl1, *fl2 = _fl2;

    if ( fl1->tag<fl2->tag )
return( -1 );
    if ( fl1->tag>fl2->tag )
return( 1 );

return( 0 );
}

static int numeric_order(const void *_i1, const void *_i2) {
    int i1 = *(const int *) _i1, i2 = *(const int *) _i2;

    if ( i1<i2 )
return( -1 );
    if ( i1>i2 )
return( 1 );

return( 0 );
}

static int LangSysMatch(struct scriptset *s,int ils1, int ils2 ) {
    struct langsys *ls1 = &s->langsys[ils1], *ls2 = &s->langsys[ils2];
    int i;

    if ( ls1->fc!=ls2->fc )
return( false );
    for ( i=0; i<ls1->fc; ++i )
	if ( ls1->feature_id[i]!=ls2->feature_id[i] )
return( false );

return( true );
}

static void FindFeatures(SplineFont *sf,int is_gpos,struct ginfo *ginfo) {
    uint32 *scripts, *langs, *features;
    OTLookup **lookups;
    int sc, lc, fc, j;

    memset(ginfo,0,sizeof(struct ginfo));

    scripts = SFScriptsInLookups(sf,is_gpos);
    if ( scripts==NULL )	/* All lookups unused */
return;
    for ( sc=0; scripts[sc]!=0; ++sc );
    ginfo->scripts = galloc(sc*sizeof(struct scriptset));
    ginfo->sc = sc;
    for ( sc=0; scripts[sc]!=0; ++sc ) {
	langs = SFLangsInScript(sf,is_gpos,scripts[sc]);
	for ( lc=0; langs[lc]!=0; ++lc );
	ginfo->scripts[sc].script = scripts[sc];
	ginfo->scripts[sc].lc = lc;
	ginfo->scripts[sc].langsys = galloc(lc*sizeof(struct langsys));
	for ( lc=0; langs[lc]!=0; ++lc ) {
	    features = SFFeaturesInScriptLang(sf,is_gpos,scripts[sc],langs[lc]);
	    for ( fc=0; features[fc]!=0; ++fc );
	    ginfo->scripts[sc].langsys[lc].lang = langs[lc];
	    ginfo->scripts[sc].langsys[lc].fc = fc;
	    ginfo->scripts[sc].langsys[lc].feature_id = galloc(fc*sizeof(int));
	    ginfo->scripts[sc].langsys[lc].same_as = -1;
	    for ( fc=0; features[fc]!=0; ++fc ) {
		lookups = SFLookupsInScriptLangFeature(sf,is_gpos,scripts[sc],langs[lc],features[fc]);
		ginfo->scripts[sc].langsys[lc].feature_id[fc] =
			FindOrMakeNewFeatureLookup(ginfo,lookups,features[fc]);
		/* lookups is freed or used by FindOrMakeNewFeatureLookup */
	    }
	    free(features);
	}
	free(langs);
    }
    free(scripts);

    qsort(ginfo->feat_lookups,ginfo->fcnt,sizeof(struct feat_lookups),feat_alphabetize);

    /* Now we've disordered the features. Find each feature_id and turn it back*/
    /*  into a feature number */
    for ( sc=0; sc<ginfo->sc; ++sc ) {
	for ( lc=0; lc<ginfo->scripts[sc].lc; ++lc ) {
	    int fcmax = ginfo->scripts[sc].langsys[lc].fc;
	    int *feature_id = ginfo->scripts[sc].langsys[lc].feature_id;
	    for ( fc=0; fc<fcmax; ++fc ) {
		int id = feature_id[fc];
		for ( j=0; j<ginfo->fcnt; ++j )
		    if ( id==ginfo->feat_lookups[j].feature_id )
		break;
		feature_id[fc] = j;
	    }
	    qsort(feature_id,fcmax,sizeof(int),numeric_order);
	}
	/* See if there are langsys tables which use exactly the same features*/
	/*  They can use the same entry in the file. This optimization seems  */
	/*  to be required for Japanese vertical writing to work in Uniscribe.*/
	for ( lc=0; lc<ginfo->scripts[sc].lc; ++lc ) {
	    for ( j=0; j<lc; ++j )
		if ( LangSysMatch(&ginfo->scripts[sc],j,lc) ) {
		    ginfo->scripts[sc].langsys[lc].same_as = j;
	    break;
		}
	}
    }
}
		

static void dump_script_table(FILE *g___,struct scriptset *ss,struct ginfo *ginfo) {
    int i, lcnt, dflt_lang = -1;
    uint32 base;
    int j, req_index;
    uint32 offset;

    /* Count the languages, and find default */
    for ( lcnt=0; lcnt<ss->lc; ++lcnt )
	if ( ss->langsys[lcnt].lang==DEFAULT_LANG )
	    dflt_lang = lcnt;
    if ( dflt_lang != -1 )
	--lcnt;

    base = ftell(g___);
    putshort(g___, 0 );				/* fill in later, Default Lang Sys */
    putshort(g___,lcnt);
    for ( i=0; i<ss->lc; ++i ) if ( i!=dflt_lang ) {
	putlong(g___,ss->langsys[i].lang);	/* Language tag */
	putshort(g___,0);			/* Fill in later, offset to langsys */
    }

    for ( lcnt=0; lcnt<ss->lc; ++lcnt ) {
	if ( ss->langsys[lcnt].same_as!=-1 )
	    offset = ss->langsys[ ss->langsys[lcnt].same_as ].offset;
	else {
	    offset = ftell(g___);
	    ss->langsys[lcnt].offset = offset;
	}
	fseek(g___,lcnt==dflt_lang ? base :
		    lcnt<dflt_lang ? base + 4 + lcnt*6 +4 :
			base + 4 + (lcnt-1)*6 +4 , SEEK_SET );
	putshort(g___,offset-base);
	fseek(g___,0,SEEK_END);
	if ( ss->langsys[lcnt].same_as==-1 ) {
	    req_index = -1;
	    for ( j=0; j<ss->langsys[lcnt].fc; ++j ) {
		if ( ginfo->feat_lookups[ ss->langsys[lcnt].feature_id[j] ].tag == REQUIRED_FEATURE ) {
		    req_index = ss->langsys[lcnt].feature_id[j];
	    break;
		}
	    }
	    putshort(g___,0);		/* LookupOrder, always NULL */
	    putshort(g___,req_index);	/* index of required feature, if any */
	    putshort(g___,ss->langsys[lcnt].fc - (req_index!=-1));
					/* count of non-required features */
	    for ( j=0; j<ss->langsys[lcnt].fc; ++j ) if ( j!=req_index )
		putshort(g___,ss->langsys[lcnt].feature_id[j]);
	}
    }
}
    
static FILE *g___FigureExtensionSubTables(OTLookup *all,int startoffset,int is_gpos) {
    OTLookup *otf;
    struct lookup_subtable *sub;
    int len, len2, gotmore;
    FILE *efile;
    int i, offset, cnt;
    int any= false;

    if ( all==NULL )
return( NULL );
    gotmore = true; cnt=len=0;
    while ( gotmore ) {
	gotmore = false;
	offset = startoffset + 8*cnt;
	for ( otf=all; otf!=NULL; otf=otf->next ) if ( otf->lookup_index!=-1 ) {
	    if ( otf->needs_extension )
	continue;
	    for ( sub = otf->subtables; sub!=NULL; sub=sub->next ) {
		if ( sub->subtable_offset==-1 )
	    continue;
		if ( sub->extra_subtables!=NULL ) {
		    for ( i=0; sub->extra_subtables[i]!=-1; ++i ) {
			if ( sub->extra_subtables[i]+offset>65535 )
		    break;
		    }
		    if ( sub->extra_subtables[i]!=-1 )
	    break;
		} else if ( sub->subtable_offset+offset>65535 )
	    break;
	    }
	    if ( sub!=NULL ) {
		if ( !any ) {
		    ff_post_notice(_("Lookup potentially too big"),
			    _("Lookup %s has an\noffset bigger than 65535 bytes. This means\nFontForge must use an extension lookup to output it.\nNot all applications support extension lookups."),
			    otf->lookup_name );
		    any = true;
		}
		otf->needs_extension = true;
		gotmore = true;
		len += 8*otf->subcnt;
		++cnt;
	    }
	    offset -= 6+2*otf->subcnt;
	}
    }

    if ( cnt==0 )		/* No offset overflows */
return( NULL );

    /* Now we've worked out which lookups need extension tables and marked them*/
    /* Generate the extension tables, and update the offsets to reflect the size */
    /* of the extensions */
    efile = tmpfile();

    len2 = 0;
    for ( otf=all; otf!=NULL; otf=otf->next ) if ( otf->lookup_index!=-1 ) {
	if ( otf->needs_extension ) {
	    putshort(efile,1);	/* exten subtable format (there's only one) */
	    putshort(efile,otf->lookup_type&0xff);
	}
	for ( sub = otf->subtables; sub!=NULL; sub=sub->next ) {
	    if ( sub->subtable_offset==-1 )
	continue;
	    if ( sub->extra_subtables!=NULL ) {
		for ( i=0; sub->extra_subtables[i]!=-1; ++i ) {
		    sub->extra_subtables[i] += len;
		    if ( otf->needs_extension ) {
			int off = ftell(efile);
			putshort(efile,1);	/* exten subtable format (there's only one) */
			putshort(efile,otf->lookup_type&0xff);
			putlong(efile,sub->extra_subtables[i]-len2);
			sub->extra_subtables[i] = off;
			len2+=8;
		    }
		}
	    } else {
		sub->subtable_offset += len;
		if ( otf->needs_extension ) {
		    int off = ftell(efile);
		    putshort(efile,1);	/* exten subtable format (there's only one) */
		    putshort(efile,otf->lookup_type&0xff);
		    putlong(efile,sub->subtable_offset-len2);
		    sub->subtable_offset = off;
			len2+=8;
		}
	    }
	}
    }

return( efile );
}

static FILE *dumpg___info(struct alltabs *at, SplineFont *sf,int is_gpos) {
    /* Dump out either a gpos or a gsub table. gpos handles kerns, gsub ligs */
    /*  we assume that SFFindUnusedLookups has been called */
    FILE *lfile, *g___, *efile;
    uint32 lookup_list_table_start, feature_list_table_start, here, scripts_start_offset;
    struct ginfo ginfo;
    int32 size_params_loc, size_params_ptr;
    int i,j, cnt, scnt, offset;
    OTLookup *otf, *all;
    struct lookup_subtable *sub;
    char *buf;

    FindFeatures(sf,is_gpos,&ginfo);
    if ( ginfo.sc==0 )
return( NULL );
    lfile = G___figureLookups(sf,is_gpos,at);

    if ( ginfo.sc==0 && ftell(lfile)==0 ) {
	/* ftell(lfile)==0 => There are no lookups for this table */
	/* ginfo.sc==0 => There are no scripts. */
	/* If both are true then we don't need to output the table */
	/* It is perfectly possible to have lookups without scripts */
	/*  (if some other table refered to them -- we don't currently */
	/*  support this, but we might some day). */
	/* It is also possible to have scripts without lookups (to get */
	/*  around a bug in Uniscribe which only processes some scripts */
	/*  if both GPOS and GSUB entries are present. So we share scripts */
	/*  between the two tables */
	fclose(lfile);
	/* if ginfo.sc==0 then there will be nothing to free in the ginfo struct*/
return( NULL );
    }

    g___ = tmpfile();

    putlong(g___,0x10000);		/* version number */
    putshort(g___,10);		/* offset to script table */
    putshort(g___,0);		/* offset to features. Come back for this */
    putshort(g___,0);		/* offset to lookups.  Come back for this */
/* Now the scripts */
    scripts_start_offset = ftell(g___);
    putshort(g___,ginfo.sc);
    for ( i=0; i<ginfo.sc; ++i ) {
	putlong(g___,ginfo.scripts[i].script);
	putshort(g___,0);	/* fix up later */
    }

    /* Ok, that was the script_list_table which gives each script an offset */
    /* Now for each script we provide a Script table which contains an */
    /*  offset to a bunch of features for the default language, and a */
    /*  a more complex situation for non-default languages. */
    offset=2+4;			/* To the script pointer at the start of table */
    for ( i=0; i<ginfo.sc; ++i ) {
	here = ftell(g___);
	fseek(g___,scripts_start_offset+offset,SEEK_SET);
	putshort(g___,here-scripts_start_offset);
	offset+=6;
	fseek(g___,here,SEEK_SET);
	dump_script_table(g___,&ginfo.scripts[i],&ginfo);
    }
    /* And that should finish all the scripts/languages */

    /* so free the ginfo script/lang data */
    for ( i=0; i<ginfo.sc; ++i ) {
	for ( j=0; j<ginfo.scripts[i].lc; ++j ) {
	    free( ginfo.scripts[i].langsys[j].feature_id );
	}
	free( ginfo.scripts[i].langsys );
    }
    free( ginfo.scripts );

/* Now the features */
    feature_list_table_start = ftell(g___);
    fseek(g___,6,SEEK_SET);
    putshort(g___,feature_list_table_start);
    fseek(g___,0,SEEK_END);
    putshort(g___,ginfo.fcnt);	/* Number of features */
    offset = 2+6*ginfo.fcnt;	/* Offset to start of first feature table from beginning of feature_list */
    for ( i=0; i<ginfo.fcnt; ++i ) {
	putlong(g___,ginfo.feat_lookups[i].tag);
	putshort(g___,offset);
	offset += 4+2*ginfo.feat_lookups[i].lcnt;
    }
    /* for each feature, one feature table */
    size_params_ptr = 0;
    for ( i=0; i<ginfo.fcnt; ++i ) {
	if ( ginfo.feat_lookups[i].tag==CHR('s','i','z','e') )
	    size_params_ptr = ftell(g___);
	putshort(g___,0);			/* No feature params (we'll come back for 'size') */
	putshort(g___,ginfo.feat_lookups[i].lcnt);/* this many lookups */
	for ( j=0; j<ginfo.feat_lookups[i].lcnt; ++j )
	    putshort(g___,ginfo.feat_lookups[i].lookups[j]->lookup_index );
	    	/* index of each lookup */
    }
    if ( size_params_ptr!=0 ) {
	size_params_loc = ftell(g___);
	fseek(g___,size_params_ptr,SEEK_SET);
	/* When Adobe first released fonts containing the 'size' feature */
	/*  they did not follow the spec, and the offset to the size parameters */
	/*  was relative to the wrong location. They claim (Aug 2006) that */
	/*  this has been fixed. FF used to do what Adobe did. Many programs */
	/*  expect broken sizes tables now. Therefore allow the user to chose */
	/*  which kind to output */
#if 0
	putshort(g___,size_params_loc-((at->gi.flags&ttf_flag_brokensize)?feature_list_table_start:size_params_ptr));
#else
	putshort(g___,size_params_loc-size_params_ptr);
#endif
	fseek(g___,size_params_loc,SEEK_SET);
	putshort(g___,sf->design_size);
	if ( sf->fontstyle_id!=0 || sf->fontstyle_name!=NULL ||
		sf->design_range_bottom!=0 || sf->design_range_top!=0 ) {
	    putshort(g___,sf->fontstyle_id);
	    at->fontstyle_name_strid = at->next_strid++;
	    putshort(g___,at->fontstyle_name_strid);
	    putshort(g___,sf->design_range_bottom);
	    putshort(g___,sf->design_range_top);
	} else {
	    putshort(g___,0);
	    putshort(g___,0);
	    putshort(g___,0);
	    putshort(g___,0);
	}
    }
    /* And that should finish all the features */

    /* so free the ginfo feature data */
    for ( i=0; i<ginfo.fcnt; ++i )
	free( ginfo.feat_lookups[i].lookups );
    free( ginfo.feat_lookups );

/* Now the lookups */
    all = is_gpos ? sf->gpos_lookups : sf->gsub_lookups;
    for ( cnt=0, otf = all; otf!=NULL; otf=otf->next ) {
	if ( otf->lookup_index!=-1 )
	    ++cnt;
    }
    lookup_list_table_start = ftell(g___);
    fseek(g___,8,SEEK_SET);
    putshort(g___,lookup_list_table_start);
    fseek(g___,0,SEEK_END);
    putshort(g___,cnt);
    offset = 2+2*cnt;		/* Offset to start of first lookup table from beginning of lookup list */
    for ( otf = all; otf!=NULL; otf=otf->next ) if ( otf->lookup_index!=-1 ) {
	putshort(g___,offset);
	for ( scnt=0, sub = otf->subtables; sub!=NULL; sub=sub->next ) {
	    if ( sub->subtable_offset==-1 )
	continue;
	    else if ( sub->extra_subtables!=NULL ) {
		for ( i=0; sub->extra_subtables[i]!=-1; ++i )
		    ++scnt;
	    } else
		++scnt;
	}
	otf->subcnt = scnt;
	offset += 6+2*scnt;		/* 6 bytes header +2 per lookup */
    }
    offset -= 2+2*cnt;
    /* now the lookup tables */
      /* do we need any extension sub-tables? */
    efile=g___FigureExtensionSubTables(all,offset,is_gpos);
    for ( otf = all; otf!=NULL; otf=otf->next ) if ( otf->lookup_index!=-1 ) {
	putshort(g___,!otf->needs_extension ? (otf->lookup_type&0xff)
			: is_gpos ? 9 : 7);
	putshort(g___,otf->lookup_flags);
	putshort(g___,otf->subcnt);
	for ( sub = otf->subtables; sub!=NULL; sub=sub->next ) {
	    if ( sub->subtable_offset==-1 )
	continue;
	    else if ( sub->extra_subtables!=NULL ) {
		for ( i=0; sub->extra_subtables[i]!=-1; ++i )
		    putshort(g___,offset+sub->extra_subtables[i]);
	    } else
		putshort(g___,offset+sub->subtable_offset);
	    
	    /* Offset to lookup data which is in the temp file */
	    /* we keep adjusting offset so it reflects the distance between */
	    /* here and the place where the temp file will start, and then */
	    /* we need to skip l->offset bytes in the temp file */
	    /* If it's a big GPOS/SUB table we may also need some extension */
	    /*  pointers, but FigureExtension will adjust for that */
	}
	offset -= 6+2*otf->subcnt;
    }

    buf = galloc(8096);
    if ( efile!=NULL ) {
	rewind(efile);
	while ( (i=fread(buf,1,8096,efile))>0 )
	    fwrite(buf,1,i,g___);
	fclose(efile);
    }
    rewind(lfile);
    while ( (i=fread(buf,1,8096,lfile))>0 )
	fwrite(buf,1,i,g___);
    fclose(lfile);
    free(buf);
    for ( otf = all; otf!=NULL; otf=otf->next ) if ( otf->lookup_index!=-1 ) {
	for ( sub = otf->subtables; sub!=NULL; sub=sub->next ) {
	    free(sub->extra_subtables);
	    sub->extra_subtables = NULL;
	}
    }
return( g___ );
}

void otf_dumpgpos(struct alltabs *at, SplineFont *sf) {
    /* Open Type, bless its annoying little heart, doesn't store kern info */
    /*  in the kern table. Of course not, how silly of me to think it might */
    /*  be consistent. It stores it in the much more complicated gpos table */
    AnchorClass *ac;

    for ( ac=sf->anchor; ac!=NULL; ac=ac->next )
	ac->processed = false;

    at->gpos = dumpg___info(at, sf,true);
    if ( at->gpos!=NULL ) {
	at->gposlen = ftell(at->gpos);
	if ( at->gposlen&1 ) putc('\0',at->gpos);
	if ( (at->gposlen+1)&2 ) putshort(at->gpos,0);
    }
}

void otf_dumpgsub(struct alltabs *at, SplineFont *sf) {
    /* substitutions such as: Ligatures, cjk vertical rotation replacement, */
    /*  arabic forms, small caps, ... */
    SFLigaturePrepare(sf);
    at->gsub = dumpg___info(at, sf, false);
    if ( at->gsub!=NULL ) {
	at->gsublen = ftell(at->gsub);
	if ( at->gsublen&1 ) putc('\0',at->gsub);
	if ( (at->gsublen+1)&2 ) putshort(at->gsub,0);
    }
    SFLigatureCleanup(sf);
}

static int LigCaretCnt(SplineChar *sc) {
    PST *pst;

    for ( pst=sc->possub; pst!=NULL; pst=pst->next ) {
	if ( pst->type == pst_lcaret )
return( pst->u.lcaret.cnt );
    }
return( 0 );
}
    
static void DumpLigCarets(FILE *gdef,SplineChar *sc) {
    PST *pst;
    int i, j, offset;

    for ( pst=sc->possub; pst!=NULL; pst=pst->next ) {
	if ( pst->type == pst_lcaret )
    break;
    }
    if ( pst==NULL )
return;

    if ( SCRightToLeft(sc) ) {
	for ( i=0; i<pst->u.lcaret.cnt-1; ++i )
	    for ( j=i+1; j<pst->u.lcaret.cnt; ++j )
		if ( pst->u.lcaret.carets[i]<pst->u.lcaret.carets[j] ) {
		    int16 temp = pst->u.lcaret.carets[i];
		    pst->u.lcaret.carets[i] = pst->u.lcaret.carets[j];
		    pst->u.lcaret.carets[j] = temp;
		}
    } else {
	for ( i=0; i<pst->u.lcaret.cnt-1; ++i )
	    for ( j=i+1; j<pst->u.lcaret.cnt; ++j )
		if ( pst->u.lcaret.carets[i]>pst->u.lcaret.carets[j] ) {
		    int16 temp = pst->u.lcaret.carets[i];
		    pst->u.lcaret.carets[i] = pst->u.lcaret.carets[j];
		    pst->u.lcaret.carets[j] = temp;
		}
    }

    putshort(gdef,pst->u.lcaret.cnt);	/* this many carets */
    offset = sizeof(uint16) + sizeof(uint16)*pst->u.lcaret.cnt;
    for ( i=0; i<pst->u.lcaret.cnt; ++i ) {
	putshort(gdef,offset);
	offset+=4;
    }
    for ( i=0; i<pst->u.lcaret.cnt; ++i ) {
	putshort(gdef,1);		/* Format 1 */
	putshort(gdef,pst->u.lcaret.carets[i]);
    }
}

int gdefclass(SplineChar *sc) {
    PST *pst;
    AnchorPoint *ap;

    if ( sc->glyph_class!=0 )
return( sc->glyph_class-1 );

    if ( strcmp(sc->name,".notdef")==0 )
return( 0 );

    for ( pst=sc->possub; pst!=NULL; pst=pst->next ) {
	if ( pst->type == pst_ligature )
return( 2 );			/* Ligature */
    }

    ap=sc->anchor;
    while ( ap!=NULL && (ap->type==at_centry || ap->type==at_cexit) )
	ap = ap->next;
    if ( ap!=NULL && (ap->type==at_mark || ap->type==at_basemark) )
return( 3 );
    else
return( 1 );
    /* I not quite sure what a componant glyph is. Probably something that */
    /*  is not in the cmap table and is referenced in other glyphs */
    /* Anyway I never return class 4 */ /* (I've never seen it used by others) */
}

void otf_dumpgdef(struct alltabs *at, SplineFont *sf) {
    /* In spite of what the open type docs say, this table does appear to be */
    /*  required (at least the glyph class def table) if we do mark to base */
    /*  positioning */
    /* I was wondering at the apperant contradiction: something can be both a */
    /*  base glyph and a ligature component, but it appears that the component*/
    /*  class is unused and everything is a base unless it is a ligature or */
    /*  mark */
    /* All my example fonts ignore the attachment list subtable and the mark */
    /*  attach class def subtable, so I shall too */
    /* Ah. Some indic fonts need the mark attach class subtable for greater */
    /*  control of lookup flags */
    /* All my example fonts contain a ligature caret list subtable, which is */
    /*  empty. Odd, but perhaps important */
    PST *pst;
    int i,j,k, lcnt, needsclass;
    int pos, offset;
    int cnt, start, last, lastval;
    SplineChar **glyphs, *sc;

    /* Don't look in the cidmaster if we are only dumping one subfont */
    if ( sf->cidmaster && sf->cidmaster->glyphs!=NULL ) sf = sf->cidmaster;
    else if ( sf->mm!=NULL ) sf=sf->mm->normal;

    glyphs = NULL;
    for ( k=0; k<2; ++k ) {
	lcnt = 0;
	needsclass = false;
	for ( i=0; i<at->gi.gcnt; ++i ) if ( at->gi.bygid[i]!=-1 ) {
	    SplineChar *sc = sf->glyphs[at->gi.bygid[i]];
	    if ( sc->glyph_class!=0 || gdefclass(sc)!=1 )
		needsclass = true;
	    for ( pst=sc->possub; pst!=NULL; pst=pst->next ) {
		if ( pst->type == pst_lcaret ) {
		    for ( j=pst->u.lcaret.cnt-1; j>=0; --j )
			if ( pst->u.lcaret.carets[j]!=0 )
		    break;
		    if ( j!=-1 )
	    break;
		}
	    }
	    if ( pst!=NULL ) {
		if ( glyphs!=NULL ) glyphs[lcnt] = sc;
		++lcnt;
	    }
	}
	if ( lcnt==0 )
    break;
	if ( glyphs!=NULL )
    break;
	glyphs = galloc((lcnt+1)*sizeof(SplineChar *));
	glyphs[lcnt] = NULL;
    }
    if ( !needsclass && lcnt==0 && sf->mark_class_cnt==0 )
return;					/* No anchor positioning, no ligature carets */

    at->gdef = tmpfile();
    putlong(at->gdef,0x00010000);		/* Version */
    putshort(at->gdef, needsclass ? 12 : 0 );	/* glyph class defn table */
    putshort(at->gdef, 0 );			/* attachment list table */
    putshort(at->gdef, 0 );			/* ligature caret table (come back and fix up later) */
    putshort(at->gdef, 0 );			/* mark attachment class table */

	/* Glyph class subtable */
    if ( needsclass ) {
	/* Mark shouldn't conflict with anything */
	/* Ligature is more important than Base */
	/* Component is not used */
#if 1		/* ttx can't seem to support class format type 1 so let's output type 2 */
	for ( j=0; j<2; ++j ) {
	    cnt = 0;
	    for ( i=0; i<at->gi.gcnt; ++i ) if ( at->gi.bygid[i]!=-1 ) {
		sc = sf->glyphs[at->gi.bygid[i]];
		if ( sc!=NULL && sc->ttf_glyph!=-1 ) {
		    lastval = gdefclass(sc);
		    start = last = i;
		    for ( ; i<at->gi.gcnt; ++i ) if ( at->gi.bygid[i]!=-1 ) {
			sc = sf->glyphs[at->gi.bygid[i]];
			if ( gdefclass(sc)!=lastval )
		    break;
			last = i;
		    }
		    --i;
		    if ( lastval!=0 ) {
			if ( j==1 ) {
			    putshort(at->gdef,start);
			    putshort(at->gdef,last);
			    putshort(at->gdef,lastval);
			}
			++cnt;
		    }
		}
	    }
	    if ( j==0 ) {
		putshort(at->gdef,2);	/* class format 2, range list by class */
		putshort(at->gdef,cnt);
	    }
	}
#else
	putshort(at->gdef,1);	/* class format 1 complete list of glyphs */
	putshort(at->gdef,0);	/* First glyph */
	putshort(at->gdef,at->maxp.numGlyphs );
	j=0;
	for ( i=0; i<at->gi.gcnt; ++i ) if ( at->gi.bygid[i]!=-1 ) {
	    for ( ; j<i; ++j )
		putshort(at->gdef,1);	/* Any hidden characters (like notdef) default to base */
	    putshort(at->gdef,gdefclass(sf->glyphs[at->gi.bygid[i]]));
	    ++j;
	}
#endif
    }

	/* Ligature caret subtable. Always include this if we have a GDEF */
    pos = ftell(at->gdef);
    fseek(at->gdef,8,SEEK_SET);			/* location of lig caret table offset */
    putshort(at->gdef,pos);
    fseek(at->gdef,0,SEEK_END);
    if ( lcnt==0 ) {
	/* It always seems to be present, even if empty */
	putshort(at->gdef,4);			/* Offset to (empty) coverage table */
	putshort(at->gdef,0);			/* no ligatures */
	putshort(at->gdef,2);			/* coverage table format 2 */
	putshort(at->gdef,0);			/* no ranges in coverage table */
    } else {
	pos = ftell(at->gdef);	/* coverage location */
	putshort(at->gdef,0);			/* Offset to coverage table (fix up later) */
	putshort(at->gdef,lcnt);
	offset = 2*lcnt+4;
	for ( i=0; i<lcnt; ++i ) {
	    putshort(at->gdef,offset);
	    offset+=2+6*LigCaretCnt(glyphs[i]);
	}
	for ( i=0; i<lcnt; ++i )
	    DumpLigCarets(at->gdef,glyphs[i]);
	offset = ftell(at->gdef);
	fseek(at->gdef,pos,SEEK_SET);
	putshort(at->gdef,offset-pos);
	fseek(at->gdef,0,SEEK_END);
	dumpcoveragetable(at->gdef,glyphs);
    }

	/* Mark Attachment Class Subtable */
    if ( sf->mark_class_cnt>0 ) {
	uint16 *mclasses = ClassesFromNames(sf,sf->mark_classes,sf->mark_class_cnt,at->maxp.numGlyphs,NULL,false);
	pos = ftell(at->gdef);
	fseek(at->gdef,10,SEEK_SET);		/* location of mark attach table offset */
	putshort(at->gdef,pos);
	fseek(at->gdef,0,SEEK_END);
	DumpClass(at->gdef,mclasses,at->maxp.numGlyphs);
	free(mclasses);
    }

    at->gdeflen = ftell(at->gdef);
    if ( at->gdeflen&1 ) putc('\0',at->gdef);
    if ( (at->gdeflen+1)&2 ) putshort(at->gdef,0);
}