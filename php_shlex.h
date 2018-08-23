/*
  +----------------------------------------------------------------------+
  | Shlex                                                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Hualong Yuan  <zimuyang01@outlook.com>                       |
  +----------------------------------------------------------------------+
*/


#ifndef PHP_SHLEX_H
#define PHP_SHLEX_H

extern zend_module_entry shlex_module_entry;
#define phpext_shlex_ptr &shlex_module_entry

#define PHP_SHLEX_VERSION "0.1.0"

#ifdef PHP_WIN32
#	define PHP_SHLEX_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_SHLEX_API __attribute__ ((visibility("default")))
#else
#	define PHP_SHLEX_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

#if defined(ZTS) && defined(COMPILE_DL_SHLEX)
ZEND_TSRMLS_CACHE_EXTERN()
#endif

#endif