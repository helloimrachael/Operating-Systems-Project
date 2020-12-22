//SUPERVISOR

#include "util.h"

sig_atomic_t volatile num_sigint;

//mi creo un array di pid_t
pid_t *pid;
//dichiaro la pipe
ppipe *new_pipe;
list_stime *L;
list_stime listastime;
int r;
int k; //k=numero processi distinti (server) che il supervisor deve lanciare
int len;

static void gestore(int signum);
static void cleanup(void);

int main(int argc, char *argv[]) {
	//controllo se al supervisor è passato il parametro k
	if (argc<2) {
		perror("ERROR: too few arguments");
		exit(EXIT_FAILURE);
	}
	if ((k = atoi(argv[1])) == 0) {
		perror("ERROR during atoi of argv[1]");
		exit(EXIT_FAILURE);
	}
	//controllo se k è un numero positivo o meno
	if (k<0) {
		perror("ERROR: k must be positive");
		exit(EXIT_FAILURE);
   	}
    //messaggio iniziale del supervisor:
    printf("SUPERVISOR STARTING %d\n", k);
	fflush(NULL);
    //installazione gestore segnali
    num_sigint = 0;
    struct sigaction s;
	memset(&s, 0, sizeof(s));
    s.sa_handler = gestore;
    if ((sigaction(SIGINT, &s, NULL)) != 0) {
        perror("ERROR in sigaction of SIGINT\n");
        exit(EXIT_FAILURE);
    }
   	if ((sigaction(SIGALRM, &s, NULL)) != 0) {
        perror("ERROR in sigaction of SIGTSTP\n");
        exit(EXIT_FAILURE);
    }
    s.sa_handler = SIG_IGN;
    if ((sigaction(SIGPIPE, &s, NULL)) != 0) {
    	perror("Error in sigaction of SIGPIPE");
    	exit(EXIT_FAILURE);
    }
    pid = (pid_t *)calloc(k, sizeof(pid_t));
    new_pipe = (ppipe*)calloc(k, sizeof(ppipe));
    //creo una lista vuota.
	L = createList_sup();
	int i;
   	for(i=0; i<k; i++) {
   		//creo una pipe per ogni server
		if((pipe(new_pipe[i].pfd)) == -1) {
			perror("ERROR: function pipe(pfd)");
			exit(EXIT_FAILURE);
		}
		pid[i] = fork();
		if(pid[i]<0) {
			perror("ERROR during fork()");
			exit(EXIT_FAILURE);	
		}
		else if (pid[i] == 0) {  //sono nel figlio
			close(new_pipe[i].pfd[0]);
			if (execl("server", "supervisor", &i, &new_pipe[i].pfd[1], (char*) NULL) == -1) {
				perror("ERROR during execl");
				exit(EXIT_FAILURE);
			}
		}
		else {  //sono nel padre
			int x = fcntl(new_pipe[i].pfd[0], F_GETFL, 0);
			fcntl(new_pipe[i].pfd[0], F_SETFL, x|O_NONBLOCK);
			close(new_pipe[i].pfd[1]);

		}
	}
	int num_s;
	while(1) {
		//prendo un server in modo casuale
		num_s = rand()%k;
		//printf("pipe in lettura: %d\n", num_pipe);
		errno = 0;
		//leggo sulla pipe
		r =  readn(new_pipe[num_s].pfd[0], &listastime, sizeof(list_stime));
		if (r > 0) {
			printf("SUPERVISOR ESTIMATE %ld FOR %lx FROM %d\n", listastime.stima, listastime.Client_ID, num_s+1);
			fflush(NULL);
	 		L = insertList_sup(L, listastime, num_s+1);
		}
		if (r <= 0) {
			//errno=11 => Resource Temporarily Unavailable
			if(errno == 11)
				continue;
		}
	}
}

//funzione che una volta arrivati i due SIGINT mi fa terminare
static void cleanup (void) {
	printList_sup(L);
	for(int i=0; i<k; i++) {
		//per ogni processo figlio devo mandare un segnale di SIGTERM per farlo terminare
		kill(pid[i], SIGTERM);//SIGTERM che normalmente procura la conclusione dell'esecuzione dei processi destinatari
		//e successivamente chiamo la waitpid per dire al padre che il figlio i è terminato
		waitpid(pid[i], NULL, WUNTRACED); //WUNTRACED = specifica di ritornare anche se i child sono fermati (stop), e lo stato non deve venire riportato.	
	}
	free(new_pipe);
	list_stime *corr = L;
	while(corr != NULL) {
		L = corr->next;
		free(corr);
		corr = L;
	}
	free(pid);
	printf("SUPERVISOR EXITING\n");
	exit(0);
}
//gestore segnali
static void gestore(int signum) {
	if (signum == SIGINT) {
		num_sigint++;
		if (num_sigint ==1) {
			alarm(1);
		}
		if (num_sigint == 2) {
			cleanup();
		}
	}
	if (signum == SIGALRM) {
		if (num_sigint == 1) {
			num_sigint = 0;	
			printList_sup(L);
		}
	}
}
