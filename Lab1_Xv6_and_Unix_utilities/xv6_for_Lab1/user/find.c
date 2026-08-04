#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void find(char *path, char *target_filename)
{
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    if ((fd = open(path, 0)) < 0)
    {
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    if (fstat(fd, &st) < 0)
    {
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    switch (st.type)
    {
    case T_DEVICE:
    case T_FILE:
        for (p = path + strlen(path); p >= path && *p != '/'; p--)
            ;
        p++;
        if (strcmp(p, target_filename) == 0)
        {
            printf("%s\n", path);
        }
        break;
    case T_DIR:
        if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf)
        {
            printf("find: path too long\n");
            break;
        }

        strcpy(buf, path);
        p = buf + strlen(buf);
        *p++ = '/';

        while (read(fd, &de, sizeof(de)) == sizeof(de))
        {
            if (de.inum == 0)
                continue;

            // Skip "." and ".." entries
            if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
                continue;

            // Construct the new path for the found entry
            memmove(p, de.name, DIRSIZ);
            p[DIRSIZ] = '\0';

            // Recursively call find on the new path
            find(buf, target_filename);
        }
    }
    close(fd);
}

int main(int argc, char *argv[])
{
    if (argc < 3 || argc > 3)
    {
        fprintf(2, "Usage: find path filename\n");
        exit(1);
    }
    find(argv[1], argv[2]);
    exit(0);
}