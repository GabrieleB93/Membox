/*
 * membox Progetto del corso di LSO 2016 
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Pelagatti, Torquati
 * 
 */
/**
 * @file membox.c
 * @brief File principale del server membox
 */
 
 
#define _POSIX_C_SOURCE 200809L
#define MAXLENGTH 1000
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
    
/* inserire gli altri include che servono */
#include <connections.h>
#include <errno.h>
#include <getopt.h>
#include <fcntl.h>
#include <time.h>
#include <queue.h>
#include <operation.h>
#include <erch.h>

struct sigaction sa;
volatile int go;

/*Struttura argomenti pthread_create*/
typedef struct args {
    connqueue *Q;
    icl_hash_t *H;
    pthread_mutex_t *L;
}args;

/*Funzione per Hash*/
static inline int ulong_key_compare( void *key1, void *key2  ) {
    return ( *(unsigned long*)key1 == *(unsigned long*)key2 );
}

/*Worker per gestire le connessioni e le operazioni*/

void * worker (void * t){
	
	long con;
	message_t *msg;
	message_t *rplmsg;
	args *argo = (args*)t;
	ec_null_alloc_exit((msg = malloc(sizeof(message_t))));
	
		
	while(run){
		
		con=pop_queue(argo->Q,MaxConnections);

		if(run==0){
			free(msg);
			pthread_exit(NULL);
		}
		
			pthread_mutex_lock(&statlock);
				mboxStats.concurrent_connections++;
			pthread_mutex_unlock(&statlock);
			
		/* Inizio a leggere i messaggi del client */		
		if(con!=-1){
			while((readReply(con,msg))!=-1 && go){		

				if(go==0){
					free(msg);
					pthread_exit(NULL);
				}
				
				/* Controllo lo stato del sistema */
				if((pthread_equal(flag_op,pthread_self())) != 0 || pthread_equal(flag_op,0) != 0 ){
					rplmsg=execute_op(msg,argo->H,argo->L);
					if(msg->hdr.op!=GET_OP){
					
						if(sendHeader(con,rplmsg)==-1)
							perror("Fallita SendHeader");
							
						free(rplmsg);
						}
	
					else{
					
						if(sendRequest(con,rplmsg)==-1)
							perror("Fallita SendRequest\n");
							
						free(rplmsg->data.buf);
						free(rplmsg);
					}
				}
				else{
					message_t *rplock;
					ec_null_alloc_exit((rplock = malloc(sizeof(message_hdr_t))));
					rplock->hdr.op = OP_LOCKED;
					rplock->hdr.key = msg->hdr.key;
					
					if(sendHeader(con,rplock)==-1)
						perror("Fallita SendHeader\n");
						
					free(rplock);
					
				}
			}	
			/* Se il sistema non ha ricevuto il segnale di Unlock dal Client, prima di accettare una nuova connessione effettuo l'unlock */
			if(pthread_equal(flag_op,pthread_self()) != 0)
				unlockop();
		}
		else{
			free(msg);
			pthread_exit(NULL);
		}
		pthread_mutex_lock(&statlock);
			mboxStats.concurrent_connections--;
		pthread_mutex_unlock(&statlock);
			close(con);
	}
	
	free(msg);
 pthread_exit(NULL);
}


/* Thread che gestisce i segnali. In base al segnale ricevuto stampa e/o setta a 0 le variabili globali Run & Go per smettere di lavorare i thread ad un preciso momento. Inoltre viene chiuso il socket descriptor del server per uscire dall'accept che altrimenti rimarrebbe bloccata in attesa di nuove connessioni  */
static void* signal_handler(void* arg) {

FILE* fout;
int sig_arrived;
unlink(StatFileName);
long fd_skt= (long)arg;
if((fout= fopen(StatFileName,"w+"))==NULL){
		perror("fopen signal");
		exit(EXIT_FAILURE);
			}	

	while(run){
	sigwait(&sa.sa_mask, &sig_arrived);
		switch(sig_arrived){
			case(SIGUSR1):{ 
				printStats(fout);
				break;
			};
			case(SIGUSR2):{
				printStats(fout);
				run = 0;
				shutdown(fd_skt, SHUT_RD);
				pthread_cond_broadcast(&qcond);
				break;			
			};
			case(SIGQUIT): {
				printStats(fout);
				run=0;
				go=0;	
				shutdown(fd_skt, SHUT_RD);
				pthread_cond_broadcast(&qcond);	
				break;
			};
		}
	}
	fclose(fout);
	pthread_exit(NULL);
}

/* Main di Membox che ha la funzione di dispatcher, ossia quella di mettersi in ascolto per accettare connessioni per metterle in coda */
int main(int argc, char *argv[]){
	
	int i,j;
	FILE *fd;
	args argo;
	message_t * p;
	icl_hash_t *rep;
	char op[] = "f";
	pthread_t* carray;
	pthread_t thandler;
	connqueue *coda=NULL;
	pthread_mutex_t *lockarray;
	long fd_skt,serverLen,fd_cln;
	
	struct sockaddr_un server;              //Server address 
	struct sockaddr_un client;              // Client address
	struct sockaddr* serverSockAddrPtr;    
	struct sockaddr* clientSockAddrPtr;    
	
	serverSockAddrPtr = (struct sockaddr*) &server;
	serverLen = sizeof (server);
	clientSockAddrPtr = (struct sockaddr*) &client;
	socklen_t clientLen = sizeof (client);
	
	ec_null_alloc_main((p = (message_t*)malloc(sizeof(message_t))));
	
	/* Setto le variabili globali run & go a 1 per far partire tutti i cicli */
	run = 1;
	go=1;
	
/* Azzero */
  memset(&sa,0,sizeof(sa));    

  /* Ignoro SIGPIPE per evitare di essere terminato da una scrittura su un socket chiuso*/
  sa.sa_handler=SIG_IGN;
	if ( (sigaction(SIGPIPE,&sa,NULL) ) == -1 ) {   
		perror("sigaction");
		return -1;
  } 
	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, SIGUSR1);
	sigaddset(&sa.sa_mask, SIGUSR2);
	sigaddset(&sa.sa_mask, SIGQUIT);
	/* Applico maschera a questo thread. La maschera poi viene ereditata dai figli */
	pthread_sigmask(SIG_BLOCK, &sa.sa_mask, NULL);

/* Inizializzo il server , controllando la presenza del flag '-f' e la presenza del file */
	if(getopt(argc, argv,op)=='f'){
			if((fd = fopen(((char*)argv[2]),"r"))==NULL){
				perror("fopen");
				exit(EXIT_FAILURE);
			}	
			if(conf(fd)==-1)
				fprintf(stderr,"Configurazione");
			
			fclose(fd);
	}
	else
	fprintf(stderr, "ERRORE: Flag mancante\n");

	

/*Creao la repository*/
		rep = icl_hash_create(BUCKETS, ulong_hash_function, ulong_key_compare);

/*Inizializzo l'array di lock */
	ec_null_alloc_main((lockarray=malloc((rep->nbuckets)*sizeof(pthread_mutex_t))));
	for(i=0;i<(rep->nbuckets);i++){
			pthread_mutex_init(&lockarray[i],NULL);
		}

/* Inizializzo la coda di connessioni */
		coda = new_queue(coda,MaxConnections);
	
/* Inizializzo l' array dei thread */
		ec_null_alloc_main((carray=(pthread_t*)malloc(ThreadsInPool*sizeof(pthread_t))));

/* Assegno gli argomenti */
		argo.H=rep;
		argo.Q=coda;
		argo.L=lockarray;

/* Creo il ThreadPool */
		for(j=0;j<ThreadsInPool;j++){
			pthread_create(&carray[j], NULL, worker,(void*)&argo);

		}
/* Creo il socket del server */
	server.sun_family = AF_UNIX;
	strncpy(server.sun_path, UnixPath,strlen(UnixPath)+1);
	unlink (UnixPath);
	fd_skt=socket(AF_UNIX,SOCK_STREAM,0);
	bind(fd_skt, serverSockAddrPtr, serverLen);
	listen(fd_skt, SOMAXCONN);


/*Creo il thread che si occuperà dei segnali*/
	pthread_create(&thandler,NULL,signal_handler,(void*)fd_skt);

/* Dispatcher : mi metto in ascolto finchè ci sono richieste di connessioni da parte del Client*/	

		while(run){
			fd_cln = accept (fd_skt, clientSockAddrPtr, &clientLen);
			if(run==0 && fd_cln == -1)
				break;
			if((Full_queue(coda,MaxConnections,mboxStats.concurrent_connections))==0){
				p->hdr.key=0;
				p->hdr.op = OP_FAIL;
				if(sendHeader(fd_cln,p)==-1){
					fprintf(stderr,"Fallita sendHeader\n");
				}			
			}else{
					push_queue(fd_cln,coda,MaxConnections);
			 }
		}
	
	/* Spengo tutti i thread, svuoto la repository e dealloco tutte le strutture del main */
	if(run==0){
		for(i=0;i<ThreadsInPool;i++)
			pthread_join(carray[i],NULL);
		pthread_join(thandler,NULL);
	}
	
	free(coda->queue);
	free(coda);
	icl_hash_destroy(rep, freeKey, freeData	);
	free(carray);
	free(lockarray);
	free(p);
	free(UnixPath);
	free(StatFileName);
	return 0;
}

 /** \file membox.c
       \authors Gabriele Barreca e Luca Murgia
     Si dichiara che il contenuto di questo file e' in ogni sua parte opera
     originale dell' autore.  */
