#include <stdio.h>
#include <string.h>


void
print_usage(void)
{
    fprintf(stderr, "usage: ./a.out nadawca|data or ./a.out <email> <subject> <content>\n");
}

void
execute(const char *cmd)
{
    FILE *proc = popen(cmd, "w");
    pclose(proc);
}

void
send_mail(const char *email, const char *subject, const char *content)
{
    char cmd[512];
    cmd[0] = 0;

    strcat(cmd, "mail -s ");
    strcat(cmd, subject);
    strcat(cmd, " ");
    strcat(cmd, email);

    FILE *proc = popen(cmd, "w");
    
    fputs(content, proc);
    fputc('\n', proc);
    fflush(proc);

    pclose(proc);
}

int
main(int argc, const char *argv[])
{
    if (argc == 2) {
        if (strcmp(argv[1], "data") == 0) {
            execute("mail -H"
                    "| awk '{ print $5 \" \" $6 \" \" $7 \" \" $0 }'"
                    "| sort -k1M -k2n -k3"
                    "| cut -d' ' -f 4-"
                    "| cut -c 8-"
                    );

        } else if (strcmp(argv[1], "nadawca") == 0) {
            execute("mail -H"
                    "| awk '{ print $3 \" \" $0 }'"
                    "| sort -k1"
                    "| cut -d' ' -f2-"
                    "| cut -c 8-"
                    );

        } else {
            print_usage();
            return -1;
        }
    } else if(argc == 4) {
        send_mail(argv[1], argv[2], argv[3]);
    } else {
        print_usage();
        return -1;
    }

    return 0;
}
