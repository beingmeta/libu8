/* Byte buffers */

#define U8_BYTEBUF_DEFAULT 1024

typedef struct U8_BYTEBUF {
  unsigned char *u8_buf, *u8_ptr, *u8_lim;
  enum {u8_output_buffer,u8_input_buffer} u8_direction;
  int u8_growbuf;} U8_BYTEBUF;
typedef struct U8_BYTEBUF *u8_bytebuf;

U8_EXPORT int _u8_bufwrite(struct U8_BYTEBUF *bb,unsigned char *buf,int len);
U8_EXPORT int u8_bbreader(unsigned char *buf,int len,struct U8_BYTEBUF *bb);
U8_EXPORT int u8_bbwriter(unsigned char *buf,int len,struct U8_BYTEBUF *bb);

static int u8_bufwrite(struct U8_BYTEBUF *bb,unsigned char *buf,int len)
{
  if (len==0) return 0;
  else if ((bb->u8_buf)&&(((bb->u8_ptr)+len)<(bb->u8_lim))) {
    memcpy(bb->u8_ptr,buf,len); bb->u8_ptr=bb->u8_ptr+len;
    return len;}
  else return _u8_bufwrite(bb,buf,len);
}

static int u8_bufread(struct U8_BYTEBUF *bb,unsigned char *buf,int len)
{
  if (len==0) return 0;
  else if ((bb->u8_ptr)<(bb->u8_lim)) {
    int avail=(bb->u8_lim)-(bb->u8_ptr);
    int nbytes=((avail>len)?(len):(avail));
    memcpy(buf,(bb->u8_ptr),nbytes);
    bb->u8_ptr=bb->u8_ptr+nbytes;
    return nbytes;}
  else return -1;
}

