//
// Created by Domenico on 04/07/2020.
//

#ifndef SCFILES_H
#define SCFILES_H
#include <sys/types.h>

/**
 * Legge "n" bytes da un descrittore di bytes.
 * (tratto da “Advanced Programming In the UNIX Environment” by W. Richard Stevens
 * and Stephen A. Rago, 2013, 3rd Edition, Addison-Wesley)
 * @param fd descrittore del file
 * @param ptr puntatore al buffer dentro il quale inserire i dati letti
 * @param n quanti bytes leggere
 * @return numero di bytes letti oppure -1 in caso di errore oppure 0 se la fine del file è stata raggiunta
 */
ssize_t readn(int fd, void *ptr, size_t n);

/**
 * Legge fino al carattere new line (\n) ma al massimo n bytes. Ritorna il numero di bytes letti, oppure -1 in caso
 * di errore oppure 0 se la fine del file è stata raggiunta.
 * @param fd descrittore del file
 * @param ptr puntatore al buffer dentro il quale inserire i dati letti
 * @param n al massimo quanti bytes leggere
 * @param offset da quale punto del file partire per leggere i bytes
 * @return numero di bytes letti. Ritorna -1 in caso di errore oppure 0 se la fine del file viene raggiunta
 */
ssize_t readline(int fd, char *ptr, size_t n, off_t *offset);

/**
 * Scrive "n" bytes in un descrittore di file.
 * (tratto da “Advanced Programming In the UNIX Environment” by W. Richard Stevens
 * and Stephen A. Rago, 2013, 3rd Edition, Addison-Wesley)
 * @param fd descrittore del file
 * @param ptr puntatore al buffer dal quale scrivere
 * @param n quanti bytes scrivere
 * @return numero di bytes scritti oppure -1 in caso di errore
 */
ssize_t writen(int fd, void *ptr, size_t n);

#endif //SCFILES_H
