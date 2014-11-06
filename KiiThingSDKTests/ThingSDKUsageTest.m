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
    kii_global_cleanup();
}

- (void)testRegistrationAndCreateObject {
    kii_app_t app = kii_init_app("your-appid", "your-appkey", KII_SITE_JP);
    
    // Register the thing.
    json_t *uData = json_object();
    json_object_set_new(uData, "_firmwareVersion", json_string("1.0.0"));
    char* accessToken = NULL;
    kii_thing_t thing = NULL;
    kii_error_code_t r = kii_register_thing(app,
                                            "thing-vender-id",
                                            "thing-password",
                                            "THERMOMETER",
                                            uData,
                                            &thing,
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
    kii_bucket_t bucket = kii_init_thing_bucket("thing-vender-id",
                                                "Temperatures");
    json_t *objData = json_object();
    json_object_set_new(objData, "temperature", json_integer(25));

    // Create new object in the bucket.
    r = kii_create_new_object(app, accessToken, bucket, objData, NULL, NULL);
    
    if (r != KIIE_OK) {// Error handling.
        kii_error_t* e = kii_get_last_error(app);
        NSLog(@"http status code: %d", e->status_code);
        NSLog(@"error code: %s", e->error_code);
    }
    
    // Release resources.
    kii_dispose_app(app);
    kii_dispose_bucket(bucket);
    kii_dispose_kii_char(accessToken);
    json_decref(uData);
    json_decref(objData);
}

- (char*) getAccessToken {
    // NOTE: Assume this returns stored access token obtained by
    // kii_register_thing. Won't show implementation details here as it would
    // depends on environment.
    return NULL;
}

- (void)testSubscription {
    kii_app_t app = kii_init_app("your-appid", "your-appkey", KII_SITE_JP);
    kii_bucket_t bucket = kii_init_thing_bucket("thing-vender-id",
                                         "Temperatures");

    const char* accessToken = [self getAccessToken];

    kii_error_code_t r = kii_subscribe_bucket(app, accessToken, bucket);
    if (r != KIIE_OK) {
        kii_error_t* e = kii_get_last_error(app);
        NSLog(@"http status code: %d", e->status_code);
        NSLog(@"error code: %s", e->error_code);
        kii_dispose_app(app);
        kii_dispose_bucket(bucket);
        return;
    }
    
    kii_topic_t topic = kii_init_thing_topic("thing-vender-id",
                                             "Control-Messages");
    r = kii_subscribe_topic(app, accessToken, topic);
    if (r != KIIE_OK) {
        kii_error_t* e = kii_get_last_error(app);
        NSLog(@"http status code: %d", e->status_code);
        NSLog(@"error code: %s", e->error_code);
    }
    kii_dispose_app(app);
    kii_dispose_bucket(bucket);
    kii_dispose_topic(topic);
}

- (void)testGetMQTTEndpoint {
    kii_app_t app = kii_init_app("your-appid", "your-appkey", KII_SITE_JP);

    // installation.
    const char* accessToken = [self getAccessToken];
    char* installationId = NULL;

    kii_error_code_t r = kii_install_thing_push(app,
                                                accessToken,
                                                KII_TRUE,
                                                &installationId);
    if (r != KIIE_OK) {
        kii_error_t* e = kii_get_last_error(app);
        NSLog(@"http status code: %d", e->status_code);
        NSLog(@"error code: %s", e->error_code);
        kii_dispose_app(app);
        return;
    }

    // Get MQTT endpoint information.
    kii_mqtt_endpoint_t* endpoint = NULL;
    kii_uint_t retryAfter = 0;
    int retry = 0;
    do {
        sleep(retryAfter);
        r = kii_get_mqtt_endpoint(app,
                                  accessToken,
                                  installationId,
                                  &endpoint,
                                  &retryAfter);
        ++retry;
    } while (r != KIIE_OK && retryAfter > 0 && retry < 3);

    if (endpoint == NULL) {
        kii_error_t* e = kii_get_last_error(app);
        NSLog(@"http status code: %d", e->status_code);
        NSLog(@"error code: %s", e->error_code);
        kii_dispose_app(app);
        kii_dispose_kii_char(installationId);
        return;
    }

    // Just dump endpoint here.
    NSLog(@"username: %s", endpoint->username);
    NSLog(@"password: %s", endpoint->password);
    NSLog(@"topic: %s", endpoint->topic);
    NSLog(@"host: %s", endpoint->host);
    NSLog(@"port tcp: %d", endpoint->port_tcp);
    NSLog(@"port ssl: %d", endpoint->port_ssl);
    NSLog(@"ttl: %ld", endpoint->ttl);

    // Connect to MQTT with some library. (paho, etc.)

    // Rerease resources.
    kii_dispose_kii_char(installationId);
    kii_dispose_mqtt_endpoint(endpoint);
    kii_dispose_app(app);
}

@end
