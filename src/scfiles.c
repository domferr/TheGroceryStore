#include "../include/scfiles.h"
#include "../include/utils.h"
#include <unistd.h>

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
        ptr = (char*) ptr + nread;
    }
    return(n - nleft); /* return >= 0 */
}

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
        ptr = (char*) ptr + nwritten;
    }
    return(n - nleft); /* return >= 0 */
}