#include "libu8/libu8.h"
#include "libu8/u8streamio.h"
#include "libu8/u8convert.h"

U8_EXPORT void u8_init_convert_c(void);

#include <strings.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

int main(int argc,char **argv)
{
  int retval=0;
  char *escape=getenv("U8ESCAPE");
  int escape_char=((escape) ? (escape[0]) : (0));
  u8_init_convert_c();
  {
    struct U8_TEXT_ENCODING *in_enc=u8_get_encoding(argv[1]);
    struct U8_TEXT_ENCODING *out_enc=u8_get_encoding(argv[2]);
    unsigned char *inbuf=malloc(1024), *writer, *outbuf;
    const unsigned char *reader;
    int bytes_read=0, out_size;
    struct U8_OUTPUT stream;
    FILE *in, *out;
    if (argc > 3) in=fopen(argv[3],"r"); else in=stdin;
    if (argc > 4) out=fopen(argv[4],"w"); else out=stdout;
    writer=inbuf; while (1) {
      int delta=fread(writer,1,1024,in);
      if (delta == 0) break;
      else if (delta<0)
	if (errno==EAGAIN) {}
	else u8_raise("Read error","u8recode",NULL);
      else {
	bytes_read=bytes_read+delta;
	inbuf=realloc(inbuf,bytes_read+1024);
	writer=inbuf+bytes_read;}}
    if (argc>3) fclose(in);
    U8_INIT_STATIC_OUTPUT(stream,bytes_read*2);
    reader=inbuf;
    u8_convert(in_enc,1,&stream,&reader,inbuf+bytes_read);
    reader=stream.u8_outbuf;
    outbuf=u8_localize(out_enc,&reader,stream.u8_outptr,
		       escape_char,0,
		       NULL,&out_size);
    retval=fwrite(outbuf,1,out_size,out);
    if (argc>4) fclose(out);}
  return 0;
}
