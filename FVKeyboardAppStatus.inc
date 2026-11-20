/* Diagnostics only. Not an authorization input. No keyboard/user content. */
static unsigned appStatusStage;
static double appStatusLast;
static void writeAppStatus(unsigned stage){
 if(stage<appStatusStage)return;
 double now=CFAbsoluteTimeGetCurrent();
 if(stage==appStatusStage&&now>=appStatusLast&&now-appStatusLast<2)return;
 appStatusStage=stage;appStatusLast=now;
 id bundle=M0(M0(objc_getClass("NSBundle"),"mainBundle"),"bundleIdentifier");
 if(!isKind(bundle,"NSString"))return;
 id packet=M0(objc_getClass("NSMutableDictionary"),"dictionary");
 M2(packet,"setObject:forKey:",bundle,text("bundleID"));
 M2(packet,"setObject:forKey:",text("0.1.20"),text("version"));
 const char *fields[]={"schema","pid","stamp","stage"};double numbers[]={1,getpid(),now,stage};
 for(unsigned i=0;i<4;i++){
  id value=((id(*)(id,SEL,double))objc_msgSend)(objc_getClass("NSNumber"),S("numberWithDouble:"),numbers[i]);
  M2(packet,"setObject:forKey:",value,text(fields[i]));
 }
 id error=0;id data=((id(*)(id,SEL,id,U,U,id*))objc_msgSend)(objc_getClass("NSPropertyListSerialization"),S("dataWithPropertyList:format:options:error:"),packet,(U)100,(U)0,&error);
 if(!isKind(data,"NSData"))return;
 U length=(U)M0(data,"length");const void *bytes=M0(data,"bytes");if(!bytes||!length||length>2048)return;
 id path=M1(NSHomeDirectory(),"stringByAppendingPathComponent:",text("Library/Caches/com.minis.floatingview.keyboard-status.plist"));
 const char *file=((const char*(*)(id,SEL))objc_msgSend)(path,S("fileSystemRepresentation"));if(!file)return;
 /* Partial diagnostic receipts are rejected by the host parser. Never read
  * this file as an enabled/inner ticket or use it to change geometry. */
 int fd=open(file,1|0x200|0x400|0x100|0x1000000|4,0600);
 if(fd>=0){write(fd,bytes,length);close(fd);}
}
