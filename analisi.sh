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

# Leggo ogni riga del file
while read -r -a line; do
  case ${line[0]} in
  "[Cliente "*)
    clientid=${line[0]#"[Cliente "}
    if [ $clientid -eq 20 ]; then # Stampo il titolo della colonna
      echo " _________________________________________________________________________"
      echo "| CLIENTE  | PRODOTTI |   TEMPO TOTALE   |   TEMPO IN CODA  | C. VISITATE |"
    fi
    # tempo di permanenza
    line[1]=${line[1]#*: }
    # prodotti acquistati
    line[2]=${line[2]#*: }
    # code cambiate
    line[3]=${line[3]#*: }
    # tempo speso in coda
    line[4]=${line[4]#*: }
    printf "|%6d%4s|%6d%4s|%12s%6s|%12s%6s|%7d%6s|\n" "$clientid" "" "${line[2]}" "" "${line[1]}" "" "${line[4]}" "" "${line[3]}" ""
    ;;
  "[Cassiere "*)
    cassaid=${line[0]#"[Cassiere "}
    if [ $cassaid -eq 0 ]; then # Stampo il titolo della colonna
      echo " __________________________________________________________________________________________"
      echo "|   CASSA   | PRODOTTI ELAB. | C. SERVITI | T. T. APERTURA | T. MEDIO SERVIZIO |  CHIUSURE |"
    fi
    len=${#line[@]}
    # clienti serviti
    line[1]=${line[1]#*: }
    # prodotti elaborati
    line[2]=${line[2]#*: }
    # numero di chiusure
    line[3]=${line[3]#*: }

    avgtime="0.000"
    opentime="0.000"
    # se la cassa è stata aperta almeno una volta
    if [ $len -ge 5 ]; then
      # periodi di apertura
      line[4]=${line[4]#*: }
      line[4]=${line[4]//"s"/"+"}
      opentime=$(echo "scale = 3; ${line[4]}0" | bc)
    fi
    # se durante l'apertura è stato servito almeno un cliente
    if [ $len -eq 6 ]; then
      # tempi di servizio dei clienti serviti
      line[5]=${line[5]#*: }
      line[5]=${line[5]//"s"/"+"}
      avgtime=$(echo "scale = 3; (${line[5]}0)/${line[1]}" | bc)
    fi
    printf "|%6s%5s|%9d%7s|%7d%5s|%10s s%4s|%11s s%6s|%6d%5s|\n" \
      "$cassaid" "" "${line[2]}" "" "${line[1]}" "" "$opentime" "" "$avgtime" "" "${line[3]}" ""
    ;;
  *)
    echo "${line[0]}"
    ;;
  esac
done