 /** \file erch.h
       \authors Gabriele Barreca e Luca Murgia
     Si dichiara che il contenuto di questo file e' in ogni sua parte opera
     originale dell' autore.  */
     
#ifndef _ECMACRO_H
#define _ECMACRO_H
#include<errno.h>

#define ec_null_alloc(x) if(x == NULL) {errno = ENOMEM; return -1;}
#define ec_null_alloc_n(x) if(x == NULL) {errno = ENOMEM; return NULL;}
#define ec_null_alloc_exit(x) if(x == NULL) {errno = ENOMEM; pthread_exit(NULL);}
#define ec_null_alloc_main(x) if(x == NULL) {errno = ENOMEM; exit(EXIT_FAILURE);}

#endif
