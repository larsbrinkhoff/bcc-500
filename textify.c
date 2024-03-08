#include <stdio.h>

void spaces(unsigned n)
{
  unsigned i;
  for (i = 0; i < n; i++)
    putchar(' ');
}

int main(void)
{
  int exit_code = 0;

  fprintf(stderr, "WARNING, THIS IS A DESTRUCTIVE TRANSFORMATION\n");

  for(;;) {
    int c = getchar();
    if (c == EOF)
      return exit_code;

    switch (c) {
    case 0136:
      printf("\xE2\x86\x91");
      break;
    case 0137:
      printf("\xE2\x86\x90");
      break;
    case 0140:
      printf("_");
      break;
    case 0175:
      c = getchar();
      if (c == EOF) {
        putchar(0175);
        return exit_code;
      } else {
        if (c < 040)
          c += 0140;
        else if (c < 0200)
          c -= 040;
        spaces((unsigned)c);
      }
      break;
    case 0176:
      printf("\xC2\xAC");
      break;
    case 0177:
      printf("\xE2\x86\x92");
      break;
    default:
      if (c > 0177)
        exit_code = 1;
      putchar(c);
      break;
    }
  }
}
