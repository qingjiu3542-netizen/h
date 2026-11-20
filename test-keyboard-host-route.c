#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "FVKeyboardHostRouteCore.h"
int main(void){unsigned n=0;
 assert(fvKHEncoding("B40@0:8@16@24@32","B@:@@@"));n++;
 assert(fvKHEncoding("c40@0:8@16@24@32","c@:@@@"));n++;
 assert(!fvKHEncoding("v40@0:8@16@24@32","B@:@@@"));n++;
 assert(fvKHEncoding("v32@0:8@16@?24","v@:@@?"));n++;
 assert(!fvKHEncoding("v32@0:8@16@24","v@:@@?"));n++;
 const char *r="{CGRect={CGPoint=dd}{CGSize=dd}}";
 char expected[256],actual[350];snprintf(expected,sizeof expected,"v@:%s%sBBB@",r,r);
 snprintf(actual,sizeof actual,"v104@0:8%s16%s48B80B84B88@96",r,r);
 assert(fvKHEncoding(actual,expected));n++;
 assert(!fvKHEncoding("v104@0:8{CGRect={CGPoint=ff}{CGSize=ff}}16B80@96",expected));n++;
 assert(!fvKHEncoding(0,"v@:"));n++;
 assert(!fvKHEncoding("v@:",0));n++;
 assert(!fvKHEncoding("v@:{CGRect={CGPoint=dd}{CGSize=dd}","v@:"));n++;
 assert(!fvKHEncoding("v@:}","v@:"));n++;
 assert(!fvKHEncoding("v@:@\"unterminated","v@:@"));n++;
 assert(fvKHEncoding("@@:\"QuotedType\"","@@:"));n++;
 char huge[450];memset(huge,'@',sizeof huge);huge[449]=0;assert(!fvKHEncoding(huge,huge));n++;
 assert(fvKHIdentifierHash(0)==0);n++;
 assert(fvKHIdentifierHash("scene-A")==fvKHIdentifierHash("scene-A"));n++;
 assert(fvKHIdentifierHash("scene-A")!=fvKHIdentifierHash("scene-B"));n++;
 puts("PASS host-route core: strict object/block/CGRect/BOOL ABI, bounded encodings and redacted identity fingerprint");
 assert(n==17);
}
