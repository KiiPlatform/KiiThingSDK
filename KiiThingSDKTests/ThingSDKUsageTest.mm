//
//  ThingSDKUsageTest.m
//  KiiThingSDK
//
//  Created by 熊野 聡 on 2014/10/08.
//  Copyright (c) 2014年 Kii. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <XCTest/XCTest.h>
#import "kii_cloud.h"

@interface ThingSDKUsageTest : XCTestCase

@end

@implementation ThingSDKUsageTest

- (void)setUp {
    [super setUp];
}

- (void)tearDown {
    [super tearDown];
}

- (void)testExample {
    kii_app_t app = kii_init_app("appid", "appkey", KII_SITE_JP);
    kii_dispose_app(app);
}

@end
