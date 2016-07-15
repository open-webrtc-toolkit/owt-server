/**
 * @file main.h  Internal interface to main polling loop
 *
 * Copyright (C) 2010 Creytiv.com
 */


#ifdef HAVE_EPOLL
bool epoll_check(void);
#endif


#ifdef __cplusplus
extern "C" {
#endif

#ifdef HAVE_ACTSCHED
void actsched_init(void);
int  actsched_start(void);
void actsched_stop(void);
void actsched_restart_timer(void);
#endif

#ifdef USE_OPENSSL
int  openssl_init(void);
void openssl_close(void);
#endif

#ifdef __cplusplus
}
#endif
