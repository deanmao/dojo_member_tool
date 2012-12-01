//
//  DLAppDelegate.m
//  DojoLoader
//
//  Created by Dean Mao on 11/29/12.
//  Copyright (c) 2012 Dean Mao. All rights reserved.
//

#import "DLAppDelegate.h"
#import "DLWindowController.h"

@implementation DLAppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
	windowController = [[DLWindowController alloc] initWithWindowNibName:@"MainWindow"];
	[windowController showWindow:self];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender
{
	return YES;
}

@end
