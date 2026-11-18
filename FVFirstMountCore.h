/* Shared by first-mount runtime adapter and native tests. No ObjC messages.
 * Pointer identity alone does not extend lifetime: runtime must retain the
 * referenced scene, cache and baseline while calling out to scene APIs. */
#ifndef FV_FIRST_MOUNT_CORE_H
#define FV_FIRST_MOUNT_CORE_H
static int fvFirstMountCurrent(
 const void *scene,const void *liveScene,
 const void *cache,const void *liveCache,
 const void *remote,const void *liveRemote,
 const void *parent,const void *liveParent,
 const void *baseline,const void *liveBaseline,
 long orientation,long liveOrientation){
 return scene && cache && remote && parent && baseline &&
  scene==liveScene && cache==liveCache && remote==liveRemote &&
  parent==liveParent && baseline==liveBaseline &&
  orientation>=1 && orientation<=4 && orientation==liveOrientation;
}
#endif
