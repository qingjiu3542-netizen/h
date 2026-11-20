#ifndef FV_KEYBOARD_HOST_ROUTE_CORE_H
#define FV_KEYBOARD_HOST_ROUTE_CORE_H
/* No geometry writes. Matching offsets are removed only outside structures. */
static inline int fvKHEncoding(const char *actual,const char *expected){
 if(!actual||!expected)return 0;
 char result[384];unsigned n=0;int depth=0,quote=0;
 for(;*actual;actual++){
  char c=*actual;
  if(c=='"'){quote=!quote;continue;}
  if(quote)continue;
  if(depth==0&&c>='0'&&c<='9')continue;
  if(n+1>=sizeof result)return 0;
  result[n++]=c;
  if(c=='{')depth++;
  if(c=='}'&&--depth<0)return 0;
 }
 result[n]=0;
 return !depth&&!quote&&!__builtin_strcmp(result,expected);
}
static inline unsigned fvKHIdentifierHash(const char *text){
 unsigned hash=2166136261u;
 if(!text)return 0;
 for(unsigned i=0;text[i]&&i<512;i++)hash=(hash^(unsigned char)text[i])*16777619u;
 return hash;
}
#endif
