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
    kii_global_clenaup();
    [super tearDown];
}

- (void)testRegisterThing {
    kii_app_t app = kii_init_app("7de6a67e",
                                 "91cf399515a6f0db2ce3bf5c213094f2",
                                 "https://qa21.internal.kii.com/api");
    NSUUID* id = [[NSUUID alloc]init];
    const char* thing_id = [id.UUIDString
                             cStringUsingEncoding:NSUTF8StringEncoding];
    
    char* accessToken = NULL;
    kii_error_code_t ret = kii_register_thing(app,
                                              thing_id,
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

@end
