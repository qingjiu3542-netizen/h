#ifndef FV_KEYBOARD_NATIVE_FLAG_CORE_H
#define FV_KEYBOARD_NATIVE_FLAG_CORE_H
/* Original floatingView 1.6.0-6, both ARM64 slices: these three named Swift
 * stored properties have objc ivar type "" and ivar metadata size 1. Never
 * accept an arbitrary untyped ivar or synthesize true. Runtime adapter must
 * resolve these names on the exact original class and use runtime offsets. */
typedef struct {
 long side,inner,record;
 unsigned long superSize,instanceSize;
 int exactClass,allPresent;
 const char *encoding[3];
} FVKBNativeLayout;
static inline int fvKBEncoding(const char *e){
 if(!e)return -1;
 if(!e[0])return 0;
 return (e[0]=='B'||e[0]=='c')&&!e[1]?1:-1;
}
static inline int fvKBLayoutValid(FVKBNativeLayout l){
 if(!l.exactClass||!l.allPresent||!l.superSize||l.instanceSize<=l.superSize||
    l.side<0||l.inner<0||l.record<0)return 0;
 if((unsigned long)l.side<l.superSize||(unsigned long)l.inner<l.superSize||
    (unsigned long)l.record<l.superSize||(unsigned long)l.side>=l.instanceSize||
    (unsigned long)l.inner>=l.instanceSize||(unsigned long)l.record>=l.instanceSize)return 0;
 /* Confirm the audited property grouping, not fixed device object offsets. */
 if(l.inner-l.side!=32||l.record-l.inner!=1)return 0;
 for(unsigned i=0;i<3;i++)if(fvKBEncoding(l.encoding[i])<0)return 0;
 return 1;
}
/* The original matched-app path sets recordOnlyInnerKeyboardIsShow before
 * its portrait (orientation 1/2) early return. innerKeyboardIsDisplay tracks
 * a subsequent item transformation, NOT all portrait keyboard visibility. */
static inline int fvKBPortraitInnerEligible(int side,int inner,int record,long orientation){
 return orientation==1&&side==0&&(inner==0||inner==1)&&record==1;
}
static inline int fvKBReadNativeFlag(const unsigned char *object,FVKBNativeLayout l,unsigned field){
 if(!object||field>=3||!fvKBLayoutValid(l))return -1;
 long offsets[3]={l.side,l.inner,l.record};
 for(unsigned i=0;i<3;i++)if(object[offsets[i]]>1)return -1;
 return object[offsets[field]];
}
#endif
