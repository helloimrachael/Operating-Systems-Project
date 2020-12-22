#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>
#include <arpa/inet.h>
#include <netinet/in.h>

// define prese da https://stackoverflow.com/questions/16375340/c-htonll-and-back
#define HTONLL(x) ((1==htonl(1)) ? (x) : (((uint64_t)htonl((x) & 0xFFFFFFFFUL)) << 32) | htonl((uint32_t)((x) >> 32)))
#define NTOHLL(x) ((1==ntohl(1)) ? (x) : (((uint64_t)ntohl((x) & 0xFFFFFFFFUL)) << 32) | ntohl((uint32_t)((x) >> 32)))

//funzione per leggere. Presa dal file conn.h in una delle soluzioni degli assegnamenti su Didawiki.
static inline int readn(long fd, void *buf, size_t size) {
    size_t left = size;
    int r;
    char *bufptr = (char*)buf;
    while(left>0) {
	if ((r=read((int)fd ,bufptr,left)) == -1) {
	    if (errno == EINTR) continue;
	    return -1;
	}
	if (r == 0) return 0;
        left    -= r;
	bufptr  += r;
    }
    return size;
}

//funzione per scrivere. Presa dal file conn.h in una delle soluzioni degli assegnamenti su Didawiki.
static inline int writen(long fd, void *buf, size_t size) {
    size_t left = size;
    int r;
    char *bufptr = (char*)buf;
    while(left>0) {
	if ((r=write((int)fd ,bufptr,left)) == -1) {
	    if (errno == EINTR) continue;
	    return -1;
	}
	if (r == 0) return 0;  
        left    -= r;
	bufptr  += r;
    }
    return 1;
}

////////////////////// SUPERVISOR //////////////////////

typedef struct _pipe {
	int pfd[2];
}ppipe;

typedef struct _list {
	uint64_t Client_ID;
	long stima;
	int n_server;
	struct _list *next;
}list_stime;

//funzione che mi crea una lista vuota
list_stime *createList_sup() {
	list_stime *L = NULL;
	return L;
}

//funzione che mi contralla se il Client passato come argomento alla funzione è già presente o meno nella Lista
int member_sup(list_stime* L, long ID) {
	list_stime *corr = L;
	int trovato=0;
	while(corr != NULL && !trovato) {
		if (corr->Client_ID == ID) {
			trovato = 1;
		}
		else
			corr = corr->next;
	}
	return trovato;
}

//funzione che mi inserisce il nuovo Client e la stima associata ad esso nella lista L
list_stime* insertList_sup(list_stime *L, list_stime el, int server) {
	if (L == NULL) {
		//se la lista è vuota inserisco il nuovo Client
		list_stime* new_C = (list_stime*)malloc(sizeof(list_stime));
		new_C->Client_ID = el.Client_ID;
		new_C->stima = el.stima;
		new_C->n_server = 1;
		new_C->next = NULL;
		L = new_C;
	}
	else {
		if (member_sup(L, el.Client_ID) == 0) { 
			//il Client non è presente nella lista e perciò lo aggiungo in testa
			list_stime *corr = L;
			list_stime* new_C = (list_stime*)malloc(sizeof(list_stime));
			new_C->Client_ID = el.Client_ID;
			new_C->stima = el.stima;
			new_C->n_server = 1;
			new_C->next = NULL;
			while(corr->next != NULL) {
				corr = corr->next;
			}
			corr->next = new_C;
		}
		else { 
			//il Client è già presente nella lista quindi vado a vedere se la stima atime_msguale è minore o no 
			//della stima passata come argomento alla funzione
			list_stime *corr = L;
			while(corr->Client_ID != el.Client_ID) {
				corr = corr->next;
			}
			if (corr->stima>el.stima){
				corr->stima = el.stima;
			}
			corr->n_server++;
		}
	}
	return L;
}

//funzine che mi stampa gli elementi della lista
void printList_sup(list_stime *L) {
	list_stime *corr = L;
	while(corr != NULL) {
		printf("SUPERVISOR ESTIMATE %ld FOR %lx BASED ON %d\n", corr->stima, corr->Client_ID, corr->n_server);
		corr = corr->next;
	}
}

///////////////////////////////////////////////////////




////////////////////// SERVER ////////////////////////

typedef struct s_ {
	pthread_t t_id;
	int fd_C;
	struct s_ *next;
}connection;

//funzione che mi crea una lista vuota
connection *createListThread() {
	connection *listThread = NULL;
	return listThread;
}

//funzione che crea l'indirizzo della socket attraverso il numero del server
char *crateSockAddr_server(int num_server) {
	//compongo l'indirizzo della socket --> OOB-server-i, dove i è num_server
	char *str_server = (char*) calloc(1, sizeof(int));
	char *str = (char*) calloc(16, sizeof(char));
	strcpy(str, "OOB-server-");
	sprintf(str_server, "%d", num_server);
	strcat(str, str_server);
	char *address_socket=(char*)calloc(16, sizeof(char));
	strcat(address_socket, str);
	free(str_server);
	free(str);
	return address_socket;
}

//creazione socket dove il server riceverà i messaggi dal client
long receiving(char *address_socket) {
	unlink(address_socket);
	long fd_skt;
	struct sockaddr_un sa;
	strcpy(sa.sun_path, address_socket);
	sa.sun_family = AF_UNIX;
	if ((fd_skt = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		perror("ERROR in function socket (server)");
		exit(EXIT_FAILURE);
	}
	if ((bind(fd_skt, (struct sockaddr *)&sa, sizeof(sa))) == -1) {
		perror("ERROR in function bind");
		exit(EXIT_FAILURE);
	}
	if ((listen(fd_skt, SOMAXCONN)) == -1) {
		perror("ERROR in function listen");
		exit(EXIT_FAILURE);
	}
	return fd_skt;
}

//funzjone che una volta accettata la connessione mi crea un thread e me lo inserisci nella lista
connection *insertListConnection(connection *listThread, connection *TH) {
	TH->next = NULL;
	if(listThread == NULL) {
		listThread = TH;
	}
	else {
		connection *corr = listThread;
		while(corr->next != NULL) {
			corr = corr->next;
		}
		corr->next = TH;
	}
	return listThread;
}

///////////////////////////////////////////////////////




////////////////////// CLIENT ////////////////////////

unsigned rand256() {
    static unsigned const limit = RAND_MAX - RAND_MAX % 256;
    unsigned result = rand();
    while ( result >= limit ) {
        result = rand();
    }
    return result % 256;
}

unsigned long long rand64bits(){
    unsigned long long results = 0ULL;
    for(int count = 8; count > 0; -- count ) {
        results = 256U * results + rand256();
    }
    return results;
}

int *zero(int *A, int p) {
	for(int i=0; i<p; i++) {
		A[i]=0;
	}
	return A;
}

//funzione che mi controlla se il numero randomico è gia stato inserito in A
int inA(int *A, int c, int limit) {
	int trovato = 0;
	int i = 0;
	while(!trovato && i<limit) {
		if(A[i] == c)
			trovato = 1;
		i++;
	}
	return trovato;
}

//funzione che mi crea l'array di server ai quali il client si deve connettere
int *initializer_cl(int *A, int p, int k) {
	int i = 0;
	int c;
	while(1) {
		if (i == p) {
			break;
		}
		c = rand()%k+1;
		if(inA(A, c, i) == 0) {
			A[i] = c;
			i++;
		} 
	}
	return A;
}

//funzione che mi crea un array di indirizzi di socket
char **createArraySockAddr(char **SA, int *A, int p, int k, int N) {
	for(int i=0; i<p; i++) {
		char *str_server = (char*) calloc(N, sizeof(int));
		char *address_socket = (char*) calloc(32, sizeof(char));
		strcpy(address_socket, "OOB-server-");
		sprintf(str_server, "%d", A[i]);
		strcat(address_socket, str_server);
		strcpy(SA[i], address_socket);
		free(str_server);
		free(address_socket);
	}
	return SA;
}

//comunicazione client-server via messaggi su socket
void sending(char **SA, int *A, int p, int k, int w, void *buf, struct timespec tempo1) {
	int *array_csfd;
	int x;
	//inizializzazione seed per generare numeri randomici
	srand(time(NULL));
	array_csfd=(int*)calloc(p, sizeof(int));
	for(int i=0; i<p; i++) {
		struct sockaddr_un sa;
		memset(&sa, 0, sizeof(sa));
		strcpy(sa.sun_path, SA[i]);
		sa.sun_family = AF_UNIX;
		if ((array_csfd[i]=socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
			perror("ERROR in function socket");
			exit(EXIT_FAILURE);
		}
		errno = 0;
		while((connect(array_csfd[i], (struct sockaddr *)&sa, sizeof(sa)))<0) {
			if (errno == ENOENT)
				sleep(1);
		}
	}
	for(int i=0; i<w; i++) {
		x = rand()%p;
		if (writen(array_csfd[x], buf, 8) == -1){
			perror("ERROR writen");
			exit(EXIT_FAILURE);
		}
		errno = 0;
		if ((nanosleep (&tempo1, NULL)) == -1) {
			perror("ERROR in nanosleep");
			exit(EXIT_FAILURE);
		}
		if (errno == EINTR) {
			perror("errno=EINTR");
		}
		if (errno == EFAULT) {
			perror("errno=EFAULT");
		}
		if (errno == EINVAL) {
			perror("errno=EINVAL");
		}
	}
	for(int i=0; i<p; i++) {
		close(array_csfd[i]);
	}
	free(array_csfd);
}
///////////////////////////////////////////////////////
