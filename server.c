//SERVER

#include "util.h"

int *pfd;
long num_server;
char *id_client;
char *out;
char *address_socket;
connection *listThread;
list_stime ls;

 
static void cleanup(void);
static void *connection_handler(void *fd_c);

int main(int argc, char *argv[]) {
	if (argc < 3) {
		perror("ERROR: too few arguments");
		exit(EXIT_FAILURE);
	}
	struct sigaction s;
	//inizializzo s a 0
	memset(&s, 0, sizeof(s));
	//gestore per ignorare un segnale:
	s.sa_handler = SIG_IGN;
	if ((sigaction(SIGINT, &s, NULL)) != 0) {
		perror("ERROR in server sigaction");
		exit(EXIT_FAILURE);
	}
    if ((atexit(cleanup)) != 0) {
    	perror("ERROR during atexit(cleanup) in server");
    	EXIT_FAILURE;
    }
    if (argc == 1) {
    	perror("ERROR: too few arguments");
    	exit(EXIT_FAILURE);
    }
	//mi salvo nella variabile num_server il numero del server corrente, passato come argomento nella execl fatta nel supervisor
	num_server = (long)(*argv[1])+1;
	address_socket=crateSockAddr_server(num_server);
	fprintf(stdout, "SERVER %ld ACTIVE\n", num_server);
	pfd = (int*)calloc(2,sizeof(int));
	//mi salvo nella variabile pfd[1] il pfd della pipe (in scrittura), passato come argomento nella execl fatta nel supervisor
	pfd[1] = (int)(*argv[2]);
	int fd_skt, fd_a;
	fd_skt=receiving(address_socket);
	errno = 0;
	//attendo le connessioni da parte dei client
	listThread = createListThread();
	while(1) {
		if ((fd_a = accept(fd_skt, NULL, 0)) == -1) {
			if (errno == EAGAIN)
				continue;
			perror("ERROR during accept");
			exit(EXIT_FAILURE);
		}
		printf("SERVER %ld CONNECT FROM CLIENT\n", num_server);
		connection *TH = (connection*)malloc(sizeof(connection));
		TH->fd_C = fd_a;
		if((pthread_create(&(TH->t_id), NULL, connection_handler, (void*)TH)) != 0) {
			perror("ERROR in pthread_create");
			exit(EXIT_FAILURE);
		}
		listThread = insertListConnection(listThread, TH);
	}
	fflush(NULL);
	return 0;
}

//funzione per la pulizia
static void cleanup(void) {
	free(pfd);
	connection *corr=listThread;
	while(corr != NULL) {
		pthread_join(corr->t_id, (void*)NULL);
		close(corr->fd_C);
		corr=corr->next;
	}
	corr = listThread;
	while(corr != NULL) {
		listThread = listThread->next;
		free(corr);
		corr=listThread;
	}
}

static void *connection_handler(void *thread) {
	connection *th = (connection *)thread;
	struct timespec time1;
	long t_prec, t_att, stima_att, stima_possibile = -1;
	int flag_firstmessage = 1;
	uint64_t id;
	while(readn((th->fd_C), &id, 8)>0) {
		id = NTOHLL(id);
		clock_gettime(CLOCK_REALTIME, &time1);
		t_att = ((time1.tv_sec%10000)*1000) + (time1.tv_nsec/1000000);
		printf("SERVER %ld INCOMING FROM %lx @ %ld\n", num_server, id, t_att);
		if (flag_firstmessage == 0) { // se entro qui sono arrivati due messaggi
			stima_att = t_att-t_prec;
			t_prec = t_att;
			if (stima_possibile == -1) {
				stima_possibile = stima_att;
			}
			else {
				if (stima_att < stima_possibile) 
					stima_possibile = stima_att;
			}
		}
		else { //se entro qui è arrivato solo un messaggio
			t_prec = t_att;
			flag_firstmessage = 0;
		}

	}
	//se la stima è -1 vuol dire che è arrivato un solo messaggio quindi non posso avere una stima del secret
	if (stima_possibile == -1) {
		pthread_exit(NULL);
	}
	//mando la stima al supervisor tramite pipe
	ls.Client_ID = id;
	ls.stima = stima_possibile%10000;
	//scrivo il messsaggio sulla pipe
	if (writen(pfd[1], &ls, sizeof(list_stime)) <= 0) {
		perror("Error writing");
		exit(EXIT_FAILURE);
	}
	printf("SERVER %ld CLOSING %lx ESTIMATE %ld\n", num_server, id, stima_possibile);
	fflush(NULL);
	pthread_exit(NULL);
}
