#include <operation.h>

struct statistics  mboxStats = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
pthread_mutex_t statlock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t OpLock = PTHREAD_MUTEX_INITIALIZER;
pthread_t  flag_op = 0;

char *UnixPath ;
int MaxConnections;
int ThreadsInPool;
int StorageSize;    
int StorageByteSize;
int MaxObjSize;
char *StatFileName;

/*legge il file di configurazione e setta le variabili globali*/
int conf (FILE * fp){
char key[128];
   char value[128];
   int ret;
   int n;
   long l;
   
   while(1) {
      ret = fscanf(fp, "%[^=\n]=%[^\n]", key, value);
      fgetc(fp); // legge '\n'
            
      if (ret == 2) {

        	if( strcmp(key,"UnixPath         ") == 0){
        			 ec_null_alloc((UnixPath= malloc((strlen(value))*sizeof(char))));
        			strcpy(UnixPath,(value +1));
        	}
         	if(strcmp(key,"MaxConnections	 ") == 0){
        		n=atoi(value);
        		MaxConnections = n;
        	}
        	if(strcmp(key,"ThreadsInPool    ") == 0){
       			n=atoi(value);
       			ThreadsInPool = n;
      			} 
      		if(strcmp(key,"StorageSize      ") == 0){
      			l=atol(value);
        		StorageSize= l;
      		}
        	if(strcmp(key,"StorageByteSize	 ") == 0){
        		l=atol(value);
        		StorageByteSize = l;
        	}
        	if(strcmp(key,"MaxObjSize       ") == 0){
        		l=atol(value);
        		MaxObjSize = l;
        	}
        	if(strcmp(key,"StatFileName     ") == 0){
							ec_null_alloc((StatFileName= malloc((strlen(value))*sizeof(char))));
        			strcpy(StatFileName,(value +1));	
        	}
        }	
      	if(ret!=2){
         int c = fgetc(fp); //leggo il prossimo carattere e vedo se sono alla fine del file
         if (c == EOF)
            break;
         else
            ungetc(c, fp); //rimetto il carattere, precedentemente preso, nello stream
      	}
   }
	return 0;
}

/* Funzioni per cancellare gli elementi della repository */
void freeData(void * data){
	message_data_t *msg = (message_data_t *) data; 
	free(msg->buf);
	free(msg);
}

void freeKey(void * key){
	membox_key_t* hdr_key = (membox_key_t*)key;
	free(hdr_key);
}

/*Inserisco un oggetto nella repository*/
op_t putop (icl_hash_t *rep, membox_key_t *key,message_data_t* data) {
	icl_entry_t *p;
	
	errno=0;
	if(rep==NULL || key==NULL || data->buf==NULL) 
		return OP_FAIL; 	
		
	if(data->len > MaxObjSize && MaxObjSize != 0)
		return OP_PUT_SIZE;
		
	if((StorageSize - mboxStats.current_objects  ==0) && StorageSize !=0)
		return OP_PUT_TOOMANY;

	if(StorageByteSize - mboxStats.current_size < sizeof(data) && StorageByteSize != 0)
		return OP_PUT_REPOSIZE;

	if((p=icl_hash_insert(rep, key, data))==NULL)
	{	
		if (errno==ENOMEM)
		 	return OP_FAIL;
		else 
			return OP_PUT_ALREADY;
	}
	else     /*la insert è andata a buon fine*/	
		return OP_OK;
}

/*Aggiorno l'oggetto nella repository*/
op_t updateop (icl_hash_t *rep, membox_key_t *key,message_data_t* data){

message_data_t *old;

	if(rep==NULL || key==NULL || data->buf==NULL)
		return OP_FAIL;
	
	if(icl_hash_find(rep, key)==NULL)
		return OP_UPDATE_NONE;
	
	old=icl_hash_find(rep, key);
	if(old->len != data->len)	
		return OP_UPDATE_SIZE;
	
	if((icl_hash_delete(rep,key,freeKey,freeData)) == -1 || (icl_hash_insert(rep, key, data) == NULL))
		return OP_FAIL;
	else
		return OP_OK;
}


/*Get*/
op_t getop (icl_hash_t *rep, membox_key_t *key,message_data_t *data){
	message_data_t* mex; 

	if(rep==NULL || key==NULL || data->len!=0)
		return OP_FAIL;

		
	mex = icl_hash_find(rep,key);

	if(mex==NULL)/*nodo non trovato*/
		return(OP_GET_NONE);
		
	else{	/*find eseguita con successo*/
		data->len = mex->len;
		data->buf = mex->buf;
		return OP_OK;
	}
	return OP_FAIL;	
}

/*Operazione di remove*/
op_t removeop(icl_hash_t *rep, membox_key_t *key,message_data_t *data){
	if(rep==NULL || key==NULL || data->len!=0)
		return OP_FAIL;

	if(icl_hash_delete(rep,key,freeKey,freeData) == -1)
		return OP_REMOVE_NONE;
	else
		return OP_OK;
}

/*Operazion di Lock*/
op_t lockop(){
	pthread_mutex_lock(&OpLock);
	if((pthread_equal(flag_op,0))!= 0){
		flag_op = pthread_self();
		pthread_mutex_unlock(&OpLock);
		return OP_OK;
	}
	else{
		pthread_mutex_unlock(&OpLock);
		return OP_LOCKED;
	}
}

/*Operazione di Unlock*/	
op_t unlockop(){
	pthread_mutex_lock(&OpLock);
	if((pthread_equal(flag_op,0))!= 0){
		pthread_mutex_unlock(&OpLock);
		return OP_LOCK_NONE;
	}
	else{
		flag_op = 0;
		pthread_mutex_unlock(&OpLock);
		return OP_OK;
	}
}
	

/*Funzioni per Hash*/
unsigned int fnv_hash_function( void *key, int len ) {
    unsigned char *p = (unsigned char*)key;
    unsigned int h   = 2166136261u;
    int i;
    for ( i = 0; i < len; i++ )
        h = ( h * 16777619 ) ^ p[i];
    return h;
}

unsigned int ulong_hash_function( void *key ) {
    int len = sizeof(unsigned long);
    unsigned int hashval = fnv_hash_function( key, len );
    return hashval;
}

/*Execute op*/
message_t * execute_op( message_t * msg, icl_hash_t * rep, pthread_mutex_t *locks) {
	
	op_t op;
	message_t *rplmsg;
	membox_key_t *tmp; 
	message_data_t *datatmp;
	ec_null_alloc_n((rplmsg  = malloc(sizeof(message_t))));
	ec_null_alloc_n((tmp     = malloc(sizeof(membox_key_t))));
	ec_null_alloc_n((datatmp = malloc(sizeof(message_data_t))));
	
	
	*tmp = msg->hdr.key;
	datatmp->len = msg->data.len;
	ec_null_alloc_n((datatmp->buf = malloc((datatmp->len)*sizeof(char))));
	memcpy(datatmp->buf,msg->data.buf,(datatmp->len));
	

	switch(msg->hdr.op) { //in base al tipo di op_t esegue un'operazione differente e aggiorna le rispettive statistiche
		case PUT_OP:{
	
	/* Ricavo l'indice della lista di trabocco per fare la lock sulla mutex corrispondente */	
			int index=ulong_hash_function(&msg->hdr.key) %  BUCKETS;
			pthread_mutex_lock (&locks[index]);

			op = putop(rep,tmp,datatmp);
			rplmsg->hdr.op  = op;
			rplmsg->hdr.key = *tmp;
			
			pthread_mutex_unlock(&locks[index]);

/* Eseguo la lock sulla struttura delle statistiche */
			pthread_mutex_lock(&statlock);
			if(op==OP_OK){	
				mboxStats.nput++;
				mboxStats.current_objects++;
				mboxStats.current_size += datatmp->len;
				if(mboxStats.current_size > mboxStats.max_size)
				mboxStats.max_size=mboxStats.current_size;
				if(mboxStats.current_objects > mboxStats.max_objects)
				mboxStats.max_objects=mboxStats.current_objects;
			}
			else{
				mboxStats.nput++;
				mboxStats.nput_failed++;
			}

			pthread_mutex_unlock(&statlock);
			free(msg->data.buf);
			return rplmsg;
		};
		case GET_OP:{
			
			/* Ricavo l'indice della lista di trabocco per fare la lock sulla mutex corrispondente */	
			int index=ulong_hash_function(&msg->hdr.key) %  BUCKETS;
			pthread_mutex_lock (&locks[index]);
			
			/* Libero memoria per la buf*/
			free(datatmp->buf);
			op = getop(rep,tmp,datatmp);
			rplmsg->hdr.op   = op;
			rplmsg->hdr.key  = *tmp;
			rplmsg->data.len = datatmp->len;
					
			ec_null_alloc_n((rplmsg->data.buf = malloc((datatmp->len)*sizeof(char))));
			memcpy(rplmsg->data.buf,datatmp->buf,(datatmp->len));
			
			pthread_mutex_unlock(&locks[index]);

			pthread_mutex_lock(&statlock);
			if(op==OP_OK)
				mboxStats.nget++;
			else{
				mboxStats.nget_failed++;
				mboxStats.nget++;
			}
			pthread_mutex_unlock(&statlock);
			free(msg->data.buf);
			free(tmp);	
			free(datatmp);
			return rplmsg;
		};
		case REMOVE_OP:{
			int index=ulong_hash_function(&msg->hdr.key) %  BUCKETS;
			pthread_mutex_lock (&locks[index]);
			
			op=removeop(rep,tmp,datatmp);
			rplmsg->hdr.op   = op;
			rplmsg->hdr.key  = *tmp;

			pthread_mutex_unlock(&locks[index]);

			pthread_mutex_lock (&statlock);
			if(op==OP_OK)
			{	
				mboxStats.nremove++;
				mboxStats.current_objects--;
				mboxStats.current_size = mboxStats.current_size - sizeof(msg->data) + sizeof(msg->hdr.key);
			}
			else{
				mboxStats.nremove++;
				mboxStats.nremove_failed++;
			}
			pthread_mutex_unlock(&statlock);
			free(msg->data.buf);
			free(tmp);
			free(datatmp->buf);
			free(datatmp);
			return rplmsg;
		};
		case UPDATE_OP:{
			int index=ulong_hash_function(&msg->hdr.key) %  BUCKETS;
			pthread_mutex_lock (&locks[index]);

			op=updateop(rep,tmp,datatmp);
			rplmsg->hdr.op   = op;
			rplmsg->hdr.key  = *tmp;

			pthread_mutex_unlock(&locks[index]);

			pthread_mutex_lock(&statlock);
			if(op==OP_OK)
				mboxStats.nupdate++;
			else{
				mboxStats.nupdate++;
				mboxStats.nupdate_failed++;
			}
			pthread_mutex_unlock(&statlock);
			free(msg->data.buf);
			return rplmsg;
		};
		case LOCK_OP:{
			op = lockop();
			rplmsg->hdr.op = op;
			rplmsg->hdr.key = msg->hdr.key;
		
			pthread_mutex_unlock(&statlock);
			if(op==OP_OK)
				mboxStats.nlock++;
			else{
				mboxStats.nlock++;
				mboxStats.nlock_failed++;
			}
			pthread_mutex_unlock(&statlock);
			free(msg->data.buf);
			free(tmp);
			free(datatmp->buf);
			free(datatmp);
			return rplmsg;
		};
		case UNLOCK_OP:{
			op=unlockop();
			rplmsg->hdr.op = op;
			rplmsg->hdr.key = msg->hdr.key;
			free(msg->data.buf);
			free(tmp);
			free(datatmp->buf);
			free(datatmp);
			return rplmsg;
		}
		default:{
			fprintf(stderr, "invalid operation");
			free(tmp);
			free(rplmsg);
			free(datatmp->buf);
			free(datatmp);
			free(msg->data.buf);
			return NULL;
		};
	}
	free(datatmp->buf);
	free(tmp);
	free(msg->data.buf);
	free(rplmsg);
return NULL;
}
 /** \file operation.c
       \authors Gabriele Barreca e Luca Murgia
     Si dichiara che il contenuto di questo file e' in ogni sua parte opera
     originale dell' autore.  */
