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
Lorem ipsum

## Limitazioni riscontrate
Lorem ipsum

## Piattaforma
Lorem ipsum

## Installazione e configurazione
Lorem ipsum