#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "FVKeyboardNativeFlagCore.h"
int main(void){
 unsigned char object[320]={0};int cases=0;
 for(int shift=0;shift<=64;shift+=64){
  FVKBNativeLayout l={120+shift,152+shift,153+shift,64,320,1,1,{"","",""}};
  memset(object,0,sizeof object);
  assert(fvKBLayoutValid(l));cases++;
  assert(fvKBReadNativeFlag(object,l,1)==0);cases++;
  object[l.inner]=1;object[l.record]=1;
  assert(fvKBReadNativeFlag(object,l,0)==0&&fvKBReadNativeFlag(object,l,1)==1&&fvKBReadNativeFlag(object,l,2)==1);cases++;
  object[l.inner]=2;assert(fvKBReadNativeFlag(object,l,1)==-1);cases++;object[l.inner]=1;
  object[l.record]=255;assert(fvKBReadNativeFlag(object,l,0)==-1);cases++;object[l.record]=1;
  l.exactClass=0;assert(fvKBReadNativeFlag(object,l,1)==-1);cases++;l.exactClass=1;
  l.allPresent=0;assert(!fvKBLayoutValid(l));cases++;l.allPresent=1;
  l.encoding[1]=0;assert(!fvKBLayoutValid(l));cases++;
  l.encoding[1]="q";assert(!fvKBLayoutValid(l));cases++;
  l.encoding[1]="B";l.encoding[0]="c";assert(fvKBReadNativeFlag(object,l,1)==1);cases++;
  l.encoding[1]="BB";assert(!fvKBLayoutValid(l));cases++;l.encoding[1]="";
  l.record++;assert(!fvKBLayoutValid(l));cases++;l.record--;
  l.side++;assert(!fvKBLayoutValid(l));cases++;l.side--;
  l.superSize=l.side+1;assert(!fvKBLayoutValid(l));cases++;l.superSize=64;
  l.instanceSize=l.record;assert(!fvKBLayoutValid(l));cases++;l.instanceSize=320;
  l.side=-1;assert(!fvKBLayoutValid(l));cases++;
 }
 FVKBNativeLayout l={120,152,153,64,320,1,1,{"","",""}};
 assert(fvKBReadNativeFlag(0,l,1)==-1);cases++;
 assert(fvKBReadNativeFlag(object,l,3)==-1);cases++;
 for(int side=0;side<2;side++)for(int inner=0;inner<2;inner++)for(int record=0;record<2;record++){
  assert(fvKBPortraitInnerEligible(side,inner,record,1)==(side==0&&record==1));cases++;
 }
 assert(!fvKBPortraitInnerEligible(-1,0,1,1));cases++;
 assert(!fvKBPortraitInnerEligible(0,-1,1,1));cases++;
 assert(!fvKBPortraitInnerEligible(0,0,-1,1));cases++;
 assert(!fvKBPortraitInnerEligible(0,0,1,3));cases++;
 assert(!fvKBPortraitInnerEligible(0,0,1,0));cases++;
 printf("PASS native keyboard flags: %d Swift layout, portrait record and fail-closed cases\n",cases);
}
