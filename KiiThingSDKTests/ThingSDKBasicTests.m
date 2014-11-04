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


static const char* APPID = "84fff36e";
static const char* APPKEY = "e45fcc2d31d6aca675af639bc5f04a26";
static const char* BASEURL = "https://api-development-jp.internal.kii.com/api";

// Pre registered thing.
static const char* ACCESS_TOKEN = "t6cV3HB65osoG9i0Yndkphk75F7XdswTt8KvL874-wY";
static const char* REGISTERED_THING_TID = "th.53ae324be5a0-f808-4e11-d106-0241b0da";
//static const char* REGISTERED_THING_VID = "96AAF4E6-0E30-472A-A3EC-902C914CBAB7";
static const char* REGISTERED_THING_TOPIC = "myTopic";

- (void)testRegisterThing {
    kii_app_t app = kii_init_app(APPID,
                                 APPKEY,
                                 BASEURL);
    NSUUID* id = [[NSUUID alloc]init];
    const char* thing_id = [id.UUIDString
                             cStringUsingEncoding:NSUTF8StringEncoding];
    
    char* accessToken = NULL;
    kii_thing_t myThing = NULL;
    char* kiiThingId = NULL;
    kii_error_code_t ret = kii_register_thing(app,
                                              thing_id,
                                              "THERMOMETER",
                                              "1234", NULL,
                                              &myThing,
                                              &accessToken);
    /* TODO examin myThing */
    XCTAssertEqual(ret, KIIE_OK, @"register failed");
    XCTAssertTrue(strlen(accessToken) > 0, @"access token invalid");
    kiiThingId = kii_thing_serialize(myThing);
    XCTAssertTrue(strlen(kiiThingId) > 0, @"thing id invalid");
    if (ret != KIIE_OK) {
        kii_error_t* err = kii_get_last_error(app);
        NSLog(@"code: %s", err->error_code);
        NSLog(@"resp code: %d", err->status_code);
    }
    kii_dispose_app(app);
    kii_dispose_kii_char(accessToken);
    kii_dispose_kii_char(kiiThingId);
    kii_dispose_thing(myThing);
}

-(void) testInstallPush {
    kii_app_t app = kii_init_app(APPID,
                                 APPKEY,
                                 BASEURL);
    char* installId = NULL;
    kii_error_code_t ret =
        kii_install_thing_push(app, ACCESS_TOKEN, true, &installId);
    XCTAssertEqual(ret, KIIE_OK, @"install failed");
    XCTAssertTrue(strlen(installId) > 0, @"installId invalid");
    if (ret != KIIE_OK) {
        kii_error_t* err = kii_get_last_error(app);
        NSLog(@"code: %s", err->error_code);
        NSLog(@"resp code: %d", err->status_code);
    }
    
    kii_mqtt_endpoint_t* endpoint = NULL;
    kii_uint_t retryAfter = 0;
    
    int retryCount = 0;
    do {
        NSLog(@"Retry after: %d ....", retryAfter);
        [NSThread sleepForTimeInterval:retryAfter];
        ret = kii_get_mqtt_endpoint(app,
                                    ACCESS_TOKEN,
                                    installId,
                                    &endpoint,
                                    &retryAfter);
        ++retryCount;
    } while (ret != KIIE_OK || retryCount < 3);
    if (ret != KIIE_OK) {
        kii_error_t* err = kii_get_last_error(app);
        NSLog(@"code: %s", err->error_code);
        NSLog(@"resp code: %d", err->status_code);
    }

    XCTAssertEqual(ret, KIIE_OK, @"get endpoint failed");
    XCTAssert(strlen(endpoint->username) > 0);
    XCTAssert(strlen(endpoint->password) > 0);
    XCTAssert(strlen(endpoint->host) > 0);
    XCTAssert(strlen(endpoint->topic) > 0);
    XCTAssert(endpoint->ttl > 0);

    kii_dispose_kii_char(installId);
    kii_dispose_mqtt_endpoint(endpoint);
    kii_dispose_app(app);
}

-(void) testSubscribeBucket {
    kii_app_t app = kii_init_app(APPID, APPKEY, BASEURL);
    kii_thing_t myThing = kii_thing_deserialize(REGISTERED_THING_TID);
    
    NSUUID* id = [[NSUUID alloc]init];
    const char* bucketName = [id.UUIDString
                              cStringUsingEncoding:NSUTF8StringEncoding];
    kii_bucket_t bucket = kii_init_thing_bucket(myThing, bucketName);

    kii_error_code_t ret = kii_subscribe_bucket(app, ACCESS_TOKEN, bucket);
    if (ret != KIIE_OK) {
        kii_error_t* err = kii_get_last_error(app);
        NSLog(@"code: %s", err->error_code);
        NSLog(@"resp code: %d", err->status_code);
    }
    XCTAssertEqual(ret, KIIE_OK, @"subscribe bucket failed");
    kii_dispose_bucket(bucket);
    kii_dispose_app(app);
}

-(void) testSubscribeTopic {
    kii_app_t app = kii_init_app(APPID, APPKEY, BASEURL);
    kii_thing_t myThing = kii_thing_deserialize(REGISTERED_THING_TID);
    kii_topic_t topic = kii_init_thing_topic(myThing, REGISTERED_THING_TOPIC);

    kii_error_code_t ret = kii_subscribe_topic(app, ACCESS_TOKEN, topic);
    if (ret != KIIE_OK) {
        kii_error_t* err = kii_get_last_error(app);
        NSLog(@"code: %s", err->error_code);
        NSLog(@"resp code: %d", err->status_code);
    }
    XCTAssertEqual(ret, KIIE_OK, @"subscribe topic failed");
    kii_dispose_topic(topic);
    kii_dispose_app(app);
}

-(void) testUnsubscribeTopic {
    kii_app_t app = kii_init_app(APPID, APPKEY, BASEURL);
    kii_thing_t myThing = kii_thing_deserialize(REGISTERED_THING_TID);
    kii_topic_t topic = kii_init_thing_topic(myThing, REGISTERED_THING_TOPIC);

    kii_error_code_t ret = kii_unsubscribe_topic(app, ACCESS_TOKEN, topic);
    if (ret != KIIE_OK) {
        kii_error_t* err = kii_get_last_error(app);
        NSLog(@"code: %s", err->error_code);
        NSLog(@"resp code: %d", err->status_code);
    }
    XCTAssertEqual(ret, KIIE_OK, @"subscribe topic failed");
    kii_dispose_topic(topic);
    kii_dispose_app(app);
}

-(void) testCreateNewObject {
    kii_app_t app = kii_init_app(APPID, APPKEY, BASEURL);
    kii_thing_t thing = kii_thing_deserialize(REGISTERED_THING_TID);
    kii_bucket_t bucket = NULL;
    json_t* contents = json_object();
    kii_char_t* out_object_id = NULL;
    kii_char_t* out_etag = NULL;
    kii_error_code_t ret =  KIIE_FAIL;

    bucket = kii_init_thing_bucket(thing, "myBucket");
    ret = kii_create_new_object(app, ACCESS_TOKEN, bucket,
            contents, &out_object_id, &out_etag);
    if (ret != KIIE_OK) {
        kii_error_t* err = kii_get_last_error(app);
        NSLog(@"code: %s", err->error_code);
        NSLog(@"resp code: %d", err->status_code);
    }
    XCTAssertEqual(ret, KIIE_OK, @"create new object failed.");
    if (out_object_id != NULL) {
        NSLog(@"objectID: %s", out_object_id);
    } else {
        XCTFail("out_object_id is NULL.");
    }
    if (out_etag != NULL) {
        NSLog(@"ETag: %s", out_etag);
    } else {
        XCTFail("out_etag is NULL.");
    }

ON_EXIT:
    kii_dispose_kii_char(out_etag);
    kii_dispose_kii_char(out_object_id);
    kii_json_decref(contents);
    kii_dispose_bucket(bucket);
    kii_dispose_thing(thing);
    kii_dispose_app(app);
}

- (void)testGetObject {
    kii_app_t app = kii_init_app(APPID, APPKEY, BASEURL);
    kii_thing_t thing = kii_thing_deserialize(REGISTERED_THING_TID);
    kii_bucket_t bucket = NULL;
    json_t* contents = json_object();
    kii_char_t* out_object_id = NULL;
    kii_char_t* out_etag = NULL;
    kii_json_t* out_contents = NULL;
    kii_error_code_t ret =  KIIE_FAIL;

    bucket = kii_init_thing_bucket(thing, "myBucket");
    ret = kii_create_new_object(app, ACCESS_TOKEN, bucket,
            contents, &out_object_id, &out_etag);
    if (ret != KIIE_OK) {
        kii_error_t* err = kii_get_last_error(app);
        NSLog(@"code: %s", err->error_code);
        NSLog(@"resp code: %d", err->status_code);
    }
    XCTAssertEqual(ret, KIIE_OK, @"create new object failed.");
    if (out_object_id != NULL) {
        NSLog(@"objectID: %s", out_object_id);
    } else {
        XCTFail(@"out_object_id is NULL.");
    }
    if (out_etag != NULL) {
        NSLog(@"ETag: %s", out_etag);
    } else {
        XCTFail(@"out_etag is NULL.");
    }

    ret = kii_get_object(app, ACCESS_TOKEN, bucket, out_object_id,
            &out_contents);
    if (ret != KIIE_OK) {
        kii_error_t* err = kii_get_last_error(app);
        NSLog(@"code: %s", err->error_code);
        NSLog(@"resp code: %d", err->status_code);
    }
    XCTAssertEqual(ret, KIIE_OK, @"get object failed.");
    const char* object_id =
        json_string_value(json_object_get(out_contents, "_id"));
    XCTAssertTrue(strcmp(out_object_id, object_id) == 0 ? YES : NO,
            @"object id unmatached: %s %s", out_object_id, object_id);

ON_EXIT:
    kii_dispose_app(app);
    kii_dispose_thing(thing);
    kii_dispose_bucket(bucket);
    kii_json_decref(contents);
    kii_dispose_kii_char(out_object_id);
    kii_dispose_kii_char(out_etag);
    kii_json_decref(out_contents);
}

-(void) testDeleteObject {
    kii_app_t app = kii_init_app(APPID, APPKEY, BASEURL);
    kii_char_t* accessToken = NULL;
    kii_thing_t thing = NULL;
    kii_bucket_t bucket = NULL;
    json_t* contents = json_object();
    kii_char_t* out_object_id = NULL;
    kii_char_t* out_etag = NULL;
    kii_json_t* out_contents = NULL;
    kii_error_code_t ret =  KIIE_FAIL;

    {
        NSUUID* id = [[NSUUID alloc] init];
        const char* thing_id = [id.UUIDString
                cStringUsingEncoding:NSUTF8StringEncoding];
        ret = kii_register_thing(app, thing_id, "THWEMOMETER", "1234",
                NULL, &thing, &accessToken);
        if (ret != KIIE_OK) {
            kii_error_t* err = kii_get_last_error(app);
            NSLog(@"code: %s", err->error_code);
            NSLog(@"resp code: %d", err->status_code);
            goto ON_EXIT;
        }
    }
    bucket = kii_init_thing_bucket(thing, "myBucket");
    ret = kii_create_new_object(app, accessToken, bucket,
            contents, &out_object_id, &out_etag);
    if (ret != KIIE_OK) {
        kii_error_t* err = kii_get_last_error(app);
        NSLog(@"code: %s", err->error_code);
        NSLog(@"resp code: %d", err->status_code);
    }
    XCTAssertEqual(ret, KIIE_OK, @"create new object failed.");
    if (out_object_id != NULL) {
        NSLog(@"objectID: %s", out_object_id);
    } else {
        XCTFail(@"out_object_id is NULL.");
    }
    if (out_etag != NULL) {
        NSLog(@"ETag: %s", out_etag);
    } else {
        XCTFail(@"out_etag is NULL.");
    }

    ret = kii_get_object(app, accessToken, bucket, out_object_id,
            &out_contents);
    if (ret != KIIE_OK) {
        kii_error_t* err = kii_get_last_error(app);
        NSLog(@"code: %s", err->error_code);
        NSLog(@"resp code: %d", err->status_code);
    }
    XCTAssertEqual(ret, KIIE_OK, @"get object failed.");
    const char* object_id =
        json_string_value(json_object_get(out_contents, "_id"));
    XCTAssertTrue(strcmp(out_object_id, object_id) == 0 ? YES : NO,
            @"object id unmatached: %s %s", out_object_id, object_id);

    ret = kii_delete_object(app, accessToken, bucket, out_object_id);
    if (ret != KII_OK) {
        kii_error_t* err = kii_get_last_error(app);
        NSLog(@"code: %s", err->error_code);
        NSLog(@"resp code: %d", err->status_code);
    }
    XCTAssertEqual(ret, KIIE_OK, @"delete object failed.");

    ret = kii_get_object(app, accessToken, bucket, out_object_id,
            &out_contents);
    XCTAssertEqual(ret, KIIE_FAIL, @"object must be removed..");
    {
        kii_error_t *err = kii_get_last_error(app);
        XCTAssertEqual(err->status_code, 404, @"status code unmatched.");
    }

ON_EXIT:
    kii_dispose_app(app);
    kii_dispose_kii_char(accessToken);
    kii_dispose_thing(thing);
    kii_dispose_bucket(bucket);
    kii_json_decref(contents);
    kii_dispose_kii_char(out_object_id);
    kii_dispose_kii_char(out_etag);
    kii_json_decref(out_contents);
}

@end
