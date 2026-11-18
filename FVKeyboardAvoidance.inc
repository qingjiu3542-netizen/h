/* Experimental outer-item avoidance, opt-in build only. Does not change
 * app input insets, Scene frame, keyboard layer ownership or native flags. */
#ifndef FV_KEYBOARD_AVOIDANCE_INCLUDED
#define FV_KEYBOARD_AVOIDANCE_INCLUDED 1
#include "FVKeyboardAvoidanceCore.h"
#ifdef FV_KEYBOARD_AVOIDANCE
extern long ivar_getOffset(void *);
extern U class_getInstanceSize(Class);
static id fvKAItems,fvKAObserver,fvKAScreen;
static Rect fvKAKeyboard;
static BOOL fvKAVisible,fvKABusy;
static char fvKAStateKey;
static Point fvKAConvertPoint(id view,Point p,id window){
 return ((Point(*)(id,SEL,Point,id))objc_msgSend)(view,S("convertPoint:toView:"),p,window);
}
static FVRect fvKARect(Rect r){return (FVRect){r.origin.x,r.origin.y,r.size.width,r.size.height};}
static BOOL fvKAIdentity(id item,id state){
 id host=fvGTarget(item);
 return host&&fvGIdentical(state,"parent",SEND(item,"superview"))&&
  fvGIdentical(state,"window",SEND(item,"window"))&&fvGIdentical(state,"scene",ivar(host,"appScene"));
}
/* No raw host pointers in notifications: weak registry -> strong snapshot. */
static void fvKARestore(id item){
 id state=objc_getAssociatedObject(item,&fvKAStateKey);if(!state)return;
 SEND(state,"retain");objc_setAssociatedObject(item,&fvKAStateKey,0,1);
 if(fvKAIdentity(item,state)){
  Point now=fvGCenter(item),last=fvGUnboxPoint(state,"last"),delta=fvGUnboxPoint(state,"delta");
  FVPoint base=fvKABaseline((FVPoint){now.x,now.y},(FVPoint){last.x,last.y},(FVPoint){delta.x,delta.y},1);
  if(fvGPoint((Point){base.x,base.y})&&(!fvGNear(now.x,base.x)||!fvGNear(now.y,base.y)))fvGSetCenter(item,(Point){base.x,base.y});
 }
 SEND(state,"release");
}
/* Read only runtime-validated native Bool ivars, never static Swift offsets. */
static int fvKAFlag(id item,const char *name){
 void *v=class_getInstanceVariable(object_getClass(item),name);if(!v)return -1;
 extern const char *ivar_getTypeEncoding(void *);
 const char *e=ivar_getTypeEncoding(v);long off=ivar_getOffset(v);
 if(!e||(__builtin_strcmp(e,"B")&&__builtin_strcmp(e,"c"))||off<0||(U)off>=class_getInstanceSize(object_getClass(item)))return -1;
 unsigned char value=*((unsigned char*)item+off);return value<=1?value:-1;
}
static BOOL fvKAEligible(id item){
 id host=fvGTarget(item),w=SEND(item,"window");
 if(!host||!w||!SEND(item,"superview")||SEND(item,"isHidden")||SEND(w,"isHidden")||!enabled())return 0;
 if(!objc_getAssociatedObject(host,&fvGStateKey)||objc_getAssociatedObject(host,&fvGLeavingKey)||objc_getAssociatedObject(host,&fvGFullscreenKey))return 0;
 if(fvGWindowCoverage(item,rect(item,"bounds"))!=0)return 0;
 /* Unknown ownership is rejected; never move every floating item. */
 return fvKAFlag(item,"isSideCard")==0&&fvKAFlag(item,"innerKeyboardIsDisplay")==1;
}
static void fvKAUpdate(id item,BOOL chosen){
 if(!chosen||!fvKAVisible||!fvKAEligible(item)){fvKARestore(item);return;}
 id w=SEND(item,"window"),parent=SEND(item,"superview"),host=fvGTarget(item);
 if(SEND(w,"screen")!=fvKAScreen){fvKARestore(item);return;}
 id state=objc_getAssociatedObject(item,&fvKAStateKey);
 if(state&&!fvKAIdentity(item,state)){objc_setAssociatedObject(item,&fvKAStateKey,0,1);state=0;}
 Point now=fvGCenter(item);FVPoint base={now.x,now.y};
 if(state){Point last=fvGUnboxPoint(state,"last"),d=fvGUnboxPoint(state,"delta");base=fvKABaseline(base,(FVPoint){last.x,last.y},(FVPoint){d.x,d.y},1);}
 Point screenNow=fvKAConvertPoint(parent,now,w),screenBase=fvKAConvertPoint(parent,(Point){base.x,base.y},w);
 Rect ir=((Rect(*)(id,SEL,Rect,id))objc_msgSend)(item,S("convertRect:toView:"),rect(item,"bounds"),w);
 ir.origin.x+=screenBase.x-screenNow.x;ir.origin.y+=screenBase.y-screenNow.y;
 id space=SEND(fvKAScreen,"coordinateSpace");
 if(!fvGHas(space,"convertRect:toCoordinateSpace:")){fvKARestore(item);return;}
 Rect kr=((Rect(*)(id,SEL,Rect,id))objc_msgSend)(space,S("convertRect:toCoordinateSpace:"),fvKAKeyboard,w);
 double shift=0;FVKAResult result=fvKAPlan(fvKARect(ir),fvKARect(kr),fvKARect(rect(w,"bounds")),8,&shift);
 if(result!=FVKAMove){fvKARestore(item);if(result==FVKANoRoom)trace("keyboard avoidance: insufficient room; viewport resizing not implemented");return;}
 Point p0=fvKAConvertPoint(parent,(Point){0,0},w),px=fvKAConvertPoint(parent,(Point){1,0},w),py=fvKAConvertPoint(parent,(Point){0,1},w);
 FVPoint delta;
 if(!fvKAVector(px.x-p0.x,px.y-p0.y,py.x-p0.x,py.y-p0.y,0,-shift,&delta)){fvKARestore(item);return;}
 Point target={base.x+delta.x,base.y+delta.y};if(!fvGPoint(target)){fvKARestore(item);return;}
 id next=SEND(objc_getClass("NSMutableDictionary"),"dictionary");
 fvGIdentity(next,"parent",parent);fvGIdentity(next,"window",w);fvGIdentity(next,"scene",ivar(host,"appScene"));
 fvGPut(next,"last",fvGBoxPoint(target));fvGPut(next,"delta",fvGBoxPoint((Point){delta.x,delta.y}));
 objc_setAssociatedObject(item,&fvKAStateKey,next,1);
 if(!fvGNear(now.x,target.x)||!fvGNear(now.y,target.y))fvGSetCenter(item,target);
}
static void fvKARefresh(void){
 if(!fvGMain()||fvKABusy||!fvKAItems)return;
 fvKABusy=1;id items=SEND(SEND(fvKAItems,"allObjects"),"retain"),chosen=0;U count=(U)SEND(items,"count");
 unsigned matches=0;
 for(U i=0;i<count;i++){id item=SEND(items,"objectAtIndex:",i);if(fvKAEligible(item)&&SEND(SEND(item,"window"),"screen")==fvKAScreen){chosen=item;matches++;}}
 if(matches!=1)chosen=0;
 for(U i=0;i<count;i++){id item=SEND(items,"objectAtIndex:",i);fvKAUpdate(item,item==chosen);}
 SEND(items,"release");fvKABusy=0;
}
static void fvKANotification(id self,SEL cmd,id note){
 (void)self;(void)cmd;if(!fvGMain())return;
 id source=SEND(note,"object");
 if(!fvGKind(source,objc_getClass("UIScreen"))){trace("keyboard avoidance: notification screen unavailable; skipped");return;}
 BOOL hiding=((BOOL(*)(id,SEL,id))objc_msgSend)(SEND(note,"name"),S("isEqualToString:"),str("UIKeyboardWillHideNotification"));
 /* A notification for another display must not replace an active session. */
 if(fvKAVisible&&source!=fvKAScreen)return;
 if(hiding){fvKAVisible=0;fvKARefresh();return;}
 id value=SEND(SEND(note,"userInfo"),"objectForKey:",str("UIKeyboardFrameEndUserInfoKey"));
 if(!fvGKind(value,objc_getClass("NSValue"))||!fvGHas(value,"CGRectValue"))return;
 const char *encoding=((const char*(*)(id,SEL))objc_msgSend)(value,S("objCType"));
 if(!encoding||__builtin_strcmp(encoding,"{CGRect={CGPoint=dd}{CGSize=dd}}"))return;
 Rect r=rect(value,"CGRectValue");if(!fvGRect(r))return;
 SEND(source,"retain");SEND(fvKAScreen,"release");fvKAScreen=source;fvKAKeyboard=r;fvKAVisible=1;fvKARefresh();
}
static void fvKAObserveItem(id item){
 if(fvKAItems&&fvGMain()&&SEND(item,"window"))SEND(fvKAItems,"addObject:",item);
}
static void fvKADetach(id item){if(!fvGMain())return;fvKARestore(item);if(fvKAItems)SEND(fvKAItems,"removeObject:",item);}
static void fvKAInstall(Class itemClass){
 (void)itemClass;if(fvKAObserver)return;
 fvKAItems=SEND(SEND(objc_getClass("NSHashTable"),"weakObjectsHashTable"),"retain");
 Class c=objc_allocateClassPair(objc_getClass("NSObject"),"FVIndependentKeyboardObserverV2",0);if(!c)return;
 class_addMethod(c,S("keyboardFrameChanged:"),(IMP)fvKANotification,"v@:@");objc_registerClassPair(c);fvKAObserver=SEND(c,"new");
 id nc=SEND(objc_getClass("NSNotificationCenter"),"defaultCenter");
 const char *names[]={"UIKeyboardWillShowNotification","UIKeyboardWillChangeFrameNotification","UIKeyboardDidChangeFrameNotification","UIKeyboardWillHideNotification"};
 for(unsigned i=0;i<4;i++)SEND(nc,"addObserver:selector:name:object:",fvKAObserver,S("keyboardFrameChanged:"),str(names[i]),(id)0);
}
#else
static void fvKARefresh(void){}
static void fvKAObserveItem(id item){(void)item;}
static void fvKADetach(id item){(void)item;}
static void fvKAInstall(Class itemClass){(void)itemClass;}
#endif
#endif
