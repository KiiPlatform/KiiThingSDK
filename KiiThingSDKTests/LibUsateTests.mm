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
#import "curl.h"

@interface KiiThingSDKTests : XCTestCase

@end

@implementation KiiThingSDKTests

- (void)setUp {
    [super setUp];
}

- (void)tearDown {
    [super tearDown];
}

- (void)testExampleJannson {
    json_t* obj = json_object();
    json_t* str = json_string("huga");
    json_object_set(obj, "hoge", str);
    char* dump = json_dumps(obj, 0);
    NSLog(@"json: %s", dump);
    free(dump);
    XCTAssert(YES, @"Pass");
}

size_t callbackWrite(char *ptr, size_t size, size_t nmemb)
{
    int dataLen = size * nmemb;
    NSLog(@"received: %s", ptr);
    return dataLen;
}

- (void)testExampleCurl {
    CURL* curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, "http://www.google.com");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callbackWrite);
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    curl = NULL;
    NSLog(@"curl res %d", res);
}

@end
