//Yifan Yu
//filecopy.c

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    /* Parse command-line options */
    if (argc != 3) {
        fprintf(stderr, "Usage: %s SourceFile TargetFile\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *src = argv[1];
    char *dst = argv[2];

    /* Open source file for reading */
    int rfd = open(src, O_RDONLY); //read only
    if (rfd < 0) {
    	fprintf(stderr, "filecopy: Unable to open %s: %s\n", src, strerror(errno));
    	return EXIT_FAILURE;
    }

    /* Open destination file for writing */ //create write only
    int wfd = open(dst, O_CREAT|O_WRONLY, 0644); // Add mode, no funky permissions
    if (wfd < 0) {
    	fprintf(stderr, "filecopy: Unable to create %s: %s\n", dst, strerror(errno));
      close(rfd);
    	return EXIT_FAILURE;
    }

    /* Copy from source to destination */
    char buffer[BUFSIZ];
    int  nread;

    int accumulated = 0;

    while ((nread = read(rfd, buffer, BUFSIZ)) > 0) {
        int nwritten = write(wfd, buffer, nread);
        while (nwritten != nread) {                 // Write in loop
            nwritten += write(wfd, buffer + nwritten, nread - nwritten);
        }
        accumulated += nwritten;
        if(nwritten < 0){                           // Error writing
          fprintf(stderr, "filecopy: Unable to write %s: %s\n", dst, strerror(errno));
          goto ERROR;
        }
    }

    if(nread < 0){                                   // Error writing
      fprintf(stderr, "filecopy: Unable to read %s: %s\n", src, strerror(errno));
      goto ERROR;
    }

    printf("filecopy: copied %d bytes from %s to %s\n", accumulated, src, dst);

    /* Cleanup */
    close(rfd);
    close(wfd);
    return EXIT_SUCCESS;


    ERROR:
    close(rfd);
    close(wfd);
    return EXIT_FAILURE;
}
