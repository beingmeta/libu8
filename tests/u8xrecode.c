#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "libu8/libu8io.h"
#include "libu8/u8xfiles.h"
#include "libu8/u8filefns.h"

U8_EXPORT void u8_init_convert_c(void);

int main(int argc,char **argv)
{
  struct U8_TEXT_ENCODING *in_enc, *out_enc; 
  U8_INPUT *in; U8_OUTPUT *out; int ch;
  u8_init_convert_c();
  in_enc=u8_get_encoding(argv[1]);
  out_enc=u8_get_encoding(argv[2]);
  in=(u8_input)u8_open_xinput(0,in_enc);
  out=(u8_output)u8_open_xoutput(1,out_enc);
  while ((ch=u8_getc(in))>=0) u8_putc(out,ch);
  u8_close((U8_STREAM *)out);
  return 0;
}
