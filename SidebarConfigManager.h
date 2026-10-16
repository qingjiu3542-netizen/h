#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@interface SBLauncherItem : NSObject
@property (nonatomic, copy) NSString *bundleID;
@property (nonatomic, copy) NSString *displayName;
@property (nonatomic, copy) NSString *urlScheme;
@property (nonatomic, copy, nullable) NSString *iconSystemName;
@property (nonatomic, copy, nullable) NSString *iconPath;
@end

@interface SidebarConfiguration : NSObject
@property (nonatomic, assign) BOOL enabled;
@property (nonatomic, assign) CGFloat triggerEdgeWidth;
@property (nonatomic, assign) CGFloat sidebarWidth;
@property (nonatomic, assign) CGFloat opacity;
@property (nonatomic, assign) CGFloat positionYRatio;
@property (nonatomic, copy) NSArray<SBLauncherItem *> *apps;
@end

@interface SidebarConfigManager : NSObject
+ (instancetype)sharedInstance;
- (void)reloadAsync:(void (^)(SidebarConfiguration *configuration))completion;
@end

NS_ASSUME_NONNULL_END
