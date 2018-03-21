#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> 
#include <sys/types.h>
#include <sys/un.h>
#include <string.h>
#include <unistd.h>
#include "connections.h"
#include <errno.h>
#define MAXLENGTH 1000
/**
 * @function openConnection
 * @brief Apre una connessione AF_UNIX verso il server membox.
 *
 * @param path Path del socket AF_UNIX 
 * @param ntimes numero massimo di tentativi di retry
 * @param secs tempo di attesa tra due retry consecutive
 *
 * @return il descrittore associato alla connessione in caso di successo
 *         -1 in caso di errore
 */
int openConnection(char* path, unsigned int ntimes, unsigned int secs){
	int result,i=0;
	int sd_client,serverlen;
	struct sockaddr_un server;
	struct sockaddr* serverSockAddPtr;
	
	serverSockAddPtr = (struct sockaddr*) &server;
	serverlen = sizeof(server);
	
	sd_client = socket(AF_UNIX,SOCK_STREAM, 0);
	server.sun_family= AF_UNIX;
	strcpy(server.sun_path,path);

	
	do{
		result = connect (sd_client,serverSockAddPtr,serverlen);
		if(result == -1){
			sleep(secs);
			i++;
		}	
	}while(result == -1 && i<ntimes);

if(result == 0)
	return sd_client;
else
	return result;	
}

/**
 * @function readHeader
 * @brief Legge l'header del messaggio
 *
 * @param fd     descrittore della connessione
 * @param hdr    puntatore all'header del messaggio da ricevere
 *
 * @return 0 in caso di successo -1 in caso di errore
 */
 
int readHeader(long fd, message_hdr_t *hdr){

int letti=0;
int daleggere=sizeof(hdr->op);

while((letti= read(fd,(&hdr->op)+letti,daleggere))<(daleggere)){
		if(letti==-1){
			fprintf(stderr,"Fallita prima read dentro la ReadHeader\n");
			return -1;
		}
		daleggere-=letti;
	}

letti=0;
daleggere=sizeof(hdr->key);
while((letti= read(fd,(&hdr->key)+letti,daleggere))<(daleggere)){
		if(letti==-1){
			fprintf(stderr,"Fallita seconda read dentro la ReadHeader\n");
			return -1;
		}
		daleggere-=letti;
	}

return 0;
}

/*
 * @function readData
 * @brief Legge il body del messaggio
 *
 * @param fd     descrittore della connessione
 * @param data   puntatore al body del messaggio
 *
 * @return 0 in caso di successo -1 in caso di errore
 */

int readData(long fd, message_data_t *data){

int daleggere=sizeof(data->len);
int letti=0;
while((letti= read(fd,(&data->len)+letti,daleggere))<(daleggere)){
		if(letti==-1){
		fprintf(stderr,"Fallita prima read dentro la ReadData\n");
		return -1;
	}
		daleggere-=letti;
	}


	data->buf = malloc((data->len)*sizeof(char));
	daleggere=data->len;
	letti=0;
	
	while((letti= read(fd,(data->buf)+letti,daleggere))<(daleggere)){
		if(letti==-1){
			fprintf(stderr,"Fallita quarta read dentro la ReadReply\n");
			return -1;
		}
		daleggere-=letti;
	}
		
	return 0 ;
}

// ------- client side ------
/**
 * @function sendRequest
 * @brief Invia un messaggio di richiesta al server membox
 *
 * @param fd     descrittore della connessione
 * @param msg    puntatore al messaggio da inviare
 *
 * @return 0 in caso di successo -1 in caso di errore
 */

int sendRequest(long fd, message_t *msg){

int dascrivere;
int scritti = 0;

if(write(fd,&msg->hdr.op,sizeof(msg->hdr.op))<sizeof(msg->hdr.op)){
	fprintf(stderr,"Fallita prima write dentro la SendRequest\n");
	return -1;
}

if(write(fd,&msg->hdr.key,sizeof(msg->hdr.key))<sizeof(msg->hdr.key)){
	fprintf(stderr,"Fallita seconda write dentro la SendRequest\n");
	return -1;
}

if(write(fd,&msg->data.len,sizeof(msg->data.len))<sizeof(msg->data.len)){
	fprintf(stderr,"Fallita terza write dentro la SendRequest\n");
	return -1;
}

dascrivere = msg->data.len;

	while((scritti=write(fd, (msg->data.buf)+scritti, dascrivere)) < dascrivere){
		if(scritti==-1){
			fprintf(stderr,"Fallita quarta read dentro la SendRequest\n");
			return -1;
		}
		dascrivere-=scritti;
	}

	return 0;
}


/*SENDHEADER*/
int sendHeader(long fd, message_t *msg){

if(write(fd,&msg->hdr.op,sizeof(msg->hdr.op))<sizeof(msg->hdr.op)){
	fprintf(stderr,"Fallita prima write dentro la SendHeader\n");
	return -1;
}

if(write(fd,&msg->hdr.key,sizeof(msg->hdr.key))<sizeof(msg->hdr.key)){
	fprintf(stderr,"Fallita seconda write dentro la SendHeader\n");
	return -1;
}
	
return 0;
}

/* Leggo la risposta */

int readReply(long fd, message_t * msg){

if(read(fd,&msg->hdr.op,sizeof(msg->hdr.op))<sizeof(msg->hdr.op)){
	fprintf(stderr,"Fallita prima read dentro la ReadReply\n");
	return -1;
}
if(read(fd,&msg->hdr.key,sizeof(msg->hdr.key))<sizeof(msg->hdr.key)){
	fprintf(stderr,"Fallita seconda read dentro la ReadReply\n");
	return -1;
}

if(read(fd,&msg->data.len,sizeof(msg->data.len))<sizeof(msg->data.len)){
	fprintf(stderr,"Fallita terza read dentro la ReadReply\n");
	return -1;
}


	msg->data.buf = malloc((msg->data.len)*sizeof(char));
	int daleggere=msg->data.len;
	int letti=0;
	
while((letti= read(fd,(msg->data.buf)+letti,daleggere))<daleggere){
	if(letti==-1){
		fprintf(stderr,"Fallita quarta read dentro la ReadReply\n");
		return -1;
	}
	daleggere-=letti;
}
return 0;
}

