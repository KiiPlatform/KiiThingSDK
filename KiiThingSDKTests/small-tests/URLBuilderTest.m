//
//  URLBuilder.m
//  KiiThingSDK
//
//  Copyright (c) 2014 Kii. All rights reserved.
//

#import <XCTest/XCTest.h>

#import "kii_prv_utils.h"

#import <string.h>

@interface URLBuilderTest : XCTestCase

@end

@implementation URLBuilderTest

- (void)setUp {
    [super setUp];
}

- (void)tearDown {
    [super tearDown];
}

- (void)testBuildUrl
{
    char *url = prv_build_url("http://hoge.com", "fuga", NULL);
    XCTAssertTrue(strcmp("http://hoge.com/fuga", url) == 0 ? YES : NO,
                 @"url unmatched: http://hoge.com/fuga , %s", url);
    free(url);
}

@end
