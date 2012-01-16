/*
 * Copyright (c) 2008-2012 Beijing Kunlun Tech Co., Ltd.  All rights reserved.
 * Use is subject to license terms.
 */
/*
  +---------------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,           |
  | that is bundled with this package in the file LICENSE, and is             |
  | available through the world-wide-web at the following url:                |
  | http://www.php.net/license/3_01.txt                                       |
  | If you did not receive a copy of the PHP license and are unable to        |
  | obtain it through the world-wide-web, please send a note to               |
  | license@php.net so we can mail you a copy immediately.                    |
  +---------------------------------------------------------------------------+
 */
/* $Id: header,v 1.16.2.1.2.1.2.1 2008/02/07 19:39:50 iliaa Exp $ */

#ifndef PHP_CONSTCACHE_H
#define PHP_CONSTCACHE_H

extern zend_module_entry constcache_module_entry;
#define phpext_constcache_ptr &constcache_module_entry

#ifdef PHP_WIN32
#	define PHP_CONSTCACHE_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_CONSTCACHE_API __attribute__ ((visibility("default")))
#else
#	define PHP_CONSTCACHE_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

PHP_MINIT_FUNCTION(constcache);
PHP_MSHUTDOWN_FUNCTION(constcache);
PHP_MINFO_FUNCTION(constcache);

PHP_METHOD(ConstCache, __construct);
PHP_METHOD(ConstCache, get);
PHP_METHOD(ConstCache, add);
PHP_METHOD(ConstCache, flush);
PHP_METHOD(ConstCache, destroy);
PHP_METHOD(ConstCache, stat);
PHP_METHOD(ConstCache, debug);
PHP_METHOD(ConstCache, dump);

/* 
  	Declare any global variables you may need between the BEGIN
	and END macros here:     
*/

ZEND_BEGIN_MODULE_GLOBALS(constcache)
	key_t  ksem;
	key_t  kmem;
	size_t memsize;
	int    hashpower;
ZEND_END_MODULE_GLOBALS(constcache)

/* In every utility function you add that needs to use variables 
   in php_constcache_globals, call TSRMLS_FETCH(); after declaring other 
   variables used by that function, or better yet, pass in TSRMLS_CC
   after the last function argument and declare your utility function
   with TSRMLS_DC after the last declared argument.  Always refer to
   the globals in your function as CONSTCACHE_G(variable).  You are 
   encouraged to rename these macros something shorter, see
   examples in any other php module directory.
*/

#ifdef ZTS
#define CONSTCACHE_G(v) TSRMG(constcache_globals_id, zend_constcache_globals *, v)
#else
#define CONSTCACHE_G(v) (constcache_globals.v)
#endif

#endif	/* PHP_CONSTCACHE_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
