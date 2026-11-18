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
 puts("PASS first-mount production identity guard: 112 cases");
 assert(count==112);return 0;
}
