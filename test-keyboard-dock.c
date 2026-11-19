#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "FVKeyboardDockCore.h"
static int near(double a,double b){return fabs(a-b)<1e-7;}
int main(void){
 FVRect source={0,0,430,932},keys={0,600,430,332};
 FVRect host={35.6,220.8,319.5,568};FVKeyboardDockPlan p;int cases=0;
 assert(fvKeyboardDockPlan(source,keys,host,319.5/430,1,&p));
 assert(near(p.keys.height,332*319.5/430));
 assert(near(p.keys.y+p.keys.height,host.y+host.height));
 assert(near(p.surface.y+p.surface.height,host.y+host.height));
 assert(near(p.reservedLogicalHeight,332));
 assert(p.surface.height>host.height); /* Do not squash the full 932pt canvas. */
 cases++;
 assert(fvKeyboardDockPlan(source,keys,host,319.5/430,0,&p));
 assert(near(p.keys.y,host.y+host.height)&&p.reservedLogicalHeight==0);cases++;
 for(int candidate=0;candidate<5;candidate++){
  double kh=280+candidate*35;
  FVRect changed={0,932-kh,430,kh};
  assert(fvKeyboardDockPlan(source,changed,host,319.5/430,1,&p));
  assert(near(p.reservedLogicalHeight,kh));
  assert(near(p.keys.y+p.keys.height,host.y+host.height));cases++;
 }
 assert(!fvKeyboardDockPlan(source,(FVRect){20,600,200,300},host,.7,1,&p));cases++;
 assert(!fvKeyboardDockPlan(source,(FVRect){0,500,430,300},host,.7,1,&p));cases++;
 assert(!fvKeyboardDockPlan(source,keys,(FVRect){0,0,319.5,200},.7,1,&p));cases++;
 assert(!fvKeyboardDockPlan(source,keys,host,0,1,&p));cases++;
 assert(!fvKeyboardDockPlan(source,keys,host,NAN,1,&p));cases++;
 assert(!fvKeyboardDockPlan((FVRect){0,0,0,932},keys,host,.7,1,&p));cases++;
 assert(!fvKeyboardDockPlan(source,(FVRect){0,NAN,430,332},host,.7,1,&p));cases++;
 assert(!fvKeyboardDockPlan(source,keys,host,.7,1,0));cases++;
 FVRect ls={0,0,932,430},lk={0,230,932,200},lh={10,20,466,500};
 assert(fvKeyboardDockPlan(ls,lk,lh,.5,1,&p));
 assert(near(p.scale,.5)&&near(p.keys.height,100)&&near(p.keys.y,420));cases++;
 FVRect offset={100,50,430,932},offsetKeys={100,650,430,332};
 assert(fvKeyboardDockPlan(offset,offsetKeys,host,319.5/430,1,&p));
 assert(near(p.keys.y+p.keys.height,host.y+host.height));cases++;
 assert(fvKeyboardDockPlanWithGap(source,keys,host,319.5/430,1,6,&p));
 assert(near(p.keys.y+p.keys.height,host.y+host.height-6));
 assert(near(p.reservedLogicalHeight,332+6/(319.5/430)));cases++;
 assert(!fvKeyboardDockPlanWithGap(source,keys,host,.7,1,-1,&p));cases++;
 assert(!fvKeyboardDockPlanWithGap(source,keys,host,.7,1,NAN,&p));cases++;
 assert(!fvKeyboardDockPlanWithGap(source,keys,host,.7,1,host.height,&p));cases++;
 assert(fvKeyboardDockPlanWithGap(source,keys,host,.7,0,6,&p));
 assert(near(p.keys.y,host.y+host.height)&&p.reservedLogicalHeight==0);cases++;
 printf("PASS keyboard dock: %d source-key-region, uniform-scale and bottom-clearance scenarios\n",cases);
}
