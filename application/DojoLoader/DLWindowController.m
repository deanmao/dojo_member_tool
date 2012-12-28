//
//  DLWindowController.m
//  DojoLoader
//
//  Created by Dean Mao on 12/1/12.
//  Copyright (c) 2012 Dean Mao. All rights reserved.
//

#import "DLWindowController.h"
#import "micronucleus_lib.h"
#import "SBJson.h"

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
    [cancelProgressButton setImage: [NSImage imageNamed:NSImageNameStopProgressFreestandingTemplate]];
    [cancelProgressButton setHidden:true];
    cancelled = NO;
}

- (IBAction)cancelProgress:(id)sender
{
    cancelled = YES;
    [self success: @"Cancelled"];
}

- (BOOL) cancelled
{
    return cancelled;
}

- (IBAction)updateFirmware:(id)sender
{
    cancelled = NO;
    if ([manualCheckBox state] == NSOnState) {
        // show open file dialog
        NSOpenPanel *panel	= [NSOpenPanel openPanel];
        [panel setAllowedFileTypes: [NSArray arrayWithObjects:@"hex", nil]];
        NSInteger button	= [panel runModal];
        if(button == NSOKButton){
            NSArray *urls = [panel URLs];
            
            dispatch_async(queue, ^{
                NSLog(@"opening firmware from file...");
                [self progress:10.0 label:@"Reading file"];
                
                NSLog(@"updating firmware");
                NSData *result = [NSData dataWithContentsOfURL:[urls objectAtIndex:0]];
                NSString* str = [[NSString alloc] initWithData:result encoding:NSUTF8StringEncoding];
                [device updateFirmware: [str UTF8String]];
            });
        }
    } else {
        // download firmware from the web
        dispatch_async(queue, ^{
            NSLog(@"downloading firmware...");
            
            [self progress:10.0 label:@"Downloading firmware..."];
            NSString *url = @"https://github.com/downloads/deanmao/dojo_member_tool/firmware.hex";
            NSURLRequest *theRequest=[NSURLRequest requestWithURL:[NSURL URLWithString:url]
                                                      cachePolicy:NSURLRequestUseProtocolCachePolicy
                                                  timeoutInterval:20.0];
            NSError        *error = nil;
            NSURLResponse  *response = nil;
            
            NSData *result = [NSURLConnection sendSynchronousRequest:theRequest returningResponse:&response error:&error];
            if (error) {
                [self failure:@"could not connect to server"];
            } else {
                [self progress:10.0 label:@"About to update firmware..."];
                NSLog(@"updating firmware, %ld", [result length]);
                NSString* str = [[NSString alloc] initWithData:result encoding:NSUTF8StringEncoding];
                [device updateFirmware: [str UTF8String]];
            }
        });
        
    }
}

- (IBAction)uploadMembers:(id)sender
{
    cancelled = NO;
    dispatch_async(queue, ^{
        NSLog(@"downloading members...");
        
        [self progress:10.0 label:@"Downloading members..."];
        NSString *url = @"http://signup.hackerdojo.com/api/rfid?maglock:key=e2842770a39a4";
        NSURLRequest *theRequest=[NSURLRequest requestWithURL:[NSURL URLWithString:url]
                                                  cachePolicy:NSURLRequestUseProtocolCachePolicy
                                              timeoutInterval:20.0];
        NSError        *error = nil;
        NSURLResponse  *response = nil;
        
        NSData *result = [NSURLConnection sendSynchronousRequest:theRequest returningResponse:&response error:&error];
        if (error) {
            [self failure:@"could not connect to server"];
        } else {
            [self progress:10.0 label:@"Parsing JSON..."];
            SBJsonParser *parser = [[SBJsonParser alloc] init];
            NSArray *hashes = [parser objectWithData:result];
            NSMutableArray *members = [[NSMutableArray alloc] init];
            for(NSDictionary *item in hashes) {
                NSString *key = [item valueForKey: @"rfid_tag"];
                [members addObject:key];
            }
            [members addObject: @"0011873927"];
            NSLog(@"uploading members");
            [device uploadMembers: members];
        }
    });
}

-(void) success:(NSString*)label
{
    [progressIndicator setDoubleValue: 100];
    [statusField setStringValue: label];
    if (!cancelled) {
        NSAlert *testAlert = [NSAlert alertWithMessageText:@"Success!"
                                             defaultButton:@"OK"
                                           alternateButton:nil
                                               otherButton:nil
                                 informativeTextWithFormat:label];
        [testAlert runModal];
    }
    [progressIndicator setDoubleValue: 0];
    [statusField setStringValue: @""];
    [progressIndicator setHidden: true];
    [cancelProgressButton setHidden: true];
    [statusField setHidden: true];
    [updateFirmwareButton setEnabled: true];
    [uploadMembersButton setEnabled: true];
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
    [cancelProgressButton setHidden: true];
    [updateFirmwareButton setEnabled: true];
    [uploadMembersButton setEnabled: true];
}

-(void) progress:(double)delta label:(NSString*)label
{
    [updateFirmwareButton setEnabled: false];
    [uploadMembersButton setEnabled: false];
    if ([statusField isHidden]) {
        [statusField setHidden: false];
    }
    if ([progressIndicator isHidden]) {
        [cancelProgressButton setHidden: false];
        [progressIndicator setHidden: false];
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
