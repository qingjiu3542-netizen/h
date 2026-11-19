#include <assert.h>
#include <stdio.h>
#include "FVRestoreCore.h"
typedef struct {
 int identity,orientation,available,accept,submitOK;
 int submits,commits,snapshot,cache,local,fullscreen;
 int replaceOnSubmit,rotateOnSubmit,replaceOnRead,rotateOnRead,mode;
} Fake;
static int identity(void *p){return ((Fake*)p)->identity;}
static int orientation(void *p){return ((Fake*)p)->orientation;}
static int available(void *p){return ((Fake*)p)->available;}
static int submit(void *p){Fake*f=p;f->submits++;if(f->replaceOnSubmit)f->identity=0;if(f->rotateOnSubmit)f->orientation=0;return f->submitOK;}
static int accepted(void *p){Fake*f=p;if(f->replaceOnRead)f->identity=0;if(f->rotateOnRead)f->orientation=0;return f->accept;}
static void commit(void *p){Fake*f=p;f->commits++;f->snapshot=0;f->cache=1;if(f->mode!=2)f->local=1;if(f->mode==1)f->fullscreen=1;}
static Fake fresh(int mode){Fake f={0};f.identity=f.orientation=f.available=f.accept=f.submitOK=f.snapshot=1;f.mode=mode;return f;}
static FVRestoreResult run(Fake*f){FVRestoreOps o={f,identity,orientation,available,submit,accepted,commit};return fvRestoreExecute(&o);}
static void pending(Fake*f){assert(f->snapshot==1);assert(!f->commits&&!f->cache&&!f->local&&!f->fullscreen);}
int main(void){
 int cases=0;
 for(int mode=0;mode<3;mode++){
  Fake f=fresh(mode);assert(run(&f)==FVRestoreDone);assert(f.submits==1&&f.commits==1&&!f.snapshot&&f.cache);assert(f.local==(mode!=2)&&f.fullscreen==(mode==1));cases++;
  f=fresh(mode);f.orientation=0;assert(run(&f)==FVRestoreOrientation);assert(!f.submits);pending(&f);f.orientation=1;assert(run(&f)==FVRestoreDone);cases++;
  f=fresh(mode);f.identity=0;assert(run(&f)==FVRestoreIdentity);assert(!f.submits);pending(&f);cases++;
  f=fresh(mode);f.available=0;assert(run(&f)==FVRestoreUnavailable);assert(!f.submits);pending(&f);cases++;
  f=fresh(mode);f.submitOK=0;assert(run(&f)==FVRestoreUnavailable);pending(&f);cases++;
  f=fresh(mode);f.accept=0;for(int i=0;i<4;i++){assert(run(&f)==FVRestoreUnaccepted);pending(&f);}f.accept=1;assert(run(&f)==FVRestoreDone);assert(f.submits==5&&f.commits==1);cases++;
  f=fresh(mode);f.replaceOnSubmit=1;assert(run(&f)==FVRestoreIdentity);pending(&f);cases++;
  f=fresh(mode);f.rotateOnSubmit=1;assert(run(&f)==FVRestoreOrientation);pending(&f);cases++;
  f=fresh(mode);f.replaceOnRead=1;assert(run(&f)==FVRestoreIdentity);pending(&f);cases++;
  f=fresh(mode);f.rotateOnRead=1;assert(run(&f)==FVRestoreOrientation);pending(&f);cases++;
 }
 int t1,t2,s1,s2,p1,p2;
 assert(fvRestoreCallbackCurrent(&t1,&t1,&s1,&s1,&p1,&p1));
 assert(!fvRestoreCallbackCurrent(&t1,&t2,&s1,&s1,&p1,&p1));
 assert(!fvRestoreCallbackCurrent(&t1,&t1,&s1,&s2,&p1,&p1));
 assert(!fvRestoreCallbackCurrent(&t1,&t1,&s1,&s1,&p1,&p2));
 assert(!fvRestoreCallbackCurrent(&t1,0,&s1,&s1,&p1,&p1));
 assert(!fvRestoreCallbackCurrent(0,0,&s1,&s1,&p1,&p1));
 assert(fvRestoreCallbackCurrent(&t1,&t1,&s1,&s1,0,0));
 int relationCases=0;
 int sceneA,sceneB;
 assert(fvRestoreSceneRelation(&sceneA,&sceneA)==FVSceneCurrent);
 assert(fvRestoreSceneRelation(&sceneA,0)==FVSceneMissing);
 assert(fvRestoreSceneRelation(&sceneA,&sceneB)==FVSceneReplaced);
 relationCases+=3;
 printf("PASS: %d restore transaction scenarios + 7 callback identity cases + %d scene relation cases\n",cases,relationCases);
}
