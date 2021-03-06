//
//  DLCallbackProgress.h
//  DojoLoader
//
//  Created by Dean Mao on 12/1/12.
//  Copyright (c) 2012 Dean Mao. All rights reserved.
//

#import <Foundation/Foundation.h>

@protocol DLCallbackProgress <NSObject>
-(void) progress:(double)delta label:(NSString*)label;
-(void) success:(NSString *)label;
-(void) failure:(NSString *)label;
-(BOOL) cancelled;
@end
