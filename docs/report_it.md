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

Una volta fatti i dovuti calcoli rispetto a caching, presenza del file, e condizioni a contorno, viene istanziato e assegnato il file descriptor relativo al file su disco, e il file viene servito leggendo in sequenza dal disco mentre il buffer fa da trasporto. Con una `sprintf` viene scritto l'header della risposta `HTTP`, mentre attraverso la `read` poi viene servito il resto del file come body della response.

```c
void handle_get(int sock_fd, char *buf, char *ext, hcontainer *headers) {
	[...]
	if ((fle = open(fn, O_RDONLY)) == -1) {
		logger(NOTFOUND, "not found", &buf[5]);
		handle_error(NOTFOUND, sock_fd);
	}

	logger(LOG, "GET", &buf[5]);
	ln = (long)lseek(fle, (off_t)0, SEEK_END);
	lseek(fle, (off_t)0, SEEK_SET);
	sprintf(
		buf,
		"HTTP/1.1 200 OK\nServer: grocery/%d.0\nContent-Length: %ld\nConnection: %s\nContent-Type: %s\n\n",
		VERSION,
		ln,
		(strlen(headers[1].val) != 0 && !strncmp(&headers[1].val[1], "close", 5)) ? "close" : "keep-alive",
		ext
	);
	write(sock_fd, buf, strlen(buf));

	while ((rt = read(fle, buf, BUFSIZE)) > 0) {
		write(sock_fd, buf, rt);
	}

	close(fle);
	[...]
}
```

Una cosa interessante da notare è che dell'eseguibile risultante dalla compilazione di Grocery è stata fatta la profilazione delle funzioni tramite Valgrind per identificare eventuali memory leak, ed il check è stato negativo rispetto a queste criticità.

## Prestazioni
Questi i risultati di un'analisi prestazionale eseguita con `httperf`. Di seguito il comando utilizzato:

```bash
httperf --server localhost --port 8080 --num-conns 10000 --rate 300 --timeout 1
```

Il comando richiede di:
- mandare 10.000 richieste;
- mandarne 300 al secondo;
- avere come soglia di timeout un secondo.

```
Total: connections 10000 requests 10000 replies 10000 test-duration 33.332 s

Connection rate: 300.0 conn/s (3.3 ms/conn, <=17 concurrent connections)
Connection time [ms]: min 1.1 avg 1.5 max 54.3 median 1.5 stddev 1.5
Connection time [ms]: connect 0.1
Connection length [replies/conn]: 1.000

Request rate: 300.0 req/s (3.3 ms/req)
Request size [B]: 62.0

Reply rate [replies/s]: min 299.8 avg 300.0 max 300.0 stddev 0.1 (6 samples)
Reply time [ms]: response 1.1 transfer 0.3
Reply size [B]: header 102.0 content 7.0 footer 0.0 (total 109.0)
Reply status: 1xx=0 2xx=10000 3xx=0 4xx=0 5xx=0

CPU time [s]: user 6.12 system 27.11 (user 18.4% system 81.3% total 99.7%)
Net I/O: 50.1 KB/s (0.4*10^6 bps)

Errors: total 0 client-timo 0 socket-timo 0 connrefused 0 connreset 0
Errors: fd-unavail 0 addrunavail 0 ftab-full 0 other 0
```

Di seguito invece le prestazioni di un server **Nginx** eseguito sulla medesima piattaforma, sulla medesima macchina:

```
Total: connections 10000 requests 10000 replies 10000 test-duration 33.331 s

Connection rate: 300.0 conn/s (3.3 ms/conn, <=5 concurrent connections)
Connection time [ms]: min 0.4 avg 0.7 max 13.4 median 0.5 stddev 0.2
Connection time [ms]: connect 0.1
Connection length [replies/conn]: 1.000

Request rate: 300.0 req/s (3.3 ms/req)
Request size [B]: 62.0

Reply rate [replies/s]: min 300.0 avg 300.0 max 300.0 stddev 0.0 (6 samples)
Reply time [ms]: response 0.6 transfer 0.0
Reply size [B]: header 238.0 content 612.0 footer 0.0 (total 850.0)
Reply status: 1xx=0 2xx=10000 3xx=0 4xx=0 5xx=0

CPU time [s]: user 7.03 system 26.20 (user 21.1% system 78.6% total 99.7%)
Net I/O: 267.2 KB/s (2.2*10^6 bps)

Errors: total 0 client-timo 0 socket-timo 0 connrefused 0 connreset 0
Errors: fd-unavail 0 addrunavail 0 ftab-full 0 other 0
```

Come è lampante anche senza un'analisi approfondita, i tempi di risposta di Nginx sono circa la metà di quelli di Grocery. L'aspetto positivo è che a fronte dei tempi di risposta nettamente superiori di Nginx, comunque il CPU time passato tra user mode e system mode tra i due è pressoché lo stesso, con un tempo relativo a Grocery leggermente maggiore nella parte system.

Ricompilando il binario di Grocery con `-O3` quindi alzando il livello di ottimizzazione, abbiamo però una notevole diminuzione delle connessioni concorrenti e del tempo di connessione medio, collegato a una notevolissima diminuzione dei massimi:

```
Total: connections 10000 requests 10000 replies 10000 test-duration 33.333 s

Connection rate: 300.0 conn/s (3.3 ms/conn, <=6 concurrent connections)
Connection time [ms]: min 1.1 avg 1.4 max 18.8 median 1.5 stddev 0.5
Connection time [ms]: connect 0.1
Connection length [replies/conn]: 1.000

Request rate: 300.0 req/s (3.3 ms/req)
Request size [B]: 62.0

Reply rate [replies/s]: min 299.8 avg 300.0 max 300.0 stddev 0.1 (6 samples)
Reply time [ms]: response 1.0 transfer 0.4
Reply size [B]: header 102.0 content 7.0 footer 0.0 (total 109.0)
Reply status: 1xx=0 2xx=10000 3xx=0 4xx=0 5xx=0

CPU time [s]: user 6.15 system 27.09 (user 18.5% system 81.3% total 99.7%)
Net I/O: 50.1 KB/s (0.4*10^6 bps)

Errors: total 0 client-timo 0 socket-timo 0 connrefused 0 connreset 0
Errors: fd-unavail 0 addrunavail 0 ftab-full 0 other 0
```

## Limitazioni riscontrate
Le limitazioni che sono state riscontrate sono esclusivamente relative al preforking, dato che ovviamente il webserver che istanzia Grocery è perfettamente in grado di scalare su un quantitativo di richieste elevato. Quello che risulta in una falla architetturale è l'arrivo di un numero di richieste superiore al migliaio per secondo, che causa parecchie connessioni rimaste appese lato client. Questo ovviamente perché si verifica un numero di file descriptor aperti superiore a quello che il sistema operativo è "programmato per gestire", nel senso più largo del termine. Questo problema è risolvibile in parte modificando le limitazioni relative ai file descriptor per sessione, ma è comunque una non-soluzione a quella che in definitiva è la criticità relativa al design scelto.

## Piattaforma
Grocery è stato sviluppato per essere eseguito su una piattaforma facente uso del sistema operativo Linux con (circa) qualsiasi versione delle GLibC. Per lo sviluppo sono stati utilizzati prevalentemente due sistemi operativi:

- Arch Linux, distribuzione Linux orientata agli approcci bleeding edge e KISS, dotata di kernel Linux 4.12;
- MacOS Sierra 10.12, utilizzato anche per il testing di interoperabilità date le metodologie scelte per il deploy dell'applicazione.

Come soluzione per l'esecuzione e l'installazione è stato scelto di utilizzare **Docker**, in modo da poter portare il software su piattaforme diverse da Linux senza doverne modificare il codice e soprattutto permettendo un'installazione senza sforzo su qualsiasi tipo di sistema operativo che supporti la containerizzazione di processi tramite questo tool.

È stato quindi usato **Docker 17** per installare Grocery dentro un container secondo le direttive immesse nel Dockerfile di cui segue il contenuto:

```dockerfile
FROM alpine:3.6

RUN apk update
RUN apk add --no-cache build-base imagemagick

RUN mkdir /grocery
RUN mkdir /grocery/www
RUN mkdir /grocery/cache

ADD . /grocery

WORKDIR /grocery
RUN make clean release

RUN apk del build-base

VOLUME /grocery/www

CMD ["./grocery", "8080"]
```

La sintassi dei Dockerfile è abbastanza verbosa e autoesplicativa di suo quindi non ci fermeremo ad analizzarla. Quello che serve sapere è che il container viene creato a partire da Alpine Linux, una distribuzione Linux minimale creata appositamente per occupare poco spazio e ottima per questo motivo per costruire container leggeri ed efficienti.

Tramite il gestore di pacchetti successivamente vengono installate le dipendenze di compilazione e di esecuzione di Grocery, che andremo a listare in seguito, e viene copiato il codice sorgente dentro il container perché possa essere compilato attraverso `make`. Successivamente viene lanciato l'effettivo goal di `make` per far partire la build, che una volta compilata dà luogo ad un eseguibile che viene poi fissato per il lancio alla partenza del container, insieme all'export del volume che consente al container di comunicare con il filesystem locale. È compito dell'utente in un secondo momento fornire una directory che Docker possa considerare come quella da cui Grocery deve leggere i file.

Per facilitare lo sviluppo e il deploy del programma, è stato utilizzato il task runner `make`, con vari comandi in base a quale build si voglia ottenere (una build di sviluppo con i simboli di debug, o una build di produzione di cui viene fatto strip di ogni orpello), e in realtà anche i comandi per costruire i container nonché quelli per compilare questo file Markdown.

Il Makefile utilizzato è il seguente:

```makefile
SOURCE := ./src/*.c
BINARY := grocery
DEBUG_PORT := 8080

CC := gcc
CFLAGS := -Wextra -Wall
OPTIMIZE := -O3
DEBUG := -g

debug:
	$(CC) $(SOURCE) $(CFLAGS) $(DEBUG) -o grocery

release:
	$(CC) $(SOURCE) $(CFLAGS) $(OPTIMIZE) -o grocery

run:
	./$(BINARY) $(DEBUG_PORT)

build_docker:
	sudo docker build -t grocery -f docker/Dockerfile .

docker_run:
	sudo docker run -d -P -p 8080:8080 \
        -v $(shell pwd)/www:/grocery/www grocery

build_docker_test:
	sudo docker build -t grocery_test -f docker/Dockerfile.test .

docker_test:
	sudo docker run -d -P -p 8080:8080 \
        --name grtest -v $(shell pwd):/grocery grocery_test \
        make debug run

markdown_to_pdf:
	pandoc -o docs/relazione.pdf docs/report_it.md \
	-V colorlinks -V fontsize="12pt" \
	--latex-engine=pdflatex

all: debug

clean:
	rm -rf *.o grocery grocery.dSYM
```

Le dipendenze per il corretto funzionamento di Gorcery sono solamente:

- GLibC o equivalenti (funzionamento testato anche con `muslc`)
- Un kernel Linux
- Imagemagick, specialmente il programma `convert`
- Un filesystem con supporto ai nomi utf8 e case sensitive.

Come dipendenze di compilazione invece abbiamo:

- `make`
- `gcc`
- `docker` (se si vuole costruire il container)
- `pandoc` (per la transizione da Markdown a PDF)

Fortunatamente gran parte dei tool utilizzati (non è un caso) fanno parte della toolchain di sviluppo base del sistema operativo, quindi vengono inclusi con relativa grossa facilità all'interno della propria distribuzione software di scelta senza troppo sforzo, quando non sono presenti in maniera predefinita.

Per il testing, viceversa, sono stati usati sia browser standard che tool a riga di comando per inviare chiamate HTTP personalizzate:

- [Mozilla Firefox](https://www.mozilla.org/it/firefox/)
- [Google Chrome](https://www.chromium.org/)
- [cURL](https://curl.haxx.se/)
- [Httpie](https://httpie.org/)

## Installazione e configurazione
Ci sono due modi sostanzialmente per avere un setup di Grocery funzionante su una macchina:

- Usare Docker o equivalenti per avere un container con un volume montato sul filesystem da cui leggere i file;
- Compilare l'eseguibile a partire dal codice sorgente su una qualsiasi distribuzione Linux.

Per comodità prenderemo come riferimento una generica release di Ubuntu.

### Installazione di Grocery su Ubuntu tramite Docker
Per avviare un'istanza di Grocery su una qualsiasi macchina possiamo installare Docker:

```bash
$ sudo apt-get install docker.io
```

E successivamente lanciare il comando che si occupa di lanciare l'immagine del container con il binding tra la porta del container esposta e una porta locale a piacere, e una directory da cui leggere i file montata come volume rispetto al mountpoint del container dove effettivamente l'eseguibile fa le sue letture (il che rende tutto configurabile via CLI):

```bash
$ sudo docker run -d -P -p 8080:8080 -v /home/user/www:/grocery/www
dottorblaster/grocery
```

### Installazione di Grocery su Ubuntu tramite compilazione del codice
Tramite una serie di comandi da shell (genericamente una Bash) andiamo ad installare le dipendenze di compilazione e di esecuzione, clonando poi il repository Git dove si trova il codice, spostandoci nella directory dove deve avvenire la build ed avviando la build con il comando `make`. In seguito possiamo avviare Grocery solo chiamando l'eseguibile e passando come argomento una porta su cui far avvenire il bind:

```bash
$ sudo apt-get install build-essential git imagemagick
$ git clone git@github.com:dottorblaster/grocery.git
$ cd grocery
$ make
$ ./grocery 8080
```

Il software tratterà la sottocartella `www` come cartella dentro la quale cercare i file che vengono richiesti dagli utenti. Se tale directory non è presente, otterremo un errore.

## Esempi di funzionamento
Di seguito alcuni esempi di funzionamento. A parte banali test tramite browser, la parte di adattamento delle risorse viene testata tramite `httpie` che permette facilmente di inviare chiamate HTTP con header personalizzati. Proviamo a richiedere una pagina statica come primo test.

Attraverso il browser possiamo testare anche che venga renderizzato l'HTML correttamente con lo stile grafico associato tramite il foglio di stile CSS dichiarato nell'header.

![Firefox renderizza la index.html servita da Grocery](docs/img/firefox.png)

Da riga di comando possiamo effettuare il medesimo test:

```html
$ http get localhost:8080
HTTP/1.1 200 OK
Connection: keep-alive
Content-Length: 201
Content-Type: text/html
Server: grocery/1.0

<!DOCTYPE html>
<html>
  <head>
    <link rel="stylesheet" type="text/css" href="style.css">
  </head>
  <body>
    <h1>Hello World!</h1>
    <p>From Grocery webserver with love.</p>
  </body>
</html>
```

Possiamo richiedere una immagine, che verrà servita esattamente come il file HTML di poc'anzi.

Andiamo a ripetere il test usando la CLI e concentrandoci sull'header della risposta:

```shell
$ http get localhost:8080/image.jpg
HTTP/1.1 200 OK
Connection: keep-alive
Content-Length: 245952
Content-Type: image/jpg
Server: grocery/1.0



+-----------------------------------------+
| NOTE: binary data not shown in terminal |
+-----------------------------------------+
```

Come possiamo vedere, la risposta è data in forma di 200 OK con un attributo `Content-Length` pari a 245952. Proviamo a rifare la stessa richiesta con un header Accept che abbassi la qualità dell'immagine in risposta:

```shell
$ http get localhost:8080/image.jpg 'Accept: image/png;q=0.1'
HTTP/1.1 200 OK
Connection: keep-alive
Content-Length: 67810
Content-Type: image/jpg
Server: grocery/1.0



+-----------------------------------------+
| NOTE: binary data not shown in terminal |
+-----------------------------------------+
```

Da questa risposta possiamo notare il medesimo header, con una variazione: il campo `Content-Length` risulta drasticamente contenuto, dato che abbiamo applicato un fattore di conversione pari al 10%. Questa immagine inoltre è stata salvata nella cache con questo grado di conversione: quando proveremo a rifare questa richiesta, il file sarà servito dal cache layer e non verrà rieffettuata la conversione.

È importante notare che il cache layer memorizza ogni tupla _nome_file - fattore_di_conversione_, quindi per un differente livello di conversione avremo un output differente e naturalmente un ulteriore artefatto nella cache:

```shell
$ http get localhost:8080/image.jpg 'Accept: image/png;q=0.4'
HTTP/1.1 200 OK
Connection: keep-alive
Content-Length: 170220
Content-Type: image/jpg
Server: grocery/1.0



+-----------------------------------------+
| NOTE: binary data not shown in terminal |
+-----------------------------------------+
```

Partendo quindi da una struttura del genere per la nostra directory www, contente i nostri file:

```bash
$ ls -l www
total 252
-rw-r--r-- 1 blaster staff 245952 Ago 12 15:09 image.jpg
-rw-r--r-- 1 blaster staff    201 Set 10 13:59 index.html
-rw-r--r-- 1 blaster staff     43 Set 10 14:49 style.css
```

Dopo svariate richieste per il file `image.jpg` di qualità talvolta identica, talvolta differente, abbiamo una cache che riflette le conversioni avvenute:

```bash
/grocery # ls -l cache
total 896
-rw-r--r--    1 root     root         67810 Sep 10 14:07 image.jpg-10
-rw-r--r--    1 root     root        148803 Sep 10 14:07 image.jpg-30
-rw-r--r--    1 root     root        170220 Sep 10 15:05 image.jpg-40
-rw-r--r--    1 root     root        230286 Sep 10 13:40 image.jpg-60
-rw-r--r--    1 root     root        287867 Sep 10 14:06 image.jpg-80
```

Tutte conversioni avvenute una volta e che abbassano di parecchio la latenza dovuta alla richiesta di un file già convertito in passato.
