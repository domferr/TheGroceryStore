#!/bin/bash

if [ $# -ne 1 ]; then
  echo "Usage: ${0} <logfile>"
  exit
elif ! [ -e $1 ]; then
  echo "$1 non esiste"
  exit
fi

exec < $1
IFS='],'

# Leggo ogni riga del file e la metto nell'array line. La riga è della forma [Cassiere %d] stat1: value, stat2: value e
# così via oppure [Cliente %d] stat1: value, stat2: value e così via oppure semplicemente una riga di stat generale
# La lettura avviene dividendo la riga letta in porzioni separate da virgola oppure dalla parentesi quadra chiusa
while read -r -a line; do
  case ${line[0]} in
  "[Cliente "*) # se la prima porzione è la statistica del cliente
    clientid=${line[0]#"[Cliente "} # prendo l'id del cliente
    if [ $clientid -eq 1 ]; then # Stampo il titolo della colonna prima del primo cliente
      echo " _____________________________________________________________________"
      echo "| CLIENTE  | PRODOTTI |  TEMPO TOTALE  |  TEMPO IN CODA | C. VISITATE |"
    fi
    # tempo di permanenza
    line[1]=${line[1]#*: }
    # prodotti acquistati
    line[2]=${line[2]#*: }
    # code cambiate
    line[3]=${line[3]#*: }
    # tempo speso in coda
    line[4]=${line[4]#*: }
    # stampo il tutto
    printf "|%6d%4s|%6d%4s|%11s%5s|%11s%5s|%7d%6s|\n" \
      "$clientid" "" "${line[2]}" "" "${line[1]}" "" "${line[4]}" "" "${line[3]}" ""
    ;;
  "[Cassiere "*) # se la prima porzione è la statistica del cassiere
    cassaid=${line[0]#"[Cassiere "}
    if [ $cassaid -eq 0 ]; then # Stampo il titolo della colonna prima del primo cassiere
      echo " _____________________________________________________________________________________"
      echo "| CASSA | PRODOTTI ELAB. | C. SERVITI | T. T. APERTURA | T. MEDIO SERVIZIO | CHIUSURE |"
    fi
    len=${#line[@]}
    # clienti serviti
    line[1]=${line[1]#*: }
    # prodotti elaborati
    line[2]=${line[2]#*: }
    # numero di chiusure
    line[3]=${line[3]#*: }
    # imposto i valori in caso la cassa non sia stata mai aperta
    avgtime="0.000"
    opentime="0.000"
    # se la cassa è stata aperta almeno una volta
    if [ $len -ge 5 ]; then
      # periodi di apertura
      line[4]=${line[4]#*: }
      line[4]=${line[4]//"s"/"+"} # creo stringa del tipo val1 + ... + valN e uso bc per avere la somma con la virgola
      opentime=$(echo "scale = 3; ${line[4]}0" | bc)
    fi
    # se durante l'apertura è stato servito almeno un cliente
    if [ $len -eq 6 ]; then
      # tempi di servizio dei clienti serviti
      line[5]=${line[5]#*: }
      line[5]=${line[5]//"s"/"+"} # creo stringa del tipo val1 + ... + valN e uso bc per avere la somma con la virgola
      avgtime=$(echo "scale = 3; (${line[5]}0)/${line[1]}" | bc)
    fi
    # stampo il tutto
    printf "|%4s%3s|%9d%7s|%7d%5s|%10s s%4s|%11s s%6s|%6d%4s|\n" \
      "$cassaid" "" "${line[2]}" "" "${line[1]}" "" "$opentime" "" "$avgtime" "" "${line[3]}" ""
    ;;
  *) # altrimenti è una statistica generale e quindi non la elaboro ma la stampo direttamente
    echo "${line[0]}"
    ;;
  esac
done