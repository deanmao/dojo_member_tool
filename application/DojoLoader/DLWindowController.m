//
//  DLWindowController.m
//  DojoLoader
//
//  Created by Dean Mao on 12/1/12.
//  Copyright (c) 2012 Dean Mao. All rights reserved.
//

#import "DLWindowController.h"
#import "micronucleus_lib.h"

@interface DLWindowController ()

@end

@implementation DLWindowController

- (id)initWithWindow:(NSWindow *)window
{
    self = [super initWithWindow:window];
    if (self) {
        device = [[DLDevice alloc] init];
        device.delegate = self;
        queue = dispatch_queue_create("com.deanmao.worker_queue", NULL);
    }
    
    return self;
}

- (void)windowDidLoad
{
    [super windowDidLoad];
	NSImage* logoImage = [NSImage imageNamed:@"hacker-dojo.png"];
    [logoImage drawAtPoint:NSMakePoint(20, 200) fromRect:NSMakeRect(0, 0, 197, 71) operation:NSCompositeSourceOver fraction:1.0];
    [progressIndicator setHidden:true];
    [progressIndicator setIndeterminate:false];
    [statusField setHidden: true];
}


- (IBAction)updateFirmware:(id)sender
{
    dispatch_async(queue, ^{
        NSLog(@"downloading firmware...");
        
        
        NSLog(@"updating firmware");
        [device updateFirmware];
    });
}

- (IBAction)uploadMembers:(id)sender
{
    dispatch_async(queue, ^{
        NSLog(@"downloading members...");
        
        
        NSLog(@"uploading members");
        [device uploadMembers];
    });
}

-(void) progress:(double)delta label:(NSString*)label
{
    if ([statusField isHidden]) {
        [progressIndicator setHidden: false];
        [statusField setHidden: false];
        [progressIndicator setDoubleValue: 0];
        [progressIndicator startAnimation:self];
    }
    if (([progressIndicator doubleValue] + delta) > 100) {
        [progressIndicator setHidden:true];
        [statusField setHidden: true];
    } else {
        [statusField setStringValue: label];
        [progressIndicator incrementBy:delta];
    }
}


@end
