//
//  ThingSDKBasicTests.m
//  KiiThingSDK
//
//  Created by 熊野 聡 on 2014/10/17.
//  Copyright (c) 2014年 Kii. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <XCTest/XCTest.h>
#include "jansson.h"
#include "kii_cloud.h"

@interface ThingSDKBasicTests : XCTestCase

@end

@implementation ThingSDKBasicTests

- (void)setUp {
    [super setUp];
    kii_global_init();
}

- (void)tearDown {
    kii_global_cleanup();
    [super tearDown];
}

static const char* ACCESS_TOKEN = "Xk899v8Jp9A5bX9__2WTZ8TBkJlHx1nUmjpp0lBprpI";
static const char* APPID = "84fff36e";
static const char* APPKEY = "e45fcc2d31d6aca675af639bc5f04a26";
static const char* BASEURL = "api-development-jp.internal.kii.com/api";

- (void)testRegisterThing {
    kii_app_t app = kii_init_app(APPID,
                                 APPKEY,
                                 BASEURL);
    NSUUID* id = [[NSUUID alloc]init];
    const char* thing_id = [id.UUIDString
                             cStringUsingEncoding:NSUTF8StringEncoding];
    
    char* accessToken = NULL;
    kii_error_code_t ret = kii_register_thing(app,
                                              thing_id,
                                              "THERMOMETER",
                                              "1234", NULL, &accessToken);
    XCTAssertEqual(ret, KIIE_OK, "register failed");
    XCTAssertTrue(strlen(accessToken) > 0, "access token invalid");
    if (ret != KIIE_OK) {
        kii_error_t* err = kii_get_last_error(app);
        NSLog(@"code: %s", err->error_code);
        NSLog(@"resp code: %d", err->status_code);
    }
    kii_dispose_app(app);
    kii_dispose_kii_char(accessToken);
}


-(void) testInstallPush {
    kii_app_t app = kii_init_app(APPID,
                                 APPKEY,
                                 BASEURL);
    char* installId = NULL;
    kii_error_code_t ret =
        kii_install_thing_push(app, ACCESS_TOKEN, true, &installId);
    XCTAssertEqual(ret, KIIE_OK, "register failed");
    XCTAssertTrue(strlen(installId) > 0, "installId invalid");
    if (ret != KIIE_OK) {
        kii_error_t* err = kii_get_last_error(app);
        NSLog(@"code: %s", err->error_code);
        NSLog(@"resp code: %d", err->status_code);
    }
    kii_dispose_app(app);
    kii_dispose_kii_char(installId);
}

@end
