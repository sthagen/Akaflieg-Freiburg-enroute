#include "ObjCAdapter.h"
#include <QtCore/QString>
#import <Foundation/Foundation.h>
#import <AudioToolbox/AudioToolbox.h>
#import <UIKit/UIKit.h>

#import <UserNotifications/UserNotifications.h>
#import <CoreLocation/CoreLocation.h>
#import "ObjectiveC.h"
#import "SafeAreaService.h"


//MARK: Vibration
void ObjCAdapter::vibrateBrief() {
    AudioServicesPlayAlertSound(1519);
}

void ObjCAdapter::vibrateError() {
    /*UINotificationFeedbackGenerator *myGen = [[UINotificationFeedbackGenerator alloc] init];
     [myGen prepare];
     [myGen notificationOccurred: UINotificationFeedbackTypeError];*/
    AudioServicesPlayAlertSound(1107);
}

void ObjCAdapter::vibrateLong() {

    AudioServicesPlayAlertSound(kSystemSoundID_Vibrate);
}


//MARK: Safe Area
double ObjCAdapter::safeAreaTopInset() {
    return [[SafeAreaService sharedInstance] safeAreaTop];
}
double ObjCAdapter::safeAreaLeftInset() {
    return [[SafeAreaService sharedInstance] safeAreaLeft];
}
double ObjCAdapter::safeAreaBottomInset() {
    return [[SafeAreaService sharedInstance] safeAreaBottom];
}
double ObjCAdapter::safeAreaRightInset() {
    return [[SafeAreaService sharedInstance] safeAreaRight];
}


//MARK: File Transfer
auto ObjCAdapter::shareContent(const QByteArray& contentByteArray, const QString& mimeType, const QString& fileNameTemplate, const QString& fileExtension) -> QString
{
    
    auto content = contentByteArray.toNSData();
    
    NSURL *tmpDirURL = [NSURL fileURLWithPath:NSTemporaryDirectory() isDirectory:YES];
    NSURL *fileURL = [[tmpDirURL URLByAppendingPathComponent:fileNameTemplate.toNSString()] URLByAppendingPathExtension:fileExtension.toNSString()];
    
    if (fileURL && [content writeToURL: fileURL atomically: YES]) {
        UIWindow *keyWindow = nil;
        for (UIWindowScene* windowScene in [UIApplication sharedApplication].connectedScenes) {
            if (windowScene.activationState == UISceneActivationStateForegroundActive) {
                keyWindow = windowScene.windows.firstObject;
                break;
            }
        }
        UIViewController *rootViewController = keyWindow.rootViewController;
        UIActivityViewController *activityController = [[UIActivityViewController alloc]
                                                        initWithActivityItems: @[fileURL]
                                                        applicationActivities: nil];
        
        if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad) {
            CGSize screenSize = [UIScreen mainScreen].bounds.size;
            activityController.popoverPresentationController.sourceView = rootViewController.view;
            activityController.popoverPresentationController.sourceRect = CGRectMake(screenSize.width - 30, 30,0,0);
        }
        [rootViewController presentViewController:activityController animated:YES completion:nil];
        [activityController release];
        return {};
    }
    
    return "Failed to write file";
}


//MARK: Misc
QString ObjCAdapter::preferredLanguage() {
    //TODO: Use preferredLocalizationsFromArray: later
    NSString *language = [[[NSBundle mainBundle] preferredLocalizations] objectAtIndex: 0];
    return QString::fromNSString(language);
}


void ObjCAdapter::disableScreenSaver() {
    [[UIApplication sharedApplication] setIdleTimerDisabled:YES];
}

void ObjCAdapter::saveToGallery(QString& path) {
    UIImage* image = [UIImage imageNamed:path.toNSString()];
    UIImageWriteToSavedPhotosAlbum(image, Nil, Nil, Nil);
}

