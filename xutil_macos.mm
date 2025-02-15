#include "xutil_macos.h"

#include <Cocoa/Cocoa.h>

#include "QtWidgets/QWidget"

namespace xutils_macos 
{

void adjustWindowButton(QWidget *w, qreal closeButtonOffset, qreal minButtonOffset, qreal zoomButtonOffset, qreal titlebarViewHeight)
{
    if(!w || !w->isWindow())
    {
        return;
    }
    NSView *nsview = (NSView *)w->winId();
    if(!nsview)
    {
        return;
    }
    NSWindow *window = [nsview window];
    if(!window)
    {
        return;
    }

    NSButton *closeBtn = [window standardWindowButton:NSWindowCloseButton];
    NSButton *miniaturizeBtn = [window standardWindowButton:NSWindowMiniaturizeButton];
    NSButton *zoomBtn = [window standardWindowButton:NSWindowZoomButton];

    closeBtn.translatesAutoresizingMaskIntoConstraints = NO;
    miniaturizeBtn.translatesAutoresizingMaskIntoConstraints = NO;
    zoomBtn.translatesAutoresizingMaskIntoConstraints = NO;

    NSLayoutConstraint *leftContraint1 = [NSLayoutConstraint constraintWithItem:closeBtn attribute:NSLayoutAttributeLeft
                    relatedBy:NSLayoutRelationEqual toItem:closeBtn.superview attribute:NSLayoutAttributeLeft multiplier:1.0 constant:closeButtonOffset];
    NSLayoutConstraint *topContraint1 = [NSLayoutConstraint constraintWithItem:closeBtn attribute:NSLayoutAttributeCenterY
                    relatedBy:NSLayoutRelationEqual toItem:closeBtn.superview attribute:NSLayoutAttributeCenterY multiplier:1.0 constant:0];
    leftContraint1.active = YES;
    topContraint1.active = YES;

    NSLayoutConstraint *leftContraint2 = [NSLayoutConstraint constraintWithItem:miniaturizeBtn attribute:NSLayoutAttributeLeft
                    relatedBy:NSLayoutRelationEqual toItem:miniaturizeBtn.superview attribute:NSLayoutAttributeLeft multiplier:1.0 constant:minButtonOffset];
    NSLayoutConstraint *topContraint2 = [NSLayoutConstraint constraintWithItem:miniaturizeBtn attribute:NSLayoutAttributeCenterY
                    relatedBy:NSLayoutRelationEqual toItem:miniaturizeBtn.superview attribute:NSLayoutAttributeCenterY multiplier:1.0 constant:0];
    leftContraint2.active = YES;
    topContraint2.active = YES;

    NSLayoutConstraint *leftContraint3 = [NSLayoutConstraint constraintWithItem:zoomBtn attribute:NSLayoutAttributeLeft
                    relatedBy:NSLayoutRelationEqual toItem:zoomBtn.superview attribute:NSLayoutAttributeLeft multiplier:1.0 constant:zoomButtonOffset];
    NSLayoutConstraint *topContraint3 = [NSLayoutConstraint constraintWithItem:zoomBtn attribute:NSLayoutAttributeCenterY
                    relatedBy:NSLayoutRelationEqual toItem:zoomBtn.superview attribute:NSLayoutAttributeCenterY multiplier:1.0 constant:0];
    leftContraint3.active = YES;
    topContraint3.active = YES;

    NSView* _titlebarView = [window standardWindowButton:NSWindowCloseButton].superview;

        //修改原生窗口的样式,分割线和外观
    window.titlebarAppearsTransparent = YES;
    [window setTitleVisibility:NSWindowTitleHidden];

    NSView* titlebarContainerView = _titlebarView.superview;
    titlebarContainerView.postsFrameChangedNotifications = YES;

    NSRect titlebarContainerFrame = titlebarContainerView.frame;
    titlebarContainerFrame.origin.y = window.frame.size.height - titlebarViewHeight;
    titlebarContainerFrame.size.height = titlebarViewHeight;
    titlebarContainerView.frame = titlebarContainerFrame;

    [[NSNotificationCenter defaultCenter]
                        addObserverForName:NSViewFrameDidChangeNotification
                                                        object:titlebarContainerView queue:nil
                                                        usingBlock:^(NSNotification *notification)
        {
        if ((window.styleMask & NSWindowStyleMaskFullScreen) != 0)
            return;
        NSRect titlebarContainerFrame = titlebarContainerView.frame;
        if (titlebarContainerFrame.origin.y >  window.frame.size.height - titlebarViewHeight
            || titlebarContainerFrame.size.height < titlebarViewHeight)
        {
            titlebarContainerFrame.origin.y = window.frame.size.height - titlebarViewHeight;
            titlebarContainerFrame.size.height = titlebarViewHeight;
            titlebarContainerView.frame = titlebarContainerFrame;
        }
        }];
}

// x, false, false, false will crash; x, false, false, true behaves strangely;
void setupDialogTitleBar(QWidget *window, bool showCloseButton, bool showMiniaturizeButton, bool showZoomButton)
{
    if(!window || !window->window())
    {
        return;
    }
    NSView* view = (__bridge NSView *)reinterpret_cast<void *>(window->window()->winId());
    if(!view)
    {
        return;
    }
    NSWindow* nsWindow = [view window];
    if(!nsWindow)
    {
        return;
    }

    NSToolbar* customToolbar = [[NSToolbar alloc] initWithIdentifier:@"main"];
    customToolbar.showsBaselineSeparator = NO;
    nsWindow.titlebarAppearsTransparent = YES;
    nsWindow.titleVisibility = NSWindowTitleHidden;
    nsWindow.toolbar = customToolbar;
    nsWindow.styleMask =  NSTitledWindowMask | NSFullSizeContentViewWindowMask;
    qreal closeButtonOffset = 10, minButtonOffset = 20, zoomButtonOffset = 30;
    qreal delta = 0;
    if(showCloseButton)
    {
        nsWindow.styleMask |= NSWindowStyleMaskClosable;
        delta = 10;
    }
    else
    {
        [[nsWindow standardWindowButton:NSWindowCloseButton] setHidden:YES];
        delta = -10;
    }
    if(showMiniaturizeButton)
    {
        nsWindow.styleMask |= NSWindowStyleMaskMiniaturizable;
        minButtonOffset +=delta;
        delta += 10;
    }
    else
    {
        [[nsWindow standardWindowButton:NSWindowMiniaturizeButton] setHidden:YES];
        delta -= 10;
    }
    if(showZoomButton)
    {
        nsWindow.styleMask |= NSWindowStyleMaskResizable;
        zoomButtonOffset += delta;
    }
    else
    {
        [[nsWindow standardWindowButton:NSWindowZoomButton] setHidden:YES];
    }
    adjustWindowButton(window, closeButtonOffset, minButtonOffset, zoomButtonOffset, 30);
    //解决使用浅色风格，暗黑模式出现一条白线问题
    //if (bUseLightStyle)
    //nsWindow.appearance = [NSAppearance appearanceNamed:NSAppearanceNameAqua];

    [customToolbar release];
    customToolbar = nil;
}

} // namespace xframelesswidget_macos;
