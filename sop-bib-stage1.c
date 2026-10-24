#define _XOPEN_SOURCE 700

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <ftw.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <unistd.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

// join 2 path. returned pointer is for newly allocated memory and must be freed
char* join_paths(const char* path1, const char* path2)
{
    char* res;
    const int l1 = strlen(path1);
    if (path1[l1 - 1] == '/')
    {
        res = malloc(strlen(path1) + strlen(path2) + 1);
        if (!res)
            ERR("malloc");
        strcpy(res, path1);
    }
    else
    {
        res = malloc(strlen(path1) + strlen(path2) + 2);  // additional space for "/"
        if (!res)
            ERR("malloc");
        strcpy(res, path1);
        res[l1] = '/';
        res[l1 + 1] = 0;
    }
    return strcat(res, path2);
}

void usage(int argc, char** argv)
{
    (void)argc;
    fprintf(stderr, "USAGE: %s path\n", argv[0]);
    exit(EXIT_FAILURE);
}

// read file line by line, split each line at ':' and cut '\n',
// keep a strdup copy of author, title and genre values,
// skip lines without ':'
void read_meta(char* path, char** author, char** title, char** genre)
{
    FILE* f = fopen(path, "r");
    if (!f)
        ERR("fopen");
    *author = NULL;
    *title = NULL;
    *genre = NULL;
    char* line = NULL;
    size_t n = 0;
    while (getline(&line, &n, f) != -1)
    {
        char* sep = strchr(line, ':');
        if (!sep)
            continue;
        *sep = 0;
        char* val = sep + 1;
        char* nl = strchr(val, '\n');
        if (nl)
            *nl = 0;
        char** dst = NULL;
        if (strcmp(line, "author") == 0)
            dst = author;
        else if (strcmp(line, "title") == 0)
            dst = title;
        else if (strcmp(line, "genre") == 0)
            dst = genre;
        if (dst)
        {
            free(*dst);
            if (!(*dst = strdup(val)))
                ERR("strdup");
        }
    }
    free(line);
    if (fclose(f))
        ERR("fclose");
}

// check args, read metadata of the given file, print author, title, genre
int main(int argc, char** argv)
{
    if (argc != 2)
        usage(argc, argv);
    char *author, *title, *genre;
    read_meta(argv[1], &author, &title, &genre);
    printf("author: %s\n", author ? author : "missing!");
    printf("title: %s\n", title ? title : "missing!");
    printf("genre: %s\n", genre ? genre : "missing!");
    free(author);
    free(title);
    free(genre);
    return EXIT_SUCCESS;
}
