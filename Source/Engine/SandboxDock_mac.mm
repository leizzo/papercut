#include "SandboxDock.h"

#import <AppKit/AppKit.h>

/** A borderless panel that takes keys without activating its app. */
@interface ResamperSandboxDockPanel : NSPanel
@end

@implementation ResamperSandboxDockPanel
- (BOOL) canBecomeKeyWindow    { return YES; }
- (BOOL) canBecomeMainWindow   { return NO; }
@end

namespace resamper::sandboxdock
{

namespace
{
    /** AppKit's desktop is bottom-up from the main display's bottom; JUCE's top-down from its top. */
    CGFloat mainDisplayHeight()
    {
        auto* screens = [NSScreen screens];
        return [screens count] > 0 ? NSMaxY ([[screens objectAtIndex: 0] frame]) : 0;
    }
}

WindowRef windowOf (juce::Component& component)
{
    auto* peer = component.getPeer();
    auto* view = peer != nullptr ? (NSView*) peer->getNativeHandle() : nil;
    auto* window = view != nil ? [view window] : nil;

    if (window == nil)
        return {};

    return { (juce::int64) [window windowNumber], (int) [window level] };
}

struct Panel::Impl
{
    juce::Component* content = nullptr;
    NSPanel* panel = nil;
    id monitor = nil;
};

Panel::Panel (juce::Component& content, std::function<void()> onClicked)
    : impl (std::make_unique<Impl>())
{
    auto* panel = [[ResamperSandboxDockPanel alloc] initWithContentRect: NSMakeRect (0, 0, content.getWidth(), content.getHeight())
                                                              styleMask: NSWindowStyleMaskBorderless | NSWindowStyleMaskNonactivatingPanel
                                                                backing: NSBackingStoreBuffered
                                                                  defer: NO];
    [panel setReleasedWhenClosed: NO];
    [panel setHidesOnDeactivate: NO];   // its app is never active
    [panel setFloatingPanel: NO];
    [panel setBecomesKeyOnlyIfNeeded: NO];
    [panel setHasShadow: NO];
    [panel setCollectionBehavior: NSWindowCollectionBehaviorFullScreenAuxiliary];
    impl->panel = panel;
    impl->content = &content;

    content.setTopLeftPosition (0, 0);
    content.setVisible (true);
    content.addToDesktop (0, (void*) [panel contentView]);

    impl->monitor = [NSEvent addLocalMonitorForEventsMatchingMask: NSEventMaskLeftMouseDown | NSEventMaskRightMouseDown
                                                          handler: ^NSEvent* (NSEvent* event)
    {
        if ([event window] == panel && onClicked)
            onClicked();

        return event;
    }];
}

Panel::~Panel()
{
    [NSEvent removeMonitor: impl->monitor];
    impl->content->removeFromDesktop();
    [impl->panel orderOut: nil];
    [impl->panel release];
}

void Panel::place (juce::Rectangle<int> area, bool visible, WindowRef above)
{
    auto* panel = impl->panel;

    if (! visible || area.isEmpty() || above.number == 0)
    {
        [panel orderOut: nil];
        return;
    }

    [panel setFrame: NSMakeRect (area.getX(), mainDisplayHeight() - area.getBottom(), area.getWidth(), area.getHeight())
            display: YES];
    [panel setLevel: above.level];
    [panel orderWindow: NSWindowAbove relativeTo: (NSInteger) above.number];
}

juce::Rectangle<int> Panel::getScreenBounds() const
{
    auto* panel = impl->panel;

    if (! [panel isVisible])
        return {};

    const auto frame = [panel frame];
    return juce::Rectangle<double> (frame.origin.x, mainDisplayHeight() - NSMaxY (frame), frame.size.width, frame.size.height)
               .toNearestInt();
}

} // namespace resamper::sandboxdock
