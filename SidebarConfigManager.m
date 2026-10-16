#import "SidebarConfigManager.h"

static NSString * const kSidebarConfigPath = @"/var/jb/Library/SidebarLauncher/config.plist";

@implementation SBLauncherItem
@end

@implementation SidebarConfiguration
@end

@implementation SidebarConfigManager

+ (instancetype)sharedInstance {
    static SidebarConfigManager *manager;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        manager = [[SidebarConfigManager alloc] init];
    });
    return manager;
}

- (void)reloadAsync:(void (^)(SidebarConfiguration *configuration))completion {
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_UTILITY, 0), ^{
        SidebarConfiguration *configuration = [self loadConfiguration];
        dispatch_async(dispatch_get_main_queue(), ^{
            if (completion) {
                completion(configuration);
            }
        });
    });
}

- (SidebarConfiguration *)loadConfiguration {
    NSDictionary *dictionary = [NSDictionary dictionaryWithContentsOfFile:kSidebarConfigPath];
    SidebarConfiguration *configuration = [[SidebarConfiguration alloc] init];
    configuration.enabled = [dictionary[@"enabled"] respondsToSelector:@selector(boolValue)] ? [dictionary[@"enabled"] boolValue] : YES;
    configuration.triggerEdgeWidth = [self sanitizeFloat:dictionary[@"triggerEdgeWidth"] fallback:16.f min:6.f max:80.f];
    configuration.sidebarWidth = [self sanitizeFloat:dictionary[@"sidebarWidth"] fallback:240.f min:140.f max:380.f];
    configuration.opacity = [self sanitizeFloat:dictionary[@"opacity"] fallback:0.85f min:0.1f max:1.f];
    configuration.positionYRatio = [self sanitizeFloat:dictionary[@"positionYRatio"] fallback:0.5f min:0.1f max:0.9f];

    NSMutableArray<SBLauncherItem *> *items = [NSMutableArray array];
    NSArray *rawApps = [dictionary[@"apps"] isKindOfClass:[NSArray class]] ? dictionary[@"apps"] : @[];
    for (NSDictionary *entry in rawApps) {
        if (![entry isKindOfClass:[NSDictionary class]]) {
            continue;
        }
        NSString *urlScheme = [entry[@"urlScheme"] isKindOfClass:[NSString class]] ? entry[@"urlScheme"] : nil;
        NSString *displayName = [entry[@"displayName"] isKindOfClass:[NSString class]] ? entry[@"displayName"] : nil;
        if (urlScheme.length == 0 || displayName.length == 0) {
            continue;
        }
        SBLauncherItem *item = [[SBLauncherItem alloc] init];
        item.bundleID = [entry[@"bundleID"] isKindOfClass:[NSString class]] ? entry[@"bundleID"] : @"unknown.bundle";
        item.displayName = displayName;
        item.urlScheme = urlScheme;
        item.iconSystemName = [entry[@"iconSystemName"] isKindOfClass:[NSString class]] ? entry[@"iconSystemName"] : nil;
        item.iconPath = [entry[@"iconPath"] isKindOfClass:[NSString class]] ? entry[@"iconPath"] : nil;
        [items addObject:item];
    }
    configuration.apps = items;
    return configuration;
}

- (CGFloat)sanitizeFloat:(id)value fallback:(CGFloat)fallback min:(CGFloat)min max:(CGFloat)max {
    CGFloat number = [value respondsToSelector:@selector(floatValue)] ? [value floatValue] : fallback;
    if (number < min) {
        return min;
    }
    if (number > max) {
        return max;
    }
    return number;
}

@end
