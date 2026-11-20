/* Called only in the existing eligible publish path, never teardown. */
#include "FVKeyboardAppStatusCore.h"
static char fvKABStatusKey;
static void fvKABReadAppStatus(id host,int cacheFD,id bundle){
 unsigned char bytes[2048];U length=0;int error=0;
 int fd=openat(cacheFD,"com.minis.floatingview.keyboard-status.plist",FV_KAB_O_NONBLOCK|FV_KAB_O_NOFOLLOW|FV_KAB_O_CLOEXEC);
 id status=0;
 if(fd<0)error=1;
 else{
  while(length<sizeof bytes){long n=read(fd,bytes+length,sizeof bytes-length);if(n<0){error=2;break;}if(!n)break;length+=(U)n;}
  close(fd);
  if(!error&&length&&length<sizeof bytes){
   id data=((id(*)(id,SEL,const void*,U))objc_msgSend)(objc_getClass("NSData"),S("dataWithBytes:length:"),bytes,length);id err=0;
   status=((id(*)(id,SEL,id,U,id,id*))objc_msgSend)(objc_getClass("NSPropertyListSerialization"),S("propertyListWithData:options:format:error:"),data,(U)0,(id)0,&err);
  }
 }
 double pid=0,stage=0;const char *label="unobserved: absent/unreadable receipt (not proof of no injection)";
 if(!error){
  label="unobserved: invalid, partial or stale receipt";
  BOOL valid=fvGKind(status,objc_getClass("NSDictionary"));
  const char *fields[]={"schema","pid","stamp","stage"};double values[4]={0};
  for(unsigned i=0;valid&&i<4;i++){
   id value=fvGGet(status,fields[i]);valid=fvGKind(value,objc_getClass("NSNumber"));
   if(valid)values[i]=((double(*)(id,SEL))objc_msgSend)(value,S("doubleValue"));
  }
  id actual=fvGGet(valid?status:0,"bundleID"),version=fvGGet(valid?status:0,"version");
  valid=valid&&fvGKind(actual,objc_getClass("NSString"))&&fvGKind(version,objc_getClass("NSString"))&&
   ((BOOL(*)(id,SEL,id))objc_msgSend)(actual,S("isEqualToString:"),bundle)&&
   ((BOOL(*)(id,SEL,id))objc_msgSend)(version,S("isEqualToString:"),str("0.1.20"))&&
   fvKAStatusValid(values[0],values[1],values[2],values[3],CFAbsoluteTimeGetCurrent());
  if(valid){pid=values[1];stage=values[3];label=stage==4?"mapped-event-observed":stage==3?"keyboard-event-observed":stage==2?"adapter-installed":"main-queue-started";}
 }
 char key[160];__builtin_snprintf(key,sizeof key,"%s pid=%.0f stage=%.0f",label,pid,stage);
 id signature=str(key),previous=objc_getAssociatedObject(host,&fvKABStatusKey);
 if(previous&&((BOOL(*)(id,SEL,id))objc_msgSend)(previous,S("isEqualToString:"),signature))return;
 objc_setAssociatedObject(host,&fvKABStatusKey,signature,1);
 static unsigned budget=30;if(!budget)return;budget--;
 const char *bid=((const char*(*)(id,SEL))objc_msgSend)(bundle,S("UTF8String"));
 char message[400];__builtin_snprintf(message,sizeof message,"KB app status v0.1.20 bundle=%.200s %s",bid?bid:"unknown",key);trace(message);
}
