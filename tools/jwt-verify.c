/* Copyright (C) 2019 Jeremy Thien <jeremy.thien@gmail.com>
   Copyright (C) 2024-2025 maClara, LLC <info@maclara-llc.com>
   This file is part of the JWT C Library

   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdio.h>
#include <stdlib.h>
#include <jwt.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <libgen.h>

extern const char *__progname;

#define MAX_LINE_LENGTH 1024

_Noreturn static void usage(const char *error, int exit_state)
{
	if (error)
		fprintf(stderr, "ERROR: %s\n\n", error);

	fprintf(stderr, "Usage: %s [OPTIONS] <token> [token(s)]\n",
		__progname);
	fprintf(stderr, "       %s [OPTIONS] -\n\n", __progname);
	fprintf(stderr, "Decode and (optionally) verify the signature for a JSON Web Token\n\n");
	fprintf(stderr, "  -h, --help            This help information\n");
	fprintf(stderr, "  -l, --list            List supported algorithms and exit\n");
	fprintf(stderr, "  -a, --algorithm=ALG   JWT algorithm to use (e.g. ES256). Only needed if the key\n");
	fprintf(stderr, "                        provided with -k does not have an \"alg\" attribute\n");
	fprintf(stderr, "  -k, --key=FILE        Filename containing a JSON Web Key\n");
	fprintf(stderr, "  -v, --verbose         Show decoded header and payload while verifying\n\n");
	fprintf(stderr, "This program will decode and validate each token on the command line.\n");
	fprintf(stderr, "If - is given as the only argument to token, then tokens will be read\n");
	fprintf(stderr, "from stdin, one per line.\n\n");
	fprintf(stderr, "If you need to convert a key to JWT (e.g. from PEM or DER format) see key2jwk(1).\n");

	exit(exit_state);
}

static int __verify_wcb(jwt_t *jwt, jwt_config_t *config)
{
	jwt_value_t jval;
	int ret;

	if (config == NULL)
		return 1;

	jwt_set_GET_JSON(&jval, NULL);
	jval.pretty = 1;
	ret = jwt_header_get(jwt, &jval);
	if (!ret) {
		printf("\033[0;95m[HEADER]\033[0m\n\033[0;96m%s\033[0m\n",
		       jval.json_val);
		free(jval.json_val);
	}

	jwt_set_GET_JSON(&jval, NULL);
	jval.pretty = 1;
	ret = jwt_grant_get(jwt, &jval);
	if (!ret) {
		printf("\033[0;95m[PAYLOAD]\033[0m\n\033[0;96m%s\033[0m\n",
		       jval.json_val);
		free(jval.json_val);
	}
	return 0;
}

static int process_one(jwt_checker_t *checker, jwt_alg_t alg, const char *token)
{
	int err = 0;

	printf("\n%s %s[TOK]\033[0m %.*s%s\n", alg == JWT_ALG_NONE ?
		"\xF0\x9F\x94\x93" : "\xF0\x9F\x94\x90",
		alg == JWT_ALG_NONE ? "\033[0;93m" : "\033[0;92m",
		60, token, strlen(token) > 60 ? "..." : "");

	if (jwt_checker_verify(checker, token)) {
		printf("\xF0\x9F\x91\x8E \033[0;91m[BAD]\033[0m %s\n",
			jwt_checker_error_msg(checker));
		err = 1;
	} else {
		printf("\xF0\x9F\x91\x8D \033[0;92m[YES]\033[0m Verfified\n");
	}

	return err;
}

int main(int argc, char *argv[])
{
	jwt_checker_auto_t *checker = NULL;
	const char *key_file = NULL;
	jwt_alg_t alg = JWT_ALG_NONE;
	jwk_set_auto_t *jwk_set = NULL;
	const jwk_item_t *item = NULL;
	int oc, err, verbose = 0;

	checker = jwt_checker_new();
	if (checker == NULL) {
		fprintf(stderr, "Could not allocate checker context\n");
		exit(EXIT_FAILURE);
	}

	char *optstr = "hk:alv";
	struct option opttbl[] = {
		{ "help",	no_argument,		NULL, 'h' },
		{ "key",	required_argument,	NULL, 'k' },
		{ "algorithm",	required_argument,	NULL, 'a' },
		{ "list",	no_argument,		NULL, 'l' },
		{ "verbose",	no_argument,		NULL, 'v' },
		{ NULL, 0, 0, 0 },
	};

	while ((oc = getopt_long(argc, argv, optstr, opttbl, NULL)) != -1) {
		switch (oc) {
		case 'h':
			usage(NULL, EXIT_SUCCESS);

		case 'l':
			printf("Algorithms supported:\n");
			for (alg = JWT_ALG_NONE; alg < JWT_ALG_INVAL; alg++)
				printf("    %s\n", jwt_alg_str(alg));
			exit(EXIT_SUCCESS);
			break;

		case 'v':
			verbose = 1;
			break;

		case 'k':
			key_file = optarg;
			break;

		case 'a':
			alg = jwt_str_alg(optarg);
			if (alg >= JWT_ALG_INVAL) {
				usage("Unknown algorithm (use -l to see a list of "
				      "supported algorithms)\n", EXIT_FAILURE);

			}
			break;

		default: /* '?' */
			usage("Uknown option", EXIT_FAILURE);
			break;
		}
	}

	argc -= optind;
	argv += optind;

	if (argc == 0)
		usage("No token(s) given", EXIT_FAILURE);

	if (key_file == NULL && alg != JWT_ALG_NONE)
		usage("An algorithm other than 'none' requires a key",
		      EXIT_FAILURE);

	/* Load JWK key */
	if (key_file) {
		jwk_set = jwks_create_fromfile(key_file);
                if (jwk_set == NULL || jwks_error(jwk_set)) {
                        fprintf(stderr, "ERR: Could not read JWK: %s\n",
                                jwks_error_msg(jwk_set));
                        exit(EXIT_FAILURE);
                }

                /* Get the first key */
                item = jwks_item_get(jwk_set, 0);
                if (jwks_item_error(item)) {
                        fprintf(stderr, "ERR: Could not read JWK: %s\n",
                                jwks_item_error_msg(item));
                        exit(EXIT_FAILURE);
                }

		if (jwks_item_alg(item) == JWT_ALG_NONE &&
		    alg == JWT_ALG_NONE) {
			usage("Key does not contain an \"alg\" attribute and no --alg given",
			      EXIT_FAILURE);
		}

                if (jwt_checker_setkey(checker, alg, item)) {
                        fprintf(stderr, "ERR Loading key: %s\n",
                                jwt_checker_error_msg(checker));
                        exit(EXIT_FAILURE);
                }
	}

	if (verbose && jwt_checker_setcb(checker, __verify_wcb, NULL)) {
		fprintf(stderr, "ERR setting callback: %s\n",
			jwt_checker_error_msg(checker));
		exit(EXIT_FAILURE);
	}

	if (item)
		printf("\xF0\x9F\x94\x91 \033[0;92m[KEY]\033[0m %s\n",
		       key_file);
	printf("\xF0\x9F\x93\x83 ");
	if (item && jwks_item_alg(item) != JWT_ALG_NONE) {
		printf("\033[0;92m[ALG]\033[0m %s (from key)",
			jwt_alg_str(jwks_item_alg(item)));
		alg = jwks_item_alg(item);
	} else {
		printf("\033[0;91m[ALG]\033[0m %s", jwt_alg_str(alg));
	}
	printf("\n");

	err = 0;

	if (!strcmp(argv[0], "-")) {
		char token[MAX_LINE_LENGTH];
		while (fgets(token, MAX_LINE_LENGTH, stdin) != NULL) {
			token[strcspn(token, "\n")] = '\0';

			err += process_one(checker, alg, token);
		}
	} else {
		for (oc = 0; oc < argc; oc++) {
			const char *token = argv[oc];

			err += process_one(checker, alg, token);
		}
	}

	exit(err);
}

