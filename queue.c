 /** \file queue.c
       \authors Gabriele Barreca e Luca Murgia
     Si dichiara che il contenuto di questo file e' in ogni sua parte opera
     originale dell' autore.  */
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> 
#include <sys/types.h>
#include <sys/un.h>
#include <string.h>
#include <unistd.h>
#include <queue.h>
#include <pthread.h>


pthread_mutex_t qlock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  qcond = PTHREAD_COND_INITIALIZER;
volatile int run;

static void LockQueue(){ 
	pthread_mutex_lock(&qlock);   
}

static void UnlockQueue(){ 
	pthread_mutex_unlock(&qlock); 
}

static void UnlockQueueAndWait(){ 
	pthread_cond_wait(&qcond, &qlock); 	
}

static void UnlockQueueAndSignal() {
 pthread_cond_signal(&qcond);
 pthread_mutex_unlock(&qlock);
}


/*Crea una nuova coda*/
connqueue* new_queue(connqueue * Q, int dim1){
	ec_null_alloc_n((Q=malloc (sizeof(connqueue))));
	ec_null_alloc_n((Q->queue= malloc((dim1)*sizeof(long))));
	Q->head=0;
	Q->len=0;
	return Q;
}

/*Controlla se la coda è vuota*/
int Empty_queue(connqueue * Q){
	if(Q->len == 0){
		return 1;
	}
	return 0;
}

int Full_queue(connqueue *Q,int dim1,int dim2){
LockQueue();
	if(Q->len == (dim1-dim2)){
		UnlockQueue();
		return 0;
	}
UnlockQueue();
return 1;
}

/* Inserisce un elemento in fondo alla coda */
int push_queue(long sd,connqueue *Q,int dim1){
	LockQueue();
	if((Q->len) == dim1){
		UnlockQueue();
		return -1;
	}
	else{
		Q->queue[(Q->head+Q->len)%dim1] = sd;
		Q->len++;
		UnlockQueueAndSignal();
		return 0;
	}
}

/* Rimuove l'elemento in testa alla coda. Se la coda è vuota aspetta che ci sia almeno un elemento, oppure esce in caso venga intercettato un segnale di chiusura */
long pop_queue(connqueue *Q,int dim1){
	LockQueue();
  while(Empty_queue(Q)) {
		UnlockQueueAndWait();
		if(run==0){
			UnlockQueue();
			return -1;
			}
	}
	if(!Empty_queue(Q)){
		long p;
		p = (long)Q->queue[Q->head];	
		Q->head = (Q->head+1)%dim1;
		Q->len--;
	 	UnlockQueue();
		return p;
	}
	else
		return -1;
}

