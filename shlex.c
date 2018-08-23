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


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>

#include "php.h"
#include "php_ini.h"
#include "php_shlex.h"
#include "php_streams.h"
#include "Zend/zend_interfaces.h"
#include "Zend/zend_exceptions.h"
#include "standard/php_var.h"
#include "standard/info.h"
#include "standard/file.h"

#define SHLEX_STRL(str) (str), (strlen(str))

struct _shlex_stream {
	php_stream *stream;
	int is_string_io;
	int read_flag;
	int is_valid;
	char *current_token;
	size_t current_token_len;
};

typedef struct _shlex_stream shlex_stream;

zend_class_entry *shlex_ce, *shlex_exception_ce;

static zval *call_function_with_2_params(char *function, zval *arg1, zval *arg2);
static zval *call_function_with_3_params(char *function, zval *arg1, zval *arg2, zval *arg3);

static void shlex_str_io_init(zval *handle, shlex_stream *str_io, char *content, size_t content_len);
static size_t shlex_str_io_write(shlex_stream *str_io, char *content, size_t content_len);
static size_t shlex_str_io_read(zval *return_value, shlex_stream *str_io, size_t len);

static void shlex_construct(zval *self, zval *instream, char *infile, size_t infile_len, zend_bool posix, zval *punctuation_chars);
static shlex_stream *shlex_current(zval *self);
static int shlex_valid(zval *self);
static void shlex_read_token(zval *self, zval *return_value);
static void shlex_push_source(zval *self, zval *newstream, zval *newfile);
static void shlex_pop_source(zval *self);
static void shlex_get_token(zval *self, zval *return_value);
static void shlex_sourcehook(zval *self, zval *return_value, char *newfile, size_t newfile_len);

// 判断当前字符串是否为UTF-8编码
static int is_utf8(char *s){

	int ret = 1;
	unsigned char* start = (unsigned char*)s;
	unsigned char* end = (unsigned char*)s + strlen(s) + 1;

	while (start < end) {
		if (*start < 0x80) { // (10000000): 值小于0x80的为ASCII字符
			start++;
		} else if (*start < (0xC0)) { // (11000000): 值介于0x80与0xC0之间的为无效UTF-8字符
			ret = 0;
			break;
		} else if (*start < (0xE0)) { // (11100000): 此范围内为2字节UTF-8字符
			if (start >= end - 1) {
				break;
			}

			if ((start[1] & (0xC0)) != 0x80) {
				ret = 0;
				break;
			}

			start += 2;
		} else if (*start < (0xF0)) { // (11110000): 此范围内为3字节UTF-8字符
			if (start >= end - 2) {
				break;
			}

			if ((start[1] & (0xC0)) != 0x80 || (start[2] & (0xC0)) != 0x80) {
				ret = 0;
				break;
			}

			start += 3;
		} else {
			ret = 0;
			break;
		}
	}

	return ret;
}

// 将两个字符串拼接为一个新的字符串
static char *str_join_len1(char *str1, char *str2, size_t len1){
	size_t len2 = strlen(str2);

	char *buf = emalloc(len1 + len2 + 1);
	memcpy(buf, str1, len1);
	memcpy(buf + len1, str2, len2 + 1);

	return buf;
}

// 将两个字符串拼接为一个新的字符串
static char *str_join_len1_len2(char *str1, char *str2, size_t len1, size_t len2){
	char *buf = emalloc(len1 + len2 + 1);
	memcpy(buf, str1, len1);
	memcpy(buf + len1, str2, len2 + 1);

	return buf;
}

static void array_unshift(zval *list, zval *new_val){
	HashTable new_hash;		/* New hashtable for the stack */
	zval *value;

	zend_hash_init(&new_hash, zend_hash_num_elements(Z_ARRVAL_P(list)) + 1, NULL, ZVAL_PTR_DTOR, 0);
	zend_hash_next_index_insert_new(&new_hash, new_val);

	if (EXPECTED(Z_ARRVAL_P(list)->u.v.nIteratorsCount == 0)) {
		ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(list), value) {
					zend_hash_next_index_insert_new(&new_hash, value);
				} ZEND_HASH_FOREACH_END();
	} else {
		uint32_t old_idx;
		uint32_t new_idx = 0;
		uint32_t iter_pos = zend_hash_iterators_lower_pos(Z_ARRVAL_P(list), 0);

		ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(list), value) {
					zend_hash_next_index_insert_new(&new_hash, value);

					old_idx = (Bucket*)value - Z_ARRVAL_P(list)->arData;

					if (old_idx == iter_pos) {
						zend_hash_iterators_update(Z_ARRVAL_P(list), old_idx, new_idx);
						iter_pos = zend_hash_iterators_lower_pos(Z_ARRVAL_P(list), iter_pos + 1);
					}
					new_idx++;
				} ZEND_HASH_FOREACH_END();
	}

	/* replace HashTable data */
	Z_ARRVAL_P(list)->u.v.nIteratorsCount = 0;
	Z_ARRVAL_P(list)->pDestructor = NULL;
	zend_hash_destroy(Z_ARRVAL_P(list));

	Z_ARRVAL_P(list)->u.v.flags         = new_hash.u.v.flags;
	Z_ARRVAL_P(list)->nTableSize        = new_hash.nTableSize;
	Z_ARRVAL_P(list)->nTableMask        = new_hash.nTableMask;
	Z_ARRVAL_P(list)->nNumUsed          = new_hash.nNumUsed;
	Z_ARRVAL_P(list)->nNumOfElements    = new_hash.nNumOfElements;
	Z_ARRVAL_P(list)->nNextFreeElement  = new_hash.nNextFreeElement;
	Z_ARRVAL_P(list)->arData            = new_hash.arData;
	Z_ARRVAL_P(list)->pDestructor       = new_hash.pDestructor;

	zend_hash_internal_pointer_reset(Z_ARRVAL_P(list));
}

static zval *array_shift(zval *list){
	zval *val, *return_value;
	uint32_t idx;
	Bucket *p;
	return_value = emalloc(sizeof(zval));
	bzero(return_value, sizeof(zval));
	ZVAL_NULL(return_value);

	if (zend_hash_num_elements(Z_ARRVAL_P(list)) == 0) {
		return return_value;
	}

	/* Get the first value and copy it into the return value */
	idx = 0;

	while (1) {
		if (idx == Z_ARRVAL_P(list)->nNumUsed) {
			return return_value;
		}

		p = Z_ARRVAL_P(list)->arData + idx;
		val = &p->val;

		if (Z_TYPE_P(val) == IS_INDIRECT) {
			val = Z_INDIRECT_P(val);
		}

		if (Z_TYPE_P(val) != IS_UNDEF) {
			break;
		}

		idx++;
	}

	ZVAL_DEREF(val);
	ZVAL_COPY(return_value, val);

	/* Delete the first value */
	if (p->key) {
		if (Z_ARRVAL_P(list) == &EG(symbol_table)) {
			zend_delete_global_variable(p->key);
		} else {
			zend_hash_del(Z_ARRVAL_P(list), p->key);
		}
	} else {
		zend_hash_index_del(Z_ARRVAL_P(list), p->h);
	}

	/* re-index like it did before */
	if (Z_ARRVAL_P(list)->u.flags & HASH_FLAG_PACKED) {
		uint32_t k = 0;

		if (EXPECTED(Z_ARRVAL_P(list)->u.v.nIteratorsCount == 0)) {
			for (idx = 0; idx < Z_ARRVAL_P(list)->nNumUsed; idx++) {
				p = Z_ARRVAL_P(list)->arData + idx;
				if (Z_TYPE(p->val) == IS_UNDEF) continue;
				if (idx != k) {
					Bucket *q = Z_ARRVAL_P(list)->arData + k;
					q->h = k;
					q->key = NULL;
					ZVAL_COPY_VALUE(&q->val, &p->val);
					ZVAL_UNDEF(&p->val);
				}
				k++;
			}
		} else {
			uint32_t iter_pos = zend_hash_iterators_lower_pos(Z_ARRVAL_P(list), 0);

			for (idx = 0; idx < Z_ARRVAL_P(list)->nNumUsed; idx++) {
				p = Z_ARRVAL_P(list)->arData + idx;
				if (Z_TYPE(p->val) == IS_UNDEF) continue;
				if (idx != k) {
					Bucket *q = Z_ARRVAL_P(list)->arData + k;
					q->h = k;
					q->key = NULL;
					ZVAL_COPY_VALUE(&q->val, &p->val);
					ZVAL_UNDEF(&p->val);
					if (idx == iter_pos) {
						zend_hash_iterators_update(Z_ARRVAL_P(list), idx, k);
						iter_pos = zend_hash_iterators_lower_pos(Z_ARRVAL_P(list), iter_pos + 1);
					}
				}
				k++;
			}
		}
		Z_ARRVAL_P(list)->nNumUsed = k;
		Z_ARRVAL_P(list)->nNextFreeElement = k;
	} else {
		uint32_t k = 0;
		int should_rehash = 0;

		for (idx = 0; idx < Z_ARRVAL_P(list)->nNumUsed; idx++) {
			p = Z_ARRVAL_P(list)->arData + idx;
			if (Z_TYPE(p->val) == IS_UNDEF) continue;
			if (p->key == NULL) {
				if (p->h != k) {
					p->h = k++;
					should_rehash = 1;
				} else {
					k++;
				}
			}
		}
		Z_ARRVAL_P(list)->nNextFreeElement = k;
		if (should_rehash) {
			zend_hash_rehash(Z_ARRVAL_P(list));
		}
	}

	zend_hash_internal_pointer_reset(Z_ARRVAL_P(list));

	return return_value;
}

static zval *array_pop(zval *list){
	zval *val, *return_value;
	uint32_t idx;
	Bucket *p;
	return_value = emalloc(sizeof(zval));
	bzero(return_value, sizeof(zval));
	ZVAL_NULL(return_value);

	if (zend_hash_num_elements(Z_ARRVAL_P(list)) == 0) {
		return return_value;
	}

	/* Get the last value and copy it into the return value */
	idx = Z_ARRVAL_P(list)->nNumUsed;

	while (1) {
		if (idx == 0) {
			return return_value;
		}

		idx--;
		p = Z_ARRVAL_P(list)->arData + idx;
		val = &p->val;

		if (Z_TYPE_P(val) == IS_INDIRECT) {
			val = Z_INDIRECT_P(val);
		}

		if (Z_TYPE_P(val) != IS_UNDEF) {
			break;
		}
	}

	ZVAL_DEREF(val);
	ZVAL_COPY(return_value, val);

	if (!p->key && Z_ARRVAL_P(list)->nNextFreeElement > 0 && p->h >= (zend_ulong)(Z_ARRVAL_P(list)->nNextFreeElement - 1)) {
		Z_ARRVAL_P(list)->nNextFreeElement = Z_ARRVAL_P(list)->nNextFreeElement - 1;
	}

	/* Delete the last value */
	if (p->key) {
		if (Z_ARRVAL_P(list) == &EG(symbol_table)) {
			zend_delete_global_variable(p->key);
		} else {
			zend_hash_del(Z_ARRVAL_P(list), p->key);
		}
	} else {
		zend_hash_index_del(Z_ARRVAL_P(list), p->h);
	}

	zend_hash_internal_pointer_reset(Z_ARRVAL_P(list));

	return return_value;
}

static php_stream *shlex_fopen(zval *return_value, char *filename, char *mode){
	php_stream *stream;
	php_stream_context *context = NULL;

	context = FG(default_context) ? FG(default_context) : (FG(default_context) = php_stream_context_alloc());

	stream = php_stream_open_wrapper_ex(filename, mode, REPORT_ERRORS, NULL, context);

	if (stream == NULL) {
		return NULL;
	}

	php_stream_to_zval(stream, return_value);

	return stream;
}

static void shlex_fgets(php_stream *stream, zval *return_value){
	char *buf = NULL;
	size_t line_len = 0;

	/* ask streams to give us a buffer of an appropriate size */
	buf = php_stream_get_line(stream, NULL, 0, &line_len);

	if (buf == NULL) {
		return;
	}

	if(return_value != NULL){
		ZVAL_STRINGL(return_value, buf, line_len);
	}

	efree(buf);
}

static void shlex_fread(zval *return_value, php_stream *stream, size_t len){
	if (len <= 0) {
		php_error_docref(NULL, E_WARNING, "Length parameter must be greater than 0");
		RETURN_FALSE;
	}

	ZVAL_NEW_STR(return_value, zend_string_alloc(len, 0));
	Z_STRLEN_P(return_value) = php_stream_read(stream, Z_STRVAL_P(return_value), len);

	Z_STRVAL_P(return_value)[Z_STRLEN_P(return_value)] = 0;
}

static int shlex_fclose(php_stream *stream){

	if ((stream->flags & PHP_STREAM_FLAG_NO_FCLOSE) != 0) {
		php_error_docref(NULL, E_WARNING, "%d is not a valid stream resource", stream->res->handle);
		return 0;
	}

	php_stream_free(stream,
					PHP_STREAM_FREE_KEEP_RSRC |
					(stream->is_persistent ? PHP_STREAM_FREE_CLOSE_PERSISTENT : PHP_STREAM_FREE_CLOSE));

	return 1;
}

static zval *call_function_with_2_params(char *function, zval *arg1, zval *arg2){
	zval *ret_ptr = emalloc(sizeof(zval));
	bzero(ret_ptr, sizeof(zval));

	zval callback = {{0}};
	ZVAL_STRING(&callback, function);
	zval params[2];
	ZVAL_COPY(&params[0], arg1);
	ZVAL_COPY(&params[1], arg2);

	if (call_user_function(EG(function_table), NULL, &callback, ret_ptr, 2, params) == FAILURE) {
		zval_ptr_dtor(ret_ptr);
		zval_ptr_dtor(&callback);
		php_error_docref(NULL, E_WARNING, "Call to %S failed", function);
		ZVAL_BOOL(ret_ptr, 0);
	}

	if(EG(exception)) {zend_bailout();}

	return ret_ptr;
}

static zval *call_function_with_3_params(char *function, zval *arg1, zval *arg2, zval *arg3){
	zval *ret_ptr = emalloc(sizeof(zval));
	bzero(ret_ptr, sizeof(zval));

	zval callback = {{0}};
	ZVAL_STRING(&callback, function);
	zval params[3];
	ZVAL_COPY(&params[0], arg1);
	ZVAL_COPY(&params[1], arg2);
	ZVAL_COPY(&params[2], arg3);

	if (call_user_function(EG(function_table), NULL, &callback, ret_ptr, 3, params) == FAILURE) {
		zval_ptr_dtor(ret_ptr);
		zval_ptr_dtor(&callback);
		php_error_docref(NULL, E_WARNING, "Call to %S failed", function);
		ZVAL_BOOL(ret_ptr, 0);
	}

	if(EG(exception)) {zend_bailout();}
	return ret_ptr;
}

static void shlex_str_io_init(zval *handle, shlex_stream *str_io, char *content, size_t content_len){
	str_io->is_string_io = 1;
	str_io->read_flag = 0;
	str_io->stream = shlex_fopen(handle, "php://memory", "r+");

	if(content != NULL && content_len > 0){
		shlex_str_io_write(str_io, content, content_len);
	}
}

static size_t shlex_str_io_write(shlex_stream *str_io, char *content, size_t content_len){
	if(content != NULL && content_len > 0){
		return php_stream_write(str_io->stream, content, content_len);
	}

	return 0;
}

static size_t shlex_str_io_read(zval *return_value, shlex_stream *str_io, size_t len){
	if(str_io->read_flag == 0){
		if (-1 == php_stream_rewind(str_io->stream)) {
			return 0;
		}

		str_io->read_flag = 1;
	}

	if (len <= 0) {
		php_error_docref(NULL, E_WARNING, "Length parameter must be greater than 0");
		return 0;
	}

	ZVAL_NEW_STR(return_value, zend_string_alloc(len, 0));
	Z_STRLEN_P(return_value) = php_stream_read(str_io->stream, Z_STRVAL_P(return_value), len);
	Z_STRVAL_P(return_value)[Z_STRLEN_P(return_value)] = 0;

	return Z_STRLEN_P(return_value);
}

static void shlex_construct(zval *self, zval *instream, char *infile, size_t infile_len, zend_bool posix, zval *punctuation_chars) {

	shlex_stream *custom_stream = emalloc(sizeof(shlex_stream));
	bzero(custom_stream, sizeof(shlex_stream));
	custom_stream->is_valid = 1;
	zval *stream = zend_read_property(shlex_ce, self, SHLEX_STRL("stream"), 1, NULL);
	zval *instream_pro = zend_read_property(shlex_ce, self, SHLEX_STRL("instream"), 1, NULL);
	ZVAL_PTR(stream, custom_stream);

	if(instream && Z_TYPE_P(instream) == IS_STRING){
		shlex_str_io_init(instream, custom_stream, Z_STRVAL_P(instream), Z_STRLEN_P(instream));
	}

	if(instream && Z_TYPE_P(instream) != IS_NULL) {
		if(custom_stream->is_string_io == 0){
			zval *return_value = instream;
			php_stream_from_res(custom_stream->stream, Z_RES_P(instream));
		}

		if(infile) zend_update_property_stringl(shlex_ce, self, SHLEX_STRL("infile"), infile, infile_len);
	} else {
		custom_stream->stream = shlex_fopen(instream, "php://stdin", "r");
	}

	ZVAL_COPY(instream_pro, instream);
	zend_update_property_bool(shlex_ce, self, SHLEX_STRL("posix"), posix);

	if(posix){
		zval *wordchars = zend_read_property(shlex_ce, self, SHLEX_STRL("wordchars"), 1, NULL);
		char *str_tmp = "ßàáâãäåæçèéêëìíîïðñòóôõöøùúûüýþÿÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖØÙÚÛÜÝÞ";
		char *new_str = str_join_len1(Z_STRVAL_P(wordchars), str_tmp, Z_STRLEN_P(wordchars));
		ZVAL_STRING(wordchars, new_str);
		efree(new_str);

	} else{
		zend_update_property_string(shlex_ce, self, SHLEX_STRL("eof"), "");
	}

	if(!zend_is_true(punctuation_chars)){
		convert_to_string(punctuation_chars);
		ZVAL_STRING(punctuation_chars, "");
	} else if(Z_TYPE_P(punctuation_chars) == IS_TRUE){
		convert_to_string(punctuation_chars);
		ZVAL_STRING(punctuation_chars, "();<>|&");
	}

	zend_update_property(shlex_ce, self, SHLEX_STRL("punctuationChars"), punctuation_chars);

	if(zend_is_true(punctuation_chars)){
		zval _punctuation_chars;
		array_init(&_punctuation_chars);
		zend_update_property(shlex_ce, self, SHLEX_STRL("_punctuationChars"), &_punctuation_chars);

		zval *wordchars = zend_read_property(shlex_ce, self, SHLEX_STRL("wordchars"), 1, NULL);
		char *str_tmp = "~-./*?=";
		char *new_str = str_join_len1(Z_STRVAL_P(wordchars), str_tmp, Z_STRLEN_P(wordchars));
		ZVAL_STRING(wordchars, new_str);
		efree(new_str);

		zval arg2;
		ZVAL_STRING(&arg2, "");
		zval *tmp = call_function_with_3_params("str_replace", punctuation_chars, &arg2, wordchars);

		ZVAL_STRING(wordchars, Z_STRVAL_P(tmp));
	}

	zval pushback, filestack;
	array_init(&pushback);
	array_init(&filestack);
	zend_update_property(shlex_ce, self, SHLEX_STRL("pushback"), &pushback);
	zend_update_property(shlex_ce, self, SHLEX_STRL("filestack"), &filestack);
}

static shlex_stream *shlex_current(zval *self){
	zval *stream = zend_read_property(shlex_ce, self, SHLEX_STRL("stream"), 1, NULL);
	return Z_PTR_P(stream);
}

static int shlex_valid(zval *self){
	zval *stream = zend_read_property(shlex_ce, self, SHLEX_STRL("stream"), 1, NULL);
	shlex_stream *custom_stream = Z_PTR_P(stream);

	if(custom_stream->is_valid == 0){
		return 0;
	}

	zval current_token;
	shlex_get_token(self, &current_token);

	if(Z_TYPE(current_token) == IS_STRING){
		custom_stream->current_token = Z_STRVAL(current_token);
		custom_stream->current_token_len = Z_STRLEN(current_token);
	} else {
		custom_stream->current_token = NULL;
		custom_stream->current_token_len = 0;
	}

	zval *eof = zend_read_property(shlex_ce, self, SHLEX_STRL("eof"), 1, NULL);
	custom_stream->is_valid = zend_is_identical(&current_token, eof) ? 0 : 1;

	return custom_stream->is_valid;
}

static void shlex_read_token(zval *self, zval *return_value){
	int quoted = 0;
	char *escapedstate = " ";
	int debug = (int)Z_LVAL_P(zend_read_property(shlex_ce, self, SHLEX_STRL("debug"), 1, NULL));
	char *new_str;

	zval *nextchar = NULL;
	zval *punctuation_chars = zend_read_property(shlex_ce, self, SHLEX_STRL("punctuationChars"), 1, NULL);
	zval *_punctuation_chars = zend_read_property(shlex_ce, self, SHLEX_STRL("_punctuationChars"), 1, NULL);
	zval *token = zend_read_property(shlex_ce, self, SHLEX_STRL("token"), 1, NULL);
	zval *posix = zend_read_property(shlex_ce, self, SHLEX_STRL("posix"), 1, NULL);

	while (1){

		if(zend_is_true(punctuation_chars) && zend_is_true(_punctuation_chars)){
			nextchar = array_pop(_punctuation_chars);
		} else {
			zval *stream = zend_read_property(shlex_ce, self, SHLEX_STRL("stream"), 1, NULL);
			shlex_stream *custom_stream = Z_PTR_P(stream);

			zval nextchar_z, tmp;
			nextchar = &nextchar_z;
			ZVAL_STRING(&nextchar_z, "");

			read_loop:
			if(custom_stream->is_string_io == 1){
				shlex_str_io_read(&tmp, custom_stream, 1);
			} else {
				shlex_fread(&tmp, custom_stream->stream, 1);
			}

			new_str = str_join_len1_len2(Z_STRVAL_P(nextchar), Z_STRVAL(tmp), Z_STRLEN_P(nextchar), Z_STRLEN(tmp));
			ZVAL_STRING(nextchar, new_str);
			efree(new_str);

			if(Z_STRLEN_P(nextchar) > 0){
				if(!is_utf8(Z_STRVAL_P(nextchar))){
					goto read_loop;
				}
			}

		}

		if(strcmp(Z_STRVAL_P(nextchar), "\n") == 0){
			Z_LVAL_P(zend_read_property(shlex_ce, self, SHLEX_STRL("lineno"), 1, NULL))++;
		}

		zval *state = zend_read_property(shlex_ce, self, SHLEX_STRL("state"), 1, NULL);

		if(debug >= 3){
			zval state_tmp;
			ZVAL_COPY(&state_tmp, state);
			convert_to_string(&state_tmp);
			php_printf("shlex: in state %s I see character: %s", Z_STRVAL(state_tmp), Z_STRVAL_P(nextchar));
		}

		if(Z_TYPE_P(state) == IS_NULL){
			ZVAL_STRING(token, "");
			break;
		} else if(strcmp(Z_STRVAL_P(state), " ") == 0){

			if(!zend_is_true(nextchar)){
				ZVAL_NULL(state);
				break;

			} else if(strstr(Z_STRVAL_P(zend_read_property(shlex_ce, self, SHLEX_STRL("whitespace"), 1, NULL)), Z_STRVAL_P(nextchar))) {

				if(debug >= 2){
					php_printf("shlex: I see whitespace in whitespace state");
				}

				if(zend_is_true(token) || (Z_TYPE_P(posix) == IS_TRUE && quoted)){
					break;
				} else {
					continue;
				}

			} else if(strstr(Z_STRVAL_P(zend_read_property(shlex_ce, self, SHLEX_STRL("commenters"), 1, NULL)), Z_STRVAL_P(nextchar))){

				zval *stream = zend_read_property(shlex_ce, self, SHLEX_STRL("stream"), 1, NULL);
				shlex_stream *custom_stream = Z_PTR_P(stream);
				shlex_fgets(custom_stream->stream, NULL);
				Z_LVAL_P(zend_read_property(shlex_ce, self, SHLEX_STRL("lineno"), 1, NULL))++;

			} else if((Z_TYPE_P(posix) == IS_TRUE) && strstr(Z_STRVAL_P(zend_read_property(shlex_ce, self, SHLEX_STRL("escape"), 1, NULL)), Z_STRVAL_P(nextchar))){

				escapedstate = "a";
				ZVAL_COPY(state, nextchar);

			} else if(strstr(Z_STRVAL_P(zend_read_property(shlex_ce, self, SHLEX_STRL("wordchars"), 1, NULL)), Z_STRVAL_P(nextchar))){

				ZVAL_COPY(token, nextchar);
				ZVAL_STRING(state, "a");

			} else if(strstr(Z_STRVAL_P(punctuation_chars), Z_STRVAL_P(nextchar))){

				ZVAL_COPY(token, nextchar);
				ZVAL_STRING(state, "c");

			} else if(strstr(Z_STRVAL_P(zend_read_property(shlex_ce, self, SHLEX_STRL("quotes"), 1, NULL)), Z_STRVAL_P(nextchar))){

				if(Z_TYPE_P(posix) == IS_FALSE){
					ZVAL_COPY(token, nextchar);
				}

				ZVAL_COPY(state, nextchar);

			} else if(zend_is_true(zend_read_property(shlex_ce, self, SHLEX_STRL("whitespaceSplit"), 1, NULL))){

				ZVAL_COPY(token, nextchar);
				ZVAL_STRING(state, "a");

			} else {

				ZVAL_COPY(token, nextchar);

				if(zend_is_true(token) || (Z_TYPE_P(posix) == IS_TRUE && quoted)){
					break;
				} else {
					continue;
				}
			}

		} else if(strstr(Z_STRVAL_P(zend_read_property(shlex_ce, self, SHLEX_STRL("quotes"), 1, NULL)), Z_STRVAL_P(state))){

			quoted = 1;

			if(!zend_is_true(nextchar)){
				if(debug >= 2){
					php_printf("shlex: I see EOF in quotes state");
				}

				zend_throw_exception(shlex_exception_ce, "No closing quotation", 0);
			}

			if(zend_is_identical(nextchar, state)){

				if(Z_TYPE_P(posix) == IS_FALSE){
					new_str = str_join_len1_len2(Z_STRVAL_P(token), Z_STRVAL_P(nextchar), Z_STRLEN_P(token), Z_STRLEN_P(nextchar));
					ZVAL_STRING(token, new_str);
					ZVAL_STRING(state, " ");
					efree(new_str);

					break;

				} else {
					ZVAL_STRING(state, "a");
				}

			} else if((Z_TYPE_P(posix) == IS_TRUE) && strstr(Z_STRVAL_P(zend_read_property(shlex_ce, self, SHLEX_STRL("escape"), 1, NULL)), Z_STRVAL_P(nextchar)) && strstr(Z_STRVAL_P(zend_read_property(shlex_ce, self, SHLEX_STRL("escapedquotes"), 1, NULL)), Z_STRVAL_P(state))){

				escapedstate = Z_STRVAL_P(state);
				ZVAL_COPY(state, nextchar);

			} else {

				new_str = str_join_len1_len2(Z_STRVAL_P(token), Z_STRVAL_P(nextchar), Z_STRLEN_P(token), Z_STRLEN_P(nextchar));
				ZVAL_STRING(token, new_str);
				efree(new_str);

			}
		} else if (strstr(Z_STRVAL_P(zend_read_property(shlex_ce, self, SHLEX_STRL("escape"), 1, NULL)), Z_STRVAL_P(state))){

			if(!zend_is_true(nextchar)){
				if(debug >= 2){
					php_printf("shlex: I see EOF in escape state");
				}

				zend_throw_exception(shlex_exception_ce, "No escaped character", 0);
			}

			if(strstr(Z_STRVAL_P(zend_read_property(shlex_ce, self, SHLEX_STRL("quotes"), 1, NULL)), escapedstate) && !zend_is_identical(nextchar, state) && strcmp(Z_STRVAL_P(nextchar), escapedstate) != 0){

				new_str = str_join_len1_len2(Z_STRVAL_P(token), Z_STRVAL_P(state), Z_STRLEN_P(token), Z_STRLEN_P(state));
				ZVAL_STRING(token, new_str);
				efree(new_str);
			}

			new_str = str_join_len1_len2(Z_STRVAL_P(token), Z_STRVAL_P(nextchar), Z_STRLEN_P(token), Z_STRLEN_P(nextchar));
			ZVAL_STRING(token, new_str);
			ZVAL_STRING(state, escapedstate);
			efree(new_str);

		} else if (strcmp(Z_STRVAL_P(state), "a") == 0 || strcmp(Z_STRVAL_P(state), "c") == 0){

			if(!zend_is_true(nextchar)){

				ZVAL_NULL(state);
				break;

			} else if (strstr(Z_STRVAL_P(zend_read_property(shlex_ce, self, SHLEX_STRL("whitespace"), 1, NULL)), Z_STRVAL_P(nextchar))){

				if(debug >= 2){
					php_printf("shlex: I see whitespace in word state");
				}

				ZVAL_STRING(state, " ");

				if(zend_is_true(token) || (Z_TYPE_P(posix) == IS_TRUE || quoted)){

					break;
				} else {
					continue;
				}

			} else if (strstr(Z_STRVAL_P(zend_read_property(shlex_ce, self, SHLEX_STRL("commenters"), 1, NULL)), Z_STRVAL_P(nextchar))){

				zval *stream = zend_read_property(shlex_ce, self, SHLEX_STRL("stream"), 1, NULL);
				shlex_stream *custom_stream = Z_PTR_P(stream);
				shlex_fgets(custom_stream->stream, NULL);

				Z_LVAL_P(zend_read_property(shlex_ce, self, SHLEX_STRL("lineno"), 1, NULL))++;

				if(Z_TYPE_P(posix) == IS_TRUE){
					ZVAL_STRING(state, " ");

					if(zend_is_true(token) || (Z_TYPE_P(posix) == IS_TRUE && quoted)){
						break;
					} else {
						continue;
					}
				}

			} else if (strcmp(Z_STRVAL_P(state), "c") == 0){

				if (strstr(Z_STRVAL_P(punctuation_chars), Z_STRVAL_P(nextchar))){

					new_str = str_join_len1_len2(Z_STRVAL_P(token), Z_STRVAL_P(nextchar), Z_STRLEN_P(token), Z_STRLEN_P(nextchar));
					ZVAL_STRING(token, new_str);
					efree(new_str);

				} else {

					if(!strstr(Z_STRVAL_P(zend_read_property(shlex_ce, self, SHLEX_STRL("whitespace"), 1, NULL)), Z_STRVAL_P(nextchar))){
						add_next_index_zval(_punctuation_chars, nextchar);
					}

					ZVAL_STRING(state, " ");

					break;
				}

			} else if (Z_TYPE_P(posix) == IS_TRUE && strstr(Z_STRVAL_P(zend_read_property(shlex_ce, self, SHLEX_STRL("quotes"), 1, NULL)), Z_STRVAL_P(nextchar))){

				ZVAL_COPY(state, nextchar);

			} else if (Z_TYPE_P(posix) == IS_TRUE && strstr(Z_STRVAL_P(zend_read_property(shlex_ce, self, SHLEX_STRL("escape"), 1, NULL)), Z_STRVAL_P(nextchar))){

				escapedstate = "a";
				ZVAL_COPY(state, nextchar);

			} else if (strstr(Z_STRVAL_P(zend_read_property(shlex_ce, self, SHLEX_STRL("wordchars"), 1, NULL)), Z_STRVAL_P(nextchar)) || strstr(Z_STRVAL_P(zend_read_property(shlex_ce, self, SHLEX_STRL("quotes"), 1, NULL)), Z_STRVAL_P(nextchar)) || zend_is_true(zend_read_property(shlex_ce, self, SHLEX_STRL("whitespaceSplit"), 1, NULL))) {

				new_str = str_join_len1_len2(Z_STRVAL_P(token), Z_STRVAL_P(nextchar), Z_STRLEN_P(token), Z_STRLEN_P(nextchar));
				ZVAL_STRING(token, new_str);
				efree(new_str);

			} else {

				if(zend_is_true(punctuation_chars)){

					add_next_index_zval(_punctuation_chars, nextchar);

				} else {

					zval *pushback = zend_read_property(shlex_ce, self, SHLEX_STRL("pushback"), 1, NULL);
					array_unshift(pushback, nextchar);
				}

				if(debug >= 2){
					php_printf("shlex: I see punctuation in word state");
				}

				ZVAL_STRING(state, " ");

				if(zend_is_true(token) || (Z_TYPE_P(posix) == IS_TRUE || quoted)){

					break;
				} else {
					continue;
				}
			}
		}
	}

	zval result;
	ZVAL_COPY(&result, token);
	ZVAL_STRING(token, "");

	if(Z_TYPE_P(posix) == IS_TRUE && !quoted && strcmp(Z_STRVAL(result), "") == 0){
		convert_to_null(&result);
	}

	if(debug >= 1){

		if(zend_is_true(&result)){
			smart_str *buf = emalloc(sizeof(smart_str));
			bzero(buf, sizeof(smart_str));
			php_var_export_ex(&result, 1, buf);
			php_printf("shlex: raw token=%s", ZSTR_VAL(buf->s));
		} else {
			php_printf("shlex: raw token=EOF");
		}
	}

	ZVAL_COPY(return_value, &result);
}

static void shlex_push_source(zval *self, zval *newstream, zval *newfile){

	zval *filestack = zend_read_property(shlex_ce, self, SHLEX_STRL("filestack"), 1, NULL);
	zval *infile = zend_read_property(shlex_ce, self, SHLEX_STRL("infile"), 1, NULL);
	zval *stream = zend_read_property(shlex_ce, self, SHLEX_STRL("stream"), 1, NULL);
	zval *instream = zend_read_property(shlex_ce, self, SHLEX_STRL("instream"), 1, NULL);
	zval *lineno = zend_read_property(shlex_ce, self, SHLEX_STRL("lineno"), 1, NULL);

	shlex_stream *custom_stream = emalloc(sizeof(shlex_stream));
	bzero(custom_stream, sizeof(shlex_stream));
	custom_stream->is_valid = 1;

	if(Z_TYPE_P(newstream) == IS_STRING){
		shlex_str_io_init(newstream, custom_stream, Z_STRVAL_P(newstream), Z_STRLEN_P(newstream));
	} else{
		zval *return_value = newstream;
		php_stream_from_res(custom_stream->stream, Z_RES_P(newstream));
	}

	zval arg2;
	array_init_size(&arg2, 4);
	add_next_index_zval(&arg2, infile);
	add_next_index_zval(&arg2, stream);
	add_next_index_zval(&arg2, instream);
	add_next_index_zval(&arg2, lineno);

	array_unshift(filestack, &arg2);

	ZVAL_COPY(infile, newfile);
	ZVAL_COPY(instream, newstream);
	ZVAL_LONG(lineno, 1);
	ZVAL_PTR(stream, custom_stream);

	zval *debug = zend_read_property(shlex_ce, self, SHLEX_STRL("debug"), 1, NULL);

	if(Z_LVAL_P(debug)){
		smart_str *buf = emalloc(sizeof(smart_str));
		bzero(buf, sizeof(smart_str));

		if(Z_TYPE_P(newfile) != IS_NULL){
			php_var_export_ex(newfile, 1, buf);
			php_printf("shlex: pushing to file %s", ZSTR_VAL(buf->s));
		} else{
			php_var_export_ex(newstream, 1, buf);
			php_printf("shlex: pushing to stream %s", ZSTR_VAL(buf->s));
		}
	}
}

static void shlex_pop_source(zval *self){
	zval *stream = zend_read_property(shlex_ce, self, SHLEX_STRL("stream"), 1, NULL);
	shlex_stream *custom_stream = Z_PTR_P(stream);
	shlex_fclose(custom_stream->stream);

	zval *filestack = zend_read_property(shlex_ce, self, SHLEX_STRL("filestack"), 1, NULL);
	zval *shift_ret = array_shift(filestack);
	zval *infile = zend_hash_index_find(Z_ARRVAL_P(shift_ret), 0);
	zval *new_stream = zend_hash_index_find(Z_ARRVAL_P(shift_ret), 1);
	zval *new_instream = zend_hash_index_find(Z_ARRVAL_P(shift_ret), 2);
	zval *lineno = zend_hash_index_find(Z_ARRVAL_P(shift_ret), 3);

	zend_update_property(shlex_ce, self, SHLEX_STRL("infile"), infile);
	ZVAL_COPY(stream, new_stream);
	zend_update_property(shlex_ce, self, SHLEX_STRL("instream"), new_instream);
	zend_update_property(shlex_ce, self, SHLEX_STRL("lineno"), lineno);

	zval *debug = zend_read_property(shlex_ce, self, SHLEX_STRL("debug"), 1, NULL);

	if(Z_LVAL_P(debug)){
		smart_str *buf1 = emalloc(sizeof(smart_str));
		smart_str *buf2 = emalloc(sizeof(smart_str));
		bzero(buf1, sizeof(smart_str));
		bzero(buf2, sizeof(smart_str));
		php_var_export_ex(new_instream, 1, buf1);
		php_var_export_ex(lineno, 1, buf2);

		php_printf("shlex: popping to %s, line %s", ZSTR_VAL(buf1->s), ZSTR_VAL(buf2->s));
	}

	zend_update_property_string(shlex_ce, self, SHLEX_STRL("state"), " ");
}

static void shlex_get_token(zval *self, zval *return_value){
	zval *pushback = zend_read_property(shlex_ce, self, SHLEX_STRL("pushback"), 1, NULL);

	if(zend_is_true(pushback)){
		zval *tok = array_shift(pushback);
		zval *debug = zend_read_property(shlex_ce, self, SHLEX_STRL("debug"), 1, NULL);

		if(Z_LVAL_P(debug) >= 1){
			smart_str *buf = emalloc(sizeof(smart_str));
			bzero(buf, sizeof(smart_str));
			php_var_export_ex(tok, 1, buf);
			php_printf("shlex: popping token %s", ZSTR_VAL(buf->s));
		}

		RETURN_ZVAL(tok, 1, 0);
	}

	zval raw;
	shlex_read_token(self, &raw);

	zval *source = zend_read_property(shlex_ce, self, SHLEX_STRL("source"), 1, NULL);

	if(Z_TYPE_P(source) != IS_NULL){
		while (zend_is_identical(&raw, source)){
			zval token, spec;
			shlex_read_token(self, &token);
			shlex_sourcehook(self, &spec, Z_STRVAL(token), Z_STRLEN(token));

			if(zend_is_true(&spec)){
				zval *newfile = zend_hash_index_find(Z_ARRVAL(spec), 0);
				zval *newstream = zend_hash_index_find(Z_ARRVAL(spec), 1);
				shlex_push_source(self, newstream, newfile);
			}

			shlex_get_token(self, &raw);
		}
	}

	zval *eof = zend_read_property(shlex_ce, self, SHLEX_STRL("eof"), 1, NULL);

	while (zend_is_identical(&raw, eof)){
		zval *filestack = zend_read_property(shlex_ce, self, SHLEX_STRL("filestack"), 1, NULL);

		if(!zend_is_true(filestack)){
			RETURN_ZVAL(eof, 1, 0);
		} else{
			shlex_pop_source(self);
			shlex_get_token(self, &raw);
		}
	}

	zval *debug = zend_read_property(shlex_ce, self, SHLEX_STRL("debug"), 1, NULL);

	if(Z_LVAL_P(debug) >= 1){
		if(!zend_is_identical(&raw, eof)){
			smart_str *buf = emalloc(sizeof(smart_str));
			bzero(buf, sizeof(smart_str));
			php_var_export_ex(&raw, 1, buf);
			php_printf("shlex: token=%s", ZSTR_VAL(buf->s));
		} else{
			php_printf("shlex: token=EOF");
		}
	}

	ZVAL_COPY(return_value, &raw);
}

static void shlex_sourcehook(zval *self, zval *return_value, char *newfile, size_t newfile_len){
	if(newfile[0] == '"'){
		char *newfile_str = emalloc(newfile_len - 1);
		memcpy(newfile_str, newfile + 1, newfile_len - 2);
		newfile_str[newfile_len - 3] = '\0';
		newfile = newfile_str;
		newfile_len = strlen(newfile_str);
	}

	zval *infile = zend_read_property(shlex_ce, self, SHLEX_STRL("infile"), 1, NULL);

	if(Z_TYPE_P(infile) == IS_STRING && !IS_ABSOLUTE_PATH(newfile, newfile_len)){
		zend_string *dir_path;
		dir_path = zend_string_init(Z_STRVAL_P(infile), Z_STRLEN_P(infile), 0);
		ZSTR_LEN(dir_path) = zend_dirname(Z_STRVAL_P(infile), Z_STRLEN_P(infile));

		newfile = str_join_len1_len2(ZSTR_VAL(dir_path), newfile, ZSTR_LEN(dir_path), newfile_len);
		newfile_len = strlen(newfile);
	}

	zval result;
	array_init_size(&result, 2);
	add_next_index_stringl(&result, newfile, newfile_len);
	zval handle;
	shlex_fopen(&handle, newfile, "r");
	add_next_index_zval(&result, &handle);

	ZVAL_COPY(return_value, &result);
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_shlex_split, 0, 0, 1)
                ZEND_ARG_INFO(0, s)
                ZEND_ARG_INFO(0, comments)
                ZEND_ARG_INFO(0, posix)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_shlex_quote, 0, 0, 1)
                ZEND_ARG_INFO(0, s)
ZEND_END_ARG_INFO()

PHP_FUNCTION(shlex_split) {
	zval *s = NULL;
	zend_bool comments = 0;
	zend_bool posix = 1;

	if (FAILURE == zend_parse_parameters_throw(ZEND_NUM_ARGS(), "z|bb", &s, &comments, &posix)) {
		return;
	}

	zval lex, punctuation_chars;
	object_init_ex(&lex, shlex_ce);
	ZVAL_FALSE(&punctuation_chars);
	shlex_construct(&lex, s, NULL, 0, posix, &punctuation_chars);

	zend_update_property_bool(shlex_ce, &lex, SHLEX_STRL("whitespaceSplit"), 1);

	if(!comments){
		zend_update_property_string(shlex_ce, &lex, SHLEX_STRL("commenters"), "");
	}

	zval list;
	array_init(&list);
	shlex_stream *custom_stream;

	while (1){
		if(!shlex_valid(&lex)){
			break;
		}

		zval token;
		custom_stream = shlex_current(&lex);
		ZVAL_STRINGL(&token, custom_stream->current_token, custom_stream->current_token_len);

		add_next_index_zval(&list, &token);
	}

	RETURN_ZVAL(&list, 1, 0);
}

PHP_FUNCTION(shlex_quote) {
	zval *s = NULL;

	if (FAILURE == zend_parse_parameters_throw(ZEND_NUM_ARGS(), "z", &s)) {
		return;
	}

	if(!zend_is_true(s)){
		RETURN_STRING("''");
	}

	zval pattern;
	ZVAL_STRING(&pattern, "/[^\\w@%+=:,.\\/-]/");

	zval *ret = call_function_with_2_params("preg_match", &pattern, s);

	if(Z_LVAL_P(ret) == 0){
		RETURN_ZVAL(s, 1, 0);
	}

	zval arg1, arg2;
	ZVAL_STRING(&arg1, "'");
	ZVAL_STRING(&arg2, "'\"'\"'");
	ret = call_function_with_3_params("str_replace", &arg1, &arg2, s);

	char *format = "'%s'";
	size_t all_size = Z_STRLEN_P(ret) + strlen(format) -2 + 1;

	char *tmp = emalloc(all_size);
	bzero(tmp, all_size);
	snprintf(tmp, all_size, format, Z_STRVAL_P(ret));

	RETVAL_STRING(tmp);
	efree(tmp);
}

const zend_function_entry shlex_functions[] = {
		PHP_FE(shlex_split,	arginfo_shlex_split)
		PHP_FE(shlex_quote,	arginfo_shlex_quote)
		PHP_FE_END
};

static void shlex_register_class(void);

PHP_MINIT_FUNCTION(shlex) {
	shlex_register_class();
	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(shlex) {
	return SUCCESS;
}

PHP_RINIT_FUNCTION(shlex) {
#if defined(COMPILE_DL_SHLEX) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif
	return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(shlex) {
	return SUCCESS;
}

PHP_MINFO_FUNCTION(shlex) {
	php_info_print_table_start();
	php_info_print_table_header(2, "shlex support", "enabled");
	php_info_print_table_row(2, "Version", PHP_SHLEX_VERSION);
	php_info_print_table_end();
}

zend_module_entry shlex_module_entry = {
	STANDARD_MODULE_HEADER,
	"shlex",
	shlex_functions,
	PHP_MINIT(shlex),
	PHP_MSHUTDOWN(shlex),
	NULL,
	NULL,
	PHP_MINFO(shlex),
	PHP_SHLEX_VERSION,
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_SHLEX
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(shlex)
#endif

// ------------------------ Shlex ------------------------

ZEND_BEGIN_ARG_INFO_EX(shlex__construct_arginfo, 0, 0, 0)
                ZEND_ARG_INFO(0, instream)
                ZEND_ARG_INFO(0, infile)
                ZEND_ARG_INFO(0, posix)
                ZEND_ARG_INFO(0, punctuation_chars)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(shlex__destruct_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(shlex_key_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(shlex_next_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(shlex_rewind_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(shlex_current_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(shlex_valid_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(shlex_push_token_arginfo, 0, 0, 1)
                ZEND_ARG_INFO(0, tok)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(shlex_push_source_arginfo, 0, 0, 1)
                ZEND_ARG_INFO(0, newstream)
                ZEND_ARG_INFO(0, newfil)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(shlex_pop_source_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(shlex_get_token_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(shlex_read_token_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(shlex_sourcehook_arginfo, 0, 0, 1)
                ZEND_ARG_INFO(0, newfile)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(shlex_error_leader_arginfo, 0, 0, 0)
                ZEND_ARG_INFO(0, infile)
                ZEND_ARG_INFO(0, ineno)
ZEND_END_ARG_INFO()

PHP_METHOD(shlex, __construct) {
	zval *instream = NULL;
	char *infile = NULL;
	size_t infile_len = 0;
	zend_bool posix = 0;
	zval *punctuation_chars, punctuation_chars_z;
	punctuation_chars = &punctuation_chars_z;
	ZVAL_FALSE(&punctuation_chars_z);

	if (FAILURE == zend_parse_parameters_throw(ZEND_NUM_ARGS(), "|zsbz", &instream, &infile, &infile_len, &posix, &punctuation_chars)) {
		return;
	}

	shlex_construct(getThis(), instream, infile, infile_len, posix, punctuation_chars);
}

PHP_METHOD(shlex, __destruct) {
	zval *stream = zend_read_property(shlex_ce, getThis(), SHLEX_STRL("stream"), 1, NULL);
	shlex_stream *custom_stream = Z_PTR_P(stream);
	shlex_fclose(custom_stream->stream);
	efree(custom_stream);
}

PHP_METHOD(shlex, key) {}

PHP_METHOD(shlex, next) {}

PHP_METHOD(shlex, rewind) {}

PHP_METHOD(shlex, current) {
	shlex_stream *custom_stream = shlex_current(getThis());
	RETURN_STRINGL(custom_stream->current_token, custom_stream->current_token_len);
}

PHP_METHOD(shlex, valid) {
	RETURN_BOOL(shlex_valid(getThis()));
}

PHP_METHOD(shlex, pushToken) {
	zval *tok = NULL;

	if (FAILURE == zend_parse_parameters_throw(ZEND_NUM_ARGS(), "z", &tok)) {
		return;
	}

	zval *debug = zend_read_property(shlex_ce, getThis(), SHLEX_STRL("debug"), 1, NULL);

	if(Z_LVAL_P(debug) >= 1){
		smart_str *buf = emalloc(sizeof(smart_str));
		bzero(buf, sizeof(smart_str));
		php_var_export_ex(tok, 1, buf);
		php_printf("shlex: pushing token %s", ZSTR_VAL(buf->s));
	}

	zval *pushback = zend_read_property(shlex_ce, getThis(), SHLEX_STRL("pushback"), 1, NULL);
	array_unshift(pushback, tok);
}

PHP_METHOD(shlex, pushSource) {
	zval *newstream = NULL;
	zval *newfile, newfile_z;
	newfile = &newfile_z;
	ZVAL_NULL(&newfile_z);

	if (FAILURE == zend_parse_parameters_throw(ZEND_NUM_ARGS(), "z|z", &newstream, &newfile)) {
		return;
	}

	shlex_push_source(getThis(), newstream, newfile);
}

PHP_METHOD(shlex, popSource) {
	shlex_pop_source(getThis());
}

PHP_METHOD(shlex, getToken) {
	shlex_get_token(getThis(), return_value);
}

PHP_METHOD(shlex, readToken) {
	shlex_read_token(getThis(), return_value);
}

PHP_METHOD(shlex, sourcehook) {

	char *newfile = NULL;
	size_t newfile_len = 0;

	if (FAILURE == zend_parse_parameters_throw(ZEND_NUM_ARGS(), "s", &newfile, &newfile_len)) {
		return;
	}

	shlex_sourcehook(getThis(), return_value, newfile, newfile_len);
}

PHP_METHOD(shlex, errorLeader) {
	zval *infile, *lineno, infile_z, lineno_z;
	infile = &infile_z;
	lineno = &lineno_z;
	ZVAL_NULL(&infile_z);
	ZVAL_NULL(&lineno_z);

	if (FAILURE == zend_parse_parameters_throw(ZEND_NUM_ARGS(), "|zz", &infile, &lineno)) {
		return;
	}

	if(Z_TYPE_P(infile) == IS_NULL){
		infile = zend_read_property(shlex_ce, getThis(), SHLEX_STRL("infile"), 1, NULL);
	}

	if(Z_TYPE_P(lineno) == IS_NULL){
		lineno = zend_read_property(shlex_ce, getThis(), SHLEX_STRL("lineno"), 1, NULL);
		zval lineno_tmp;
		ZVAL_COPY(&lineno_tmp, lineno);
		lineno = &lineno_tmp;
	}

	char *format = "\"%s\", line %s: ";
	convert_to_string(lineno);

	size_t ret_size = strlen(format) + Z_STRLEN_P(infile) + Z_STRLEN_P(lineno) - 4 + 1; // 2个%s总共4个字节
	char *ret = emalloc(ret_size);
	bzero(ret, ret_size);
	snprintf(ret, ret_size, format, Z_STRVAL_P(infile), Z_STRVAL_P(lineno));

	RETVAL_STRINGL(ret, ret_size - 1);
	efree(ret);
}

const zend_function_entry shlex_methods[] = {
        PHP_ME(shlex, __construct, shlex__construct_arginfo, ZEND_ACC_CTOR|ZEND_ACC_PUBLIC)
        PHP_ME(shlex, __destruct, shlex__destruct_arginfo, ZEND_ACC_DTOR|ZEND_ACC_PUBLIC)
        PHP_ME(shlex, key, shlex_key_arginfo, ZEND_ACC_PUBLIC)
        PHP_ME(shlex, next, shlex_next_arginfo, ZEND_ACC_PUBLIC)
        PHP_ME(shlex, rewind, shlex_rewind_arginfo, ZEND_ACC_PUBLIC)
        PHP_ME(shlex, current, shlex_current_arginfo, ZEND_ACC_PUBLIC)
        PHP_ME(shlex, valid, shlex_valid_arginfo, ZEND_ACC_PUBLIC)
        PHP_ME(shlex, pushToken, shlex_push_token_arginfo, ZEND_ACC_PUBLIC)
        PHP_ME(shlex, pushSource, shlex_push_source_arginfo, ZEND_ACC_PUBLIC)
        PHP_ME(shlex, popSource, shlex_pop_source_arginfo, ZEND_ACC_PUBLIC)
        PHP_ME(shlex, getToken, shlex_get_token_arginfo, ZEND_ACC_PUBLIC)
        PHP_ME(shlex, readToken, shlex_read_token_arginfo, ZEND_ACC_PUBLIC)
        PHP_ME(shlex, sourcehook, shlex_sourcehook_arginfo, ZEND_ACC_PUBLIC)
        PHP_ME(shlex, errorLeader, shlex_error_leader_arginfo, ZEND_ACC_PUBLIC)
        PHP_FE_END
};

const zend_function_entry shlex_exception_methods[] = {
		PHP_FE_END
};

static void shlex_register_class(void) {
	zend_class_entry ce_shlex, ce_shlex_exception;

	INIT_CLASS_ENTRY(ce_shlex, "Shlex", shlex_methods);
	shlex_ce = zend_register_internal_class(&ce_shlex);
	zend_class_implements(shlex_ce, 1, zend_ce_iterator);
	zend_declare_property_null(shlex_ce, SHLEX_STRL("instream"), ZEND_ACC_PUBLIC);
	zend_declare_property_null(shlex_ce, SHLEX_STRL("stream"), ZEND_ACC_PRIVATE);
	zend_declare_property_null(shlex_ce, SHLEX_STRL("infile"), ZEND_ACC_PUBLIC);
	zend_declare_property_null(shlex_ce, SHLEX_STRL("posix"), ZEND_ACC_PRIVATE);
	zend_declare_property_null(shlex_ce, SHLEX_STRL("eof"), ZEND_ACC_PUBLIC);
	zend_declare_property_string(shlex_ce, SHLEX_STRL("commenters"), "#", ZEND_ACC_PUBLIC);
	zend_declare_property_string(shlex_ce, SHLEX_STRL("wordchars"), "abcdfeghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_", ZEND_ACC_PUBLIC);
	zend_declare_property_string(shlex_ce, SHLEX_STRL("whitespace"), " \t\r\n", ZEND_ACC_PUBLIC);
	zend_declare_property_bool(shlex_ce, SHLEX_STRL("whitespaceSplit"), 0, ZEND_ACC_PUBLIC);
	zend_declare_property_string(shlex_ce, SHLEX_STRL("quotes"), "'\"", ZEND_ACC_PUBLIC);
	zend_declare_property_string(shlex_ce, SHLEX_STRL("escape"), "\\", ZEND_ACC_PUBLIC);
	zend_declare_property_string(shlex_ce, SHLEX_STRL("escapedquotes"), "\"", ZEND_ACC_PUBLIC);
	zend_declare_property_string(shlex_ce, SHLEX_STRL("state"), " ", ZEND_ACC_PRIVATE);
	zend_declare_property_null(shlex_ce, SHLEX_STRL("pushback"), ZEND_ACC_PRIVATE);
	zend_declare_property_long(shlex_ce, SHLEX_STRL("lineno"), 1, ZEND_ACC_PUBLIC);
	zend_declare_property_long(shlex_ce, SHLEX_STRL("debug"), 0, ZEND_ACC_PUBLIC);
	zend_declare_property_string(shlex_ce, SHLEX_STRL("token"), "", ZEND_ACC_PUBLIC);
	zend_declare_property_null(shlex_ce, SHLEX_STRL("filestack"), ZEND_ACC_PRIVATE);
	zend_declare_property_null(shlex_ce, SHLEX_STRL("source"), ZEND_ACC_PUBLIC);
	zend_declare_property_null(shlex_ce, SHLEX_STRL("punctuationChars"), ZEND_ACC_PUBLIC);
	zend_declare_property_null(shlex_ce, SHLEX_STRL("_punctuationChars"), ZEND_ACC_PRIVATE);

	INIT_CLASS_ENTRY(ce_shlex_exception, "ShlexException", shlex_exception_methods);
	shlex_exception_ce = zend_register_internal_class_ex(&ce_shlex_exception, zend_ce_exception);
}