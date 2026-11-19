#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "FVKeyboardModeCore.h"
int main(void){
 assert(!strcmp(fvKAOriginalModeKey(1),"wayKeyboardShowIntoFloatingView_1_2"));
 assert(!strcmp(fvKAOriginalModeKey(2),"wayKeyboardShowIntoFloatingView_1_2"));
 assert(!strcmp(fvKAOriginalModeKey(3),"wayKeyboardShowIntoFloatingView_3_4"));
 assert(!strcmp(fvKAOriginalModeKey(4),"wayKeyboardShowIntoFloatingView_3_4"));
 assert(!fvKAOriginalModeKey(0));assert(!fvKAOriginalModeKey(5));
 assert(fvKAKeyboardAdaptEnabled(1,FVKAOriginalModeInner,1,1));
 assert(fvKAKeyboardAdaptEnabled(1,FVKAOriginalModeOuter,1,1));
 assert(!fvKAKeyboardAdaptEnabled(0,FVKAOriginalModeInner,1,1));
 assert(!fvKAKeyboardAdaptEnabled(1,FVKAOriginalModeUnknown,1,1));
 assert(!fvKAKeyboardAdaptEnabled(1,FVKAOriginalModeOuter,0,1));
 assert(!fvKAKeyboardAdaptEnabled(1,FVKAOriginalModeInner,1,2));
 double w=0,h=0,s=0;
 assert(fvKATargetViewport(390,693,234,1,&w,&h,&s));
 assert(w==390&&h==693&&fabs(s-.6)<1e-12);
 assert(!fvKATargetViewport(390,693,234,0,&w,&h,&s));
 assert(!fvKATargetViewport(390,693,0,1,&w,&h,&s));
 assert(!fvKATargetViewport(NAN,693,234,1,&w,&h,&s));
 FVRect host={40,60,234,416},screen={0,0,390,844};
 FVKAPlacement plan;
 assert(fvKAPlanPlacement(1,FVKAOriginalModeInner,1,1,host,screen,390,300,.6,&plan));
 assert(fabs(plan.keyboardScale-.6)<1e-12);
 assert(fabs(plan.keyboard.height-180)<1e-12 && fabs(plan.reservedLogicalHeight-300)<1e-12);
 assert(fabs(plan.keyboard.y+plan.keyboard.height-476)<1e-12);
 assert(fvKAPlanPlacement(1,FVKAOriginalModeOuter,1,1,host,screen,390,300,.6,&plan));
 assert(plan.keyboard.y==476 && plan.reservedLogicalHeight==0);
 assert(!fvKAPlanPlacement(1,FVKAOriginalModeOuter,0,1,host,screen,390,300,.6,&plan));
 assert(!fvKAPlanPlacement(1,FVKAOriginalModeOuter,1,2,host,screen,390,300,.6,&plan));
 assert(!fvKAPlanPlacement(1,FVKAOriginalModeOuter,1,1,(FVRect){40,428,234,416},screen,390,300,.6,&plan));
 puts("PASS keyboard policy and placement: ownership, width fit, docking and logical occlusion");
 return 0;
}
