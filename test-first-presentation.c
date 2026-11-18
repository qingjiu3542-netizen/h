#include <assert.h>
#include <stdio.h>
#define FV_FP_CORE_TEST 1
#include "FVFirstPresentation.inc"
int main(void){
 assert(fvFPEncoding("v24@0:8@16","v@:@"));
 assert(fvFPEncoding("v16@0:8","v@:"));
 assert(fvFPEncoding("{CGRect={CGPoint=dd}{CGSize=dd}}16@0:8","{CGRect={CGPoint=dd}{CGSize=dd}}@:"));
 assert(!fvFPEncoding("v24@0:8q16","v@:@"));
 assert(!fvFPEncoding("@16@0:8","v@:"));
 assert(!fvFPEncoding("v32@0:8@16@24","v@:@"));
 assert(!fvFPEncoding("{CGRect={CGPoint=ff}{CGSize=ff}}16@0:8","{CGRect={CGPoint=dd}{CGSize=dd}}@:"));
 assert(!fvFPEncoding(0,"v@:"));
 assert(!fvFPEncoding("{CGRect=dd16@0:8","v@:"));
 assert(!fvFPEncoding("}16@0:8","v@:"));
 char oversized[240];for(unsigned i=0;i<239;i++)oversized[i]='v';oversized[239]=0;
 assert(!fvFPEncoding(oversized,"v@:"));
 unsigned n=0;for(unsigned i=0;i<128;i++)assert(fvFPTake(&n,128));
 for(unsigned i=0;i<1000;i++)assert(!fvFPTake(&n,128));
 assert(n==128);
 puts("PASS first-presentation: ABI acceptance/rejection, malformed/oversized encoding, 128-event hard cap");
 return 0;
}
