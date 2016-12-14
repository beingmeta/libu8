#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "libu8/libu8io.h"
#include "libu8/u8xfiles.h"
#include "libu8/u8ctype.h"

int main(int argc,char **argv)
{
  int code=u8_entity2code(argv[1]);
  if (code<0)
    printf("unknown entity\n");
  else printf("code=0x%x\n",code);
  return 0;
}
