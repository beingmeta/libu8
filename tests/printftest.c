#include "strings.h"
#include "stdlib.h"
#include "stdio.h"
#include "libu8/libu8io.h"
#include "libu8/u8timefns.h"
#include "libu8/u8printf.h"
#include "libu8/u8stdio.h"

int main(int argc,char **argv)
{
  /*
  u8_fprintf(stdout,"Did you know %s %d+%d=%d\n","that",2,2,4);
  u8_fprintf(stdout,"Did you know %s %d+%d=%lld\n","that",2,2,4);
  */
  u8_fprintf(stdout,"Did you know %s %d+%d=%d\n","that",500000,500000,1000000);
  return 0;
}
