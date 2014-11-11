//
//  ThingSDKLargeTests.m
//  KiiThingSDK
//
//  Created by 熊野 聡 on 2014/11/11.
//  Copyright (c) 2014年 Kii. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <XCTest/XCTest.h>
#import "jansson.h"
#import "kii_cloud.h"

@interface ThingSDKLargeTests : XCTestCase

@end

@implementation ThingSDKLargeTests

static const char* APPID = "84fff36e";
static const char* APPKEY = "e45fcc2d31d6aca675af639bc5f04a26";
static const char* BASEURL = "https://api-development-jp.internal.kii.com/api";

- (void)setUp {
    [super setUp];
}

- (void)tearDown {
    [super tearDown];
}

-(void) testLargeJSONOnRegister {
    // Prepare large json
    json_error_t jErr;
    NSString* path = [[NSBundle bundleForClass:[self class]]
                      pathForResource:@"test" ofType:@"json"];
    const char* cPath = [path cStringUsingEncoding:NSUTF8StringEncoding];
    json_t* testJson = json_load_file(cPath, 0, &jErr);
    XCTAssertTrue(testJson != NULL);

    // Register thing
    kii_app_t app = kii_init_app(APPID, APPKEY, BASEURL);
    kii_thing_t thing = NULL;
    kii_char_t* token = NULL;
    
    const kii_char_t* uuidString = [[[[NSUUID alloc]init]UUIDString]
                                    cStringUsingEncoding:NSUTF8StringEncoding];
    kii_error_code_t ret = kii_register_thing(app,
                                              uuidString,
                                              "1234",
                                              NULL,
                                              testJson,
                                              &thing,
                                              &token);
    XCTAssertTrue(ret == KIIE_OK);
    XCTAssertTrue(thing != NULL);
    XCTAssertTrue(token != NULL);
    XCTAssertTrue(strlen(token) > 0);

    json_decref(testJson);
    kii_dispose_app(app);
    kii_dispose_thing(thing);
    kii_dispose_kii_char(token);
}
@end
