/*
 * alacloud.h
 *
 *  Created on: 2016Äê5ÔÂ27ÈÕ
 *      Author: Administrator
 */
#include "esp_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#ifndef APP_USER_ALACLOUD_H_
#define APP_USER_ALACLOUD_H_

void ala_init(char* server,uint16 port,char* deviceid,char* devicetype,uint8 version,char *ip,char *wifiid,char *wifipwd,char *phoneuuid);
typedef bool (*ala_callback_setter)(char* value);
typedef char* (*ala_callback_getter)();

void ala_attach_attribute(char* name,ala_callback_setter,ala_callback_getter);
int ala_parse_receive(uint8 local,char* deviceid,char type,int size,char *value);
short ala_connect_platform(char* ret);
uint32 ala_get_delay(char* value);
void ala_debug();
void ala_free();
void ala_pwd(char *pwd);

void ala_timer();

#endif /* APP_USER_ALACLOUD_H_ */
