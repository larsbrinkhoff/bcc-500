#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>

#define RECORD_SIZE  1024
#define PAGE_SIZE    2048

typedef int word_t;

static void process_index(int n);
static void process_data(int n);
static void (*process_record)(int n);

word_t record[RECORD_SIZE];
char directory[10*PAGE_SIZE];
word_t idx[PAGE_SIZE];
word_t page[PAGE_SIZE];
int directory_file;
int index_pages;
int index_page;
char current_directory[50];

word_t get_word(FILE *f)
{
  int c1 = fgetc(f), c2, c3;
  if (c1 == EOF)
    return -1;
  c2 = fgetc(f);
  if (c2 == EOF)
    return -1;
  c3 = fgetc(f);
  if (c3 == EOF)
    return -1;
  return (c1 << 16) | (c2 << 8) | c3;
}

uint32_t get_reclen(FILE *f)
{
  int c1 = fgetc(f), c2, c3, c4;
  if (c1 == EOF)
    return -1;
  c2 = fgetc(f);
  if (c2 == EOF)
    return -1;
  c3 = fgetc(f);
  if (c3 == EOF)
    return -1;
  c4 = fgetc(f);
  if (c4 == EOF)
    return -1;
  return c1 | (c2 << 8) | (c3 << 16) | (c4 << 24);
}

int error (const char *message)
{
  fprintf(stderr, "Error: %s\n", message);
  return -1;
}

int get_record(FILE *f, word_t *buffer)
{
  int n1 = get_reclen(f), n2, i;
  if (n1 == -1)
    return error("Physical end of tape.");
  if (n1 == 0)
    return 0;
  if (n1 > 3*RECORD_SIZE)
    return error("Record too large.");
  for (i = 0; i < n1 / 3; i++) {
    buffer[i] = get_word(f);
    if (feof(f))
      return error("Record incomplete.\n");
  }
  for (i = 0; i < n1 % 3; i++) {
    if (fgetc(f) == EOF)
      return error("Record incomplete.\n");
  }
  n2 = get_reclen(f);
  if (n1 != n2)
    return error("Malformed record framing.\n");
  return n1 / 3;
}

int ascii6(int x)
{
  x &= 077;
  return x + 040;
}

int ascii8(int x)
{
  x &= 0377;
  if (x < 0200) {
    x += 040;
    x &= 0177;
  }
  return x;
}

void dump_record(int n)
{
  int i;
  for (i = 0; i < n; i += 4) {
    printf("%08o %08o %08o %08o   ",
           record[i+0], record[i+1], record[i+2], record[i+3]);

    putchar(ascii6(record[i+0] >> 18));
    putchar(ascii6(record[i+0] >> 12));
    putchar(ascii6(record[i+0] >>  6));
    putchar(ascii6(record[i+0] >>  0));
    putchar(ascii6(record[i+1] >> 18));
    putchar(ascii6(record[i+1] >> 12));
    putchar(ascii6(record[i+1] >>  6));
    putchar(ascii6(record[i+1] >>  0));
    putchar(ascii6(record[i+2] >> 18));
    putchar(ascii6(record[i+2] >> 12));
    putchar(ascii6(record[i+2] >>  6));
    putchar(ascii6(record[i+2] >>  0));
    putchar(ascii6(record[i+3] >> 18));
    putchar(ascii6(record[i+3] >> 12));
    putchar(ascii6(record[i+3] >>  6));
    putchar(ascii6(record[i+3] >>  0));
    
    fputs("   ", stdout);

    putchar(ascii8(record[i+0] >> 16));
    putchar(ascii8(record[i+0] >>  8));
    putchar(ascii8(record[i+0] >>  0));
    putchar(ascii8(record[i+1] >> 16));
    putchar(ascii8(record[i+1] >>  8));
    putchar(ascii8(record[i+1] >>  0));
    putchar(ascii8(record[i+2] >> 16));
    putchar(ascii8(record[i+2] >>  8));
    putchar(ascii8(record[i+2] >>  0));
    putchar(ascii8(record[i+3] >> 16));
    putchar(ascii8(record[i+3] >>  8));
    putchar(ascii8(record[i+3] >>  0));
    putchar('\n');
  }
}

void print_record(int n)
{
  int i;
  for (i = 0; i < n; i++) {
    if (0) {
      putchar(ascii6(record[i] >> 18));
      putchar(ascii6(record[i] >> 12));
      putchar(ascii6(record[i] >>  6));
      putchar(ascii6(record[i] >>  0));
    } else if (1) {
      putchar(ascii8(record[i] >> 16));
      putchar(ascii8(record[i] >>  8));
      putchar(ascii8(record[i] >>  0));
    } else if (0) {
      putchar(ascii8(record[i] >>  0));
      putchar(ascii8(record[i] >>  8));
      putchar(ascii8(record[i] >> 16));
    }
  }
}

static void print_ac(int x)
{
  x &= 3;
  switch(x) {
  case 0: putchar('i'); break;
  case 1: putchar('n'); break;
  case 2: putchar('r'); break;
  case 3: putchar('w'); break;
  }
}

static void print_date(int x)
{
  int year = (x >> 14) & 017;
  year += 1977;
  printf("%4d-%02d-%-2d", year, (x >> 10) & 017, (x >> 5) & 037);
}

static char * page_file(void);

static void get_directory(void)
{
  const char *p;
  char *x;
  int i;

  printf("Get directory: %d\n", directory_file);
  if (directory_file == 1) {
    strcpy(current_directory, ".");
    return;
  }
  
  p = directory;
  i = 0;
  while (i < directory_file - 2) {
    if (*p == 0) {
      strcpy(current_directory, ".");
      return;
    }
    if (*p++ == 014)
      i++;
  }

  while (*p++ != '@')
    ;
  x = current_directory;
  while (*p != 0175)
    *x++ = *p++;
  *x++ = ' ';
  p += 2;
  while (*p != ')')
    *x++ = *p++;
  *x++ = ')';
  *x = 0;
}

static void process_index(int n)
{
  int i;

  if (n == 0) {
    fprintf(stderr, "End of tape.\n");
    exit(0);
  }

  directory_file++;
  get_directory();

  memcpy(idx, record, sizeof record);
  n = get_record(stdin, idx+RECORD_SIZE);
  if (n != RECORD_SIZE) {
    fprintf(stderr, "Error: Malformed index record.\n");
    exit(1);
  }

  printf("Header:\n");
  printf("%08o %08o %08o %08o ", idx[0], idx[1], idx[2], idx[3]);
  printf("%08o %08o %08o %08o\n", idx[4], idx[5], idx[6], idx[7]);

  for (i = 8; i < PAGE_SIZE; i+= 6) {
    if (idx[i] == 0)
      continue;
    if (idx[i] == 077777777)
      continue;

    putchar(ascii6(idx[i+0] >> 18));
    putchar(ascii6(idx[i+0] >> 12));
    putchar(ascii6(idx[i+0] >>  6));
    putchar(ascii6(idx[i+0] >>  0));
    putchar(ascii6(idx[i+1] >> 18));
    putchar(ascii6(idx[i+1] >> 12));
    putchar(ascii6(idx[i+1] >>  6));
    putchar(ascii6(idx[i+1] >>  0));
    putchar(ascii6(idx[i+2] >> 18));
    putchar(ascii6(idx[i+2] >> 12));
    putchar(ascii6(idx[i+2] >>  6));
    putchar(ascii6(idx[i+2] >>  0));
    putchar(ascii6(idx[i+3] >> 18));

    putchar(' ');
    putchar(idx[i+3] & 0400000 ? 'R' : 'S');

    putchar(' ');
    switch(idx[i+3] & 0300000) {
    case 0100000: putchar(' '); break;
    case 0200000: putchar('E');
      remove(page_file());
      break;
    default:      putchar('?'); break;
    }

    putchar(' ');
    putchar('0' + ((idx[i+3] >> 12) & 7));

    printf(" %04o", idx[i+3] & 07777);

    putchar(' ');
    print_ac(idx[4] >> 22);
    print_ac(idx[4] >> 20);
    print_ac(idx[4] >> 18);
    
    putchar(' ');
    print_date(idx[4]);

    putchar('\n');
  }
  process_record = process_data;
  index_page = 0;
}

static char * page_file(void)
{
  static char file[14];
  int i = 8 + 6 * index_page;

  file[0] = ascii6(idx[i+0] >> 18);
  file[1] = ascii6(idx[i+0] >> 12);
  file[2] = ascii6(idx[i+0] >>  6);
  file[3] = ascii6(idx[i+0] >>  0);
  file[4] = ascii6(idx[i+1] >> 18);
  file[5] = ascii6(idx[i+1] >> 12);
  file[6] = ascii6(idx[i+1] >>  6);
  file[7] = ascii6(idx[i+1] >>  0);
  file[8] = ascii6(idx[i+2] >> 18);
  file[9] = ascii6(idx[i+2] >> 12);
  file[10] = ascii6(idx[i+2] >>  6);
  file[11] = ascii6(idx[i+2] >>  0);
  file[12] = ascii6(idx[i+3] >> 18);

  for(i = 12; i > 0; i--) {
    if (file[i] == ' ')
      file[i] = 0;
  }

  return file;
}

static void process_data(int n)
{
  FILE *f = NULL;
  long offset;
  int words;
  int whence;
  int i;

  if (n == 0) {
    process_record = process_index;
    return;
  }

  memcpy(page, record, sizeof record);
  n = get_record(stdin, page+RECORD_SIZE);
  if (n != RECORD_SIZE) {
    fprintf(stderr, "Error: Malformed page record.\n");
    exit(1);
  }

  do {
    i = 8 + 6 * index_page;
    if (idx[i] == 077777777) {
      printf("Skipping bad index entry\n");
      index_page++;
    }
  } while(idx[i] == 077777777);

  switch(idx[i+3] & 0300000) {
  case 0100000:
    offset = idx[i+3] & 07777;
    whence = SEEK_SET; 
    words = PAGE_SIZE;
    break;
  case 0200000:
    offset = 0;
    whence = SEEK_END;
    words = idx[i+3] & 07777;
    break;
  default:
    goto end;
  }

  printf("Directory %s, page %d, file %s, offset %lo\n",
         current_directory, index_page, page_file(), offset);

  char file_name[100];
  mkdir(current_directory, 0755);
  sprintf(file_name, "%s/%s", current_directory, page_file());
  f = fopen(file_name, "ab");
  if (f == NULL) {
    fprintf(stderr, "Error opening %s: %s\n", page_file(), strerror(errno));
    goto end;
  }
  offset *= 3*PAGE_SIZE;
  if (fseek(f, offset, whence) == -1) {
    fprintf(stderr, "Error seeking in %s: %s\n", page_file(), strerror(errno));
    goto end;
  }

  if (directory_file == 1 && whence == SEEK_END)
    offset = 3*PAGE_SIZE;

  for (i = 0; i < words; i++) {
    fputc(ascii8(page[i] >> 16), f);
    fputc(ascii8(page[i] >>  8), f);
    fputc(ascii8(page[i] >>  0), f);

    if (directory_file == 1) {
      directory[offset++] = ascii8(page[i] >> 16);
      directory[offset++] = ascii8(page[i] >>  8);
      directory[offset++] = ascii8(page[i] >>  0);
      directory[offset] = 0;
    }
  }

 end:
  if (f != NULL)
    fclose(f);
  index_page++;
}

int main(void)
{
  int n;
  memset(directory, 0, sizeof directory);
  process_record = process_index;
  directory_file = 0;
  for (;;) {
    n = get_record(stdin, record);
    if (n == -1)
      exit(0);
    //printf("Record, %d words\n", n);
    process_record(n);
    //dump_record(n);
    //print_record(n);
  }
}
