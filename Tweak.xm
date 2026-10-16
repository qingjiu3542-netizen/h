#import <UIKit/UIKit.h>
#import "SidebarConfigManager.h"

@interface SidebarController : NSObject
+ (instancetype)sharedInstance;
- (void)installIfNeeded;
@end

@implementation SidebarController {
    UIWindow *_window;
    UIView *_overlayView;
    UIView *_triggerView;
    UIView *_panelView;
    SidebarConfiguration *_config;
    BOOL _visible;
}

+ (instancetype)sharedInstance {
    static SidebarController *controller;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        controller = [[SidebarController alloc] init];
    });
    return controller;
}

- (void)installIfNeeded {
    __weak __typeof(self) weakSelf = self;
    [[SidebarConfigManager sharedInstance] reloadAsync:^(SidebarConfiguration *configuration) {
        __strong __typeof(weakSelf) self = weakSelf;
        self->_config = configuration;
        [self setupWindowIfNeeded];
        [self applyLayout];
        [self rebuildAppButtons];
    }];

    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(applyLayout) name:UIApplicationDidChangeStatusBarOrientationNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(applyLayout) name:UIApplicationDidChangeStatusBarFrameNotification object:nil];
}

- (void)dealloc {
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)setupWindowIfNeeded {
    if (_window != nil) {
        return;
    }

    CGRect frame = UIScreen.mainScreen.bounds;
    _window = [[UIWindow alloc] initWithFrame:frame];
    _window.windowLevel = UIWindowLevelAlert + 1;
    _window.backgroundColor = UIColor.clearColor;
    _window.hidden = NO;

    _overlayView = [[UIView alloc] initWithFrame:frame];
    _overlayView.backgroundColor = UIColor.clearColor;
    _overlayView.hidden = YES;

    UITapGestureRecognizer *overlayTap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(hideSidebar)];
    [_overlayView addGestureRecognizer:overlayTap];

    _triggerView = [[UIView alloc] initWithFrame:CGRectZero];
    _triggerView.backgroundColor = UIColor.clearColor;

    UIPanGestureRecognizer *triggerPan = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(handleTriggerPan:)];
    [_triggerView addGestureRecognizer:triggerPan];

    _panelView = [[UIView alloc] initWithFrame:CGRectZero];
    _panelView.backgroundColor = [UIColor colorWithWhite:0.12 alpha:0.85];
    _panelView.layer.cornerRadius = 18.f;
    _panelView.layer.masksToBounds = NO;
    _panelView.layer.shadowColor = [UIColor blackColor].CGColor;
    _panelView.layer.shadowOpacity = 0.25;
    _panelView.layer.shadowRadius = 12.f;
    _panelView.layer.shadowOffset = CGSizeMake(-2.f, 0.f);

    UIPanGestureRecognizer *panelPan = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(handlePanelPan:)];
    [_panelView addGestureRecognizer:panelPan];

    [_window addSubview:_overlayView];
    [_window addSubview:_panelView];
    [_window addSubview:_triggerView];
}

- (void)applyLayout {
    if (!_window) {
        return;
    }

    CGRect bounds = UIScreen.mainScreen.bounds;
    _window.frame = bounds;
    _overlayView.frame = bounds;

    BOOL enabled = _config ? _config.enabled : YES;
    _triggerView.hidden = !enabled;
    _panelView.hidden = !enabled;
    if (!enabled) {
        _overlayView.hidden = YES;
        return;
    }

    CGFloat triggerWidth = _config ? _config.triggerEdgeWidth : 16.f;
    CGFloat sidebarWidth = _config ? _config.sidebarWidth : 240.f;
    CGFloat opacity = _config ? _config.opacity : 0.85f;
    CGFloat centerYRatio = _config ? _config.positionYRatio : 0.5f;

    CGFloat panelHeight = MIN(bounds.size.height - 80.f, MAX(220.f, (CGFloat)(_config.apps.count * 60.f + 30.f)));
    CGFloat centerY = bounds.size.height * centerYRatio;
    CGFloat panelXVisible = bounds.size.width - sidebarWidth - 10.f;
    CGFloat panelXHidden = bounds.size.width + 10.f;

    _triggerView.frame = CGRectMake(bounds.size.width - triggerWidth, 0, triggerWidth, bounds.size.height);
    _panelView.alpha = opacity;
    _panelView.frame = CGRectMake(_visible ? panelXVisible : panelXHidden,
                                  centerY - panelHeight / 2.f,
                                  sidebarWidth,
                                  panelHeight);
}

- (void)rebuildAppButtons {
    NSArray<UIView *> *existingSubviews = [_panelView.subviews copy];
    for (UIView *view in existingSubviews) {
        [view removeFromSuperview];
    }

    UIButton *closeButton = [UIButton buttonWithType:UIButtonTypeSystem];
    closeButton.frame = CGRectMake(_panelView.bounds.size.width - 42.f, 8.f, 34.f, 34.f);
    [closeButton setTitle:@"×" forState:UIControlStateNormal];
    closeButton.titleLabel.font = [UIFont systemFontOfSize:28.f weight:UIFontWeightLight];
    [closeButton setTitleColor:UIColor.whiteColor forState:UIControlStateNormal];
    [closeButton addTarget:self action:@selector(hideSidebar) forControlEvents:UIControlEventTouchUpInside];
    [_panelView addSubview:closeButton];

    CGFloat y = 44.f;
    for (NSUInteger index = 0; index < _config.apps.count; index++) {
        SBLauncherItem *item = _config.apps[index];
        UIButton *button = [UIButton buttonWithType:UIButtonTypeSystem];
        button.frame = CGRectMake(12.f, y, _panelView.bounds.size.width - 24.f, 48.f);
        button.tag = index;
        button.layer.cornerRadius = 12.f;
        button.backgroundColor = [UIColor colorWithWhite:1 alpha:0.08];
        button.contentHorizontalAlignment = UIControlContentHorizontalAlignmentLeft;
        button.contentEdgeInsets = UIEdgeInsetsMake(0, 12.f, 0, 10.f);
        [button setTitle:item.displayName forState:UIControlStateNormal];
        [button setTitleColor:UIColor.whiteColor forState:UIControlStateNormal];
        button.titleLabel.font = [UIFont systemFontOfSize:16.f weight:UIFontWeightSemibold];

        UIImage *icon = nil;
        if (item.iconPath.length > 0) {
            icon = [UIImage imageWithContentsOfFile:item.iconPath];
        }
        if (!icon && item.iconSystemName.length > 0) {
            icon = [UIImage systemImageNamed:item.iconSystemName];
        }
        if (icon) {
            [button setImage:icon forState:UIControlStateNormal];
            button.tintColor = UIColor.whiteColor;
            button.imageEdgeInsets = UIEdgeInsetsMake(0, 0, 0, 8.f);
        }

        [button addTarget:self action:@selector(openApp:) forControlEvents:UIControlEventTouchUpInside];
        [_panelView addSubview:button];
        y += 56.f;
    }
}

- (void)openApp:(UIButton *)sender {
    if (sender.tag >= _config.apps.count) {
        return;
    }
    SBLauncherItem *item = _config.apps[(NSUInteger)sender.tag];
    NSURL *url = [NSURL URLWithString:item.urlScheme];
    if (url == nil) {
        return;
    }

    UIApplication *app = UIApplication.sharedApplication;
    if ([app canOpenURL:url]) {
        [app openURL:url options:@{} completionHandler:nil];
        [self hideSidebar];
    }
}

- (void)handleTriggerPan:(UIPanGestureRecognizer *)gesture {
    CGPoint translation = [gesture translationInView:_triggerView];
    if (gesture.state == UIGestureRecognizerStateEnded && translation.x < -20.f) {
        [self showSidebar];
    }
}

- (void)handlePanelPan:(UIPanGestureRecognizer *)gesture {
    CGPoint translation = [gesture translationInView:_panelView];
    if (gesture.state == UIGestureRecognizerStateEnded && translation.x > 20.f) {
        [self hideSidebar];
    }
}

- (void)showSidebar {
    if (_visible) {
        return;
    }
    _visible = YES;
    _overlayView.hidden = NO;
    [UIView animateWithDuration:0.2 animations:^{
        [self applyLayout];
    }];
}

- (void)hideSidebar {
    if (!_visible) {
        return;
    }
    _visible = NO;
    [UIView animateWithDuration:0.2 animations:^{
        [self applyLayout];
    } completion:^(__unused BOOL finished) {
        self->_overlayView.hidden = YES;
    }];
}

@end

%hook SpringBoard

- (void)applicationDidFinishLaunching:(id)application {
    %orig;
    [[SidebarController sharedInstance] installIfNeeded];
}

%end
