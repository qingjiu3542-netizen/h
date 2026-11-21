#ifndef FV_SCENE_SAFE_AREA_CORE_H
#define FV_SCENE_SAFE_AREA_CORE_H
/* The SAME transaction runs in SpringBoard and in portable mock regressions.
 * No frames, settings blobs, keyboard heights or app payload in this core. */
typedef struct { double top,left,bottom,right; } FVSAInsets;
typedef struct { unsigned long scene; int present,abi; FVSAInsets value; } FVSASnapshot;
typedef struct {
 unsigned long scene; FVSAInsets original,last; int owned,busy,cancel,spent;
} FVSAState;
typedef struct {
 void *ctx;
 int (*current)(void *,int restoring);
 int (*read)(void *,FVSASnapshot *);
 int (*write)(void *,FVSASnapshot expected,FVSAInsets value,int restoring);
} FVSAOps;
enum { FVSARejected=0,FVSAApplied=1,FVSARestored=2,FVSASuperseded=3,FVSANoop=4,FVSAReentry=5,FVSAPending=6 };
static int fvSAEqual(FVSAInsets a,FVSAInsets b){
 return a.top==b.top&&a.left==b.left&&a.bottom==b.bottom&&a.right==b.right;
}
static int fvSAValid(FVSAInsets a){
 return __builtin_isfinite(a.top)&&__builtin_isfinite(a.left)&&__builtin_isfinite(a.bottom)&&__builtin_isfinite(a.right)&&
  a.top>=0&&a.left>=0&&a.bottom>=0&&a.right>=0&&a.top<4096&&a.left<4096&&a.bottom<4096&&a.right<4096;
}
static int fvSASnapshotValid(FVSAState *s,FVSASnapshot r){
 return r.scene==s->scene&&r.present&&r.abi&&fvSAValid(r.value);
}
static int fvSATransact(FVSAState *s,FVSAOps *o,int restore,FVSAInsets target){
 if(s->busy){if(restore)s->cancel=1;return FVSAReentry;}
 if((!restore&&s->spent)||!o||!o->current||!o->read||!o->write)return FVSARejected;
 s->busy=1;int result=FVSARejected;FVSASnapshot r={0},after={0};
 if(!o->current(o->ctx,restore)||!o->read(o->ctx,&r)||!fvSASnapshotValid(s,r))goto done;
 if(s->owned&&!fvSAEqual(r.value,s->last)){
  s->owned=0;s->spent=1;result=FVSASuperseded;goto done;
 }
 if(restore){
  s->spent=1;if(!s->owned){result=FVSANoop;goto done;}target=s->original;
 }else{
  if(s->cancel||!fvSAValid(target))goto done;
  if(!s->owned){s->original=r.value;s->last=r.value;}
 }
 if(fvSAEqual(r.value,target)){if(restore)s->owned=0;result=FVSANoop;goto done;}
 if(!o->current(o->ctx,restore)||(!restore&&s->cancel))goto done;
 /* Publish ownership BEFORE the synchronous native write can reenter. */
 s->last=target;s->owned=1;
 if(!o->write(o->ctx,r,target,restore)){
  /* A callback may have run despite an unaccepted return. Read back rather
   * than assuming rollback; never retry a stale full settings snapshot. */
  result=FVSAPending;
 }else result=restore?FVSARestored:FVSAApplied;
 if(o->read(o->ctx,&after)&&fvSASnapshotValid(s,after)){
  if(!fvSAEqual(after.value,target)){
   s->owned=0;s->spent=1;result=FVSASuperseded;
  }else if(restore)s->owned=0;
 }else result=FVSAPending;
 if(!restore&&s->cancel)s->spent=1;
 done:s->busy=0;return result;
}
#endif
