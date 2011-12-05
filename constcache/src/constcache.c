/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2008 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* $Id: header,v 1.16.2.1.2.1.2.1 2008/02/07 19:39:50 iliaa Exp $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_constcache.h"

#include "ext/standard/php_string.h"
#include "ext/standard/php_var.h"
#include "ext/standard/php_smart_str.h"

#include "fastcache.h"

ZEND_DECLARE_MODULE_GLOBALS(constcache)

/* True global resources - no need for thread safety here */
static int le_constcache;
static zend_class_entry *constcache_ce = NULL;
static int constcache_inited = 0;

typedef struct {
	zend_object zo;
	unsigned testval:1;
} php_constcache_t;

const zend_function_entry constcache_functions[] = {
	{NULL, NULL, NULL}	/* Must be the last line in constcache_functions[] */
};

zend_module_entry constcache_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"constcache",
	NULL,
	PHP_MINIT(constcache),
	PHP_MSHUTDOWN(constcache),
	NULL,
	NULL,
	PHP_MINFO(constcache),
#if ZEND_MODULE_API_NO >= 20010901
	"0.4", /* Replace with version number for your extension */
#endif
	STANDARD_MODULE_PROPERTIES
};

ZEND_BEGIN_ARG_INFO(klarginfo_void, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(klarginfo___construct, 0, 0, 0)
	ZEND_ARG_INFO(0, param)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(klarginfo_add, 0, 0, 2)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, value)
	ZEND_ARG_INFO(0, flag)
	ZEND_ARG_INFO(0, expiration)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(klarginfo_get, 0, 0, 1)
	ZEND_ARG_INFO(0, key)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(klarginfo_debug, 0, 0, 1)
	ZEND_ARG_INFO(0, filepath)
	ZEND_ARG_INFO(0, isDetail)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(klarginfo_dump, 0, 0, 1)
	ZEND_ARG_INFO(0, filepath)
ZEND_END_ARG_INFO()

#define CONSTCACHE_ME(name, args) PHP_ME(ConstCache, name, args, ZEND_ACC_PUBLIC)
static zend_function_entry constcache_class_methods[] = {
	CONSTCACHE_ME(__construct, klarginfo___construct)

	CONSTCACHE_ME(get,                klarginfo_get)
	CONSTCACHE_ME(add,                klarginfo_add)
	CONSTCACHE_ME(flush,              klarginfo_void)
	CONSTCACHE_ME(destroy,            klarginfo_void)
	CONSTCACHE_ME(stat,               klarginfo_void)
	CONSTCACHE_ME(debug,              klarginfo_debug)
	CONSTCACHE_ME(dump,               klarginfo_dump)

	{ NULL, NULL, NULL }
};

#ifdef COMPILE_DL_CONSTCACHE
ZEND_GET_MODULE(constcache)
#endif

PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("constcache.ksem", 
		"0", PHP_INI_SYSTEM, 
		OnUpdateLongGEZero, 
		ksem, 
		zend_constcache_globals, 
		constcache_globals)
    STD_PHP_INI_ENTRY("constcache.kmem", 
		"0", PHP_INI_SYSTEM, 
		OnUpdateLongGEZero, 
		kmem, 
		zend_constcache_globals, 
		constcache_globals)
    STD_PHP_INI_ENTRY("constcache.memsize", 
		"0", PHP_INI_SYSTEM, 
		OnUpdateLongGEZero, 
		memsize, 
		zend_constcache_globals, 
		constcache_globals)
    STD_PHP_INI_ENTRY("constcache.hashpower", 
		"0", PHP_INI_SYSTEM, 
		OnUpdateLongGEZero, 
		hashpower, 
		zend_constcache_globals, 
		constcache_globals)
PHP_INI_END()

static void php_constcache_init_globals(zend_constcache_globals *constcache_globals TSRMLS_DC)
{
	constcache_globals->ksem = 0;
	constcache_globals->kmem = 0;
	constcache_globals->memsize = 0;
	constcache_globals->hashpower = 0;
}

static void php_constcache_shutdown_globals(zend_constcache_globals *constcache_globals TSRMLS_DC)
{
}

static void php_constcache_destroy(php_constcache_t *i_obj TSRMLS_DC)
{
	efree(i_obj);
}

static void php_constcache_free_storage(php_constcache_t *i_obj TSRMLS_DC)
{
	zend_object_std_dtor(&i_obj->zo TSRMLS_CC);
	php_constcache_destroy(i_obj TSRMLS_CC);
}

zend_object_value php_constcache_new(zend_class_entry *ce TSRMLS_DC)
{
	zend_object_value retval;
	php_constcache_t *i_obj;
	zval *tmp;

	i_obj = ecalloc(1, sizeof(*i_obj));
	zend_object_std_init( &i_obj->zo, ce TSRMLS_CC );
	zend_hash_copy(i_obj->zo.properties, &ce->default_properties, (copy_ctor_func_t) zval_add_ref, (void *) &tmp, sizeof(zval *));

	//i_obj->testval = 0;

	retval.handle = zend_objects_store_put(i_obj, (zend_objects_store_dtor_t)zend_objects_destroy_object, (zend_objects_free_object_storage_t)php_constcache_free_storage, NULL TSRMLS_CC);
	retval.handlers = zend_get_std_object_handlers();

	return retval;
}

ZEND_RSRC_DTOR_FUNC(php_constcache_dtor)
{
	if (rsrc->ptr) {
		php_constcache_t *i_obj = (php_constcache_t *)rsrc->ptr;
		php_constcache_destroy(i_obj TSRMLS_CC);
		rsrc->ptr = NULL;
	}
}

PHP_MINIT_FUNCTION(constcache)
{
	ZEND_INIT_MODULE_GLOBALS(constcache, php_constcache_init_globals, php_constcache_shutdown_globals);
	REGISTER_INI_ENTRIES();

	if (CONSTCACHE_G(ksem) && CONSTCACHE_G(kmem)) {
		fc_conf_t conf;
		fc_conf_init(&conf);
		conf.ksem = CONSTCACHE_G(ksem);
		conf.kmem = CONSTCACHE_G(kmem);
		conf.memsize = CONSTCACHE_G(memsize)*1024*1024;
		if(CONSTCACHE_G(hashpower)) conf.hashpower = CONSTCACHE_G(hashpower);

		if (0 == fc_init(&conf)) {
			constcache_inited = 1;
		} else {
			php_error_docref(NULL TSRMLS_CC, E_WARNING,
				"constcache init failed:%d %s", errno, strerror(errno));
		}
	} else {
		php_error_docref(NULL TSRMLS_CC, E_WARNING,
			"'constcache.ksem','constcache.kmem','constcache.memsize' not "
			"configured correctly");
	}

	zend_class_entry ce;
	le_constcache = zend_register_list_destructors_ex(NULL, php_constcache_dtor, "constcache object", module_number);
	INIT_CLASS_ENTRY(ce, "ConstCache", constcache_class_methods);
	constcache_ce = zend_register_internal_class(&ce TSRMLS_CC);
	constcache_ce->create_object = php_constcache_new;

	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(constcache)
{
#ifdef ZTS
	ts_free_id(constcache_globals_id);
#else
	php_constcache_shutdown_globals(&constcache_globals);
#endif
	UNREGISTER_INI_ENTRIES();
	return SUCCESS;
}

PHP_MINFO_FUNCTION(constcache)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "constcache support", "enabled");
	php_info_print_table_header(2, "constcache version", FCVERSION);
	php_info_print_table_end();

	DISPLAY_INI_ENTRIES();
}

////////////////////////////////////////////////////////////////////////
#define CC_CHECK_INIT() do { \
	if (!constcache_inited) { \
		php_error_docref(NULL TSRMLS_CC, E_WARNING, \
			"constcache not inited correctly"); \
		RETURN_FALSE; \
	} \
} while(0)

PHP_METHOD(ConstCache, __construct)
{
	zval *object = getThis();
	php_constcache_t *i_obj;
	/*
	char *param = NULL;
	int param_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|s", &param,
		&param_len) == FAILURE) {
		ZVAL_NULL(object);
		return;
	}*/

	i_obj = (php_constcache_t *) zend_object_store_get_object(object TSRMLS_CC);
	//i_obj->testval = 0;
}

// ::get(string key)
// Returns a value for the given key or false
PHP_METHOD(ConstCache, get)
{
	char *key = NULL;
	size_t key_len = 0;
	fc_item_t item = {0};

	CC_CHECK_INIT();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &key, &key_len
		) == FAILURE) {
		return;
	}
	if (key_len == 0) {
		RETURN_FALSE;
	}
	item.key = (char *)key;
	item.klen = key_len;
	if (0 != fc_get(&item)) {
		RETURN_FALSE;
	}
	switch(item.flags) {
		case IS_STRING:
		case IS_LONG:
		case IS_DOUBLE: {
		//case IS_BOOL: {
			ZVAL_STRINGL(return_value, item.value, item.vlen, 1);
			break;
		}
		default: {
			const unsigned char *value_tmp = item.value;
			php_unserialize_data_t var_hash;

			PHP_VAR_UNSERIALIZE_INIT(var_hash);
			if(!php_var_unserialize(&return_value, &value_tmp, value_tmp + item.vlen, &var_hash TSRMLS_CC)) {
				PHP_VAR_UNSERIALIZE_DESTROY(var_hash);
				//zval_dtor(return_value);
				php_error_docref(NULL TSRMLS_CC, E_NOTICE, "unable to unserialize data");
				RETURN_FALSE;
			}
			PHP_VAR_UNSERIALIZE_DESTROY(var_hash);
			break;
		}
	}
}

// ::add(string key, mixed value [, int $flag  [, int $expire  ]] )
// Add the value for the given key
PHP_METHOD(ConstCache, add)
{
	char *key = NULL;
	int   key_len = 0;
	zval *value;
	long flag = 0;
	long expiration = 0;
	fc_item_t item;
	int nret = -1;

	CC_CHECK_INIT();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz|ll", &key, &key_len,
									  &value, &flag, &expiration) == FAILURE) {
		return;
	}
	if (key_len == 0) {
		RETURN_FALSE;
	}
	if (key_len > MAXKEYLEN) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING,
			"length of key '%s'(%d) too long", key, key_len);
		RETURN_FALSE;
	}
	item.key = key;
	item.klen = key_len;
	item.flags = Z_TYPE_P(value);
	switch(item.flags) {
		case IS_STRING:
			item.value = Z_STRVAL_P(value);
			item.vlen = Z_STRLEN_P(value);
			nret = fc_add(&item);
			break;
		case IS_LONG:
		case IS_DOUBLE: {
		//case IS_BOOL: {
			zval value_copy;
			/* FIXME: we should be using 'Z' instead of this, but unfortunately it's PHP5-only */
			value_copy = *value;
			zval_copy_ctor(&value_copy);
			convert_to_string(&value_copy);
			item.value = Z_STRVAL(value_copy);
			item.vlen = Z_STRLEN(value_copy);
			nret = fc_add(&item);
			zval_dtor(&value_copy);
			break;
		}
		default: {
			smart_str buf = {0};
			php_serialize_data_t var_hash;

			PHP_VAR_SERIALIZE_INIT(var_hash);
			php_var_serialize(&buf, &value, &var_hash TSRMLS_CC);
			PHP_VAR_SERIALIZE_DESTROY(var_hash);

			if (!buf.c) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "failed to serialize value");
				RETURN_FALSE;
			}
			item.value = buf.c;
			item.vlen = buf.len;
			nret = fc_add(&item);
			smart_str_free(&buf);
			break;
		}
	}
	if (0 == nret) {
		RETURN_TRUE;
	}
	//php_error_docref(NULL TSRMLS_CC, E_WARNING, 
	//	"failed to add value(%d %s)", errno, strerror(errno));
	RETURN_FALSE;
}

PHP_METHOD(ConstCache, flush)
{
	CC_CHECK_INIT();
	if (0 != fc_reset()) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING,
			"failed to flush cc:%d %s", errno, strerror(errno));
		RETURN_FALSE;
	}
	RETURN_TRUE;
}

PHP_METHOD(ConstCache, destroy)
{
	CC_CHECK_INIT();
	if (0 != fc_destroy()) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING,
			"failed to destroy cc:%d %s", errno, strerror(errno));
		RETURN_FALSE;
	}
	RETURN_TRUE;
}

PHP_METHOD(ConstCache, stat)
{
	CC_CHECK_INIT();
	fc_stat_t st;
	if( 0 != fc_stat(&st)) {
		RETURN_FALSE;
	}
	
	array_init(return_value);
	add_assoc_string(return_value, "magic", st.magic, 1);
	add_assoc_long(return_value, "total_size", st.total_size);
	add_assoc_long(return_value, "free_size", st.free_size);
	add_assoc_long(return_value, "add_cmds", st.add_cmds);
	add_assoc_long(return_value, "add_hit", st.add_hit);
	add_assoc_long(return_value, "get_cmds", st.get_cmds);
	add_assoc_long(return_value, "get_hit", st.get_hit);
	add_assoc_long(return_value, "total_items", st.total_items);
	add_assoc_long(return_value, "fail_items", st.fail_items);
	add_assoc_long(return_value, "hash_size", st.hash_size);
	add_assoc_long(return_value, "hash_filled", st.hash_filled);
}

// ::debug(string filepath, [, bool detail] )
PHP_METHOD(ConstCache, debug)
{
	char *filepath;
	int fplen, result;
	FILE *fp;
	zend_bool onoff = 0;

	CC_CHECK_INIT();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|b",
		&filepath, &fplen, &onoff) == FAILURE) {
		return;
	}
	if (!(fp=fopen(filepath, "ab"))) { /* filepath shoud be zero terminated */
		php_error_docref(NULL TSRMLS_CC, E_WARNING,
			"failed open file %s:%d %s", filepath, errno, strerror(errno));
		RETURN_FALSE;
	}

	result = fc_debug(fp, onoff);
	fclose(fp);
	if (0 != result) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING,
			"failed to debug cc:%d %s", errno, strerror(errno));
		RETURN_FALSE;
	}
	RETURN_TRUE;
}

// ::dump(string filepath)
PHP_METHOD(ConstCache, dump)
{
	char *filepath;
	unsigned int fplen;
	int result;
	FILE *fp;

	CC_CHECK_INIT();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
		&filepath, &fplen) == FAILURE) {
		return;
	}
	if (!(fp=fopen(filepath, "wb"))) { /* filepath shoud be zero terminated */
		php_error_docref(NULL TSRMLS_CC, E_WARNING,
			"failed open file %s:%d %s", filepath, errno, strerror(errno));
		RETURN_FALSE;
	}

	result = fc_dump(fp);
	fclose(fp);
	if (0 != result) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING,
			"failed to dump cc:%d %s", errno, strerror(errno));
		RETURN_FALSE;
	}
	RETURN_TRUE;
}
