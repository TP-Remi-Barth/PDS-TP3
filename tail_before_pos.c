#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>

#define BUFFER_SIZE 512

static ssize_t index_tail_buffer(
  const char *buffer, int bufsize, ssize_t ntail, ssize_t *nlines){

  ssize_t skipped = 0, i;

  for (i = bufsize - 1; i >= 0 && skipped < ntail; i -= 1){
    if (buffer[i] == '\n'){
      skipped += 1;
    }
  }

  if (skipped == ntail){
    return i + 1;
  }
  else if (skipped < ntail){
    *nlines = skipped;
    return -1;
  }
  assert(!(skipped > ntail));
  return 0;
}

static int tail_before_pos(int fd, off_t pos, ssize_t ntail){

  char buffer[BUFFER_SIZE];
  ssize_t bs, i;
  off_t cur_pos;
  ssize_t nlines;

  if ((cur_pos = lseek(fd, -pos, SEEK_END)) == -1){
    return -1;
  }
  else if (cur_pos == 0){
    return 0;
  }
  bs = (ssize_t)cur_pos < BUFFER_SIZE ? cur_pos : BUFFER_SIZE;

  if (pos == 0){
    if (lseek(fd, (off_t)-1, SEEK_END) == -1){
      return -1;
    }
    if (read(fd, buffer, 1) != 1){
      return -1;
    }
    if (buffer[0] == '\n'){
      ntail += 1;
    }
  }

  if (lseek(fd, -(pos + bs), SEEK_END) == -1){
    return -1;
  }

  if (read(fd, buffer, bs) != bs){
    return -1;
  }

  if ((i = index_tail_buffer(buffer, bs, ntail, &nlines)) == -1){
    ntail -= nlines;
    if (tail_before_pos(fd, pos + bs, nlines) == -1){
      return -1;
    }
    write(1, buffer, bs);
  }
  else {
    write(1, buffer + i + 1, bs - i - 1);
  }
  return 0;
}

static int tail(const char *path, ssize_t ntail){
  int fd, ret;

  if ((fd = open(path, O_RDONLY)) == -1){
      perror(path);
      exit(EXIT_FAILURE);
    }

  ret = tail_before_pos(fd, (off_t)0, ntail);

  close(fd);

  return ret;
}

int main(int ac, char **av){

  int n;

  if (ac < 3){
    fprintf(stderr, "usage: %s file ntail\n", av[0]);
    exit(EXIT_FAILURE);
  }

  n = atoi(av[2]);
  if (n < 0){
    fprintf(stderr, "invlid integer n\n");
    exit(EXIT_FAILURE);
  }

  return tail(av[1], (ssize_t)n);
}
