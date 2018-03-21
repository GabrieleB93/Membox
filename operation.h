 /** \file operation.h
       \authors Gabriele Barreca e Luca Murgia
     Si dichiara che il contenuto di questo file e' in ogni sua parte opera
     originale dell' autore.  */

#if !defined(OPERATION_T)
#define OPERATION_T
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>	
#include <icl_hash.h>
#include <connections.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>
#include <ops.h>
#include <stats.h>
#include <pthread.h>
#include <message.h>
#include <unistd.h>	
#include <sys/types.h>
#include <sys/un.h>
#include <erch.h>

#define MAXLENGTH 1000
#define BUCKETS 10000

extern struct statistics  mboxStats;
extern char *UnixPath ;
extern int MaxConnections;
extern int ThreadsInPool;
extern int StorageSize;    
extern int StorageByteSize;
extern int MaxObjSize;
extern char *StatFileName;
extern pthread_mutex_t statlock;
extern pthread_mutex_t Oplock;
extern pthread_t flag_op;

int conf (FILE * fp);
op_t putop (icl_hash_t *rep, membox_key_t *key,message_data_t* data);
op_t updateop (icl_hash_t *rep,membox_key_t *key,message_data_t* data) ;
op_t getop (icl_hash_t *rep, membox_key_t *key,message_data_t* data);
op_t removeop(icl_hash_t *rep, membox_key_t *key, message_data_t* data);    
void freeData(void * data);
void freeKey(void * key);
unsigned int fnv_hash_function( void *key, int len ); 
unsigned int ulong_hash_function( void *key );
op_t unlockop();
op_t lockop();       
message_t * execute_op( message_t * msg, icl_hash_t * rep, pthread_mutex_t *locks);

#endif
