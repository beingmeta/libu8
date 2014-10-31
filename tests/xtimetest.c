#include "strings.h"
#include "stdlib.h"
#include "stdio.h"
#include "libu8/libu8io.h"
#include "libu8/u8timefns.h"
#include "libu8/u8printf.h"
#include "libu8/u8stdio.h"

static u8_string dow_names[7]=
  {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};

int main(int argc,char **argv)
{
  struct U8_XTIME xtime, gmtime;
  struct U8_OUTPUT out; U8_INIT_STATIC_OUTPUT(out,128);
  if (argc>1)
    u8_iso8601_to_xtime(argv[1],&xtime);
  else u8_now(&xtime);
  u8_init_xtime(&gmtime,xtime.u8_tick,u8_second,0,0,0);
  u8_printf(&out,"TICK=%ld\nISO=",xtime.u8_tick);
  u8_xtime_to_iso8601(&out,&xtime);
  u8_printf(&out,"\nISOGM=");
  u8_xtime_to_iso8601(&out,&gmtime);
  u8_printf(&out,"\nctime=%s",ctime(&xtime.u8_tick));
  u8_printf(&out,"\ndow=%s",dow_names[xtime.u8_wday]);
  u8_printf(&out,"\ngmdow=%s",dow_names[gmtime.u8_wday]);
  fprintf(stderr,"%s\n",out.u8_outbuf);
  return 0;
}
