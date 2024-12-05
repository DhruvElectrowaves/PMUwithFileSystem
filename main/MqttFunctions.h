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
//extern void prepare_mqtt_send_config_data(char *unique_charger_id, char *send_config_data_topic , char *send_config_data_payload)

#endif /* MAIN_MQTTFUNCTIONS_H_ */
