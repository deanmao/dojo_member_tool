//
//  DLAppDelegate.h
//  DojoLoader
//
//  Created by Dean Mao on 11/29/12.
//  Copyright (c) 2012 Dean Mao. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@class DLWindowController;

@interface DLAppDelegate : NSObject <NSApplicationDelegate>
{
    DLWindowController *windowController;
}

@end
