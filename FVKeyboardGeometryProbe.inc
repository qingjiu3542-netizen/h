/* SpringBoard-only, opt-in observation of original keyboard presentation. */
#ifdef FV_KEYBOARD_GEOMETRY_PROBE
#include "FVKeyboardModeCore.h"
extern int snprintf(char *,U,const char *,...);
static IMP fvKGPOriginalFrame;
static unsigned fvKGPBudget=24;
static int fvKGPInstalled;
static char fvKGPPending;

static Rect fvKGPInScreen(id view,id window,id space){
 Rect local=((Rect(*)(id,SEL,Rect,id))objc_msgSend)(view,S("convertRect:toView:"),rect(view,"bounds"),window);
 return ((Rect(*)(id,SEL,Rect,id))objc_msgSend)(window,S("convertRect:toCoordinateSpace:"),local,space);
}
static FVRect fvKGPRect(Rect r){return (FVRect){r.origin.x,r.origin.y,r.size.width,r.size.height};}
static double fvKGPClippedHeight(id layer,id window,id space,Rect surface,unsigned *clips){
 FVRect visible=fvKGPRect(surface);
 id view=SEND(layer,"superview");
 for(unsigned n=0;view&&n<16;n++,view=SEND(view,"superview")){
  // The containing window is a presentation boundary even without clipsToBounds.
  if(view==window||((BOOL(*)(id,SEL))objc_msgSend)(view,S("clipsToBounds"))){
   FVRect bounds=fvKGPRect(fvKGPInScreen(view,window,space));
   if(!fvKAValidRect(bounds))return -1;
   (*clips)++;
   if(!fvKAClipRect(visible,bounds,&visible))return 0;
  }
  if(view==window)return visible.height;
 }
 return -1;
}
static void fvKGPReport(id work,const char *phase){
 id layer=fvGGet(work,"layer"),host=fvGGet(work,"host"),item=fvGGet(work,"item"),scene=fvGGet(work,"scene");
 id parent=SEND(layer,"superview"),window=SEND(item,"window"),layerWindow=SEND(layer,"window");
 id screen=SEND(window,"screen");
 if(enabled()&&window&&screen&&layerWindow&&SEND(layerWindow,"screen")==screen&&
    fvGTarget(item)==host&&ivar(host,"appScene")==scene&&
    !objc_getAssociatedObject(host,&fvGLeavingKey)){
  id container=parent,owner=0;
  Class ownerClass=objc_getClass("_TtC12floatingView16FVKeyboardWindow");
  for(unsigned i=0;i<5&&container;i++,container=SEND(container,"superview")){
   if(fvGKind(container,ownerClass)&&ivar(container,"keyboardView")==parent){owner=container;break;}
  }
  id space=SEND(screen,"coordinateSpace"),sceneSettings=SEND(scene,"settings");
  if(parent&&space&&fvGHas(window,"convertRect:toCoordinateSpace:")&&fvGHas(layerWindow,"convertRect:toCoordinateSpace:")&&fvGHas(sceneSettings,"frame")){
   Rect surface=fvKGPInScreen(layer,layerWindow,space),clip=fvKGPInScreen(parent,layerWindow,space),
        itemRect=fvKGPInScreen(item,window,space),logical=rect(sceneSettings,"frame");
   unsigned clips=0;
   double visibleHeight=fvKGPClippedHeight(layer,layerWindow,space,surface,&clips);
   char line[320];
   snprintf(line,sizeof line,"KB geometry phase=%s owner=%s item=%.1f,%.1f,%.1f,%.1f scene=%.1fx%.1f layer=%.1f,%.1f,%.1f,%.1f parent=%.1f,%.1f,%.1f,%.1f clipCount=%u visibleHeight=%.1f",
    phase,owner?"outer":"unconfirmed",itemRect.origin.x,itemRect.origin.y,itemRect.size.width,itemRect.size.height,
    logical.size.width,logical.size.height,surface.origin.x,surface.origin.y,surface.size.width,surface.size.height,
    clip.origin.x,clip.origin.y,clip.size.width,clip.size.height,clips,visibleHeight);
   trace(line);
  }
 }
}
static void fvKGPSample(void *context){
 id work=context,layer=fvGGet(work,"layer");
 fvKGPReport(work,"settled");
 objc_setAssociatedObject(layer,&fvKGPPending,0,0);
 SEND(work,"release");
}
static void fvKGPFrame(id layer,SEL cmd,Rect frame){
 ((void(*)(id,SEL,Rect))fvKGPOriginalFrame)(layer,cmd,frame);
 if(!fvGMain()||!fvKGPBudget||objc_getAssociatedObject(layer,&fvKGPPending))return;
 id host=layer;
 for(unsigned i=0;i<3&&host;i++)host=SEND(host,"superview");
 if(!fvGKind(host,fvGHostClass)||!enabled())return;
 id item=SEND(host,"superview"),scene=ivar(host,"appScene");
 if(!scene||fvGTarget(item)!=host||objc_getAssociatedObject(host,&fvGLeavingKey))return;
 id work=SEND(objc_getClass("NSMutableDictionary"),"dictionary");
 if(!work)return;
 fvGPut(work,"layer",layer);fvGPut(work,"host",host);fvGPut(work,"item",item);fvGPut(work,"scene",scene);
 objc_setAssociatedObject(layer,&fvKGPPending,work,0); /* cleared before retained task is released */
 fvKGPBudget--;
 fvKGPReport(work,"attached");
 dispatch_after_f(dispatch_time(0,80000000LL),&_dispatch_main_q,SEND(work,"retain"),fvKGPSample);
}
static void fvKGPInstall(void){
 if(fvKGPInstalled)return;
 id bundle=SEND(SEND(objc_getClass("NSBundle"),"mainBundle"),"bundleIdentifier");
 if(!((BOOL(*)(id,SEL,id))objc_msgSend)(bundle,S("isEqualToString:"),str("com.apple.springboard")))return;
 Class cls=objc_getClass("_UIKeyboardLayerHostView");
 Method method=cls?class_getInstanceMethod(cls,S("setFrame:")):0;
 const char *enc=method?method_getTypeEncoding(method):0;
 if(!enc)return;
 char normalized[96];unsigned n=0;int depth=0;
 for(;*enc;enc++){
  if(!depth&&*enc>='0'&&*enc<='9')continue;
  if(n+1>=sizeof normalized)return;
  normalized[n++]=*enc;
  if(*enc=='{')depth++;
  if(*enc=='}')depth--;
 }
 normalized[n]=0;
 if(depth||__builtin_strcmp(normalized,"v@:{CGRect={CGPoint=dd}{CGSize=dd}}"))return;
 fvGHook(cls,"setFrame:",(IMP)fvKGPFrame,&fvKGPOriginalFrame);
 fvKGPInstalled=1;
 trace("KB geometry probe installed");
}
#endif
