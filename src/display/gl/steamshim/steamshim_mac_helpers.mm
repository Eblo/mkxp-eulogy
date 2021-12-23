//
//  steamshim_mac_helpers.mm
//  shim
//
//  Created by ゾロアーク on 1/3/21.
//

#import <Foundation/Foundation.h>
#import "steamshim_mac_helpers.h"

std::string execPath() {
    NSString *p = [NSBundle.mainBundle.executablePath stringByDeletingLastPathComponent];
    std::string ret([NSString pathWithComponents:@[p, @(GAME_LAUNCH_NAME)]].UTF8String);
    return ret;
}

std::string appResourcePath() {
    return std::string(NSBundle.mainBundle.resourcePath.UTF8String);
}
