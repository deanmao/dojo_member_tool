//
//  DLWindowController.h
//  DojoLoader
//
//  Created by Dean Mao on 12/1/12.
//  Copyright (c) 2012 Dean Mao. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "DLDevice.h"
#import "DLCallbackProgress.h"

@interface DLWindowController : NSWindowController <DLCallbackProgress>
{
    IBOutlet NSButton *updateFirmwareButton;
    IBOutlet NSButton *uploadMembersButton;
    IBOutlet NSTextField *statusField;
    IBOutlet NSProgressIndicator *progressIndicator;
    DLDevice *device;
    dispatch_queue_t queue;
}

- (IBAction)uploadMembers:(id)sender;
- (IBAction)updateFirmware:(id)sender;

@end