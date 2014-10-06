//
//  KiiThingSDKTests.m
//  KiiThingSDKTests
//
//  Created by 熊野 聡 on 2014/10/06.
//  Copyright (c) 2014年 Kii. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <XCTest/XCTest.h>
#import "jansson.h"

@interface KiiThingSDKTests : XCTestCase

@end

@implementation KiiThingSDKTests

- (void)setUp {
    [super setUp];
}

- (void)tearDown {
    [super tearDown];
}

- (void)testExample {
    json_t* obj = json_object();
    json_t* str = json_string("huga");
    json_object_set(obj, "hoge", str);
    char* dump = json_dumps(obj, 0);
    NSLog(@"json: %s", dump);
    free(dump);
    XCTAssert(YES, @"Pass");
}


@end
