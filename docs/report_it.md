# Reti di Calcolatori – progetto Grocery
## Web Server con adattamento dinamico di contenuti statici
**Grocery**  è un webserver con adattamento dinamico di contenuti statici. Di seguito andremo a descrivere alcune delle particolarità di questo software, le scelte progettuali effettuate, le limitazioni riscontrate e soprattutto alcuni punti di interesse per quanto riguarda l''infrastruttura software utilizzata per lo sviluppo. Grocery infatti è un webserver scritto per essere eseguito su un sistema operativo Linux, ma che grazie ad alcuni layer di astrazione può girare su qualsiasi piattaforma che supporti la containerizzazione di processi tramite cgroups, in particolare **Docker**.

Il design del software, in linea generale, segue il **principio KISS** secondo il quale dovrebbero essere evitate complicazioni non necessarie durante il concepimento di un'architettura software di qualsiasi tipo. [Da Wikipedia](https://en.wikipedia.org/wiki/KISS_principle) una definizione di questo concetto:

> KISS is an acronym for "Keep it simple, stupid" as a design principle noted by the U.S. Navy in 1960. The KISS principle states that most systems work best if they are kept simple rather than made complicated; therefore simplicity should be a key goal in design and unnecessary complexity should be avoided. The phrase has been associated with aircraft engineer Kelly Johnson (1910–1990). The term "KISS principle" was in popular use by 1970. Variations on the phrase include "Keep it Simple, Silly", "keep it short and simple", "keep it simple and straightforward" and "keep it small and simple".

Il codice è stato suddiviso il tutto in una serie di entità logiche basate su funzioni di singola responsabilità. Questo può essere osservato a partire dalla procedura di build, basata su `make` e un `Makefile` dotato di svariati target (uno dei quali compila persino il codice sorgente di questo documento), continuando con i vari moduli in cui il codice è suddiviso.

Il risultato di questo approccio è un singolo binario, una volta soddisfatte le dipendenze di compilazione illustrate più avanti, che una volta avviato fa `bind` di un socket su una determinata porta ed è pronto, in ascolto, a ricevere richieste alle quali rispondere in modo opportuno. Come da specifiche, inoltre, è stato implementato un meccanismo di **conversione adattiva di contenuti statici** in copie con qualità inferiore, per consentire un risparmio di banda non indifferente agli utenti, facente uso di un meccanismo di caching in grado di ridurre l'overhead della conversione in caso di richiesta di un'immagine già convertita precedentemente.

## Architettura
L'architettura generale di Grocery si basa su cinque punti fondamentali elencati di seguito:

- Un **processo principale**, che esegue il `bind` sulla porta passata come parametro all'eseguibile del server;
- Un **processo figlio** che viene creato per ogni richiesta ricevuta tramite una chiamata di sistema `fork`;
- Un **request handler**, all'interno del processo figlio, che analizza la richiesta e la processa adeguatamente smistandola all'opportuna funzione che si occuperà poi di identificare eventuali errori e restituire il contenuto in una risposta `HTTP` valida.
- Un **caching layer** che fa uso del filesystem per persistere le immagini di cui è stata abbassata la qualità rispetto all'originale, mediante l'header `Accept` in capo alla richiesta;
- Il **programma *convert***, all'interno della suite Imagemagick, che si occupa di abbassare la qualità delle immagini di una data percentuale se viene fatta opportuna richiesta tramite l'header `HTTP` `Accept`.

![Diagramma architetturale del ciclo di vita applicativo](./docs/img/grocery_diagram.png)

Cenni generali sull'architettura, cenni sull'implementazione dei supported methods

Il fatto di istanziare un processo figlio tramite `fork` per ogni richiesta è una scelta tattica. Pur non essendo resiliente come un webserver con prefork di ultima generazione che fa back-pressure su un pool di processi figli a cui vengono assegnate le richieste da consumare, questo metodo permette di conservare le funzionalità di un webserver senza complicarne l'architettura di base, permettendo quindi di concentrarsi su altri aspetti che al fine di fornire le funzionalità richieste risultano maggiormente cruciali, come il parsing degli header e lo smistamento delle richieste con header opportuno allla suite ImageMagick.

Il layer di caching è architettato secondo il medesimo principio, facendo uso del filesystem per la persistenza con un meccanismo di correlazione tra file di partenza e file di destinazione basato su filename, del quale viene fatto un precalcolo in un dato momento in modo da poter poi verificare l'esistenza o meno di una copia in cache. Il meccanismo di naming della copia in cache deriva direttamente dagli header di cui viene fatto il parsing attraverso una specifica funzione che restituisce una struttura dati popolata con il valore degli header necessari al funzionamento di Grocery, e le opportune chiavi.

Una parte che merita una menzione speciale è la logica in base alla quale vengono gestiti gli handler specifici per i vari metodi HTTP. In particolare, il request handler è stato architettato in modo da essere facilmente estendibile con nuovi metodi, e per poter portare ad uno stadio futuro più avanzato l'odierno supporto minimale ad `HTTP 1.1`. Allo stato attuale si possono utilizzare solo GET e HEAD, mentre il resto dei metodi daranno luogo ad un errore gestito tramite un particolare handler denominato `unsupported_method`.

## Implementazione
A livello implementativo, Grocery non è molto più che ciò che è stato descritto a livello architetturale. La funzione main che dà origine al programma è stata divisa in due funzioni, una che fa lo spawn del server e una parte di controlli preliminari, preposta a guardia del ciclo di vita applicativo, che secondo la tecnica di programmazione difensiva, non fa altro che accertarsi che l'ambiente dove viene fatto girare l'eseguibile abbia tutte le carte in regola per funzionare.

```c
int main(int argc, char **argv) {
	preliminary_checks(argc, argv);
	spawn_server(argv);
	
	return 0;
}
```

Segue un esempio dei check preliminari con relativo incapsulamento nella rispettiva funzione di responsabilità:

```c
void preliminary_checks(int argc, char **argv) {
	if (argc != 2 || !strcmp(argv[1], "-h")) {
		print_help();
		exit(0);
	}
	[...]
}
```

All'interno della funzione `spawn_server` viene eseguito il bind del socket, e successivamente vengono eseguite le chiamate di sistema che permettono agli handler di essere eseguiti per rispondere alle connessioni `TCP` in maniera congrua. Il sorgente riportato qui di seguito è una versione modificata rispetto all'originale, che ha lo scopo solo di illustrare in linea di massima l'implementazione. È da notare la gestione degli errori inline per tutte le syscall dato che questa è la parte maggiormente _mission-critical_, nonché l'uso del logger personalizzato scritto appositamente in un modulo a parte.

```c
void spawn_server(char **argv) {
	[...]

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(atoi(argv[1]));

	optv = 1;
	l = sizeof(optv);
	if((lfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		logger(ERROR, "Error during a syscall", "socket()");
	}
	if (setsockopt(lfd, SOL_SOCKET, SO_KEEPALIVE, &optv, l) < 0) {
		logger(ERROR, "Error during a syscall", "setsockopt()");
	}
	if (bind(lfd, (struct sockaddr *)&server, sizeof(server)) < 0) {
		logger(ERROR, "Error during a syscall", "bind()");
	}
	if (listen(lfd, 64) < 0) {
		logger(ERROR, "Error during a syscall", "listen()");
	}
	[...]
}
```

E successivamente sempre all'interno della stessa funzione possiamo osservare come venga effettuato il `fork` tra processi. Il processo padre resta a gestire le nuove richieste in entrata, mentre il processo figlio entra nel suo personale ciclo di vita che lo porterà a gestire la richiesta e a terminare. La parte di `if-else` poteva essere scritta in maniera più elegante, facendo anche uso di ternary operator i quali tramite `gcc -O3` producono anche un [codice macchina migliore](http://www.nynaeve.net/?p=178) sulle CPU Intel.

```c
void spawn_server(char **argv) {
	[...]
	for (;;) {
		l = sizeof(cli);

		if ((sock_fd = accept(lfd, (struct sockaddr *)&cli, &l)) < 0) {
			logger(ERROR, "Error during a syscall", "accept");
		}

		if ((pid = fork()) < 0) {
			logger(ERROR, "Error during a syscall", "fork()");
		}
		else {
			if (pid == 0) {
				request_handler(sock_fd, 0);
			} else {
				close(sock_fd);
			}
		}
	}
}
```

Il request handler a sua volta alloca il buffer per la lettura della richiesta, altra memoria per operazioni interne, discrimina in base ai metodi supportati, e innesca la logica relativa al metodo specifico `HTTP` che è stato invocato.

```c
void request_handler(int sock_fd, int keepalive) {
	[...]
	method = whichreq(buf);
	if (!strncmp(&method[0], "get", 3)) {
		handle_get(sock_fd, buf, ext, headers);
	} else if (!strncmp(&method[0], "head", 4)) {
		handle_head(sock_fd, buf, ext);
	} else {
		handle_unsupported_method(sock_fd, buf);
	}
}
```

All'interno del request handler generico, viene anche invocata una funzione che parsa gli header della richiesta necessari al funzionamento del webserver e memorizza il tutto in una struttura dati di tipo key-value (funzione peraltro esternalizzata in un modulo a parte):

```c
char * hlook(char *label, char *buf) {
	char *tk, *str, *dbuf;
	dbuf = str = strdup(buf);
	while ((tk = strsep(&str, "\n")) != NULL) {
		if (!strncmp(&tk[0], label, strlen(label))) {
			strsep(&tk, ": ");
			free(dbuf);
			return strdup(tk);
		}
	}
	free(dbuf);
	return "";
}
```

All'interno dell'handler `handle_get` troviamo uno degli ingranaggi fondamentali, ovvero il plug relativo al meccanismo di caching, esternalizzato in una funzione:

```c
int cachehit(char *buf, hcontainer *headers) {
	[...]
    if (access(toaccess, F_OK | R_OK) == 0) {
        logger(LOG, "Cache hit!", "serving the cached file");
	} else {
        logger(LOG, "Cache didn't hit", "still need to convert the file");
        convert_img(buf, cachedfn, q);
    }

    strcpy(buf, cachedfn);
	return 1;
}
```

Se viene rilevata la necessità di dover servire un file dalla cache, viene modificato il buffer che verrà poi consumato ancora dall'handler in modo da eseguire un aliasing tra il file richiesto e la copia in cache. Successivamente, in caso di necessità di immagini con una qualità ridotta, e in caso di assenza di una copia in cache, viene effettuata la conversione on-the-fly attraverso un wrapper parametrizzato del programma `convert`.

```c
int convert_img(char *source, char *dest, int quality) {
    char cmd[sizeof(source)+sizeof(dest)+211];

    sprintf(
        cmd,
        "convert ./www/%s -quality %d ./cache/%s",
        source,
        quality,
        dest
    );

    logger(LOG, "RUNNING", cmd);

    system(cmd);

    return 1;
}
```

## Limitazioni riscontrate
Lorem ipsum

## Piattaforma
Lorem ipsum

## Installazione e configurazione
Lorem ipsum