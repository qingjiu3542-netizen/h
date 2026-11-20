#ifdef FV_KEYBOARD_APP_AVOIDANCE_BRIDGE
extern double CFAbsoluteTimeGetCurrent(void);
extern int openat(int,const char*,int,...);extern int renameat(int,const char*,int,const char*);extern int unlinkat(int,const char*,int);
extern int fsync(int);extern int fchmod(int,unsigned short);extern long read(int,void*,U);
#define FV_KAB_O_DIRECTORY 0x00100000
#define FV_KAB_O_NOFOLLOW 0x00000100
#define FV_KAB_O_CLOEXEC 0x01000000
#define FV_KAB_O_NONBLOCK 0x00000004
#define FV_KAB_DIR_FLAGS (FV_KAB_O_DIRECTORY|FV_KAB_O_NOFOLLOW|FV_KAB_O_CLOEXEC)
#define FV_KAB_FILE_FLAGS (1|0x00000200|0x00000800|FV_KAB_O_NOFOLLOW|FV_KAB_O_CLOEXEC)
static char fvKABSceneKey,fvKABStampKey,fvKABPathKey,fvKABBundleKey;
static void fvKABPutNumber(id d,const char *key,double n){
 id v=((id(*)(id,SEL,double))objc_msgSend)(objc_getClass("NSNumber"),S("numberWithDouble:"),n);fvGPut(d,key,v);
}
static id fvKABBundleID(id host){
 id app=ivar(host,"currentActivityApp");if(!app||!fvGHas(app,"bundleIdentifier"))return 0;
 id bundle=SEND(app,"bundleIdentifier");if(!fvGKind(bundle,objc_getClass("NSString"))||!fvGHas(bundle,"length"))return 0;
 U n=(U)SEND(bundle,"length");if(!n||n>255)return 0;
 const char *s=((const char*(*)(id,SEL))objc_msgSend)(bundle,S("UTF8String"));if(!s)return 0;
 unsigned dots=0;for(const unsigned char*p=(const unsigned char*)s;*p;p++){
  if(*p=='.'){dots++;continue;}
  if(!((*p>='a'&&*p<='z')||(*p>='A'&&*p<='Z')||(*p>='0'&&*p<='9')||*p=='-'))return 0;
 }
 return dots?bundle:0;
}
static int fvKABOpenCaches(id bundle,const char **reason,id *ticketPath){
#define KABFAIL(message) do{if(reason)*reason=message;return -1;}while(0)
 Class proxyClass=objc_getClass("LSApplicationProxy");
 if(!proxyClass||!fvGHas(proxyClass,"applicationProxyForIdentifier:"))KABFAIL("LaunchServices proxy class or lookup method unavailable");
 id proxy=SEND(proxyClass,"applicationProxyForIdentifier:",bundle);
 if(!proxy||!fvGHas(proxy,"applicationIdentifier")||!fvGHas(proxy,"dataContainerURL"))KABFAIL("application proxy lacks identity or data container");
 id actual=SEND(proxy,"applicationIdentifier");
 if(!fvGKind(actual,objc_getClass("NSString"))||!((BOOL(*)(id,SEL,id))objc_msgSend)(actual,S("isEqualToString:"),bundle))KABFAIL("application proxy identifier mismatch");
 id url=SEND(proxy,"dataContainerURL");
 if(!url||!fvGHas(url,"isFileURL")||!((BOOL(*)(id,SEL))objc_msgSend)(url,S("isFileURL"))||!fvGHas(url,"path"))KABFAIL("application proxy data container is not a file URL");
 id container=SEND(url,"path"),standard=container&&fvGHas(container,"stringByStandardizingPath")?SEND(container,"stringByStandardizingPath"):0;
 const char *root=standard?((const char*(*)(id,SEL))objc_msgSend)(standard,S("UTF8String")):0;
 const char *rootA="/var/mobile/Containers/Data/Application/",*rootB="/private/var/mobile/Containers/Data/Application/";
 U prefix=root&&!__builtin_strncmp(root,rootA,__builtin_strlen(rootA))?__builtin_strlen(rootA):__builtin_strlen(rootB);
 if(!root||(__builtin_strncmp(root,rootA,__builtin_strlen(rootA))&&__builtin_strncmp(root,rootB,__builtin_strlen(rootB)))||!root[prefix])KABFAIL("application data container path outside expected mobile container root");
 for(const unsigned char*p=(const unsigned char*)root+prefix;*p;p++)if(!((*p>='0'&&*p<='9')||(*p>='a'&&*p<='f')||(*p>='A'&&*p<='F')||*p=='-'))KABFAIL("application data container identifier is malformed");
 int rootFD=open(root,FV_KAB_DIR_FLAGS);if(rootFD<0)KABFAIL("app data container directory unavailable or symlinked");
 int libraryFD=openat(rootFD,"Library",FV_KAB_DIR_FLAGS);close(rootFD);
 if(libraryFD<0)KABFAIL("app Library directory unavailable or symlinked");
 int cacheFD=openat(libraryFD,"Caches",FV_KAB_DIR_FLAGS);close(libraryFD);
 if(cacheFD<0)KABFAIL("app Caches directory unavailable or symlinked");
 if(ticketPath){
  id library=SEND(standard,"stringByAppendingPathComponent:",str("Library"));
  id caches=SEND(library,"stringByAppendingPathComponent:",str("Caches"));
  *ticketPath=SEND(caches,"stringByAppendingPathComponent:",str("com.minis.floatingview.keyboard-layout.plist"));
 }
 if(reason)*reason="resolved";return cacheFD;
#undef KABFAIL
}
static BOOL fvKABAtomicWrite(int cacheFD,id packet){
 id error=0;id data=((id(*)(id,SEL,id,U,U,id*))objc_msgSend)(objc_getClass("NSPropertyListSerialization"),S("dataWithPropertyList:format:options:error:"),packet,(U)100,(U)0,&error);
 if(!fvGKind(data,objc_getClass("NSData")))return 0;
 U length=(U)SEND(data,"length");const unsigned char *bytes=(const unsigned char*)SEND(data,"bytes");if(!bytes||!length||length>8192)return 0;
 static unsigned sequence;char temp[96];
 __builtin_snprintf(temp,sizeof temp,".fvkb-ticket-%llu-%u.tmp",(unsigned long long)(CFAbsoluteTimeGetCurrent()*1000000.0),++sequence);
 int fd=openat(cacheFD,temp,FV_KAB_FILE_FLAGS,0600);if(fd<0)return 0;
 BOOL ok=fchmod(fd,0644)==0;U written=0;
 while(ok&&written<length){long n=write(fd,bytes+written,length-written);if(n<=0){ok=0;break;}written+=(U)n;}
 if(ok&&fsync(fd)!=0)ok=0;
 if(close(fd)!=0)ok=0;
 if(ok&&renameat(cacheFD,temp,cacheFD,"com.minis.floatingview.keyboard-layout.plist")!=0)ok=0;
 if(ok&&fsync(cacheFD)!=0)ok=0;
 if(!ok)unlinkat(cacheFD,temp,0);
 return ok;
}
static id fvKABReadTicket(int cacheFD){
 int fd=openat(cacheFD,"com.minis.floatingview.keyboard-layout.plist",FV_KAB_O_NONBLOCK|FV_KAB_O_NOFOLLOW|FV_KAB_O_CLOEXEC);if(fd<0)return 0;
 unsigned char bytes[8192];U length=0;BOOL ok=1;
 while(length<sizeof bytes){long n=read(fd,bytes+length,sizeof(bytes)-length);if(n<0){ok=0;break;}if(!n)break;length+=(U)n;}
 if(length==sizeof bytes)ok=0;close(fd);if(!ok||!length)return 0;
 id data=M2(objc_getClass("NSData"),"dataWithBytes:length:",bytes,length);id error=0;
 id value=((id(*)(id,SEL,id,U,id,id*))objc_msgSend)(objc_getClass("NSPropertyListSerialization"),S("propertyListWithData:options:format:error:"),data,(U)0,(id)0,&error);
 return fvGKind(value,objc_getClass("NSDictionary"))?value:0;
}
static id fvKABSceneName(id host){
 id scene=ivar(host,"appScene");if(!fvGHas(scene,"identifier"))return 0;
 id name=SEND(scene,"identifier");return fvGKind(name,objc_getClass("NSString"))?name:0;
}
static void fvKABReport(const char*message){static unsigned budget=12;if(budget){budget--;trace(message);}}
static void fvKDPublishHost(id host){
 if(!fvGMain()||!enabled()||!host||objc_getAssociatedObject(host,&fvGLeavingKey)||objc_getAssociatedObject(host,&fvGFullscreenKey))return;
 id item=SEND(host,"superview"),window=SEND(host,"window"),scene=ivar(host,"appScene");
 id snapshot=objc_getAssociatedObject(host,&fvGStateKey),remote=ivar(host,"hostingView");
 if(!window||!scene||!snapshot||fvGGet(snapshot,"restorePending")||fvGTarget(item)!=host||SEND(remote,"superview")!=host)return;
 id parent=SEND(item,"superview"),items=SEND(parent,"subviews");U count=(U)SEND(items,"count");unsigned eligible=0;
 for(U i=0;i<count;i++){
  id next=SEND(items,"objectAtIndex:",i);
  if(fvGKind(next,fvGItemClass)&&!((BOOL(*)(id,SEL))objc_msgSend)(next,S("isHidden"))&&fvKDFlag(next,"isSideCard")==0)eligible++;
 }
 if(eligible!=1){fvKDClearHostTicket(host);return;}
 id settings=SEND(scene,"settings");
 if(!fvGHas(settings,"frame")||!fvGHas(settings,"interfaceOrientation"))return;
 long orientation=((long(*)(id,SEL))objc_msgSend)(settings,S("interfaceOrientation"));
 if(orientation!=1||fvKDMode(orientation)!=FVKAOriginalModeInner)return;
 id sceneName=fvKABSceneName(host),bundle=fvKABBundleID(host),screen=SEND(window,"screen");
 if(!sceneName||!bundle||!screen){fvKABReport("KB app bridge skipped: current activity bundle ID, Scene ID, or screen unavailable");return;}
 Rect target=rect(settings,"frame"),source=rect(screen,"bounds");
 if(!fvGRect(target)||!fvGRect(source)||fabs(target.size.height-target.size.width*16.0/9.0)>.5||
    fabs(target.size.width-source.size.width)>.5||source.size.height<=target.size.height+.5)return;
 double now=CFAbsoluteTimeGetCurrent();
 id prior=objc_getAssociatedObject(host,&fvKABSceneKey),priorStamp=objc_getAssociatedObject(host,&fvKABStampKey);
 double previous=priorStamp?((double(*)(id,SEL))objc_msgSend)(priorStamp,S("doubleValue")):0;
 id priorBundle=objc_getAssociatedObject(host,&fvKABBundleKey);
 if(prior&&priorBundle&&((BOOL(*)(id,SEL,id))objc_msgSend)(prior,S("isEqualToString:"),sceneName)&&
    ((BOOL(*)(id,SEL,id))objc_msgSend)(priorBundle,S("isEqualToString:"),bundle)&&now>=previous&&now-previous<2)return;
 const char *pathReason=0;id path=0;int cacheFD=fvKABOpenCaches(bundle,&pathReason,&path);
 if(cacheFD<0){char line[240];__builtin_snprintf(line,sizeof line,"KB app bridge skipped: %s",pathReason?pathReason:"app container unresolved");fvKABReport(line);return;}
 id packet=SEND(objc_getClass("NSMutableDictionary"),"dictionary");
 fvGPut(packet,"scene",sceneName);fvGPut(packet,"bundleID",bundle);
 fvKABPutNumber(packet,"schema",2);fvKABPutNumber(packet,"enabled",1);
 fvKABPutNumber(packet,"inner",1);fvKABPutNumber(packet,"stamp",now);
 fvKABPutNumber(packet,"targetX",0);fvKABPutNumber(packet,"targetY",0);
 fvKABPutNumber(packet,"targetWidth",target.size.width);fvKABPutNumber(packet,"targetHeight",target.size.height);
 fvKABPutNumber(packet,"sourceX",source.origin.x);fvKABPutNumber(packet,"sourceY",source.origin.y);
 fvKABPutNumber(packet,"sourceWidth",source.size.width);fvKABPutNumber(packet,"sourceHeight",source.size.height);
 BOOL ok=fvKABAtomicWrite(cacheFD,packet);close(cacheFD);
 if(ok){
  objc_setAssociatedObject(host,&fvKABSceneKey,sceneName,1);
  objc_setAssociatedObject(host,&fvKABBundleKey,bundle,1);
  objc_setAssociatedObject(host,&fvKABPathKey,path,1);
  id stamp=((id(*)(id,SEL,double))objc_msgSend)(objc_getClass("NSNumber"),S("numberWithDouble:"),now);
  objc_setAssociatedObject(host,&fvKABStampKey,stamp,1);
 }
 char line[240];__builtin_snprintf(line,sizeof line,ok?"KB app bridge: per-app Scene ticket published":"KB app bridge: per-app ticket atomic write failed");fvKABReport(line);
}
static void fvKABRevokeDeferred(void *context){
 id work=context,path=fvGGet(context,"path"),bundle=fvGGet(context,"bundleID");
 id expected=fvGGet(context,"scene"),expectedStamp=fvGGet(context,"stamp");
 const char *why=0;id currentPath=0;int cacheFD=bundle?fvKABOpenCaches(bundle,&why,&currentPath):-1;
 if(cacheFD>=0&&path&&currentPath&&((BOOL(*)(id,SEL,id))objc_msgSend)(path,S("isEqualToString:"),currentPath)){
  id old=fvKABReadTicket(cacheFD),name=fvGGet(old,"scene"),actualBundle=fvGGet(old,"bundleID"),stamp=fvGGet(old,"stamp");
  BOOL same=name&&actualBundle&&stamp&&((BOOL(*)(id,SEL,id))objc_msgSend)(expected,S("isEqualToString:"),name)&&
   ((BOOL(*)(id,SEL,id))objc_msgSend)(bundle,S("isEqualToString:"),actualBundle)&&
   ((double(*)(id,SEL))objc_msgSend)(stamp,S("doubleValue"))==((double(*)(id,SEL))objc_msgSend)(expectedStamp,S("doubleValue"));
  if(same){
   id revoked=SEND(old,"mutableCopy");fvKABPutNumber(revoked,"enabled",0);fvKABPutNumber(revoked,"stamp",CFAbsoluteTimeGetCurrent());
   if(!fvKABAtomicWrite(cacheFD,revoked))fvKABReport("KB app bridge: per-app ticket revocation write failed");
  }
 }
 if(cacheFD>=0)close(cacheFD);
 if(work)SEND(work,"release");
}
static void fvKDClearHostTicket(id host){
 if(!fvGMain()||!host)return;
 id path=objc_getAssociatedObject(host,&fvKABPathKey);
 id expected=objc_getAssociatedObject(host,&fvKABSceneKey),bundle=objc_getAssociatedObject(host,&fvKABBundleKey);
 id stamp=objc_getAssociatedObject(host,&fvKABStampKey);
 id work=SEND(objc_getClass("NSMutableDictionary"),"dictionary");
 if(path&&expected&&bundle&&stamp){
  fvGPut(work,"path",path);fvGPut(work,"scene",expected);fvGPut(work,"bundleID",bundle);fvGPut(work,"stamp",stamp);
 }
 objc_setAssociatedObject(host,&fvKABSceneKey,0,1);objc_setAssociatedObject(host,&fvKABBundleKey,0,1);
 objc_setAssociatedObject(host,&fvKABPathKey,0,1);objc_setAssociatedObject(host,&fvKABStampKey,0,1);
 if(path&&expected&&bundle&&stamp){
  SEND(work,"retain");dispatch_async_f(&_dispatch_main_q,work,fvKABRevokeDeferred);
 }
}
#endif
