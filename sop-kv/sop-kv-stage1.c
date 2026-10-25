#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

void usage(char *pname)
{
    fprintf(stderr, "USAGE: %s -d STORE_DIR\n", pname);
    exit(EXIT_FAILURE);
}

// line: one line from stdin, already without '\n'
// first word is the command; empty line is ignored,
// SET, GET, DEL, LIST -> print the command name, anything else -> print "unknown"
void process_line(char *line)
{
    char *cmd = strtok(line, " ");
    if (cmd == NULL)
        return;
    // TODO: recognize the command and print it
}

// parse -d STORE_DIR, create STORE_DIR if it does not exist (existing dir is not an error),
// then read stdin line by line until EOF and pass each line to process_line
int main(int argc, char **argv)
{
    char *dir = NULL;
    int c;
    while ((c = getopt(argc, argv, "d:")) != -1)
        switch (c)
        {
            case 'd':
                dir = optarg;
                break;
            default:
                usage(argv[0]);
        }
    if (dir == NULL || argc > optind)
        usage(argv[0]);

    // TODO: create STORE_DIR

    char *line = NULL;
    size_t n = 0;
    while (getline(&line, &n, stdin) != -1)
    {
        char *nl = strchr(line, '\n');
        if (nl)
            *nl = 0;
        process_line(line);
    }
    free(line);
    return EXIT_SUCCESS;
}
