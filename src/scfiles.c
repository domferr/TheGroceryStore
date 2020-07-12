#include "scfiles.h"
#include <sys/types.h>
#include <unistd.h>

/**
 * Legge "n" bytes da un descrittore di bytes.
 * (tratto da “Advanced Programming In the UNIX Environment” by W. Richard Stevens
 * and Stephen A. Rago, 2013, 3rd Edition, Addison-Wesley)
 * @param fd descrittore del file
 * @param ptr puntatore al buffer dentro il quale inserire i dati letti
 * @param n quanti bytes leggere
 * @return numero di bytes letti oppure -1 in caso di errore oppure 0 se la fine del file è stata raggiunta
 */
ssize_t readn(int fd, void *ptr, size_t n) {
    size_t   nleft;
    ssize_t  nread;

    nleft = n;
    while (nleft > 0) {
        if((nread = read(fd, ptr, nleft)) < 0) {
            if (nleft == n) return -1; /* error, return -1 */
            else break; /* error, return amount read so far */
        } else if (nread == 0) break; /* EOF */
        nleft -= nread;
        ptr   += nread;
    }
    return(n - nleft); /* return >= 0 */
}

/**
 * Legge fino al carattere new line (\n) ma al massimo n bytes. Ritorna il numero di bytes letti, oppure -1 in caso
 * di errore oppure 0 se la fine del file è stata raggiunta.
 * @param fd descrittore del file
 * @param ptr puntatore al buffer dentro il quale inserire i dati letti
 * @param n al massimo quanti bytes leggere
 * @param offset da quale punto del file partire per leggere i bytes
 * @return numero di bytes letti. Ritorna -1 in caso di errore oppure 0 se la fine del file viene raggiunta
 */
ssize_t readline(int fd, char *ptr, size_t n, off_t *offset) {
    ssize_t nread = 0;
    ssize_t index = 0;
    char *temp;

    if (lseek(fd, *offset, SEEK_SET) != -1)
        nread = readn(fd, ptr, n);

    if (nread == -1)
        return -1;  //errore, ritorna -1
    if (nread == 0)
        return 0;   /* EOF */

    temp = ptr;
    while (index < nread && *temp != '\n') { temp++; index++; }
    *temp = '\0';   //Aggiungo carattere di fine stringa
    if (index == nread) {   //Se il carattere '\n' non è presente
        *offset += nread;
    } else {
        *offset += index+1;
        *temp = 0;
    }

    return index;
}

/**
 * Scrive "n" bytes in un descrittore di file.
 * (tratto da “Advanced Programming In the UNIX Environment” by W. Richard Stevens
 * and Stephen A. Rago, 2013, 3rd Edition, Addison-Wesley)
 * @param fd descrittore del file
 * @param ptr puntatore al buffer dal quale scrivere
 * @param n quanti bytes scrivere
 * @return numero di bytes scritti oppure -1 in caso di errore
 */
ssize_t writen(int fd, void *ptr, size_t n) {
    size_t   nleft;
    ssize_t  nwritten;

    nleft = n;
    while (nleft > 0) {
        if((nwritten = write(fd, ptr, nleft)) < 0) {
            if (nleft == n) return -1; /* error, return -1 */
            else break; /* error, return amount written so far */
        } else if (nwritten == 0) break;
        nleft -= nwritten;
        ptr   += nwritten;
    }
    return(n - nleft); /* return >= 0 */
}