#ifndef FV_PRESENTATION_COMMIT_CORE_H
#define FV_PRESENTATION_COMMIT_CORE_H
/* Getters must read live values. Every setter may synchronously run native
 * layout and invalidate any earlier comparison or the entire mount identity. */
typedef struct {
 void *context;
 int (*current)(void *);
 int (*matches)(void *,unsigned);
 void (*assign)(void *,unsigned);
} FVPresentationOps;
static inline int fvPresentationCommit(FVPresentationOps *o){
 if(!o||!o->current||!o->matches||!o->assign)return 0;
 for(unsigned field=0;field<5;field++){
  if(!o->current(o->context))return 0;
  int matches=o->matches(o->context,field);
  if(!o->current(o->context))return 0;
  if(!matches)o->assign(o->context,field);
 }
 if(!o->current(o->context))return 0;
 for(unsigned field=0;field<5;field++){
  if(!o->matches(o->context,field)||!o->current(o->context))return 0;
 }
 return 1;
}
#endif
