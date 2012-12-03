#import <Foundation/Foundation.h>
#import "micronucleus_lib.h"
#import "DLCallbackProgress.h"

@interface DLDevice : NSObject
{
    id delegate;
    micronucleus *device;
}

@property (nonatomic) id delegate;

- uploadMembers:(NSArray*)members;
- updateFirmware:(void*)string;
- waitForDevice;

@end
