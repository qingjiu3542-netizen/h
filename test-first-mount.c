#include <assert.h>
#include <stdio.h>
#include "FVFirstMountCore.h"
int main(void){
 int objects[12]={0};const void *p[10];
 for(int i=0;i<5;i++)p[2*i]=p[2*i+1]=&objects[i];
 int count=0;
 for(long o=1;o<=4;o++){
  assert(fvFirstMountCurrent(p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7],p[8],p[9],o,o));count++;
  for(int i=0;i<10;i++){
   const void *saved=p[i];p[i]=0;
   assert(!fvFirstMountCurrent(p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7],p[8],p[9],o,o));count++;
   p[i]=&objects[11];
   assert(!fvFirstMountCurrent(p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7],p[8],p[9],o,o));count++;
   p[i]=saved;
  }
  for(long n=-1;n<=6;n++)if(n!=o){
   assert(!fvFirstMountCurrent(p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7],p[8],p[9],o,n));count++;
  }
 }
 double w=0,h=0;
 for(long o=1;o<=4;o++){
  int landscape=o>=3;
  assert(fvRatioSceneSize(landscape?932:430,landscape?430:932,o,&w,&h));
  double visibleW=landscape?h:w;
  double visibleH=landscape?w:h;
  assert(visibleW==430 && __builtin_fabs(visibleH/visibleW-16.0/9.0)<1e-12);
 }
 assert(!fvRatioSceneSize(932,430,1,&w,&h));
 assert(!fvRatioSceneSize(430,932,4,&w,&h));
 assert(!fvRatioSceneSize(0,932,1,&w,&h));
 assert(!fvRatioSceneSize(430,932,0,&w,&h));
 puts("PASS shared Scene dimensions and first-mount identity");
 assert(count==112);return 0;
}
