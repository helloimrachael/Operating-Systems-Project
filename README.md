# Operating-Systems-Project

Indice
1.	Introduzione
2.	Libreria
3.	Supervisor
4.	Server
5.	Client
6.	Makefile
7.	Test
8.	Misura
 


1.	Introduzione
 Per questo progetto è stato chiesto di realizzare un sistema per l’out-of- band signaling, ovvero di realizzare un sistema client-server, in cui i client possiedono un codice secret, il secret, e vogliono comunicarlo ad un server centrale, senza però farlo intercettare da chi sta catturando i dati in transito. In questo progetto avremo n client, k server e 1 supervisor. Per primo viene lanciato il supervisor, avente come argomento il parametro k che indica il numero di server che devono essere attivati. Il supervisor creerà quindi k processi distinti. Gli n client verranno lanciati indipendentemente, ciascuno in tempi diversi.


2.	Libreria
La libreria corrisponde al file util.h, in cui sono racchiuse tutte quelle strutture e funzioni che verranno poi rispettivamente utilizzate e chiamate nei file principali, ovvero supervisor.c, server.c e client.c. Nella prima parte del file .h troveremo le varie strutture e funzioni che verranno usate dal supervisor, successivamente quelle del server ed infine quelle del client.


3.	Supervisor
Il supervisor corrisponde al file supervisor.c. Al supervisor viene passato come argomento il paramentro k, che rappresenta il numero di server che andrà a creare. Inizialmente implementa il gestore dei segnali SIGINT, SIGALRM e SIGPIPE. Successivamente effettua un ciclo for che va da zero a k, dove per ogni iterazione:
-	si crea una pipe per ogni server creato;
-	si salva la coppia di file descriptor della pipe nell’array di interi “new_pipe[i].pfd”, dove il parametro i rappresenta il numero del server e new_pipe è una struttura di tipo ppipe contenente solo l’array di interi pfd;
-	esegue una fork e si salva il suo valore di ritorno nell’array “pid” di tipo pid_t;
 
-	nel caso in cui dopo la fork ci troviamo nel figlio allora viene eseguita una execl, in cui viene avviato il server in questione, al quale viene passato il suo id e il file descriptor della pipe per la scrittura.
Una volta terminato il ciclo for, inizia un ciclo while infinito dove ad ogni iterazione sceglie un server casualmente ed effettua una readn sulla pipe di quel server. Nel caso in cui il server abbia mandato al supervisor un messaggio contenente l’ID del client che ha comunicato con quel server e la stima possibile di quel client, il supervisor dopo aver letto tali valori li inseriti nella lista “L”. Tale lista verrà stampata ogni volta che il supervisor riceverà un segnale di SIGINT. Nel caso in cui riceve un doppio SIGINT deve stampare la lista e subito dopo terminare. Prima di terminare il supervisor invoca la funzione cleanup, in modo tale dal liberare la memoria.


4.	Server
Il server corrisponde al file server.c. Al server vengono passati come argomenti il proprio id e il file descriptor in scrittura della pipe creata dal supervisor su cui andrà a scrivere un messaggio contenente l’ID del client e la stima del secret per quel client. Inizialmente il server crea l’indirizzo della socket con il quale andrà a comunicare con il client. L’indirizzo è composto dalla stringa “OOB-server-i”, dove i corrisponde al numero del server. Successivamente invoca la funzione receiving che prende come parametro l’indirizzo della socket e restituisce il file descriptor della socket che verrà salvato nella variabile fd_skt di tipo long. In questa funzione vengono invocate le tre funzioni per gestire la creazione della socket: socket, bind e listen. Una volta creata la socket il server effettua un ciclo while infinito in cui per ogni iterazione invoca la funzione accept con la quale attende le connessioni da parte dei client. Una volta accettata la connesione invoca la funzione pthread_create, con cui crea un thread per ogni connesione e ogni thread viene messo in una lista di thread, listThread, tramite la funzione insertListConnection. Alla funzione passa come argomento la funzione connection_handler nella quale:
-	invoca la readn sulla socket per leggere il messaggio inviatogli dal client contenente l’ID di quest’ultimo;
-	salva l’ID del client nella variabile id e lo porta in host byte order
con la NTHOLL;
-	si calcola il tempo di arrivo di ogni messaggio;
-	calcola una stima del secret basandosi sui tempi di arrivo dei vari messaggi;
-	quando il client chiude la socket il server salva in un buffer l’ID del client e la stima calcolata (il buffer corrisponde al messaggio da scrivere sulla pipe che collega quel server con il supervisor);
-	per inviare il messaggio al supervisor invoca la writen;
-	inviato il messaggio invoca la pthread_exit.
Come ultima cosa invoca la funzione cleanup per liberare la memoria.


5.	Client
Il client corrisponde al file client.c. Al client vengono passati come argomenti i seguenti parametri:
-	p, che rappresenta il numero di server a cui si deve collegare casualmente;
-	k, che rappresenta il numero totale di server creati;
-	w, il numero complessivo di messaggi che il client deve mandare.
Per prima cosa il client genera il suo ID casualmente con la funzione rand64bits. Questa funzione genera un intero a 64 bit. Una volta generato l’ID lo converte in network byte order. Dopodiché genera il secret casualmente. Il secret inizialmente ha come unità di misura i millisecondi, infatti viene poi portato in secondi e in nanosecondi. Una volta conclusa questa parte sceglie casualmente p server distinti e crea i vari indirizzi della socket. Una volta creati questi indirizzi, questi vengono salvati nell’array di stringhe SA. A questo punto invoca la funzione sendig, nella quale:
-	esegue un ciclo for da zero a p dove invoca la funzione socket per creare la socket e inizia un ciclo while dove invoca la funzione connect per creare una connessione con i server;
-	esegue un altro ciclo for da zero a w dove sceglie casualmente i server a cui mandare messaggi. L’invio del messaggio avviene tramite la funzione writen. 
Una volta che ha mandato tutti i w messaggi invoca la funzione cleanup
per liberare la memoria.


6.	Makefile
Nel makefile sono presenti i target all, clean e test. Attraverso il target
test viene lanciato lo script test.sh.


7.	Test.sh
Il test prevede che venga lanciato il supervisor con 8 server e, dopo un’attesa di due secondi dovrà lanciare un totale di 20 client, ciascuno con parametri 5, 8 e 20. I client vengono lanciati a coppie e con un attesa di un secondo fra ogni coppia. L’output del supervisor e del client vengono rispettivamente reindirizzati nei file supervisor.log e client.log. Dopo aver lanciato l’ultima coppia il test dovrà attendere 60 secondi inviando al supervisor un segnale di SIGINT ogni 10 secondi. Al termine dovrà lanciare un doppio SIGINT e lanciare lo script misura.


8.	Misura.sh
Allo script misura.sh vengono passati come argomenti i file supervisor.log e client.log. Questi file vengono letti riga per riga al fine di calcolare quanti secret sono stati stimati dal supervisor e calcolare l’errore medio. Più dettagliatamente:
-	Analizza il file client.log:
-	Cerca le righe che contengono la parola “SECRET” e si salva in un array di stringhe l’ID del client con il rispettivo secret;
-	Analizza il file supervisor.log:
-	Cerca le righe che contengono la parola “BASED” e si salva in un array di stringhe gli ID dei client con le rispettive stime del secret calcolate dal supervisor;
-	Confronta i due array e calcola il numero delle stime corrette, il numero delle stime sbagliate e l’errore medio.
