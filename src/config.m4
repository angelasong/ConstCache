dnl $Id$
dnl config.m4 for extension constcache

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

PHP_ARG_WITH(constcache, for constcache support,
	[  --with-constcache             Include constcache support])

dnl Otherwise use enable:

dnl PHP_ARG_ENABLE(constcache, whether to enable constcache support,
dnl Make sure that the comment is aligned:
dnl [  --enable-constcache           Enable constcache support])

if test "$PHP_CONSTCACHE" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-constcache -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/constcache.h"  # you most likely want to change this
  dnl if test -r $PHP_CONSTCACHE/$SEARCH_FOR; then # path given as parameter
  dnl   CONSTCACHE_DIR=$PHP_CONSTCACHE
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for constcache files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       CONSTCACHE_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$CONSTCACHE_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the constcache distribution])
  dnl fi

  dnl # --with-constcache -> add include path
  dnl PHP_ADD_INCLUDE($CONSTCACHE_DIR/include)

  dnl # --with-constcache -> check for lib and symbol presence
  dnl LIBNAME=constcache # you may want to change this
  dnl LIBSYMBOL=constcache # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $CONSTCACHE_DIR/lib, CONSTCACHE_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_CONSTCACHELIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong constcache lib version or lib not found])
  dnl ],[
  dnl   -L$CONSTCACHE_DIR/lib -lm
  dnl ])
  dnl
  dnl PHP_SUBST(CONSTCACHE_SHARED_LIBADD)

  PHP_NEW_EXTENSION(constcache, constcache.c fastcache_imp.c assoc.c hash.c items.c slabs.c, $ext_shared)
fi
