#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BUFFER_SIZE 512

static ssize_t nlines_buffer(const char *buffer, ssize_t bs){
  ssize_t i, nlines;

  nlines = 0;
  for (i = 0; i < bs; i += 1){
    if (buffer[i] == '\n'){
      nlines += 1;
    }
  }

  return nlines;
}

static ssize_t nlines_file(int fd, char *buffer){
  ssize_t nlines = 0, bs;

  while ((bs = read(fd, buffer, BUFFER_SIZE)) > 0){
    nlines += nlines_buffer(buffer, bs);
    if (bs < BUFFER_SIZE && buffer[bs - 1] != '\n'){
      nlines += 1;
    }
  }

  if (bs == -1){
    return -1;
  }

  return nlines;
}

static int skip_nlines(int fd, ssize_t nlines, char *buffer){

  ssize_t skipped = 0, bs, i;

  while (skipped < nlines){

    if ((bs = read(fd, buffer, BUFFER_SIZE)) <= 0){
      return -1;
    }

    for (i = 0; i < bs && skipped < nlines; i += 1){
      if (buffer[i] == '\n'){
        skipped += 1;
        if (skipped == nlines){
          off_t step_back = bs - i - 1;
          if (step_back > 0){
            if (lseek(fd, -step_back,SEEK_CUR) == -1){
              return -1;
            }
          }
        }
      }
    } /* endfor */

  } /* endwhile */

  return 0;
}

static int print_remaining_file_content(int fd, char *buffer){

  ssize_t bs;

  while ((bs = read(fd, buffer, BUFFER_SIZE)) > 0){
    write(1, buffer, bs);
  }

  return bs == -1 ? -1 : 0;
}

static int tail_simple(int fd, ssize_t n){
  char buffer[BUFFER_SIZE];
  ssize_t nlines, nlines_to_skip;

  if (n == 0){
    return 0;
  }

  if ((nlines = nlines_file(fd, buffer)) == -1){
    return -1;
  }

  if (lseek(fd, (off_t)0, SEEK_SET) == -1){
    return -1;
  }

  nlines_to_skip = (nlines >= n ? nlines - n : 0);
  if (nlines_to_skip > 0){
    if (skip_nlines(fd, nlines - n, buffer) == -1){
      return -1;
    }
  }

  if (print_remaining_file_content(fd, buffer) == -1){
    return -1;
  }

  return 0;
}

int main(int ac, char **av){

  int ret, n, fd;

  if (ac < 3){
    fprintf(stderr, "invlid syntax\n");
    exit(EXIT_FAILURE);
  }

  n = atoi(av[2]);
  if (n < 0){
    fprintf(stderr, "invlid integer n\n");
    exit(EXIT_FAILURE);
  }

  if ((fd = open(av[1], O_RDONLY)) == -1){
    perror(av[1]);
    exit(EXIT_FAILURE);
  }

  ret = tail_simple(fd, (ssize_t)n);
  close(fd);

  if (ret == -1){
    perror(av[1]);
    exit(EXIT_FAILURE);
  }

  return EXIT_SUCCESS;
}
