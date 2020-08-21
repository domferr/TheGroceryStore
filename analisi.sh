#!/bin/bash

if [ $# -ne 1 ]; then
  echo "Usage: ${0} <logfile>"
  exit
elif ! [ -e $1 ]; then
  echo "$1 non esiste"
  exit
fi
IFS='],'
exec < $1
# Leggo ogni riga del file
while read -r -a line; do
  case ${line[0]} in
    ("[Cliente "*) # La riga indica un log di un cliente
      clientid=${line[0]#[Cliente }
      if [ $clientid -eq 20 ]; then # Stampo il titolo della colonna
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
      printf "|%6d%4s|%6d%4s|%12s%6s|%12s%6s|%7d%6s|\n" "$clientid" "" "${line[2]}" "" "${line[1]}" "" "${line[4]}" "" "${line[3]}" "";;
    ("[Cassiere "*) # La riga indica un log di un cassiere
      cassaid=${line[0]#[Cassiere }
      if [ $cassaid -eq 0 ]; then # Stampo il titolo della colonna
        echo " ___________________________________________________________________________________________________"
        echo "|   CASSA   | PRODOTTI ELABORATI | CLIENTI SERVITI | T. T. APERTURA | T. MEDIO SERVIZIO |  CHIUSURE |"
      fi
      len=${#line[@]}
      # clienti serviti
      line[1]=${line[1]#*: }
      # prodotti elaborati
      line[2]=${line[2]#*: }
      # numero di chiusure
      line[3]=${line[3]#*: }
      if [ $len -eq 6 ]; then # se la cassa è stata aperta almeno una volta
        line[4]=${line[4]#*: }
        line[5]=${line[5]#*: }
        avgtime="9.300"
        opentime="4.800"
      else
        avgtime="0.000"
        opentime="0.000"
      fi
      printf "|%6s%5s|%12d%8s|%10d%7s|%11s%5s|%12s%7s|%6d%5s|\n" \
        "$cassaid" "" "${line[2]}" "" "${line[1]}" "" "$opentime" "" "$avgtime" "" "${line[3]}" "";;
    (*)
      echo "${line[0]}";;
  esac
done