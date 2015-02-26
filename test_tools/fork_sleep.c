#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

extern char *optarg;

void usage(void) {
    printf("fork_sleep [-n process count] [-s sleep time]\n\
default:1024 process sleep 60 seconds");
}

int main(int argc, char *argv[]) {
    pid_t pid;
    int c, i, p = 1024, s = 60, w;

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
        pid = fork();
        if (pid > 0) {
            printf("%d: fork pid %d\n", i, pid);
            continue;
        } else if (pid < 0) {
            printf("%d: fork pid failed\n", i);
            pid = waitpid(-1, &w, 0);
            printf("%d: waitpid return %d\n", i, pid);
            i--;
            continue;
        } else {
            sleep(s);
            //printf("%d: process _exit(0)\n", i); 
            _exit(0);
	}
    }
    waitpid(0, &w, 0);
exit:
    return 0;
}
