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
    if ([manualCheckBox state] == NSOnState) {
        // show open file dialog
        NSOpenPanel *panel	= [NSOpenPanel openPanel];
        [panel setAllowedFileTypes: [NSArray arrayWithObjects:@"hex", nil]];
        NSInteger button	= [panel runModal];
        if(button == NSOKButton){
            NSArray *urls = [panel URLs];
            
            dispatch_async(queue, ^{
                NSLog(@"downloading firmware...");
                
                
                NSLog(@"updating firmware");
                //[device updateFirmware];
            });
        }
    } else {
        // download firmware from the web
        dispatch_async(queue, ^{
            NSLog(@"downloading firmware...");
            
            
            NSLog(@"updating firmware");
            //[device updateFirmware];
        });
        
    }
}

- (IBAction)uploadMembers:(id)sender
{
    dispatch_async(queue, ^{
        NSLog(@"downloading members...");
        
        
        NSLog(@"uploading members");
        NSArray  *members = [NSArray arrayWithObjects:@"0000241450", @"0000246317", @"0011873927",nil];
        [device uploadMembers: members];
    });
}

-(void) success:(NSString*)label
{
    [progressIndicator setDoubleValue: 100];
    [statusField setStringValue: label];
}

-(void) failure:(NSString*)label
{
    NSAlert *testAlert = [NSAlert alertWithMessageText:@"OOPS!"
                                         defaultButton:@"OK"
                                       alternateButton:nil
                                           otherButton:nil
                             informativeTextWithFormat:label];
    [testAlert runModal];
    [progressIndicator setHidden: true];
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
        [progressIndicator setDoubleValue: 100];
    } else {
        [statusField setStringValue: label];
        [progressIndicator incrementBy:delta];
    }
}


@end
