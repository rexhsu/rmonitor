#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

void usage(void) {
    printf("fork_sleep [-p process count] [-s sleep time]\n\
default:1024 process sleep 60 seconds");
}

int main(int argc, char *argv[]) {
    pid_t pid;
    int c, i, p = 1024, s = 60;

    while((c = getopt(argc, argv, "n:s:")) != -1) {
        switch (c) {
        case 'n':
            p = strtol(optarg);
            break;
        case 's':
            s = strtol(optarg);
            break;
        default:
            usage();
            goto exit;
        }
    }

    for (i = 0; i < p; i++) {
        pid = fork();
        if (pid) {
            printf("%d: fork pid %d\n", i, pid);
            continue;
        } else {
            sleep(s);
            break;
	}
    }
exit:
    return 0;
}
