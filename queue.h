 /** \file queue.c
       \authors Gabriele Barreca e Luca Murgia
     Si dichiara che il contenuto di questo file e' in ogni sua parte opera
     originale dell' autore.  */
#ifndef QUEUE_H_
#define QUEUE_H_

#include<pthread.h>
#include <erch.h>

/* Struttura coda */
typedef struct connqueue	{
	long *queue;	
	int head;
	int len;
}connqueue;


extern pthread_mutex_t qlock;
extern pthread_cond_t  qcond;

/* Variabile globale ceh verrà settata da SIGQUIT*/
extern volatile int run;

/* Alloca ed inizializza una coda */
connqueue* new_queue(connqueue * Q, int dim1);

/* Controlla se la coda è piena */
int Full_queue(connqueue *Q,int dim1,int dim2);

/* Cotnrollo se la coda è vuota*/
int Empty_queue(connqueue * Q);


/* Inserisce un dato nella coda. */
int push_queue(long sd,connqueue *Q,int dim1);

/* Estrae una connessione dalla coda e ne restituisce il file descriptor */
long pop_queue(connqueue *Q, int dim1);


#endif /* QUEUE_H_ */
