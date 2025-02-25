# c_project

## Descrizione del Programma

Questo programma implementa un semplice gioco di battaglia navale tra due client. Ogni client nasconde una nave in una griglia e cerca di indovinare la posizione della nave dell'avversario. Il gioco continua finché uno dei due client non indovina la posizione della nave dell'altro.

## Funzionamento del Programma

1. Il server si avvia e si mette in ascolto su una porta specifica (8080).
2. Due client si connettono al server.
3. Ogni client nasconde una nave in una griglia 5x5.
4. I client si alternano per indovinare la posizione della nave dell'avversario.
5. Il gioco termina quando uno dei due client indovina la posizione della nave dell'altro.
6. I client possono decidere se riavviare la partita o terminare il gioco.

## Dettagli Tecnici

- Il server utilizza socket TCP per la comunicazione con i client.
- Ogni client ha una griglia 5x5 per nascondere la nave e una griglia 5x5 per registrare i tentativi di indovinare la nave dell'avversario.
- Le comunicazioni tra server e client avvengono tramite messaggi inviati e ricevuti sui socket.

## Connessione dei Client tramite NC

Per connettere i client al server, è possibile utilizzare il comando `nc`:

1. Avviare il server:
   ```sh
   ./project
   ```

2. Connettere il primo client:
   ```sh
   nc indirizzo_ip_server 8080
   ```

3. Connettere il secondo client:
   ```sh
   nc indirizzo_ip_servr 8080
   ```

Una volta connessi, i client possono seguire le istruzioni visualizzate per nascondere la nave e indovinare la posizione della nave dell'avversario.

