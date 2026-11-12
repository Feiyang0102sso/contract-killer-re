#pragma once
#import <UIKit/UIKit.h>

@class TJCVideoObject;

/** Tapjoy's post-playback offer and dismissal controls. */
@interface TJCVideoLayer : UIView {
    UILabel* statusLabel_;
    NSMutableArray* linkButtons_;
    NSMutableArray* linkURLs_;
    UIButton* doneButton_;
}
- (id)initWithVideo:(TJCVideoObject*)video target:(id)target dismissAction:(SEL)action;
- (void)enableShowVideoCompleteScreen:(BOOL)show;
- (void)buttonAction1;
- (void)buttonAction2;
@end
