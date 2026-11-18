#ifndef FV_KEYBOARD_AVOIDANCE_CORE_H
#define FV_KEYBOARD_AVOIDANCE_CORE_H
/* Pure production geometry. Every rectangle is in the same window space.
 * This translates the whole item; it does not implement client input insets. */
typedef struct { double x,y,width,height; } FVRect;
typedef struct { double x,y; } FVPoint;
typedef enum { FVKAInvalid=0, FVKAClear=1, FVKAMove=2, FVKANoRoom=3 } FVKAResult;
static inline double fvKACoreMin(double a,double b){return a<b?a:b;}
static inline double fvKACoreMax(double a,double b){return a>b?a:b;}
static inline int fvKAFinite(double a){return __builtin_isfinite(a)&&a>-1000000&&a<1000000;}
static inline int fvKAValidRect(FVRect r){
 return fvKAFinite(r.x)&&fvKAFinite(r.y)&&fvKAFinite(r.width)&&fvKAFinite(r.height)&&r.width>0&&r.height>0;
}
static inline int fvKAIntersects(FVRect a,FVRect b){
 return fvKAValidRect(a)&&fvKAValidRect(b)&&a.x<b.x+b.width&&a.x+a.width>b.x&&a.y<b.y+b.height&&a.y+a.height>b.y;
}
/* Event policy: a source-less hide may only clear an existing session;
 * events from a different display never replace the current keyboard. */
static inline int fvKAEvent(int active,int sourceKnown,int sourceMatches,int hiding,int visible){
 if(hiding)return active&&(!sourceKnown||sourceMatches)?-1:0;
 if(!sourceKnown||(active&&!sourceMatches))return 0;
 return visible?1:-1;
}
static inline FVKAResult fvKAPlan(FVRect item,FVRect keyboard,FVRect viewport,double gap,double *offset){
 if(!offset)return FVKAInvalid;
 *offset=0;
 if(!fvKAValidRect(item)||!fvKAValidRect(keyboard)||!fvKAValidRect(viewport)||!fvKAFinite(gap)||gap<0)return FVKAInvalid;
 double left=fvKACoreMax(keyboard.x,viewport.x),right=fvKACoreMin(keyboard.x+keyboard.width,viewport.x+viewport.width);
 double top=fvKACoreMax(keyboard.y,viewport.y),bottom=fvKACoreMin(keyboard.y+keyboard.height,viewport.y+viewport.height);
 if(right<=left||bottom<=top||item.x+item.width<=left||item.x>=right||item.y+item.height<=top||item.y>=bottom)return FVKAClear;
 double required=item.y+item.height-top+gap;
 if(item.y-required<viewport.y)return FVKANoRoom;
 *offset=required;return FVKAMove;
}
/* Convert a window displacement into the parent's coordinates. Includes
 * ancestor scale/rotation; translation cancels out in this vector operation. */
static inline int fvKAVector(double a,double b,double c,double d,double dx,double dy,FVPoint *out){
 if(!out||!fvKAFinite(a)||!fvKAFinite(b)||!fvKAFinite(c)||!fvKAFinite(d)||!fvKAFinite(dx)||!fvKAFinite(dy))return 0;
 double det=a*d-b*c;if(!fvKAFinite(det)||__builtin_fabs(det)<1e-9)return 0;
 FVPoint p={(d*dx-c*dy)/det,(-b*dx+a*dy)/det};
 if(!fvKAFinite(p.x)||!fvKAFinite(p.y))return 0;
 *out=p;return 1;
}
static inline int fvKAPointNear(FVPoint a,FVPoint b){return __builtin_fabs(a.x-b.x)<0.01&&__builtin_fabs(a.y-b.y)<0.01;}
/* Only undo a displacement we still own. A drag/original layout supersedes it. */
static inline FVPoint fvKABaseline(FVPoint current,FVPoint last,FVPoint delta,int applied){
 if(applied&&fvKAPointNear(current,last))return (FVPoint){current.x-delta.x,current.y-delta.y};
 return current;
}
#endif
