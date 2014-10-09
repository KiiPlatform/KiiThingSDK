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
    // global init should be called once.
    kii_error_code_t r = kii_global_init();
    if (r != KIIE_OK) {
        abort();
    }
}

- (void)tearDown {
    [super tearDown];
    // global cleanup should be called onece for global init.
    kii_global_clenaup();
}

- (void)testExample {
    kii_app_t app = kii_init_app("your-appid", "your-appkey", KII_SITE_JP);
    
    // Register the thing.
    json_t *uData = json_object();
    json_object_set_new(uData, "_thingType", json_string("THERMOMETER"));
    json_object_set_new(uData, "_firmwareVersion", json_string("1.0.0"));
    char* accessToken = NULL;
    kii_error_code_t r = kii_register_thing(app,
                                            "thing-vender-id",
                                            "thing-password",
                                            uData,
                                            &accessToken);

    if (r != KIIE_OK) {// Error handling.
        kii_error_t* e = kii_get_last_error(app);
        NSLog(@"http status code: %d", e->status_code);
        NSLog(@"error code: %s", e->error_code);
        kii_dispose_app(app);
        json_decref(uData);
        return;
    }
    
    // Create thing scope object.
    // Instantiate bucket.
    kii_bucket_t bukcet = kii_init_thing_bucket("thing-vender-id",
                                                "Tempertures");
    json_t *objData = json_object();
    json_object_set_new(objData, "temperture", json_integer(25));

    // Create new object in the bucket.
    r = kii_create_new_object(app, bukcet, objData, accessToken, NULL, NULL);
    
    if (r != KIIE_OK) {// Error handling.
        kii_error_t* e = kii_get_last_error(app);
        NSLog(@"http status code: %d", e->status_code);
        NSLog(@"error code: %s", e->error_code);
    }
    
    // Release resources.
    kii_dispose_app(app);
    kii_dispose_bucket(bukcet);
    kii_dispose_kii_char(accessToken);
    json_decref(uData);
    json_decref(objData);
}

@end
