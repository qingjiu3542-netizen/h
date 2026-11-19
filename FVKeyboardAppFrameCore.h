#ifndef FV_KEYBOARD_APP_FRAME_CORE_H
#define FV_KEYBOARD_APP_FRAME_CORE_H
/* Per-scene conversion of an existing docked keyboard event. No synthesized
 * events and no app-specific inset/scroll-view mutation. */
typedef struct {double x,y,width,height;} FVKARect;
static inline int fvKAFNumber(double v){return __builtin_isfinite(v)&&__builtin_fabs(v)<1000000;}
static inline int fvKAFRect(FVKARect r){return fvKAFNumber(r.x)&&fvKAFNumber(r.y)&&fvKAFNumber(r.width)&&fvKAFNumber(r.height)&&r.width>0&&r.height>0;}
static inline int fvKAFEventScreenMatch(int hasObject,int objectIsScreen,int sameScreen){
 return !hasObject||(objectIsScreen==1&&sameScreen==1);
}
static inline int fvKAFNear(double a,double b){return __builtin_fabs(a-b)<0.6;}
static inline int fvKAFAuthorize(int sameScene,int foreground,int keyWindow,int portrait,int inner,
 double now,double stamp,FVKARect local,FVKARect target){
 return sameScene==1&&foreground==1&&keyWindow==1&&portrait==1&&inner==1&&fvKAFRect(local)&&fvKAFRect(target)&&
  fvKAFNear(local.x,target.x)&&fvKAFNear(local.y,target.y)&&fvKAFNear(local.width,target.width)&&fvKAFNear(local.height,target.height)&&
  fvKAFNear(target.height,target.width*16.0/9.0)&&__builtin_isfinite(now)&&__builtin_isfinite(stamp)&&
  now>=0&&stamp>=0&&now>=stamp&&now-stamp<=120;
}
static inline int fvKAFMap(FVKARect source,FVKARect target,FVKARect input,FVKARect *out){
 if(!out)return 0;
 *out=input;
 if(!fvKAFRect(source)||!fvKAFRect(target)||!fvKAFRect(input)||
    source.height<=source.width||target.height<=target.width||
    !fvKAFNear(source.width,target.width)||target.height>=source.height-.6||
    !fvKAFNear(input.x,source.x)||!fvKAFNear(input.width,source.width))return 0;
 /* Correct only a full-width keyboard already docked to the original canvas.
  * An already-corrected frame, split keyboard or interactive undocking passes
  * through unchanged. Height is the measured key region, not canvas height. */
 if(!fvKAFNear(input.y+input.height,source.y+source.height)||input.height>=target.height-1)return 0;
 FVKARect result={target.x,target.y+target.height-input.height,target.width,input.height};
 if(!fvKAFRect(result))return 0;
 *out=result;return 1;
}
static inline int fvKAFMapBegin(FVKARect source,FVKARect target,FVKARect input,FVKARect *out){
 if(fvKAFMap(source,target,input,out))return 1;
 if(!out||!fvKAFRect(source)||!fvKAFRect(target)||!fvKAFRect(input)||
    !fvKAFNear(source.width,target.width)||!fvKAFNear(input.x,source.x)||
    !fvKAFNear(input.width,source.width)||target.height>=source.height-.6||
    input.y<source.y+source.height-.6||input.height>=target.height-1)return 0;
 *out=(FVKARect){target.x,target.y+target.height+(input.y-source.y-source.height),target.width,input.height};
 return fvKAFRect(*out);
}
#endif
