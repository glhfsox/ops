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

// create symlink, report duplicate name (EEXIST) and keep going
void make_link(char* target, char* link)
{
    if (symlink(target, link))
    {
        if (errno == EEXIST)
            fprintf(stderr, "Duplicate name: %s\n", link);
        else
            ERR("symlink");
    }
}

// link every file into by-visible-title,
// then read its metadata: skip if no title, truncate title and genre to 64 bytes,
// link into by-title and into by-genre/<genre> (directory created on first use)
int walk(const char* path, const struct stat* s, int type, struct FTW* f)
{
    (void)s;
    if (type != FTW_F)
        return 0;
    char* link = join_paths("index/by-visible-title", path + f->base);
    char* target = join_paths("../..", path);
    make_link(target, link);
    free(link);

    char *author, *title, *genre;
    read_meta((char*)path, &author, &title, &genre);
    if (title)
    {
        if (strlen(title) > 64)
            title[64] = 0;
        link = join_paths("index/by-title", title);
        make_link(target, link);
        free(link);
        if (genre)
        {
            if (strlen(genre) > 64)
                genre[64] = 0;
            char* dir = join_paths("index/by-genre", genre);
            if (mkdir(dir, 0755) && errno != EEXIST)
                ERR("mkdir");
            link = join_paths(dir, title);
            char* target2 = join_paths("../../..", path);
            make_link(target2, link);
            free(link);
            free(target2);
            free(dir);
        }
    }
    free(target);
    free(author);
    free(title);
    free(genre);
    return 0;
}

// read 68-byte records (4 bytes size + 64 bytes title) with readv,
// look each title up in index/by-title with stat:
// ENOENT -> missing, otherwise compare sizes
void check_db(char* path)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0)
        ERR("open");
    int size;
    char title[65];
    struct iovec iov[2];
    iov[0].iov_base = &size;
    iov[0].iov_len = 4;
    iov[1].iov_base = title;
    iov[1].iov_len = 64;
    struct stat st;
    int r;
    while ((r = readv(fd, iov, 2)) > 0)
    {
        if (r != 68)
        {
            fprintf(stderr, "Invalid database\n");
            break;
        }
        title[64] = 0;
        char* link = join_paths("index/by-title", title);
        if (stat(link, &st))
        {
            if (errno == ENOENT)
                printf("Book \"%s\" is missing\n", title);
            else
                ERR("stat");
        }
        else if (st.st_size != size)
            printf("Book \"%s\" size mismatch (%d vs %d)\n", title, size, (int)st.st_size);
        free(link);
    }
    if (r < 0)
        ERR("readv");
    if (close(fd))
        ERR("close");
}

// optional arg: database path; build the index, then check the database
int main(int argc, char** argv)
{
    if (argc > 2)
        usage(argc, argv);
    if (mkdir("index", 0755))
        ERR("mkdir");
    if (mkdir("index/by-visible-title", 0755))
        ERR("mkdir");
    if (mkdir("index/by-title", 0755))
        ERR("mkdir");
    if (mkdir("index/by-genre", 0755))
        ERR("mkdir");
    if (nftw("library", walk, MAXFD, FTW_PHYS))
        ERR("nftw");
    if (argc == 2)
        check_db(argv[1]);
    return EXIT_SUCCESS;
}
