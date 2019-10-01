/* list.c */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define streq(a, b) (strcmp(a, b) == 0)

int main(int argc, char *argv[]) {
    char path[BUFSIZ];

    if (argc == 2) {
        strncpy(path, argv[1], BUFSIZ);
    } else {
        getcwd(path, BUFSIZ);
    }

    /* Unsorted listing */

    DIR *d = opendir(path);     // Open directory
    if (!d) {
        fprintf(stderr, "Unable to opendir on %s: %s\n", path, strerror(errno));
        return EXIT_FAILURE;
    }

    // Loop through directory entries
    for (struct dirent *e = readdir(d); e; e = readdir(d)) {
        if (streq(e->d_name, ".") || streq(e->d_name, ".."))
            continue;           // Skip current and parent directories

        // Display entry type, name, and inode number
        printf("%c. %s (%ld)\n", (e->d_type == DT_DIR ? 'D' : 'F'), e->d_name, e->d_ino);
    }

    closedir(d);

    /* Sorted alternative */

    struct dirent **entries;
    int n;

    // Build sorted array of entries
    n = scandir(path, &entries, NULL, alphasort);
    if (n < 0) {
        fprintf(stderr, "Unable to scandir on %s: %s\n", path, strerror(errno));
        return EXIT_FAILURE;
    }

    // Loop through sorted array of entries
    for (int i = 0; i < n; i++) {
        puts(entries[i]->d_name);
        free(entries[i]);       // Deallocate each entry
    }
    free(entries);              // Deallocate array

    return EXIT_SUCCESS;
}

/* vim: set sts=4 sw=4 ts=8 expandtab ft=c: */
