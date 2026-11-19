#ifdef FV_KEYBOARD_APP_AVOIDANCE_BRIDGE
extern double CFAbsoluteTimeGetCurrent(void);
static char fvKABSceneKey,fvKABStampKey;
static const char *fvKABPath="/var/mobile/Library/Preferences/com.minis.floatingview.keyboard-layout.plist";
static void fvKABPutNumber(id d,const char *key,double n){
 id v=((id(*)(id,SEL,double))objc_msgSend)(objc_getClass("NSNumber"),S("numberWithDouble:"),n);fvGPut(d,key,v);
}
static id fvKABSceneName(id host){
 id scene=ivar(host,"appScene");if(!fvGHas(scene,"identifier"))return 0;
 id name=SEND(scene,"identifier");return fvGKind(name,objc_getClass("NSString"))?name:0;
}
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
 id name=fvKABSceneName(host),screen=SEND(window,"screen");if(!name||!screen)return;
 Rect target=rect(settings,"frame"),source=rect(screen,"bounds");
 if(!fvGRect(target)||!fvGRect(source)||fabs(target.size.height-target.size.width*16.0/9.0)>.5||
    fabs(target.size.width-source.size.width)>.5||source.size.height<=target.size.height+.5)return;
 double now=CFAbsoluteTimeGetCurrent();
 id prior=objc_getAssociatedObject(host,&fvKABSceneKey),priorStamp=objc_getAssociatedObject(host,&fvKABStampKey);
 double previous=priorStamp?((double(*)(id,SEL))objc_msgSend)(priorStamp,S("doubleValue")):0;
 if(prior&&((BOOL(*)(id,SEL,id))objc_msgSend)(prior,S("isEqualToString:"),name)&&now>=previous&&now-previous<2)return;
 id packet=SEND(objc_getClass("NSMutableDictionary"),"dictionary");
 fvGPut(packet,"scene",name);fvKABPutNumber(packet,"schema",1);fvKABPutNumber(packet,"enabled",1);
 fvKABPutNumber(packet,"inner",1);fvKABPutNumber(packet,"stamp",now);
 fvKABPutNumber(packet,"targetX",0);fvKABPutNumber(packet,"targetY",0);
 fvKABPutNumber(packet,"targetWidth",target.size.width);fvKABPutNumber(packet,"targetHeight",target.size.height);
 fvKABPutNumber(packet,"sourceX",source.origin.x);fvKABPutNumber(packet,"sourceY",source.origin.y);
 fvKABPutNumber(packet,"sourceWidth",source.size.width);fvKABPutNumber(packet,"sourceHeight",source.size.height);
 BOOL ok=((BOOL(*)(id,SEL,id,BOOL))objc_msgSend)(packet,S("writeToFile:atomically:"),str(fvKABPath),1);
 if(ok){
  objc_setAssociatedObject(host,&fvKABSceneKey,name,1);
  id stamp=((id(*)(id,SEL,double))objc_msgSend)(objc_getClass("NSNumber"),S("numberWithDouble:"),now);
  objc_setAssociatedObject(host,&fvKABStampKey,stamp,1);
 }
 static unsigned budget=4;
 if(budget){budget--;trace(ok?"KB app bridge: portrait Scene ticket published; application delivery unverified":"KB app bridge: state file write failed; app adapter must skip");}
}
static void fvKDClearHostTicket(id host){
 if(!fvGMain()||!host)return;
 id old=SEND(objc_getClass("NSDictionary"),"dictionaryWithContentsOfFile:",str(fvKABPath));
 id expected=objc_getAssociatedObject(host,&fvKABSceneKey),name=fvGGet(old,"scene");
 BOOL same=expected&&name&&((BOOL(*)(id,SEL,id))objc_msgSend)(expected,S("isEqualToString:"),name);
 objc_setAssociatedObject(host,&fvKABSceneKey,0,1);
 objc_setAssociatedObject(host,&fvKABStampKey,0,1);
 if(!same)return;
 id revoked=SEND(objc_getClass("NSMutableDictionary"),"dictionary");
 fvKABPutNumber(revoked,"schema",1);fvKABPutNumber(revoked,"enabled",0);
 ((BOOL(*)(id,SEL,id,BOOL))objc_msgSend)(revoked,S("writeToFile:atomically:"),str(fvKABPath),1);
}
#endif
