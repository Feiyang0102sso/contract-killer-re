// GluVerticalCell.m - side bar cell of the vertical (iPad) Glu Games Network tab bar:
// icon centred in the cell, title below it.
#import "GluVerticalCell.h"

@implementation GluVerticalCell

- (id) initWithStyle: (UITableViewCellStyle) style reuseIdentifier: (NSString*) reuseIdentifier
{
    self = [super initWithStyle: style reuseIdentifier: reuseIdentifier];
    if (self)
    {
        [self setSelectionStyle: UITableViewCellSelectionStyleGray];
    }
    return self;
}

- (void) layoutSubviews
{
    [super layoutSubviews];

    self.textLabel.font = [UIFont fontWithName: @"Helvetica" size: 11];
    self.textLabel.textColor = [UIColor whiteColor];
    [self.textLabel sizeToFit];

    if (self.accessoryView == nil)
    {
        // Icon in the centre, title at the mirrored height below it
        [self.imageView setCenter: CGPointMake(self.contentView.center.x, self.contentView.center.y)];
        [self.textLabel setCenter: CGPointMake(self.contentView.center.x, self.contentView.bounds.size.height - self.textLabel.center.y)];
    }
    else
    {
        // Leave room for the accessory (badge) view
        [self.imageView setCenter: CGPointMake(self.contentView.center.x + 11, self.contentView.center.y)];
        [self.textLabel setCenter: CGPointMake(self.contentView.center.x + 11, self.contentView.bounds.size.height - self.textLabel.center.y)];
    }
}

@end
