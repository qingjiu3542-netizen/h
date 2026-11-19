/* Restore the scene before UIKit tears down the floating host relationship.
 * Pending geometry passes must never reapply the ratio after teardown starts.
 */
static IMP fvGHostWillSuperview,fvGItemWillSuperview;
static IMP fvKAItemWillMoveWindow;
static IMP fvGHostWillWindow,fvGItemWillWindow;
static void fvGRestoreScene(id h){fvGRestore(h,2);}
static void fvGRestoreRetry(void *context){
 id work=context;
 if(fvGWorkCurrent(work,&fvGLeavingKey))fvGRestoreScene(fvGGet(work,"host"));
 SEND(work,"release");
}
static void fvGLeave(id h){
 if(!fvGMain()||!h)return;
#ifdef FV_COMPLETE_MODE
 objc_setAssociatedObject(h,&fvFMMountTokenKey,0,1);
 objc_setAssociatedObject(h,&fvFMMountWorkKey,0,1);
 objc_setAssociatedObject(h,&fvFMMountRetryCountKey,0,1);
#endif
 if(!objc_getAssociatedObject(h,&fvGStateKey)||objc_getAssociatedObject(h,&fvGLeavingKey))return;
 id token=SEND(objc_getClass("NSObject"),"new");
 objc_setAssociatedObject(h,&fvGLeavingKey,token,1);
 objc_setAssociatedObject(h,&fvGPendingKey,0,1);
 fvGRestoreScene(h);
 /* Bounded retries are tied to this exact exit, scene and snapshot. */
 if(objc_getAssociatedObject(h,&fvGStateKey))for(int i=1;i<=3;i++){
  id work=fvGWork(h,token);
  dispatch_after_f(dispatch_time(0,(long long)i*150000000),&_dispatch_main_q,work,fvGRestoreRetry);
 }
 SEND(token,"release");
}
static void fvGLifecycle(id self,id destination,BOOL window){
 id h=fvGTarget(self);if(!h)return;
 if(!destination)fvGLeave(h);
 else if(objc_getAssociatedObject(h,&fvGLeavingKey)){
  objc_setAssociatedObject(h,&fvGLeavingKey,0,1);
  fvGRequest(h);
 }
}
static void fvGHostSuperviewHook(id self,SEL cmd,id dest){
 if(SEND(self,"superview")!=dest)fvKADetach(SEND(self,"superview"));
 fvGLifecycle(self,dest,0);if(fvGHostWillSuperview)((void(*)(id,SEL,id))fvGHostWillSuperview)(self,cmd,dest);
}
static void fvGItemSuperviewHook(id self,SEL cmd,id dest){
 if(SEND(self,"superview")!=dest)fvKADetach(self);
 fvGLifecycle(self,dest,0);if(fvGItemWillSuperview)((void(*)(id,SEL,id))fvGItemWillSuperview)(self,cmd,dest);
}
static void fvGHostWindowHook(id self,SEL cmd){
 fvGLifecycle(self,SEND(self,"window"),1);
 if(fvGHostWillWindow)((void(*)(id,SEL))fvGHostWillWindow)(self,cmd);
#ifdef FV_COMPLETE_MODE
 if(SEND(self,"window"))fvFMRequestMount(self,ivar(self,"hostingView"));
#endif
}
static void fvKAItemWillWindowHook(id self,SEL cmd,id destination){
 if(SEND(self,"window")!=destination)fvKADetach(self);
 if(fvKAItemWillMoveWindow)((void(*)(id,SEL,id))fvKAItemWillMoveWindow)(self,cmd,destination);
}
static void fvGItemWindowHook(id self,SEL cmd){
 fvGLifecycle(self,SEND(self,"window"),1);
 if(fvGItemWillWindow)((void(*)(id,SEL))fvGItemWillWindow)(self,cmd);
}
