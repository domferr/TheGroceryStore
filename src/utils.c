#include "utils.h"
#include <time.h>

int msleep(int milliseconds) {
    struct timespec req = {MS_TO_SEC(milliseconds), MS_TO_NANOSEC(milliseconds)}, rem = {0, 0};
    int res = EINTR;

    while (res == EINTR) {
        rem.tv_sec = 0;
        rem.tv_nsec = 0;
        res = nanosleep(&req, &rem);
        req = rem;
    }

    return res;
}
