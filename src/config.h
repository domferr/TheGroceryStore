//
// Created by Domenico on 03/07/2020.
//

#ifndef CONFIGLOADER_H
#define CONFIGLOADER_H

#define MIN_T 10

struct config {
    int k;  //numero di thread attivi come cassieri
    int c;  //numero di thread attivi come clienti all'interno del supermercato
    int e;  //quanto è grande il gruppo di clienti che entra nel supermercato.
    int t;  //massimo tempo per gli acquisti per un cliente. Deve essere > 10
    int p;  //massimo numero di prodotti che acquisterà un cliente. Deve essere > 0
    int s;  //ogni quanti millisecondi il cliente decide se spostarsi o meno
    int s1; //valore soglia per chiusura di una cassa
    int s2; //valore soglia per apertura di una cassa
};

struct config* load(char* filename);
int validate(struct config* config);
void parseRow(char* row, int row_length, struct config* config);

#endif //CONFIGLOADER_H
