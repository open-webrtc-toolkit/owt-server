/**
 * @file main.c  Main application code
 *
 * Copyright (C) 2010 - 2011 Creytiv.com
 */
#ifdef SOLARIS
#define __EXTENSIONS__ 1
#endif
#include <stdlib.h>
#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_GETOPT
#include <getopt.h>
#endif

#include <sipua.h>

static int mock_gw;
int main(int argc, char *argv[])
{
	int err = -1;
	struct sipua_entity *se1 = NULL;
	struct sipua_entity *se3 = NULL;
	const char *cfg1[4] = {"10.239.34.6", "user1", "pwd1", "U1"};
	const char *cfg3[4] = {"10.239.34.6", "user3", "pwd3", "U3"};

	char * callee2 = "user2@10.239.34.6";

	(void)argc;
	(void)argv;

	log_enable_debug(true);
	log_enable_stderr(true);

	err = sipua_mod_init();
	if (err){
		printf("SIPUA module init failed.\n");
	}

	for(;;){
		char c = 0;
		printf("Please enter:\n");
		scanf("%c", &c);
		if (c == '1'){
			sipua_new(&se1, &mock_gw, cfg1[0], cfg1[1], cfg1[2], cfg1[3]);
		}else if (c == '2') {
			sipua_delete(se1);
		}else if (c == '3') {
			sipua_new(&se3, &mock_gw, cfg3[0], cfg3[1], cfg3[2], cfg3[3]);
		}else if (c == '4') {
			sipua_delete(se3);
		}else if (c == 'c') {
			sipua_call(se1, true, false, callee2);
		}else if (c == 'h') {
			sipua_hangup(se1);
		}else if (c == 'a') {
			sipua_accept(se1);
		}else if (c == 'q'){
			break;
		}else{
			continue;
		}
	}

	sipua_mod_close();
	return err;
}
