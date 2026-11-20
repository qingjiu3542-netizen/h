#ifndef FV_KEYBOARD_APP_STATUS_CORE_H
#define FV_KEYBOARD_APP_STATUS_CORE_H
/* Receipt is diagnostics only, never permission to alter layout. */
static inline int fvKAStatusValid(double schema,double pid,double stamp,double stage,double now){
 return schema==1&&__builtin_isfinite(pid)&&pid>0&&pid<2147483648.0&&__builtin_floor(pid)==pid&&
  __builtin_isfinite(stage)&&stage>=1&&stage<=4&&__builtin_floor(stage)==stage&&
  __builtin_isfinite(stamp)&&__builtin_isfinite(now)&&stamp>=0&&now>=stamp&&now-stamp<=120;
}
#endif
