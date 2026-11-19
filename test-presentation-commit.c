#include <assert.h>
#include <stdio.h>
#include "FVPresentationCommitCore.h"
typedef struct {
 int live[5],target[5],valid,writes,resetOnParent,changeOnAssign,changeOnRead,reject;
} Fake;
static int current(void *v){return ((Fake*)v)->valid;}
static int matches(void *v,unsigned field){
 Fake *f=v;
 if(f->changeOnRead==(int)field)f->valid=0;
 return f->live[field]==f->target[field];
}
static void assign(void *v,unsigned field){
 Fake *f=v;f->writes++;
 if(f->reject!=(int)field)f->live[field]=f->target[field];
 if(field==0&&f->resetOnParent){f->live[1]=932;f->live[2]=932;f->live[3]=932;f->live[4]=932;}
 if(f->changeOnAssign==(int)field)f->valid=0;
}
static Fake fresh(void){Fake f={{568,568,764,743,284},{568,568,764,743,284},1,0,0,-1,-1,-1};return f;}
static int run(Fake *f){FVPresentationOps ops={f,current,matches,assign};return fvPresentationCommit(&ops);}
int main(void){
 int cases=0;
 Fake f=fresh();assert(run(&f)&&f.writes==0);cases++;
 f=fresh();f.live[0]=932;f.resetOnParent=1;assert(run(&f)&&f.writes==5);cases++;
 for(int i=0;i<5;i++){
  f=fresh();f.live[i]=0;f.changeOnAssign=i;assert(!run(&f)&&f.writes==1);cases++;
  f=fresh();f.live[i]=0;f.changeOnRead=i;assert(!run(&f)&&f.writes==0);cases++;
  f=fresh();f.live[i]=0;f.reject=i;assert(!run(&f));cases++;
 }
 f=fresh();f.valid=0;assert(!run(&f)&&!f.writes);cases++;
 assert(!fvPresentationCommit(0));cases++;
 printf("PASS presentation commit: %d live read, native reentry and identity cases\n",cases);
}
