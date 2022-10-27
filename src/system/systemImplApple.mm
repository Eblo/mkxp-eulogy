//
//  systemImplApple.m
//  Player
//
//  Created by ゾロアーク on 11/22/20.
//

#import <AppKit/AppKit.h>
#import <Metal/Metal.h>

#import <sys/sysctl.h>
#import "system.h"
#import "SettingsMenuController.h"

std::string systemImpl::getSystemLanguage() {
    NSString *languageCode = NSLocale.currentLocale.languageCode;
    NSString *countryCode = NSLocale.currentLocale.countryCode;
    return std::string([NSString stringWithFormat:@"%@_%@", languageCode, countryCode].UTF8String);
}

std::string systemImpl::getUserName() {
    return std::string(NSUserName().UTF8String);
}

int systemImpl::getScalingFactor() {
    return NSApplication.sharedApplication.mainWindow.backingScaleFactor;
}

bool systemImpl::isWine() {
    return false;
}

bool systemImpl::isRosetta() {
    int translated = 0;
    size_t size = sizeof(translated);
    int result = sysctlbyname("sysctl.proc_translated", &translated, &size, NULL, 0);
    
    if (result == -1)
        return false;
    
    return translated;
}

systemImpl::WineHostType systemImpl::getRealHostType() {
    return WineHostType::Mac;
}


// constant, if it's not nil then just raise the menu instead
SettingsMenu *smenu = nil;
void openSettingsWindow() {
    if (smenu == nil) {
        smenu = [SettingsMenu openWindow];
        return;
    }
    [smenu raise];
}

bool isMetalSupported() {
    if (@available(macOS 10.13.0, *)) {
        return MTLCreateSystemDefaultDevice() != nil;
    }
    return false;
}
