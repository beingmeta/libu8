#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "libu8/libu8.h"
#include "libu8/u8streamio.h"

int main(int argc,char **argv)
{
  U8_OUTPUT f;
  char *scan=argv[1];
  U8_INIT_STATIC_OUTPUT(f,256);
  while (*scan) {u8_putc(&f,*scan); scan++;}
  fprintf(stderr,"%s\n",f.u8_outbuf);
  return 0;
}


