# Private macros for the teTeX / TeX live (TL) tree.
# Copyright (C) 2009 Peter Breitenlohner <tex-live@tug.org>
#
# This file is free software; the copyright holder
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# serial 0

# KPSE_WEB2C_PREPARE
# ------------------
# AC_DEFUN'ed so it can enforce inclusion of this file.
AC_DEFUN([KPSE_WEB2C_PREPARE], [])

# KPSE_WITH_MKTEX(PROG, YES-OR-NO, TEXT, STEM)
# --------------------------------------------
# Provide configure options --enable-mktex*-default and normalize result.
m4_define([KPSE_WITH_MKTEX],
[AC_ARG_ENABLE([$1-default],
               AS_HELP_STRING([--]m4_if($2, [yes], [dis], [en])[able-$1-default],
                              m4_if($2, [yes],
                                    [do not ])[run $1 if $3 missing],
                              [28]))[]dnl
AS_CASE([$enable_$1_default], [yes|no], ,
                            [enable_$1_default=$2])[]dnl
]) # KPSE_WITH_MKTEX

# KPSE_MKTEX_DEFINE(PROG, YES-OR-NO, TEXT, STEM)
# ----------------------------------------------
# Defines for enable-mktex*-default
m4_define([KPSE_MKTEX_DEFINE],
[AS_IF([test "x$enable_$1_default" = xyes],
       [AC_DEFINE([MAKE_$4_BY_DEFAULT], 1,
                  [Define to 1 if you want to run $1 if $3 is missing,
                   and to 0 if you don't])],
       [AC_DEFINE([MAKE_$4_BY_DEFAULT], 0)])[]dnl
]) # KPSE_MKTEX_DEFINE

# KPSE_WITH_XTEX(PROG, YES-OR-NO, TEXT, REQUIRED-LIBS)
# ---------------------------------------------------
# Provide configure options --enable-*tex and normalize result.
m4_define([KPSE_WITH_XTEX],
[AC_ARG_ENABLE([$1],
               AS_HELP_STRING([--]m4_if($2, [yes], [dis], [en])[able-$1],
                              m4_if($2, [yes],
                                    [do not ])[compile and install $3],
                              [28]))[]dnl
AS_CASE([$enable_$1], [yes|no], ,
                    [enable_$1=$2])[]dnl
m4_ifval([$4], [
test "x$enable_$1" = xno || {
AC_FOREACH([Kpse_Lib], [$4], [  need_[]AS_TR_SH(Kpse_Lib)=yes
])}
])[]dnl m4_ifval
]) # KPSE_WITH_XTEX

# KPSE_XTEX_COND(PROG, YES-OR-NO, TEXT, REQUIRED-LIBS)
# ----------------------------------------------------
# Conditionals for --with-*tex
m4_define([KPSE_XTEX_COND],
[AM_CONDITIONAL(AS_TR_CPP($1), [test "x$enable_$1" = xyes])[]dnl
]) # KPSE_XTEX_COND

# KPSE_WITH_MFWIN(WINDOW, DEFINE, TEXT)
# -------------------------------------
# Provide configure options --enable-*win.
m4_define([KPSE_WITH_MFWIN],
[AC_ARG_ENABLE([$1win],
               AS_HELP_STRING([--enable-$1win],
                              [include $3 window support],
                              [28]))[]dnl
]) # KPSE_WITH_MFWIN

# KPSE_MFWIN_DEFINE(WINDOW, DEFINE, TEXT)
# ---------------------------------------
# Defines for enable-*win
m4_define([KPSE_MFWIN_DEFINE],
[AS_IF([test "x$enable_$1win" = xyes],
       [AC_DEFINE([$2WIN], ,
                  [metafont: Define to include $3 window support.])])[]dnl
]) # KPSE_MFWIN_DEFINE
