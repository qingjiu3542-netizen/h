#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "FVKeyboardAvoidanceCore.h"
static int checks;
#define CHECK(x) do { assert(x); checks++; } while(0)
static void near(double actual,double expected){CHECK(fabs(actual-expected)<1e-8);}
static void plan(FVRect item,FVRect key,FVRect view,FVKAResult result,double expected){
 double offset=-99;CHECK(fvKAPlan(item,key,view,8,&offset)==result);near(offset,expected);
}
int main(void){
 FVRect view={0,0,430,932},item={50,360,240,300};
 plan(item,(FVRect){0,600,430,332},view,FVKAMove,68);
 plan(item,(FVRect){0,550,430,382},view,FVKAMove,118);
 plan(item,(FVRect){0,640,430,292},view,FVKAMove,28);
 plan(item,(FVRect){0,700,430,232},view,FVKAClear,0);
 plan(item,(FVRect){0,932,430,332},view,FVKAClear,0);
 plan(item,(FVRect){320,500,100,200},view,FVKAClear,0);
 plan(item,(FVRect){290,500,100,200},view,FVKAClear,0);
 plan(item,(FVRect){120,500,100,200},view,FVKAMove,168);
 plan(item,(FVRect){0,0,430,200},view,FVKAClear,0);
 plan((FVRect){20,50,240,650},(FVRect){0,600,430,332},view,FVKANoRoom,0);
 plan((FVRect){50,100,200,300},(FVRect){0,408,430,200},view,FVKAClear,0);
 /* Viewport origin must be respected: the item must never be pushed offscreen. */
 plan((FVRect){130,350,240,300},(FVRect){100,600,430,332},(FVRect){100,100,430,932},FVKAMove,58);
 double offset=42;
 CHECK(fvKAPlan((FVRect){NAN,0,1,1},item,view,8,&offset)==FVKAInvalid);near(offset,0);
 CHECK(fvKAPlan(item,(FVRect){0,0,INFINITY,1},view,8,&offset)==FVKAInvalid);
 CHECK(fvKAPlan(item,item,(FVRect){0,0,0,100},8,&offset)==FVKAInvalid);
 CHECK(fvKAPlan(item,item,view,NAN,&offset)==FVKAInvalid);
 CHECK(fvKAPlan(item,item,view,-1,&offset)==FVKAInvalid);
 CHECK(fvKAPlan(item,item,view,8,0)==FVKAInvalid);
 /* All four scene rotations and scaled ancestors. A screen-up displacement
  * must map back to screen-up rather than the old build13 portrait tx shift. */
 for(int orientation=0;orientation<4;orientation++)for(int s=1;s<=4;s++){
  const double a0[]={1,0,-1,0},b0[]={0,1,0,-1};double scale=s*.4;
  double a=a0[orientation]*scale,b=b0[orientation]*scale,c=-b,d=a;
  FVPoint local={0};CHECK(fvKAVector(a,b,c,d,0,-68,&local));
  near(a*local.x+c*local.y,0);near(b*local.x+d*local.y,-68);
 }
 FVPoint delta;
 CHECK(!fvKAVector(0,0,0,0,0,-68,&delta));
 CHECK(!fvKAVector(1,0,0,INFINITY,0,-68,&delta));
 CHECK(!fvKAVector(1,0,0,1,NAN,-68,&delta));
 /* Repeated show/frame-change/hide use one stable baseline, not cumulative
  * movement. The lower keyboard restores part of the displacement. */
 FVPoint base={170,510},current=base,last=base,applied={0,0};int owns=0;
 const double tops[]={600,600,550,640,700,932};
 const double expected[]={442,442,392,482,510,510};
 for(unsigned i=0;i<sizeof(tops)/sizeof(tops[0]);i++){
  base=fvKABaseline(current,last,applied,owns);near(base.y,510);
  double amount=0;FVKAResult r=fvKAPlan(item,(FVRect){0,tops[i],430,332},view,8,&amount);
  CHECK(r==FVKAMove||r==FVKAClear);applied=(FVPoint){0,-amount};
  current=(FVPoint){base.x+applied.x,base.y+applied.y};last=current;owns=r==FVKAMove;
  near(current.x,170);near(current.y,expected[i]);
 }
 FVPoint restored=fvKABaseline((FVPoint){170,442},(FVPoint){170,442},(FVPoint){0,-68},1);
 near(restored.y,510); /* hide, disable and detach share the same restoration */
 FVPoint dragged=fvKABaseline((FVPoint){200,450},(FVPoint){170,442},(FVPoint){0,-68},1);
 near(dragged.x,200);near(dragged.y,450); /* never replay an obsolete center */
 FVPoint original=fvKABaseline((FVPoint){170,520},(FVPoint){170,442},(FVPoint){0,-68},1);
 near(original.y,520); /* native animation supersedes our last write */
 printf("PASS keyboard avoidance: %d geometry, rotation, numeric and restoration checks\n",checks);
 return 0;
}
