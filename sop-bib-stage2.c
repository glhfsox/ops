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
#define MAXFD 20

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

// for every regular file make a link index/by-visible-title/<name> -> ../../<path>,
// duplicate name (EEXIST) is reported and skipped
int walk(const char* path, const struct stat* s, int type, struct FTW* f)
{
    (void)s;
    if (type != FTW_F)
        return 0;
    char* link = join_paths("index/by-visible-title", path + f->base);
    char* target = join_paths("../..", path);
    if (symlink(target, link))
    {
        if (errno == EEXIST)
            fprintf(stderr, "Duplicate name: %s\n", path + f->base);
        else
            ERR("symlink");
    }
    free(link);
    free(target);
    return 0;
}

// no args: create index and index/by-visible-title, walk the library
int main(int argc, char** argv)
{
    if (argc != 1)
        usage(argc, argv);
    if (mkdir("index", 0755))
        ERR("mkdir");
    if (mkdir("index/by-visible-title", 0755))
        ERR("mkdir");
    if (nftw("library", walk, MAXFD, FTW_PHYS))
        ERR("nftw");
    return EXIT_SUCCESS;
}
