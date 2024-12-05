/*
 * MqttFunctions.c
 *
 *  Created on: 04-Dec-2024
 *      Author: Dhruv Joshi & Niharika Singh
 */

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <math.h>
#include "driver/gpio.h"
#include "driver/twai.h"
#include "esp_log.h"
#include "PMUFinalmain.h"
#include "CAN.h"
#include "mqtt_client.h"
#include "cJSON.h"

void prepare_mqtt_get_Charger_Id_data(char *unique_serial_number, char *get_Charger_Id_topic, char *get_Charger_Id_payload){
	// Construct the MQTT topic
    snprintf(get_Charger_Id_topic, 50, "getChargerId/%s", unique_serial_number);
	
   // Construct the JSON payload using cJSON library
    cJSON *get_chargerId_root = cJSON_CreateObject();
    if (get_chargerId_root == NULL) {
        return;
    }

	// Convert empty JSON object to string
    strcpy(get_Charger_Id_payload, cJSON_PrintUnformatted(get_chargerId_root)); //converts this empty JSON object to a JSON string which represent an empty message
    cJSON_Delete(get_chargerId_root);
}

void prepare_mqtt_Status_data(char *unique_charger_id, char *status_topic, char *status_payload) {
    // Construct the MQTT topic
    snprintf(status_topic, 50, "chargerStatus/%s", unique_charger_id);

    // Construct the JSON payload using cJSON library
    cJSON *status_root = cJSON_CreateObject();
    if (status_root == NULL) {
        return;
    }
    
    // 1) Create Array for connectorStatus 
    cJSON *connectorStatusArr = cJSON_CreateArray();
     for (int i = 0; i < sizeof(connectorStatus) / sizeof(connectorStatus[0]); i++) {
        cJSON_AddItemToArray(connectorStatusArr, cJSON_CreateNumber(connectorStatus[i]));
    }
    cJSON_AddItemToObject(status_root, "connectorStatus", connectorStatusArr);
    
    
    //2)Add the WebSocket Status which needs to be sent 
    cJSON_AddNumberToObject(status_root, "webSocketStatus", webSocketStatus );
    
    //3) Create Array for Fault Status
    cJSON *faultStatusArr = cJSON_CreateArray();
    for (int i = 0; i < sizeof(faultStatus) / sizeof(faultStatus[0]); i++) {
        cJSON_AddItemToArray(faultStatusArr, cJSON_CreateNumber(faultStatus[i]));
    }
    cJSON_AddItemToObject(status_root, "faultStatus", faultStatusArr);
    
    // Convert JSON object to string
    strcpy(status_payload, cJSON_Print(status_root));
    cJSON_Delete(status_root);
  }

void prepare_mqtt_charging_session_data(char *unique_charger_id, char *charging_session_topic, char *charging_session_payload){
	// Construct the MQTT topic
    snprintf(charging_session_topic, 50, "chargingSession/%s", unique_charger_id); 
    
    // Construct the JSON payload using cJSON library
    cJSON *charging_session_root = cJSON_CreateObject();
    if (charging_session_root == NULL) {
        return;
    }
    
    cJSON_AddNumberToObject(charging_session_root, "connector", connector);
    // Add Event to the JSON Object 
	cJSON_AddNumberToObject(charging_session_root, "Event", event);
	
	if(event == 3){
	// Add finishReason to the JSON Object 
	cJSON_AddNumberToObject(charging_session_root, "finishReason", finishReason);
	}
	
	// Convert JSON object to string
    strcpy(charging_session_payload, cJSON_Print(charging_session_root));
    cJSON_Delete(charging_session_root);
}

/*
void prepare_mqtt_send_config_data(char *unique_charger_id, char *send_config_data_topic , char *send_config_data_payload){
	// Construct the MQTT topic
    snprintf(send_config_data_topic, 50, "sendConfigData/%s", unique_charger_id);
    
    // Construct the JSON payload using cJSON library
    cJSON *status_root = cJSON_CreateObject();
    if (status_root == NULL) {
        return;
    }
    
    //1)Add Brand name to Send 
    cJSON_AddNumberToObject(status_root, "Brand", Brand);
    
    //2)Add AutoCharge Status to Send 
    cJSON_AddNumberToObject(status_root, "AutoCharge", AutoCharge);
    
    // Create a nested JSON object
    cJSON *limit_info = cJSON_CreateObject();
    if (limit_info == NULL) {
        cJSON_Delete(status_root);
        printf("Failed to create nested JSON object\n");
        return;
    }
    
    cJSON_AddNumberToObject(limit_info, "chargePower", chargePower);
    cJSON_AddNumberToObject(limit_info, "gunAVoltage", gunAVoltage);
    cJSON_AddNumberToObject(limit_info, "gunACurrent", gunACurrent);
    cJSON_AddNumberToObject(limit_info, "gunAPower", gunAPower);
    
    cJSON_AddNumberToObject(limit_info, "gunBVoltage", gunBVoltage);
    cJSON_AddNumberToObject(limit_info, "gunBCurrent", gunBCurrent);
    cJSON_AddNumberToObject(limit_info, "gunBPower", gunBPower);
    
    cJSON_AddItemToObject(status_root, "limitInfo", limit_info);
    
    cJSON *version_info = cJSON_CreateObject();
    if (version_info == NULL) {
        cJSON_Delete(status_root);
        printf("Failed to create nested JSON object\n");
        return;
    }
    
    cJSON_AddStringToObject(version_info, "HMIVersion", hmiVersion);
    cJSON_AddStringToObject(version_info, "LCUVersion", LCU);
    cJSON_AddStringToObject(version_info, "MCUVersion", MCU);
    
     // 1) Create Array for evevseController 
    cJSON *evevseControllerArr = cJSON_CreateArray();
     for (int i = 0; i < sizeof(evevseCommContro) / sizeof(evevseCommContro[0]); i++) {
        cJSON_AddItemToArray(evevseControllerArr, cJSON_CreateString(evevseCommContro[i]));
    }
    cJSON_AddItemToObject(version_info, "evevseCommController", evevseControllerArr);
    
    // 2) Create Array for PCM  
    cJSON *PCMArr = cJSON_CreateArray();
     for (int i = 0; i < sizeof(PCM) / sizeof(PCM[0]); i++) {
        cJSON_AddItemToArray(PCMArr, cJSON_CreateString(PCM[i]));
    }
    cJSON_AddItemToObject(version_info, "PCM", PCMArr);
    
    cJSON_AddStringToObject(version_info, "PMUVersion", PMU);
    
    cJSON_AddItemToObject(status_root, "VersionInfo", version_info);
     
    // Convert JSON object to string
    strcpy(send_config_data_payload, cJSON_Print(status_root));
    cJSON_Delete(status_root);
    
}
*/

