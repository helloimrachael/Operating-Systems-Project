//CLIENT

#include "util.h"

#define UNIX_PATH_MAX 108

uint64_t ID;
uint64_t id;
int secret;
int p;
int k;
int w;
char **SA;
int *A;

static void cleanup (void);

int main(int argc, char *argv[]) {
	if(argc<3) {
		perror("ERROR: too few arguments");
		exit(EXIT_FAILURE);
	}
	if ((atexit(cleanup)) != 0) {
    	perror("ERROR during atexit(cleanup) in server");
    	EXIT_FAILURE;
    }
	p = strtol(argv[1], NULL, 10);
	k = strtol(argv[2], NULL, 10);
	w = strtol(argv[3], NULL, 10);
	if(p == 0){
		perror("ERROR: 0 server");
		exit(EXIT_FAILURE);
	}
	if (p>k){
		perror("ERROR: too few servers created");
		exit(EXIT_FAILURE);
	}
	if (w<=(3*p)){
		perror("ERROR: too few messages");
		exit(EXIT_FAILURE);
	}
	if (p<0 || k<0 || w<0){
		perror("ERROR: p,k,w must be positive number");
		exit(EXIT_FAILURE);
	}
    srand(time(NULL) * getpid());
    //genero l'ID del client casualmente come un intero a 64 bit
  	id = rand64bits();
  	//converto l'ID in network byte order
	ID = HTONLL(id);
	//genero il secret casualmente
	secret = rand()%3000;
	//inizialmente l'unità di misura del secret sono i millisecondi
	struct timespec time1;
	//converto il secret in secondi
	int secondi = secret/1000;
	//mi salvo il secret in nanosecondi
	time1.tv_nsec = (secret%1000)*1000000;
	time1.tv_sec = secondi;
	printf("CLIENT %lx SECRET %d\n", id, secret);
	//alloco memoria per un array d stringhe che andrà a contenere gli indirizzi delle socket
	SA = (char**)calloc(p, sizeof(char*));
	for(int i=0; i<p; i++) {
		SA[i] = (char*)calloc(32, sizeof(char));
	}
	A = (int*)calloc(p, sizeof(int));
	A = initializer_cl(A, p, k);
	SA = createArraySockAddr(SA, A, p, k, 1);
	sending(SA, A, p, k, w, &ID, time1);
	printf("CLIENT %lx DONE\n", id);
	printf("\n");
	fflush(stdout);
	return 0;
}

//funzione per la pulizia
static void cleanup() {
	for(int i=0; i<p; i++) {
		free(SA[i]);
	}
	free(SA);
	free(A);
}
