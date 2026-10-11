/*
* Programming Assignment 02: ls
* Version 1.1.0 - adds the -l (long listing) option
* Usage:
*       $ ls
*       $ ls -l
*       $ ls -l /home /etc
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <sys/stat.h>

extern int errno;

void do_ls(const char *dir, int long_flag);
void do_ls_long(const char *dir);
void mode_to_string(mode_t mode, char *str);
const char *user_name(uid_t uid, char *buf, size_t n);
const char *group_name(gid_t gid, char *buf, size_t n);

int main(int argc, char *argv[])
{
    int opt;
    int long_flag = 0;

    /* getopt scans the command line for options such as -l */
    while ((opt = getopt(argc, argv, "l")) != -1)
    {
        switch (opt)
        {
        case 'l':
            long_flag = 1;
            break;
        default:
            fprintf(stderr, "Usage: %s [-l] [directory...]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    /* optind is the index of the first non-option argument */
    if (optind == argc)
    {
        do_ls(".", long_flag);
    }
    else
    {
        for (int i = optind; i < argc; i++)
        {
            printf("Directory listing of %s : \n", argv[i]);
            do_ls(argv[i], long_flag);
            puts("");
        }
    }
    return 0;
}

void do_ls(const char *dir, int long_flag)
{
    /* Branch: long listing or the original simple display */
    if (long_flag)
    {
        do_ls_long(dir);
        return;
    }

    struct dirent *entry;
    DIR *dp = opendir(dir);
    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory : %s\n", dir);
        return;
    }
    errno = 0;
    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;
        printf("%s\n", entry->d_name);
    }

    if (errno != 0)
    {
        perror("readdir failed");
    }

    closedir(dp);
}

/* Long listing: like "ls -l". Two passes so the columns line up. */
void do_ls_long(const char *dir)
{
    DIR *dp = opendir(dir);
    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory : %s\n", dir);
        return;
    }

    struct dirent *entry;
    struct stat st;
    char path[PATH_MAX];
    char buf[64];
    char ubuf[32], gbuf[32];
    int w_link = 0, w_user = 0, w_group = 0, w_size = 0;
    long long total_blocks = 0;

    /* Pass 1: find the widest value in each column and the block total */
    errno = 0;
    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;
        snprintf(path, sizeof(path), "%s/%s", dir, entry->d_name);
        if (lstat(path, &st) == -1)
        {
            perror(path);
            continue;
        }
        total_blocks += st.st_blocks;

        int n = snprintf(buf, sizeof(buf), "%lu", (unsigned long)st.st_nlink);
        if (n > w_link)
            w_link = n;
        n = (int)strlen(user_name(st.st_uid, ubuf, sizeof(ubuf)));
        if (n > w_user)
            w_user = n;
        n = (int)strlen(group_name(st.st_gid, gbuf, sizeof(gbuf)));
        if (n > w_group)
            w_group = n;
        n = snprintf(buf, sizeof(buf), "%lld", (long long)st.st_size);
        if (n > w_size)
            w_size = n;
    }

    /* st_blocks counts 512-byte blocks, ls prints 1K blocks */
    printf("total %lld\n", total_blocks / 2);

    /* Pass 2: print every entry */
    rewinddir(dp);
    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;
        snprintf(path, sizeof(path), "%s/%s", dir, entry->d_name);
        if (lstat(path, &st) == -1)
            continue;

        char perms[11];
        mode_to_string(st.st_mode, perms);

        /* ctime() gives "Wed Jun 30 21:49:08 1993\n"; keep "Jun 30 21:49" */
        char timestr[16];
        char *t = ctime(&st.st_mtime);
        snprintf(timestr, sizeof(timestr), "%.12s", t + 4);

        printf("%s %*lu %-*s %-*s %*lld %s %s",
               perms,
               w_link, (unsigned long)st.st_nlink,
               w_user, user_name(st.st_uid, ubuf, sizeof(ubuf)),
               w_group, group_name(st.st_gid, gbuf, sizeof(gbuf)),
               w_size, (long long)st.st_size,
               timestr,
               entry->d_name);

        /* For symbolic links, show where they point */
        if (S_ISLNK(st.st_mode))
        {
            char target[PATH_MAX];
            ssize_t len = readlink(path, target, sizeof(target) - 1);
            if (len != -1)
            {
                target[len] = '\0';
                printf(" -> %s", target);
            }
        }
        printf("\n");
    }

    closedir(dp);
}

/* Convert st_mode into a string like "drwxr-xr-x" */
void mode_to_string(mode_t mode, char *str)
{
    /* Character 0: file type */
    if (S_ISDIR(mode))
        str[0] = 'd';
    else if (S_ISLNK(mode))
        str[0] = 'l';
    else if (S_ISCHR(mode))
        str[0] = 'c';
    else if (S_ISBLK(mode))
        str[0] = 'b';
    else if (S_ISFIFO(mode))
        str[0] = 'p';
    else if (S_ISSOCK(mode))
        str[0] = 's';
    else
        str[0] = '-';

    /* Owner */
    str[1] = (mode & S_IRUSR) ? 'r' : '-';
    str[2] = (mode & S_IWUSR) ? 'w' : '-';
    if (mode & S_ISUID)
        str[3] = (mode & S_IXUSR) ? 's' : 'S';
    else
        str[3] = (mode & S_IXUSR) ? 'x' : '-';

    /* Group */
    str[4] = (mode & S_IRGRP) ? 'r' : '-';
    str[5] = (mode & S_IWGRP) ? 'w' : '-';
    if (mode & S_ISGID)
        str[6] = (mode & S_IXGRP) ? 's' : 'S';
    else
        str[6] = (mode & S_IXGRP) ? 'x' : '-';

    /* Others */
    str[7] = (mode & S_IROTH) ? 'r' : '-';
    str[8] = (mode & S_IWOTH) ? 'w' : '-';
    if (mode & S_ISVTX)
        str[9] = (mode & S_IXOTH) ? 't' : 'T';
    else
        str[9] = (mode & S_IXOTH) ? 'x' : '-';

    str[10] = '\0';
}

/* Turn a user ID into a name; fall back to the number if unknown */
const char *user_name(uid_t uid, char *buf, size_t n)
{
    struct passwd *pw = getpwuid(uid);
    if (pw != NULL)
        return pw->pw_name;
    snprintf(buf, n, "%u", (unsigned)uid);
    return buf;
}

/* Turn a group ID into a name; fall back to the number if unknown */
const char *group_name(gid_t gid, char *buf, size_t n)
{
    struct group *gr = getgrgid(gid);
    if (gr != NULL)
        return gr->gr_name;
    snprintf(buf, n, "%u", (unsigned)gid);
    return buf;
}
