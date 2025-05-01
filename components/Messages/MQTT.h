#ifndef MQTT_H
#define MQTT_H

#include <cJSON.h>
#include <stdint.h>

extern uint8_t process_ComponentStatus(cJSON *payload, const char *messageId);
extern void process_MQTT_msg(void *pvParameters);
extern uint8_t is_valid_utc_timestamp(const char *timestamp);
extern uint8_t process_configurationData_payload(cJSON *payload, const char* uuid);
extern uint8_t process_periodicData_payload(cJSON *payload);
extern uint8_t process_faultLog_payload(cJSON *payload);

#endif // MQTT_H