/* Public domain, no copyright. Use at your own risk. */

#ifndef JWT_TESTS_H
#define JWT_TESTS_H

#include <jwt.h>
#include <check.h>

#include "config.h"

/* Compatibility with older libCheck versions */
/* Older check doesn't have this. */
#ifndef ck_assert_ptr_ne
#define ck_assert_ptr_ne(X, Y) ck_assert(X != Y)
#define ck_assert_ptr_eq(X, Y) ck_assert(X == Y)
#endif

#ifndef ck_assert_int_gt
#define ck_assert_int_gt(X, Y) ck_assert(X > Y)
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))
#endif

/* Constant time to make tests consistent. */
#define TS_CONST	1475980545L

__attribute__((unused))
static const char *jwt_test_ops[] = {
#ifdef HAVE_OPENSSL
	"openssl",
#endif
#ifdef HAVE_GNUTLS
	"gnutls",
#endif
	NULL
};

/* Macro to allocate a new JWT with checks. */
#define ALLOC_JWT(__jwt) do {		\
	int __ret = jwt_new(__jwt);	\
	ck_assert_int_eq(__ret, 0);	\
	ck_assert_ptr_ne(__jwt, NULL);	\
} while(0)

#define JWT_TEST_MAIN(__title) ({					\
	int number_failed = 0;						\
	SRunner *sr;							\
	Suite *s;							\
									\
	s = libjwt_suite(__title);					\
	sr = srunner_create(s);						\
									\
	srunner_run_all(sr, CK_VERBOSE);				\
	number_failed += srunner_ntests_failed(sr);			\
	srunner_free(sr);						\
									\
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;	\
})

#define SET_OPS() ({							\
	ck_assert_int_eq(jwt_set_crypto_ops(jwt_test_ops[_i]), 0);	\
	const char *ops = jwt_get_crypto_ops();				\
	ck_assert_str_eq(ops, jwt_test_ops[_i]);			\
})

__attribute__((unused)) static unsigned char key[16384];
__attribute__((unused)) static size_t key_len;

__attribute__((unused))
static void read_key(const char *key_file)
{
	FILE *fp = fopen(key_file, "r");
	char *key_path;
	int ret = 0;

	ret = asprintf(&key_path, KEYDIR "/%s", key_file);
	ck_assert_int_gt(ret, 0);

	fp = fopen(key_path, "r");
	ck_assert_ptr_ne(fp, NULL);

	jwt_free_str(key_path);

	key_len = fread(key, 1, sizeof(key), fp);
	ck_assert_int_ne(key_len, 0);

	ck_assert_int_eq(ferror(fp), 0);

	fclose(fp);

	key[key_len] = '\0';
}

__attribute__((unused))
static void __verify_jwt(const char *jwt_str, const jwt_alg_t alg, const char *file)
{
	jwt_t *jwt = NULL;
	int ret = 0;

	read_key(file);

	ret = jwt_decode(&jwt, jwt_str, key, key_len);
	ck_assert_int_eq(ret, 0);
	ck_assert_ptr_ne(jwt, NULL);

	ck_assert(jwt_get_alg(jwt) == alg);

	jwt_free(jwt);
}

__attribute__((unused))
static void __test_alg_key(const jwt_alg_t alg, const char *file, const char *pub)
{
	jwt_t *jwt = NULL;
	int ret = 0;
	char *out;

	ALLOC_JWT(&jwt);

	read_key(file);

	ret = jwt_add_grant(jwt, "iss", "files.maclara-llc.com");
	ck_assert_int_eq(ret, 0);

	ret = jwt_add_grant(jwt, "sub", "user0");
	ck_assert_int_eq(ret, 0);

	ret = jwt_add_grant(jwt, "ref", "XXXX-YYYY-ZZZZ-AAAA-CCCC");
	ck_assert_int_eq(ret, 0);

	ret = jwt_add_grant_int(jwt, "iat", TS_CONST);
	ck_assert_int_eq(ret, 0);

	ret = jwt_set_alg(jwt, alg, key, key_len);
	ck_assert_int_eq(ret, 0);

	out = jwt_encode_str(jwt);
	ck_assert_ptr_ne(out, NULL);

	__verify_jwt(out, alg, pub);

	jwt_free_str(out);
	jwt_free(jwt);
}

__attribute__((unused))
static void __verify_alg_key(const char *key_file, const char *jwt_str,
			     const jwt_alg_t alg)
{
	jwt_valid_t *jwt_valid = NULL;
	jwt_t *jwt = NULL;
	int ret = 0;

	read_key(key_file);

	ret = jwt_decode(&jwt, jwt_str, key, key_len);
	ck_assert_int_eq(ret, 0);
	ck_assert_ptr_nonnull(jwt);

	jwt_valid_new(&jwt_valid, alg);

	ret = jwt_validate(jwt, jwt_valid);
	ck_assert_int_eq(JWT_VALIDATION_SUCCESS, ret);

	jwt_valid_free(jwt_valid);
	jwt_free(jwt);
}

__attribute__((unused))
static void __compare_alg_key(const char *key_file, const char *jwt_str,
			      const jwt_alg_t alg)
{
	jwt_t *jwt = NULL;
	int ret = 0;
	char *out;

	ALLOC_JWT(&jwt);

	read_key(key_file);

	ret = jwt_add_grant(jwt, "iss", "files.maclara-llc.com");
	ck_assert_int_eq(ret, 0);

	ret = jwt_add_grant(jwt, "sub", "user0");
	ck_assert_int_eq(ret, 0);

	ret = jwt_add_grant(jwt, "ref", "XXXX-YYYY-ZZZZ-AAAA-CCCC");
	ck_assert_int_eq(ret, 0);

	ret = jwt_add_grant_int(jwt, "iat", TS_CONST);
	ck_assert_int_eq(ret, 0);

	ret = jwt_set_alg(jwt, alg, key, key_len);
	ck_assert_int_eq(ret, 0);

	out = jwt_encode_str(jwt);
	ck_assert_ptr_ne(out, NULL);

	ck_assert_str_eq(out, jwt_str);

	jwt_free_str(out);
	jwt_free(jwt);
}

#endif /* JWT_TESTS_H */
