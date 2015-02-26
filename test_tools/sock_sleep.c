#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>

extern char *optarg;

void usage(void) {
    printf("sock_sleep [-n socket count] [-s sleep time]\n\
default:1024 process sleep 60 seconds");
}

int main(int argc, char *argv[]) {
    int c, i, p = 10, s = 2, w, sck;

    while((c = getopt(argc, argv, "n:s:")) != -1) {
        //printf("%s getopt %c optarg %s\n", __func__, c, optarg);
        switch (c) {
        case 'n':
            p = atoi(optarg);
            break;
        case 's':
            s = atoi(optarg);
            break;
        default:
            usage();
            goto exit;
        }
    }

    for (i = 0; i < p; i++) {
        sck = socket(PF_LOCAL, SOCK_STREAM, 0); 
          if (sck >= 0) {
               printf("%d: socket %d\n", i, sck);
               continue;
           } else {
               printf("%d: socket failed\n", i);
               continue;
           }

    }

    sleep(s);
exit:
    return 0;
}
