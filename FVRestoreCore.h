/* Shared by the runtime adapter and native C tests. No UIKit/Swift ABI. */
#ifndef FV_RESTORE_CORE_H
#define FV_RESTORE_CORE_H
typedef enum {
 FVRestoreDone=1, FVRestoreIdentity=-1, FVRestoreOrientation=-2,
 FVRestoreUnavailable=-3, FVRestoreUnaccepted=-4
} FVRestoreResult;
typedef enum { FVSceneCurrent=1, FVSceneMissing=0, FVSceneReplaced=-1 } FVSceneRelation;
static FVSceneRelation fvRestoreSceneRelation(const void *saved,const void *live){
 if(!live)return FVSceneMissing;
 return saved==live?FVSceneCurrent:FVSceneReplaced;
}
typedef struct {
 void *context;
 int (*identity)(void *);
 int (*orientation)(void *);
 int (*available)(void *);
 int (*submit)(void *);
 int (*accepted)(void *);
 void (*commit)(void *);
} FVRestoreOps;
/* Validate again after message sends: a synchronous callback may replace the
 * scene or snapshot. Only verified acceptance permits cache/local mutation. */
static FVRestoreResult fvRestoreExecute(FVRestoreOps *o){
 if(!o->identity(o->context))return FVRestoreIdentity;
 if(!o->available(o->context))return FVRestoreUnavailable;
 if(!o->orientation(o->context))return FVRestoreOrientation;
 if(!o->identity(o->context))return FVRestoreIdentity;
 if(!o->submit(o->context))return FVRestoreUnavailable;
 if(!o->identity(o->context))return FVRestoreIdentity;
 if(!o->orientation(o->context))return FVRestoreOrientation;
 if(!o->accepted(o->context))return FVRestoreUnaccepted;
 if(!o->identity(o->context))return FVRestoreIdentity;
 if(!o->orientation(o->context))return FVRestoreOrientation;
 o->commit(o->context);
 return FVRestoreDone;
}
/* The snapshot and leave/queue token are object identities, never counters
 * that can wrap. Retained callback contexts keep these identities alive. */
static int fvRestoreCallbackCurrent(const void *token,const void *currentToken,
 const void *scene,const void *currentScene,const void *snapshot,const void *currentSnapshot){
 return token && token==currentToken && scene==currentScene && snapshot==currentSnapshot;
}
#endif
