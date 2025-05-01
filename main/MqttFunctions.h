/*
 * MqttFunctions.h
 *
 *  Created on: 04-Dec-2024
 *      Author: Dhruv Joshi & Niharika Singh
 */

#ifndef MAIN_MQTTFUNCTIONS_H_
#define MAIN_MQTTFUNCTIONS_H_

extern void prepare_mqtt_get_Charger_Id_data(char *unique_serial_number, char *get_Charger_Id_topic, char *get_Charger_Id_payload);
extern void prepare_mqtt_Status_data(char *unique_charger_id, char *status_topic, char *status_payload);
extern void prepare_mqtt_charging_session_data(char *unique_charger_id, char *charging_session_topic, char *charging_session_payload);
extern cJSON *create_PCM_and_ComponentStatus_payloadJSON(int startIndex, int endIndex, uint8_t action);
extern cJSON* ev_evseComm_jsonObject(int connectorIndex);
extern void create_componentStatus_response(const char* messageId, int global_component_value, int id);
extern void create_chargerPeriodicData_message();
extern void create_PCM_Periodic_message(uint8_t action);
extern void prepare_mqtt_send_config_data();
extern void create_chargingSession_message();
extern void create_upTime_charger();
extern void increment_time(timeDuration_t *timeStatus, uint8_t seconds_to_add);
extern void convert_seconds_to_time(uint32_t total_seconds, timeDuration_t *timeStatus);
extern double roundToDecimals(double value, int decimals);
extern void create_faultLog_message();
extern void check_memory_status();
extern void ntp_init(void);
extern void read_rtc_time();
extern bool isTimeSet;
extern ChargerPeriodicStatus_t chargerPeriodicStatusMQTT;
extern chargerConfig_t chargerConfigIPC;
extern char faultLogFilePath[50];


#endif /* MAIN_MQTTFUNCTIONS_H_ */
