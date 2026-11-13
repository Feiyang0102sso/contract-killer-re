// AppView.h - the application view (from the nib, CAEAGLLayer-backed): it publishes the GL layer and
// the screen size to CApplet and forwards touches to the CApplet as system events.
#import <UIKit/UIKit.h>

@interface AppView : UIView {
    float m_viewScale;      // content scale factor (2 on Retina screens)
    int m_width;            // view size in points (16.16 rounded down)
    int m_height;
}

@end

// The nib's AppView, set by -[AppView initWithCoder:]; defined in AppDelegate_mm.mm (original _g_appView).
extern AppView* g_appView;
