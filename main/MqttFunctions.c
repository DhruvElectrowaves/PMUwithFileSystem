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
#include "MqttFunctions.h"
#include <time.h>
#include <sys/time.h>
#include "esp_sntp.h"
#include "../components/Operation/FormMessage.h"
#include "../components/Operation/ValidateMessage.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <float.h>

ObjectDeleteFlag_t ObjectDeleteFlags = {0}; // All fields initialized to false (0)
PowerModuleDeleteFlag_t powerModuleDeleteFlags = {0};
char *json_array_string; 

void increment_time(timeDuration_t *timeStatus, uint8_t seconds_to_add);
void convertToTimeDuration(uint32_t totalSeconds, timeDuration_t *timeDuration);

double roundToDecimals(double value, int decimals){
    double scale = pow(10,decimals);
    return round(value * scale)/ scale; 
}

//N: Start
void ntp_init(void) {
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "time.google.com"); // Reliable NTP server
    esp_sntp_init();

    setenv("TZ", "UTC", 1); // Set timezone to IST (or "UTC" for UTC time)
    tzset();

    // Wait for synchronization
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 5;

    while (timeinfo.tm_year < (1970 - 1900) && ++retry < retry_count) {
        ESP_LOGI(NTP_TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(5000 / portTICK_PERIOD_MS); // Wait 5 seconds for synchronization
        time(&now);
        gmtime_r(&now, &timeinfo); // Use gmtime_r for UTC or localtime_r for local time
    }

    if (timeinfo.tm_year < (1970 - 1900)) {
        ESP_LOGE(NTP_TAG, "Failed to synchronize time");
        return;
    }

    ESP_LOGI(NTP_TAG, "Time synchronized: %s", asctime(&timeinfo));

    // Set RTC time after NTP sync
    struct timeval tv;
    tv.tv_sec = mktime(&timeinfo); // Convert struct tm to seconds since epoch
    tv.tv_usec = 0;

    if (settimeofday(&tv, NULL) != 0) {
        ESP_LOGE(NTP_TAG, "Failed to set RTC time");
    } else {
        ESP_LOGI(NTP_TAG, "RTC time successfully set");
        isTimeSet = true;
    }
}

void read_rtc_time(void) {
    struct timeval tv;
    struct tm timeinfo;

    // Get the current time from the RTC
    if (gettimeofday(&tv, NULL) == 0) {
        // Convert the seconds since epoch to a readable struct tm
        localtime_r(&tv.tv_sec, &timeinfo);  // Use localtime_r for local time

        ESP_LOGI(NTP_TAG, "Current RTC time: %s", asctime(&timeinfo));
    } else {
        ESP_LOGE(NTP_TAG, "Failed to read RTC time");
    }
}

cJSON *create_PCM_and_ComponentStatus_payloadJSON(int startIndex, int endIndex, uint8_t action) {
    cJSON *power_ModuleRoot = cJSON_CreateObject();
     if (power_ModuleRoot == NULL) {
        printf("Failed to create power_ModuleRoot\n");
        return NULL;
    }

    // Get current UTC timestamp in ISO 8601 format
    time_t now = time(NULL);
    struct tm *utc_time = gmtime(&now); // Convert to UTC
    char utc_timestamp[25];            // Buffer for ISO 8601 timestamp
    strftime(utc_timestamp, sizeof(utc_timestamp), "%Y-%m-%dT%H:%M:%SZ", utc_time);

    //Add TimeStamp in the JSON
    cJSON_AddStringToObject(power_ModuleRoot, "timestamp", utc_timestamp);

    if(action == componentStatus){
        cJSON_AddNumberToObject(power_ModuleRoot, "component", global_component_value);
        cJSON_AddNumberToObject(power_ModuleRoot, "id", id_requested);
    }

    //Creating the PowerModule Array
    cJSON *powerModuleArr = cJSON_CreateArray();
    if (powerModuleArr == NULL) {
        cJSON_Delete(power_ModuleRoot);
        printf("Failed to create connectorsInfo array\n");
        return NULL;
    }
    
    for (int i= (startIndex); i < (endIndex); i++) {
        cJSON *powerModule = cJSON_CreateObject();
        if (powerModule == NULL) {
            cJSON_Delete(powerModuleArr);
            cJSON_Delete(power_ModuleRoot);
            printf("Failed to create powerModule object\n");
            return NULL;
        }            
        //json data for pcmPeriodicData and including component 0 of sendComponentStatus topic
        //Manually Creating the Objects for the No of PCMs
        cJSON_AddNumberToObject(powerModule, "id", i + 1);
        if(powerModulesMQTT[i].CANFailure == 1){
            cJSON_AddNumberToObject(powerModule, "commStatus", 0);
            // printf("Power Module[%d] CANFailure: %d\n", i + 1, powerModulesMQTT[i].CANFailure);
        }
        else{
            cJSON_AddNumberToObject(powerModule, "commStatus", 1);
            // printf("Power Module[%d] CANFailure: %d\n", i + 1, powerModulesMQTT[i].CANFailure);
            // Add acVoltages array
            if(powerModulesMQTT[i].acVoltages[0] != 0xFF || powerModulesMQTT[i].acVoltages[1] != 0xFF || powerModulesMQTT[i].acVoltages[2] != 0xFF){
                cJSON *acVoltages = cJSON_CreateArray();
                if (acVoltages == NULL) {
                    cJSON_Delete(powerModule);
                    cJSON_Delete(powerModuleArr);
                    cJSON_Delete(power_ModuleRoot); // Free previously allocated memory
                    printf("Error: Memory allocation failed for acVoltages\n");
                    return NULL;
                }
                
                cJSON_AddItemToArray(acVoltages, cJSON_CreateNumber(roundToDecimals(powerModulesMQTT[i].acVoltages[0],1)));
                cJSON_AddItemToArray(acVoltages, cJSON_CreateNumber(roundToDecimals(powerModulesMQTT[i].acVoltages[1],1)));
                cJSON_AddItemToArray(acVoltages, cJSON_CreateNumber(roundToDecimals(powerModulesMQTT[i].acVoltages[2],1)));
                cJSON_AddItemToObject(powerModule, "acVoltages", acVoltages);
            }

            // Add acCurrents array
            if(powerModulesMQTT[i].acCurrents[0] != 0xFF || powerModulesMQTT[i].acCurrents[1] != 0xFF || powerModulesMQTT[i].acCurrents[2] != 0xFF){
                cJSON *acCurrents = cJSON_CreateArray();
                if (acCurrents == NULL) {
                    printf("Error: Memory allocation failed for acCurrents\n");
                    cJSON_Delete(powerModule);
                    cJSON_Delete(powerModuleArr);
                    cJSON_Delete(power_ModuleRoot);
                    return NULL;
                }
                cJSON_AddItemToArray(acCurrents, cJSON_CreateNumber(roundToDecimals(powerModulesMQTT[i].acCurrents[0],2)));
                cJSON_AddItemToArray(acCurrents, cJSON_CreateNumber(roundToDecimals(powerModulesMQTT[i].acCurrents[1],2)));
                cJSON_AddItemToArray(acCurrents, cJSON_CreateNumber(roundToDecimals(powerModulesMQTT[i].acCurrents[2],2)));
                cJSON_AddItemToObject(powerModule, "acCurrents", acCurrents);
            }

            if(powerModulesMQTT[i].dcVoltages.Vdcp != 0xFF || powerModulesMQTT[i].dcVoltages.Vdcn != 0xFF || powerModulesMQTT[i].dcVoltages.Vdc != 0xFF 
            || powerModulesMQTT[i].dcVoltages.Vdc2_1 != 0xFF || powerModulesMQTT[i].dcVoltages.Vdc2_2 != 0xFF || powerModulesMQTT[i].dcVoltages.Vout1 != 0xFF
            || powerModulesMQTT[i].dcVoltages.fanVoltageMaster !=  FLT_MAX || powerModulesMQTT[i].dcVoltages.fanVoltageSlave != FLT_MAX)
            {
                // Add dcVoltages object
                cJSON *dcVoltage = cJSON_CreateObject();
                if (dcVoltage == NULL) {
                    printf("Error: Memory allocation failed for dcVoltage\n");
                    cJSON_Delete(powerModule);
                    cJSON_Delete(powerModuleArr);
                    cJSON_Delete(power_ModuleRoot);
                    return NULL;
                }
                cJSON_AddNumberToObject(dcVoltage, "vDcp", roundToDecimals(powerModulesMQTT[i].dcVoltages.Vdcp,1));
                cJSON_AddNumberToObject(dcVoltage, "vDcn", roundToDecimals(powerModulesMQTT[i].dcVoltages.Vdcn,1));
                cJSON_AddNumberToObject(dcVoltage, "vDc", roundToDecimals(powerModulesMQTT[i].dcVoltages.Vdc,1));
                cJSON_AddNumberToObject(dcVoltage, "vDc2_1", roundToDecimals(powerModulesMQTT[i].dcVoltages.Vdc2_1,1));
                cJSON_AddNumberToObject(dcVoltage, "vDc2_2", roundToDecimals(powerModulesMQTT[i].dcVoltages.Vdc2_2,1));
                cJSON_AddNumberToObject(dcVoltage, "vOut1", roundToDecimals(powerModulesMQTT[i].dcVoltages.Vout1,1));
                cJSON_AddNumberToObject(dcVoltage, "vOut2", roundToDecimals(powerModulesMQTT[i].dcVoltages.Vout2,1));

                if(powerModulesMQTT[i].dcVoltages.fanVoltageMaster != 6553.5)
                    cJSON_AddNumberToObject(dcVoltage, "fanVoltageMaster", roundToDecimals(powerModulesMQTT[i].dcVoltages.fanVoltageMaster,1));

                cJSON_AddNumberToObject(dcVoltage, "fanVoltageSlave", roundToDecimals(powerModulesMQTT[i].dcVoltages.fanVoltageSlave, 1));
                //cJSON_AddNumberToObject(dcVoltage, "fanVoltageSlave", powerModulesMQTT[i].dcVoltages.fanVoltageSlave);
                cJSON_AddItemToObject(powerModule, "dcVoltages", dcVoltage);
            }

            if(powerModulesMQTT[i].dcCurrents.iTrf != 6553.5 || powerModulesMQTT[i].dcCurrents.Iout1 != 0xFF || powerModulesMQTT[i].dcCurrents.Iout2 != 0xFF){
                // Add dcCurrents object
                cJSON *dcCurrent = cJSON_CreateObject();
                if (dcCurrent == NULL) {
                    printf("Error: Memory allocation failed for dcCurrent\n");
                    cJSON_Delete(powerModule);
                    cJSON_Delete(powerModuleArr);
                    cJSON_Delete(power_ModuleRoot);
                    return NULL;
                }
                if(powerModulesMQTT[i].dcCurrents.iTrf != 6553.5)
                    cJSON_AddNumberToObject(dcCurrent, "iTrf",roundToDecimals(powerModulesMQTT[i].dcCurrents.iTrf,2));

                cJSON_AddNumberToObject(dcCurrent, "iOut1", roundToDecimals(powerModulesMQTT[i].dcCurrents.Iout1,2));
                cJSON_AddNumberToObject(dcCurrent, "iOut2", roundToDecimals(powerModulesMQTT[i].dcCurrents.Iout2,2));
                cJSON_AddItemToObject(powerModule, "dcCurrents", dcCurrent);
            }

            if(powerModulesMQTT[i].Temperatures.GSC != 0xFF || powerModulesMQTT[i].Temperatures.HFIInv != 0xFF || powerModulesMQTT[i].Temperatures.HFRectifier != 0xFF 
            ||powerModulesMQTT[i].Temperatures.Buck != 0xFF){
                // Add Temperatures object
                cJSON *temp = cJSON_CreateObject();
                if (temp == NULL) {
                    printf("Error: Memory allocation failed for temp\n");
                    cJSON_Delete(powerModule);
                    cJSON_Delete(powerModuleArr);
                    cJSON_Delete(power_ModuleRoot);
                    return NULL;
                }
                cJSON_AddNumberToObject(temp, "gsc", roundToDecimals(powerModulesMQTT[i].Temperatures.GSC,1));
                cJSON_AddNumberToObject(temp, "hfiInv", roundToDecimals(powerModulesMQTT[i].Temperatures.HFIInv,1));
                cJSON_AddNumberToObject(temp, "hfRectifier", roundToDecimals(powerModulesMQTT[i].Temperatures.HFRectifier,1));
                cJSON_AddNumberToObject(temp, "buck", roundToDecimals(powerModulesMQTT[i].Temperatures.Buck,1));
                cJSON_AddItemToObject(powerModule,"temperatures", temp);
            }

            if(powerModulesMQTT[i].fault != 0xFF || powerModulesMQTT[i].gridAvailability != 0xFF){
                // Add fault and gridAvailable
                cJSON_AddNumberToObject(powerModule, "fault", powerModulesMQTT[i].fault);
                cJSON_AddNumberToObject(powerModule, "gridAvailable", powerModulesMQTT[i].gridAvailability);
            }
        }  
        cJSON_AddItemToArray(powerModuleArr, powerModule); // this can be shifted outside the else
    }
    if(action==componentStatus){
        cJSON *status = cJSON_CreateObject();
        if (status == NULL) {
            printf("Failed to create status\n");
            cJSON_Delete(status);
            cJSON_Delete(power_ModuleRoot);
            return NULL;
        }
        cJSON_AddItemToObject(status,"powerModules", powerModuleArr);
        cJSON_AddItemToObject(power_ModuleRoot,"status", status);
    }
    else{
        cJSON_AddItemToObject(power_ModuleRoot,"powerModules", powerModuleArr);
    }
    // Return the root JSON object
    return power_ModuleRoot;
}

void create_componentStatus_response(const char* messageId, int global_component_value, int id){
    int startIndex = 0;
    int endIndex = 0;

    switch(global_component_value){
        case 1:{
            // Component requested = PCM
            if(id>=0 && id <= chargerConfigMQTT.numPCM){
                if(id == 0){
                    startIndex = 0;
                    endIndex = chargerConfigMQTT.numPCM;
                }
                else{
                    startIndex = id-1;
                    endIndex = id;
                }

                cJSON* componentStatus_Root = create_PCM_and_ComponentStatus_payloadJSON(startIndex, endIndex, componentStatus);
    
                //To Do OCPP format creation or JSON Array creation and File storage here only.
                json_array_string = create_json_call_result_string(messageId, componentStatus_Root);
                
                // cJSON_Delete(componentStatus_Root); // might double delete occur here
            
                // if (json_array_string) {
                //     //publish to broker
                //     if (IS_MQTT_NETWORK_ONLINE){
                //         // esp_mqtt_client_publish(mqtt_client, pubTopic, jsonString, 0, 1, 0);
                //         if (esp_mqtt_client_publish(mqtt_client, pubTopic, json_array_string, 0, 1, 0) != -1) 
                //         {
                //             ESP_LOGI(OCPP_TAG,"json_string_Component_Status:%s", json_array_string);
                //             printf("Message send successfully");
                //         }
                //     }
                //     free(json_array_string);
                //     json_array_string = NULL;
                // }
                //Storing the File in SPIFFS System 
                // save_payload_to_spiffs(FILEPATH, json_array_string);
            }
            else{
                // Invalid component id requested
                error = ID_NOT_SUPPORTED; 
                send_json_call_error(messageId, error);
            }
            break;
        }
        
        case 2:{
            //Component requested = EV EVSE Communication Controller
            ESP_LOGW("DEBUG-COMPONENT", "Code reched to case 2");
            if(id==0 || id <= chargerConfigMQTT.numConnectors){
                // Payload creation: START
                cJSON *payload = cJSON_CreateObject();
                if (payload == NULL) {
                    printf("Failed to create payload (case 2 component status)\n");
                    return;
                }

                // Get current UTC timestamp in ISO 8601 format
                time_t now = time(NULL);
                struct tm *utc_time = gmtime(&now); // Convert to UTC
                char utc_timestamp[25];            // Buffer for ISO 8601 timestamp
                strftime(utc_timestamp, sizeof(utc_timestamp), "%Y-%m-%dT%H:%M:%SZ", utc_time);

                //Add TimeStamp in the JSON
                cJSON_AddStringToObject(payload, "timestamp", utc_timestamp);
                cJSON_AddNumberToObject(payload, "component", global_component_value);
                cJSON_AddNumberToObject(payload, "id", id_requested);

                int connStart = 0;
                int connEnd = 0;
                if(id==0){
                    // Return info for all communication controllers
                    connStart = 0;
                    connEnd = chargerConfigMQTT.numConnectors;
                }
                else{
                    connStart = id-1;
                    connEnd = id;
                }
                
                cJSON* connectorInfoArray = cJSON_CreateArray();
                if(connectorInfoArray == NULL)
                {
                    ESP_LOGE(MQTT_TAG, "Error: Memory allocation failed for connectorInfoArray\n");
                    return;
                }
                int connNumber = 0;
                for(connNumber=connStart; (connNumber < connEnd); connNumber++)
                {
                    cJSON* connectorInfoObject = cJSON_CreateObject();
                    if(connectorInfoObject == NULL)
                    {
                        ESP_LOGE(MQTT_TAG, "Error: Memory allocation failed for connectorInfoObject\n");
                        cJSON_Delete(connectorInfoArray);
                        return;
                    }
                    cJSON_AddNumberToObject(connectorInfoObject, "id", (connNumber + 1));

                    cJSON* commControllerObject = cJSON_CreateObject();
                    if(commControllerObject == NULL)
                    {
                        ESP_LOGE(MQTT_TAG, "Error: Memory allocation failed for commControllerObject\n");
                        cJSON_Delete(connectorInfoObject);
                        return;
                    }
                    cJSON_AddNumberToObject(commControllerObject, "commStatus", chargerPeriodicStatusMQTT.ev_evseCommRecv[connNumber]);
                    if(chargerPeriodicStatusMQTT.ev_evseCommRecv[connNumber] == FINE && chargerPeriodicStatusMQTT.ErrorCode0Recv[connNumber] != 0 && chargerPeriodicStatusMQTT.ErrorCode0Recv[connNumber] != 0xFF)
                    {
                        cJSON_AddNumberToObject(commControllerObject, "errorCode", chargerPeriodicStatusMQTT.ErrorCode0Recv[connNumber]);
                    }
                    cJSON_AddItemToObject(connectorInfoObject, "commController", commControllerObject);
                    cJSON_AddItemToArray(connectorInfoArray, connectorInfoObject);
                }

                cJSON* status = cJSON_CreateObject();
                if(status == NULL)
                {
                    ESP_LOGE(MQTT_TAG, "Error: Memory allocation failed for status\n");
                    return;
                }
                cJSON_AddItemToObject(status, "connectorInfo", connectorInfoArray);
                cJSON_AddItemToObject(payload, "status", status);

                if (payload != NULL)
                    json_array_string = create_json_call_result_string(messageId, payload);

                // Payload creation: END
            }// end if valid id received
            else{
                // Invalid id requested
                error = ID_NOT_SUPPORTED; 
                send_json_call_error(messageId, error);
            }
            break;
        }// end case 2
        
        case 3:{
            //Component requested = OCPP Controller Info
            // Payload creation: START
            if(id==0)
            {
                cJSON *payload = cJSON_CreateObject();
                if (payload == NULL) {
                    printf("Failed to create payload (case 2 component status)\n");
                    return;
                }

                // Get current UTC timestamp in ISO 8601 format
                time_t now = time(NULL);
                struct tm *utc_time = gmtime(&now); // Convert to UTC
                char utc_timestamp[25];            // Buffer for ISO 8601 timestamp
                strftime(utc_timestamp, sizeof(utc_timestamp), "%Y-%m-%dT%H:%M:%SZ", utc_time);

                //Add TimeStamp in the JSON
                cJSON_AddStringToObject(payload, "timestamp", utc_timestamp);
                cJSON_AddNumberToObject(payload, "component", global_component_value);
                cJSON_AddNumberToObject(payload, "id", id_requested);

                cJSON* ocppControllerObject = cJSON_CreateObject();
                if(ocppControllerObject == NULL)
                {
                    ESP_LOGE(MQTT_TAG, "Error: Memory allocation failed for ocppControllerObject\n");
                    return;
                }
                cJSON_AddNumberToObject(ocppControllerObject, "commStatus", chargerPeriodicStatusMQTT.LCUCommStatus);
                if(chargerPeriodicStatusMQTT.LCUCommStatus == FINE && chargerConfigMQTT.configKeyValue == ONLINE)
                {
                    //--------------------Calculation for Uptime for Internet starts from here------------------//
                    //Creating jSON Object of Internet
                    cJSON* internetObject = cJSON_CreateObject();
                    if(internetObject == NULL){
                        ESP_LOGE(MQTT_TAG, "Error: Memory allocation failed for internetObject\n");
                        cJSON_Delete(ocppControllerObject);
                        cJSON_Delete(payload);
                        return ;
                    }
        
                    cJSON *upTime = cJSON_CreateObject();
                    if (upTime == NULL) {
                        cJSON_Delete(internetObject);
                        cJSON_Delete(ocppControllerObject);
                        cJSON_Delete(payload);
                        return;
                    }
        
                    cJSON *downTime = cJSON_CreateObject();
                    if (downTime == NULL){
                        cJSON_Delete(upTime);
                        cJSON_Delete(internetObject);
                        cJSON_Delete(ocppControllerObject);
                        cJSON_Delete(payload);
                        return;
                    }
                    //Calculating UpTime for LCUInternet
                    LCUinternetStatusComponent.totalUpTime = read_nvs_value(NVS_INTERNET_TOTAL_UP_TIME);
                    convert_seconds_to_time(LCUinternetStatusComponent.totalUpTime, &LCUinternetStatusComponent.upTime);
                    increment_time(&LCUinternetStatusComponent.upTime, LCUInternetUp);  // Internet is UP, increment uptime

                    cJSON_AddNumberToObject(internetObject, "connectionStatus",chargerPeriodicStatusMQTT.LCUInternetStatus);
                    
                    cJSON_AddNumberToObject(upTime,"days",LCUinternetStatusComponent.upTime.days);
                    cJSON_AddNumberToObject(upTime,"hours",LCUinternetStatusComponent.upTime.hours);
                    cJSON_AddNumberToObject(upTime,"minutes",LCUinternetStatusComponent.upTime.minutes);
                    cJSON_AddNumberToObject(upTime,"seconds",LCUinternetStatusComponent.upTime.seconds);
                    cJSON_AddItemToObject(internetObject,"upTime",upTime);
                    LCUinternetStatusComponent.totalUpTime = (LCUinternetStatusComponent.upTime.days*86400)+(LCUinternetStatusComponent.upTime.hours*3600)+(LCUinternetStatusComponent.upTime.minutes*60)+(LCUinternetStatusComponent.upTime.seconds);

                    //Calculating downTime for LCUInternet
                    LCUinternetStatusComponent.totalDownTime = read_nvs_value(NVS_INTERNET_TOTAL_DOWN_TIME);
                    convert_seconds_to_time(LCUinternetStatusComponent.totalDownTime, &LCUinternetStatusComponent.downTime);
                    increment_time(&LCUinternetStatusComponent.downTime, LCUInternetDown); // Internet is DOWN, increment downtime

                    cJSON_AddNumberToObject(downTime,"days",LCUinternetStatusComponent.downTime.days);
                    cJSON_AddNumberToObject(downTime,"hours",LCUinternetStatusComponent.downTime.hours);
                    cJSON_AddNumberToObject(downTime,"minutes",LCUinternetStatusComponent.downTime.minutes);
                    cJSON_AddNumberToObject(downTime,"seconds",LCUinternetStatusComponent.downTime.seconds);
                    cJSON_AddItemToObject(internetObject,"downTime",downTime);
                    LCUinternetStatusComponent.totalDownTime = (LCUinternetStatusComponent.downTime.days*86400)+(LCUinternetStatusComponent.downTime.hours*3600)+(LCUinternetStatusComponent.downTime.minutes*60)+(LCUinternetStatusComponent.downTime.seconds);
        
                    // Calculate UpTime Percent for Internet
                    if ((LCUinternetStatusComponent.totalUpTime + LCUinternetStatusComponent.totalDownTime) > 0) {
                        LCUinternetStatusComponent.upTimePercent = ((double)LCUinternetStatusComponent.totalUpTime /
                                                        (LCUinternetStatusComponent.totalUpTime + LCUinternetStatusComponent.totalDownTime))*100.0;
                    }else {
                        LCUinternetStatusComponent.upTimePercent = 0.0;  // Prevent division by zero
                    }
                    cJSON_AddNumberToObject(internetObject,"upTimePercent",roundToDecimals(LCUinternetStatusComponent.upTimePercent,2));
                    cJSON_AddItemToObject(ocppControllerObject,"internet",internetObject);
        
            //--------------------Calculation for Uptime for websocket starts from here------------------//
                    //Creating jSON Object of websocket
                    cJSON* websocketObject = cJSON_CreateObject();
                    if(websocketObject == NULL){
                        ESP_LOGE(MQTT_TAG, "Error: Memory allocation failed for websocketObject\n");
                        cJSON_Delete(ocppControllerObject);
                        cJSON_Delete(payload);
                        return ;
                    }
        
                    cJSON *websocketUpTime = cJSON_CreateObject();
                    if (websocketUpTime == NULL) {
                        cJSON_Delete(websocketObject);
                        cJSON_Delete(ocppControllerObject);
                        cJSON_Delete(payload);
                        return;
                    }
        
                    cJSON *websocketDownTime = cJSON_CreateObject();
                    if (websocketDownTime == NULL){
                        cJSON_Delete(websocketUpTime);
                        cJSON_Delete(websocketObject);
                        cJSON_Delete(ocppControllerObject);
                        cJSON_Delete(payload);
                        return;
                    }
                       
                    // Websocket is UP, increment uptime
                    websocketStatusComponent.totalUpTime = read_nvs_value(NVS_WEBSOCKET_TOTAL_UP_TIME);
                    convert_seconds_to_time(websocketStatusComponent.totalUpTime, &websocketStatusComponent.upTime);
                    increment_time(&websocketStatusComponent.upTime, LCUWebsocketUp);
                    
                    cJSON_AddNumberToObject(websocketObject, "connectionStatus",chargerPeriodicStatusMQTT.websocketConnection);

                    cJSON_AddNumberToObject(websocketUpTime,"days",websocketStatusComponent.upTime.days);
                    cJSON_AddNumberToObject(websocketUpTime,"hours",websocketStatusComponent.upTime.hours);
                    cJSON_AddNumberToObject(websocketUpTime,"minutes",websocketStatusComponent.upTime.minutes);
                    cJSON_AddNumberToObject(websocketUpTime,"seconds",websocketStatusComponent.upTime.seconds);
                    cJSON_AddItemToObject(websocketObject,"upTime",websocketUpTime);
                    websocketStatusComponent.totalUpTime = (websocketStatusComponent.upTime.days*86400)+(websocketStatusComponent.upTime.hours*3600)+(websocketStatusComponent.upTime.minutes*60)+(websocketStatusComponent.upTime.seconds);
                    
                    // Websocket is Down, increment downtime
                    websocketStatusComponent.totalDownTime = read_nvs_value(NVS_WEBSOCKET_TOTAL_DOWN_TIME);
                    convert_seconds_to_time(websocketStatusComponent.totalDownTime, &websocketStatusComponent.downTime);
                    increment_time(&websocketStatusComponent.downTime, LCUWebsocketDown);

                    cJSON_AddNumberToObject(websocketDownTime,"days",websocketStatusComponent.downTime.days);
                    cJSON_AddNumberToObject(websocketDownTime,"hours",websocketStatusComponent.downTime.hours);
                    cJSON_AddNumberToObject(websocketDownTime,"minutes",websocketStatusComponent.downTime.minutes);
                    cJSON_AddNumberToObject(websocketDownTime,"seconds",websocketStatusComponent.downTime.seconds);
                    cJSON_AddItemToObject(websocketObject,"downTime",websocketDownTime);
                    websocketStatusComponent.totalDownTime = (websocketStatusComponent.downTime.days*86400)+(websocketStatusComponent.downTime.hours*3600)+(websocketStatusComponent.downTime.minutes*60)+(websocketStatusComponent.downTime.seconds);
        
                    // Calculate UpTime Percent for websocket
                    if ((websocketStatusComponent.totalUpTime + websocketStatusComponent.totalDownTime) > 0) {
                        websocketStatusComponent.upTimePercent = ((double)websocketStatusComponent.totalUpTime /
                                                        (websocketStatusComponent.totalUpTime + websocketStatusComponent.totalDownTime))*100.0;
                    }else {
                        websocketStatusComponent.upTimePercent = 0.0;  // Prevent division by zero
                    }
        
                    cJSON_AddNumberToObject(websocketObject,"upTimePercent",roundToDecimals(websocketStatusComponent.upTimePercent,2));
                    
                    cJSON_AddItemToObject(ocppControllerObject,"websocket",websocketObject);
                }

                cJSON* status = cJSON_CreateObject();
                if(status == NULL)
                {
                    ESP_LOGE(MQTT_TAG, "Error: Memory allocation failed for status\n");
                    return;
                }
                cJSON_AddItemToObject(status, "ocppController", ocppControllerObject);
                cJSON_AddItemToObject(payload, "status", status);
                if (payload != NULL) 
                    json_array_string = create_json_call_result_string(messageId, payload);
            }
            else
            {
                error = ID_NOT_SUPPORTED; 
                send_json_call_error(messageId, error);
            }
            // Payload creation: END
            break;
        }// case 3 end
        
        case 4:{
            //Component requested = Grid Status
            // Payload creation: START
            if(id==0)
            {
                cJSON *payload = cJSON_CreateObject();
                if (payload == NULL) {
                    printf("Failed to create payload (case 2 component status)\n");
                    return;
                }

                // Get current UTC timestamp in ISO 8601 format
                time_t now = time(NULL);
                struct tm *utc_time = gmtime(&now); // Convert to UTC
                char utc_timestamp[25];            // Buffer for ISO 8601 timestamp
                strftime(utc_timestamp, sizeof(utc_timestamp), "%Y-%m-%dT%H:%M:%SZ", utc_time);

                //Add TimeStamp in the JSON
                cJSON_AddStringToObject(payload, "timestamp", utc_timestamp);
                cJSON_AddNumberToObject(payload, "component", global_component_value);
                cJSON_AddNumberToObject(payload, "id", id_requested);

                cJSON* status = cJSON_CreateObject();
                if(status == NULL)
                {
                    ESP_LOGE(MQTT_TAG, "Error: Memory allocation failed for status\n");
                    return;
                }
                
                int gridSum = 0, healthyPCMs = 0;

                for(int i=0;i<chargerConfigMQTT.numPCM;i++){
                    if(powerModulesMQTT[i].CANFailure == 0){
                        healthyPCMs++;
                        if(powerModulesMQTT[i].gridAvailability == 1)
                            gridSum++;
                    }
                }
                if(gridSum == healthyPCMs && healthyPCMs >= 1)
                    cJSON_AddNumberToObject(status,"gridAvailable",1);
                else
                    cJSON_AddNumberToObject(status,"gridAvailable",0);

                cJSON_AddItemToObject(payload, "status", status);
                if (payload != NULL) 
                    json_array_string = create_json_call_result_string(messageId, payload);
                // Payload creation: END
            }
            else{
                error = ID_NOT_SUPPORTED; 
                send_json_call_error(messageId, error);
            }
            break;
        }// case 4 end

        default:{
            // Invalid component requested
            error = COMPONENT_NOT_SUPPORTED; 
            send_json_call_error(messageId, error);
            break;
        }
    }
    if (json_array_string) {
        //publish to broker
        if (IS_MQTT_NETWORK_ONLINE){
            // esp_mqtt_client_publish(mqtt_client, pubTopic, jsonString, 0, 1, 0);
            if (esp_mqtt_client_publish(mqtt_client, pubTopic, json_array_string, 0, 1, 0) != -1) 
            {
                ESP_LOGI(OCPP_TAG,"json_string_Component_Status:%s", json_array_string);
                // printf("Message send successfully");
            }
        }
        free(json_array_string);
        json_array_string = NULL;
    }
}

void create_PCM_Periodic_message(uint8_t action)
{
    int startIndex = 0;
    int endIndex = chargerConfigMQTT.numPCM;
    // int i = startIndex;
    cJSON* power_ModuleRoot = create_PCM_and_ComponentStatus_payloadJSON(startIndex, endIndex, PCMPeriodicData);
    
    //To Do OCPP format creation or JSON Array creation and File storage here only.
    char* json_array_string = create_json_call_string("PCMPeriodicData",power_ModuleRoot);
    
    cJSON_Delete(power_ModuleRoot); //might double delete occur here
   
    //Storing the File in SPIFFS System 
    save_payload_to_spiffs(FILEPATH, json_array_string);
    ESP_LOGI(OCPP_TAG,"json_string_PCM_Periodic:%s", json_array_string);

    // counterPeriodic = 0;
    free(json_array_string);
    json_array_string = NULL;
}

void create_chargerPeriodicData_message()
{
    cJSON *json = cJSON_CreateObject();
    if (json == NULL){
        ESP_LOGE(MQTT_TAG, "Error: Memory allocation failed for json\n");
        return;
    }

    // Add timestamp in ISO 8601 format
    struct timeval tv;
    gettimeofday(&tv, NULL); // Get the current time
    struct tm timeinfo;
    gmtime_r(&tv.tv_sec, &timeinfo); // Convert to UTC time

    char timestamp[30]; // Buffer to hold the timestamp string
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
    cJSON_AddStringToObject(json, "timestamp", timestamp); // Add formatted timestamp

    cJSON_AddNumberToObject(json, "mcuComm", chargerPeriodicStatusMQTT.MCUComm);

    if(chargerPeriodicStatusMQTT.MCUComm == FINE)
    {
        ESP_LOGI("energyMeterInsidePeriodic", "energyMeterSetting: %d", chargerConfigMQTT.EnergyMeterSetting);
        //condition for when actual energy meter is used
        //project can be any DCFC or AC project for key "acMeter"
       if(chargerConfigMQTT.EnergyMeterSetting == ACTUAL)
        {
            
            cJSON* acMeterObject = cJSON_CreateObject();
            if (acMeterObject == NULL){
                cJSON_Delete(json);
                ESP_LOGE(MQTT_TAG, "Error: Memory allocation failed for acMeterObject\n");
                return;
            }
            cJSON_AddNumberToObject(acMeterObject,"commStatus",chargerPeriodicStatusMQTT.acMeterComm);
            
            if(chargerPeriodicStatusMQTT.acMeterComm == FINE)
            {
                cJSON_AddNumberToObject(acMeterObject,"energy",chargerPeriodicStatusMQTT.acEnergy);

                cJSON *gridVoltagesArray = cJSON_CreateArray();
                if (gridVoltagesArray == NULL){
                    ESP_LOGE(MQTT_TAG, "Error: Memory allocation failed for gridVoltagesArray\n");
                    cJSON_Delete(acMeterObject);
                    cJSON_Delete(json);
                    return;
                }
                if(chargerPeriodicStatusMQTT.gridVoltages[0])
                cJSON_AddItemToArray(gridVoltagesArray, cJSON_CreateNumber(roundToDecimals(chargerPeriodicStatusMQTT.gridVoltages[0],1)));
                cJSON_AddItemToArray(gridVoltagesArray, cJSON_CreateNumber(roundToDecimals(chargerPeriodicStatusMQTT.gridVoltages[1],1)));
                cJSON_AddItemToArray(gridVoltagesArray, cJSON_CreateNumber(roundToDecimals(chargerPeriodicStatusMQTT.gridVoltages[2],1)));

                cJSON_AddItemToObject(acMeterObject, "gridVoltage", gridVoltagesArray);

                cJSON *gridCurrentsArray = cJSON_CreateArray();
                if (gridCurrentsArray == NULL){
                    ESP_LOGE(MQTT_TAG, "Error: Memory allocation failed for gridCurrentsArray\n");
                    cJSON_Delete(acMeterObject);
                    cJSON_Delete(json);
                    return;
                }
                cJSON_AddItemToArray(gridCurrentsArray, cJSON_CreateNumber(roundToDecimals(chargerPeriodicStatusMQTT.gridCurrents[0],2)));
                cJSON_AddItemToArray(gridCurrentsArray, cJSON_CreateNumber(roundToDecimals(chargerPeriodicStatusMQTT.gridCurrents[1],2)));
                cJSON_AddItemToArray(gridCurrentsArray, cJSON_CreateNumber(roundToDecimals(chargerPeriodicStatusMQTT.gridCurrents[2],2)));

                cJSON_AddItemToObject(acMeterObject, "gridCurrent", gridCurrentsArray);

                cJSON *acMeterLineVoltagesArray = cJSON_CreateArray();
                if (acMeterLineVoltagesArray == NULL){
                    ESP_LOGE(MQTT_TAG, "Error: Memory allocation failed for acMeterLineVoltagesArray\n");
                    cJSON_Delete(acMeterObject);
                    cJSON_Delete(json);
                    return;
                }
                int i = 0;
                for(i=0; i<3; i++)
                {
                    cJSON_AddItemToArray(acMeterLineVoltagesArray, cJSON_CreateNumber(roundToDecimals(chargerPeriodicStatusMQTT.acMeterLineVoltages[i],1)));
                }
                cJSON_AddItemToObject(acMeterObject, "lineVoltage", acMeterLineVoltagesArray);
                cJSON_AddNumberToObject(acMeterObject, "realPowerFactor", roundToDecimals(chargerPeriodicStatusMQTT.RealPowerFactor,2));
                cJSON_AddNumberToObject(acMeterObject, "avgPowerFactor", roundToDecimals(chargerPeriodicStatusMQTT.AvgPowerFactor,1));
            }
            cJSON_AddItemToObject(json, "acMeter", acMeterObject);
        }
        cJSON_AddNumberToObject(json, "chargerStatus", chargerPeriodicStatusMQTT.chargerStatus);
        cJSON_AddNumberToObject(json, "chargerFaultStatus", chargerPeriodicStatusMQTT.chargerFaultCode);

//----------------adding connector info (array of json objects) to root json-----------------------------------//
        cJSON* connectorInfoArray = cJSON_CreateArray();
        if(connectorInfoArray == NULL)
        {
            ESP_LOGE(MQTT_TAG, "Error: Memory allocation failed for connectorInfoArray\n");
            cJSON_Delete(json);
            return;
        }
        int connNumber = 0;
        for(connNumber=0; (connNumber < chargerConfigMQTT.numConnectors); connNumber++)
        {
            cJSON* connectorInfoObject = cJSON_CreateObject();
            if(connectorInfoObject == NULL)
            {
                ESP_LOGE(MQTT_TAG, "Error: Memory allocation failed for connectorInfoObject\n");
                cJSON_Delete(connectorInfoArray);
                cJSON_Delete(json);
                return;
            }
            cJSON_AddNumberToObject(connectorInfoObject, "id", (connNumber + 1));

            cJSON* commControllerObject = cJSON_CreateObject();
            if(commControllerObject == NULL)
            {
                ESP_LOGE(MQTT_TAG, "Error: Memory allocation failed for commControllerObject\n");
                cJSON_Delete(connectorInfoObject);
                cJSON_Delete(connectorInfoArray);
                cJSON_Delete(json);
                return;
            }
            cJSON_AddNumberToObject(commControllerObject, "commStatus", chargerPeriodicStatusMQTT.ev_evseCommRecv[connNumber]);
            if(chargerPeriodicStatusMQTT.ev_evseCommRecv[connNumber] == FINE && chargerPeriodicStatusMQTT.ErrorCode0Recv[connNumber] != 0 && chargerPeriodicStatusMQTT.ErrorCode0Recv[connNumber] != 0xFF)
            {
                cJSON_AddNumberToObject(commControllerObject, "errorCode", chargerPeriodicStatusMQTT.ErrorCode0Recv[connNumber]);
                //cJSON_AddNumberToObject(commControllerObject, "errorCode", 101);
                
            }
            cJSON_AddItemToObject(connectorInfoObject, "commController", commControllerObject);
            
            cJSON_AddNumberToObject(connectorInfoObject, "connectorStatus", chargerPeriodicStatusMQTT.connectorsStatus[connNumber]);
            //ESP_LOGI(MQTT_TAG,"connectorStatus is %d",chargerPeriodicStatusMQTT.connectorsStatus[connNumber]);
            
            // //start with adding dcMeter object
            if(chargerConfigMQTT.projectInfo == DCFC && chargerConfigMQTT.EnergyMeterSetting == ACTUAL)
            {
                cJSON* dcMeterObject = cJSON_CreateObject();
                if(dcMeterObject == NULL)
                {
                    ESP_LOGE(MQTT_TAG, "Error: Memory allocation failed for dcMeterObject\n");
                    cJSON_Delete(connectorInfoObject);
                    cJSON_Delete(connectorInfoArray);
                    cJSON_Delete(json);
                    return;
                }
                cJSON_AddNumberToObject(dcMeterObject, "commStatus", chargerPeriodicStatusMQTT.dcMeterComm[connNumber]);
                if(chargerPeriodicStatusMQTT.dcMeterComm[connNumber] == FINE)
                {
                    cJSON_AddNumberToObject(dcMeterObject, "current", roundToDecimals(chargerPeriodicStatusMQTT.dcCurrent[connNumber],2));
                    cJSON_AddNumberToObject(dcMeterObject, "energy", chargerPeriodicStatusMQTT.dcEnergy[connNumber]);
                    cJSON_AddNumberToObject(dcMeterObject, "power", chargerPeriodicStatusMQTT.dcMeterPower[connNumber]);
                    cJSON_AddNumberToObject(dcMeterObject, "voltage", roundToDecimals(chargerPeriodicStatusMQTT.dcVoltage[connNumber],2));
                }
                cJSON_AddItemToObject(connectorInfoObject, "dcMeter", dcMeterObject);
            }
            
            cJSON_AddNumberToObject(connectorInfoObject, "faultStatus", chargerPeriodicStatusMQTT.connectorFaultCode[connNumber]);
           
            cJSON_AddItemToArray(connectorInfoArray, connectorInfoObject);
        }
        cJSON_AddItemToObject(json, "connectorInfo", connectorInfoArray);

        //+++++++++++++++++++++++ Creating this for OCPP Controller ++++++++++++++++++++++++++++++ 

        cJSON* ocppControllerObject = cJSON_CreateObject();
        if(ocppControllerObject == NULL)
        {
            ESP_LOGE(MQTT_TAG, "Error: Memory allocation failed for ocppControllerObject\n");
            cJSON_Delete(json);
            return;
        }
        cJSON_AddNumberToObject(ocppControllerObject, "commStatus", chargerPeriodicStatusMQTT.LCUCommStatus);
        
        if(chargerPeriodicStatusMQTT.LCUCommStatus == FINE && chargerConfigMQTT.configKeyValue == ONLINE)
        {
            //Creating jSON Object of Internet
            cJSON* internetObject = cJSON_CreateObject();
            if(internetObject == NULL){
                ESP_LOGE(MQTT_TAG, "Error: Memory allocation failed for internetObject\n");
                cJSON_Delete(ocppControllerObject);
                cJSON_Delete(json);
                return ;
            }

            cJSON *upTime = cJSON_CreateObject();
            if (upTime == NULL) {
                cJSON_Delete(internetObject);
                cJSON_Delete(ocppControllerObject);
                cJSON_Delete(json);
                return;
            }

            cJSON *downTime = cJSON_CreateObject();
            if (downTime == NULL){
                cJSON_Delete(internetObject);
                cJSON_Delete(ocppControllerObject);
                cJSON_Delete(upTime);
                cJSON_Delete(json);
                return;
            }

            LCUinternetStatus.totalUpTime = read_nvs_value(NVS_INTERNET_TOTAL_UP_TIME);
            ESP_LOGI("DEBUG", "\x1b[34mInternetStatus.totalUpTime: %" PRIu32  "\x1b[0m", LCUinternetStatus.totalUpTime);
            
            LCUinternetStatus.totalDownTime = read_nvs_value(NVS_INTERNET_TOTAL_DOWN_TIME);
            
            convertToTimeDuration(LCUinternetStatus.totalUpTime,&LCUinternetStatus.upTime);
            convertToTimeDuration(LCUinternetStatus.totalDownTime,&LCUinternetStatus.downTime);

            cJSON_AddNumberToObject(internetObject, "connectionStatus",chargerPeriodicStatusMQTT.LCUInternetStatus);
            ESP_LOGI("DEBUG", "\x1b[34mConnectionStatusMQTT: %d\x1b[0m", chargerPeriodicStatusMQTT.LCUInternetStatus);

            cJSON_AddNumberToObject(upTime,"days",LCUinternetStatus.upTime.days);
            cJSON_AddNumberToObject(upTime,"hours",LCUinternetStatus.upTime.hours);
            cJSON_AddNumberToObject(upTime,"minutes",LCUinternetStatus.upTime.minutes);
            cJSON_AddNumberToObject(upTime,"seconds",LCUinternetStatus.upTime.seconds);
            cJSON_AddItemToObject(internetObject,"upTime",upTime);
    
            cJSON_AddNumberToObject(downTime,"days",LCUinternetStatus.downTime.days);
            cJSON_AddNumberToObject(downTime,"hours",LCUinternetStatus.downTime.hours);
            cJSON_AddNumberToObject(downTime,"minutes",LCUinternetStatus.downTime.minutes);
            cJSON_AddNumberToObject(downTime,"seconds",LCUinternetStatus.downTime.seconds);
            cJSON_AddItemToObject(internetObject,"downTime",downTime);
        
            // Calculate UpTime Percent
            if ((LCUinternetStatus.totalUpTime + LCUinternetStatus.totalDownTime) > 0) {
                LCUinternetStatus.upTimePercent = ((double)LCUinternetStatus.totalUpTime /
                                                (LCUinternetStatus.totalUpTime + LCUinternetStatus.totalDownTime))*100.0;
            }else {
                LCUinternetStatus.upTimePercent = 0.0;  // Prevent division by zero
            }

            cJSON_AddNumberToObject(internetObject,"upTimePercent",roundToDecimals(LCUinternetStatus.upTimePercent,2));
            
            cJSON_AddItemToObject(ocppControllerObject,"internet",internetObject);

            //Creating jSON Object of websocket
            cJSON* websocketObject = cJSON_CreateObject();
            if(websocketObject == NULL){
                ESP_LOGE(MQTT_TAG, "Error: Memory allocation failed for websocketObject\n");
                cJSON_Delete(ocppControllerObject);
                cJSON_Delete(json);
                return ;
            }

            cJSON *websocketUpTime = cJSON_CreateObject();
            if (websocketUpTime == NULL) {
                cJSON_Delete(websocketObject);
                cJSON_Delete(ocppControllerObject);
                cJSON_Delete(json);
                return;
            }

            cJSON *websocketDownTime = cJSON_CreateObject();
            if (websocketDownTime == NULL){
                cJSON_Delete(websocketUpTime);
                cJSON_Delete(websocketObject);
                cJSON_Delete(ocppControllerObject);
                cJSON_Delete(json);
                return;
            }

            websocketStatus.totalUpTime = read_nvs_value(NVS_WEBSOCKET_TOTAL_UP_TIME);
            websocketStatus.totalDownTime = read_nvs_value(NVS_WEBSOCKET_TOTAL_DOWN_TIME);
            
            convertToTimeDuration(websocketStatus.totalUpTime, &websocketStatus.upTime);
            convertToTimeDuration(websocketStatus.totalDownTime,&websocketStatus.downTime);
            
            cJSON_AddNumberToObject(websocketObject, "connectionStatus",chargerPeriodicStatusMQTT.websocketConnection);
            ESP_LOGI("DEBUG", "\x1b[34mwebSocetConnectionStatusMQTT: %d\x1b[0m", chargerPeriodicStatusMQTT.websocketConnection);

            cJSON_AddNumberToObject(websocketUpTime,"days",websocketStatus.upTime.days);
            cJSON_AddNumberToObject(websocketUpTime,"hours",websocketStatus.upTime.hours);
            cJSON_AddNumberToObject(websocketUpTime,"minutes",websocketStatus.upTime.minutes);
            cJSON_AddNumberToObject(websocketUpTime,"seconds",websocketStatus.upTime.seconds);
            cJSON_AddItemToObject(websocketObject,"upTime",websocketUpTime);
            //websocketStatus.totalUpTime = (websocketStatus.upTime.days*86400)+(websocketStatus.upTime.hours*3600)+(websocketStatus.upTime.minutes*60)+(websocketStatus.upTime.seconds);
            //write_nvs_value(NVS_WEBSOCKET_TOTAL_UP_TIME,websocketStatus.totalUpTime);
            
            cJSON_AddNumberToObject(websocketDownTime,"days",websocketStatus.downTime.days);
            cJSON_AddNumberToObject(websocketDownTime,"hours",websocketStatus.downTime.hours);
            cJSON_AddNumberToObject(websocketDownTime,"minutes",websocketStatus.downTime.minutes);
            cJSON_AddNumberToObject(websocketDownTime,"seconds",websocketStatus.downTime.seconds);
            cJSON_AddItemToObject(websocketObject,"downTime",websocketDownTime);
            //websocketStatus.totalDownTime = (websocketStatus.downTime.days*86400)+(websocketStatus.downTime.hours*3600)+(websocketStatus.downTime.minutes*60)+(websocketStatus.downTime.seconds);
            //write_nvs_value(NVS_WEBSOCKET_TOTAL_DOWN_TIME,websocketStatus.totalDownTime);
            
            // Calculate UpTime Percent
            if ((websocketStatus.totalUpTime + websocketStatus.totalDownTime) > 0) {
                websocketStatus.upTimePercent = ((double)websocketStatus.totalUpTime / (websocketStatus.totalUpTime + websocketStatus.totalDownTime))*100.0;
            }else {
                websocketStatus.upTimePercent = 0.0;  // Prevent division by zero
            }

            cJSON_AddNumberToObject(websocketObject,"upTimePercent",roundToDecimals(websocketStatus.upTimePercent,2));
            
            cJSON_AddItemToObject(ocppControllerObject,"websocket",websocketObject);
        }

        cJSON_AddItemToObject(json,"ocppController",ocppControllerObject);

        cJSON_AddNumberToObject(json, "virtualEnergyMeter", chargerPeriodicStatusMQTT.virtualEnergyMeter);
    
    }
    
    //call form message function here
    char *json_array_string = create_json_call_string("ChargerPeriodicData", json);

    // Cleanup: Ensure objects are deleted only once
    cJSON_Delete(json);

//--------//add request msg string returned from above function to spiffs file//------------filename: req.txt//
    ESP_LOGI(OCPP_TAG,"json_string_chargerPeriodic:%s", json_array_string);
    save_payload_to_spiffs(FILEPATH, json_array_string);
    free(json_array_string);
    json_array_string = NULL;
}

void prepare_mqtt_send_config_data(){
  
    // Create the root JSON object
    cJSON *config_root = cJSON_CreateObject();
    if (config_root == NULL) {
        printf("Failed to create root JSON object\n");
        return;
    }
    
// Add timestamp in ISO 8601 format
    struct timeval tv;
    gettimeofday(&tv, NULL); // Get the current time
    struct tm timeinfo;
    gmtime_r(&tv.tv_sec, &timeinfo); // Convert to UTC time

    char timestamp[30]; // Buffer to hold the timestamp string
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);

    //Add TimeStamp in the JSON
    cJSON_AddStringToObject(config_root, "timestamp", timestamp);

    //Add pmuEnabled Key in the JSON 
    if(flagConfig.pmuEnabled == 1){
        cJSON_AddNumberToObject(config_root,"pmuStatus", chargerConfigMQTT.pmuEnable);
        flagConfig.pmuEnabled = 0;
    }

    if(chargerConfigMQTT.pmuEnable == 1){
        if(flagConfig.AutoChargeFlag == 1) {
            cJSON_AddNumberToObject(config_root, "autoCharge", chargerConfigMQTT.AutoCharge);
            flagConfig.AutoChargeFlag = 0;
        }

        if(flagConfig.BrandFlag == 1) {
            cJSON_AddNumberToObject(config_root, "brand", chargerConfigMQTT.Brand);
            flagConfig.BrandFlag = 0;
        }

        if(flagConfig.chargerPowerFlag == 1 && chargerConfigMQTT.limitSetting.chargerPower != FLT_MAX) 
        {
            cJSON_AddNumberToObject(config_root, "chargerPowerLimit", chargerConfigMQTT.limitSetting.chargerPower);
            flagConfig.chargerPowerFlag = 0;
        }

        if(flagConfig.serverChargerPowerFlag == 1){
            cJSON_AddNumberToObject(config_root, "serverChargerPowerLimit", roundToDecimals(chargerConfigMQTT.limitSetting.serverChargerPower,2));
            flagConfig.serverChargerPowerFlag = 0;
        }

        if(flagConfig.numConnectorsFlag == 1) {
            cJSON *connectorsInfoArr = cJSON_CreateArray();
            if (connectorsInfoArr == NULL) {
                cJSON_Delete(config_root);
                printf("Failed to create connectorsInfo array\n");
                return;
            }

            //Manually create and add the first connector
            if(flagConfig.numConnector1 == 1){
                cJSON *connector1 = cJSON_CreateObject();
                if (connector1 == NULL) {
                    cJSON_Delete(connectorsInfoArr);
                    cJSON_Delete(config_root);
                    printf("Failed to create connector1 object\n");
                    return;
                }

                cJSON_AddNumberToObject(connector1, "id", 1);
                if(flagConfig.commController1 == 1) {
                    cJSON_AddStringToObject(connector1, "commControllerVersion", chargerConfigMQTT.versionInfo.evseCommContoller[0]);
                    flagConfig.commController1 = 0;
                }
                if(flagConfig.gunAVoltageFlag == 1 && chargerConfigMQTT.limitSetting.gunAVoltage != FLT_MAX) {
                    cJSON_AddNumberToObject(connector1, "voltageLimit", roundToDecimals(chargerConfigMQTT.limitSetting.gunAVoltage,2));
                    flagConfig.gunAVoltageFlag = 0;
                }
                if(flagConfig.gunACurrentFlag == 1 && chargerConfigMQTT.limitSetting.gunACurrent != FLT_MAX) {
                    cJSON_AddNumberToObject(connector1, "currentLimit", roundToDecimals(chargerConfigMQTT.limitSetting.gunACurrent,2));
                    flagConfig.gunACurrentFlag = 0;
                }
                if(flagConfig.gunAPowerFlag == 1 && chargerConfigMQTT.limitSetting.gunAPower != FLT_MAX) {
                    cJSON_AddNumberToObject(connector1, "powerLimit", roundToDecimals(chargerConfigMQTT.limitSetting.gunAPower,2));
                    flagConfig.gunAPowerFlag = 0;
                }
                if(flagConfig.serverConnectorAPowerFlag == 1){
                    cJSON_AddNumberToObject(connector1, "serverPowerLimit", roundToDecimals(chargerConfigMQTT.limitSetting.serverConnectorAPower,2));
                    flagConfig.serverConnectorAPowerFlag = 0;
                }
                if(flagConfig.serverConnectorACurrentFlag == 1){
                    cJSON_AddNumberToObject(connector1, "serverCurrentLimit", roundToDecimals(chargerConfigMQTT.limitSetting.serverConnectorACurrent,2));
                    flagConfig.serverConnectorACurrentFlag = 0;
                }
                cJSON_AddItemToArray(connectorsInfoArr, connector1);
                flagConfig.numConnector1 = 0;
            }

            // Manually create and add the second connector
            if(flagConfig.numConnector2 == 1){
                cJSON *connector2 = cJSON_CreateObject();
                if (connector2 == NULL) {
                    cJSON_Delete(connectorsInfoArr);
                    cJSON_Delete(config_root);
                    printf("Failed to create connector2 object\n");
                    return;
                }
                
                cJSON_AddNumberToObject(connector2, "id", 2);
                if(flagConfig.commController2 == 1) {
                    cJSON_AddStringToObject(connector2, "commControllerVersion", chargerConfigMQTT.versionInfo.evseCommContoller[1]);
                    flagConfig.commController2 = 0 ;
                }
                if(flagConfig.gunBVoltageFlag == 1 && chargerConfigMQTT.limitSetting.gunBVoltage != FLT_MAX) {
                    cJSON_AddNumberToObject(connector2, "voltageLimit", roundToDecimals(chargerConfigMQTT.limitSetting.gunBVoltage,2));
                    flagConfig.gunBVoltageFlag = 0;
                }
                if(flagConfig.gunBCurrentFlag == 1 && chargerConfigMQTT.limitSetting.gunBCurrent != FLT_MAX) {
                    cJSON_AddNumberToObject(connector2, "currentLimit", roundToDecimals(chargerConfigMQTT.limitSetting.gunBCurrent,2));
                    flagConfig.gunBCurrentFlag = 0;
                }
                if(flagConfig.gunBPowerFlag == 1 && chargerConfigMQTT.limitSetting.gunBPower != FLT_MAX) {
                    cJSON_AddNumberToObject(connector2, "powerLimit", roundToDecimals(chargerConfigMQTT.limitSetting.gunBPower,2));
                    flagConfig.gunBPowerFlag = 0;
                }
                if(flagConfig.serverConnectorBPowerFlag == 1){
                    cJSON_AddNumberToObject(connector2, "serverPowerLimit", roundToDecimals(chargerConfigMQTT.limitSetting.serverConnectorBPower,2));
                    flagConfig.serverConnectorBPowerFlag = 0;
                }
                if(flagConfig.serverConnectorBCurrentFlag == 1){
                    cJSON_AddNumberToObject(connector2, "serverCurrentLimit", roundToDecimals(chargerConfigMQTT.limitSetting.serverConnectorBCurrent,2));
                    flagConfig.serverConnectorBCurrentFlag = 0;
                }
                cJSON_AddItemToArray(connectorsInfoArr, connector2);
                flagConfig.numConnector2 = 0;
            }
            // Add the connectorsInfo array to the root object
            cJSON_AddItemToObject(config_root, "connectorInfo", connectorsInfoArr);
            flagConfig.numConnectorsFlag = 0;
        }

        if(flagConfig.versionInfoFlag == 1) {
            cJSON *version_info = cJSON_CreateObject();
            if(version_info == NULL) {
                cJSON_Delete(config_root);
                printf("Failed to create VersionInfo object\n");
                return;
            }
            if(flagConfig.hmiVersionFlag == 1){
                cJSON_AddStringToObject(version_info, "hmi", chargerConfigMQTT.versionInfo.hmiVersion);
                flagConfig.hmiVersionFlag = 0;
            }  
            if(flagConfig.lcuVersionFlag == 1){  
                cJSON_AddStringToObject(version_info, "lcu", chargerConfigMQTT.versionInfo.lcuVersion);
                flagConfig.lcuVersionFlag = 0;
            }
            if(flagConfig.mcuVersionFlag == 1){
                cJSON_AddStringToObject(version_info, "mcu", chargerConfigMQTT.versionInfo.mcuVersion);
                flagConfig.mcuVersionFlag = 0;
            }

            if(flagConfig.numPCMFlag == 1) {
                cJSON *PCMVersionArr = cJSON_CreateArray();
                if (PCMVersionArr == NULL) {
                    cJSON_Delete(version_info);
                    cJSON_Delete(config_root);
                    printf("Failed to create PCMVersionArr array\n");
                    return;
                }
                for (int i = 0; i < chargerConfigMQTT.numPCM; i++) {
                    if(powerModulesMQTT[i].CANFailure == 1)
                        cJSON_AddItemToArray(PCMVersionArr, cJSON_CreateString("CommunicationFault"));
                    else{
                        char versionArr[10];
                        sprintf(versionArr, "%u.%u", powerModulesMQTT[i].versionMajorNumber, powerModulesMQTT[i].versionMinorNumber);
                        
                        cJSON_AddItemToArray(PCMVersionArr, cJSON_CreateString(versionArr));
                    }
                }
                cJSON_AddItemToObject(version_info, "pcm", PCMVersionArr);
                flagConfig.numPCMFlag = 0;
            }

            if(flagConfig.pmuVersionFlag == 1){
                cJSON_AddStringToObject(version_info, "pmu", "1.0");
                flagConfig.pmuVersionFlag = 0;
            }

            cJSON_AddItemToObject(config_root, "versionInfo", version_info);
            flagConfig.versionInfoFlag = 0;
        }

        if(flagConfig.DynamicPowerSharingFlag == 1) {
            cJSON_AddNumberToObject(config_root, "dynamicPowerSharing", chargerConfigMQTT.DynamicPowerSharing);
            flagConfig.DynamicPowerSharingFlag = 0;
        }

        if(flagConfig.EnergyMeterSettingFlag == 1) {
            cJSON_AddNumberToObject(config_root, "energyMeterSetting", chargerConfigMQTT.EnergyMeterSetting);
            flagConfig.EnergyMeterSettingFlag = 0;
        }

        if(flagConfig.ZeroMeterStartEnableFlag == 1) {
            cJSON_AddNumberToObject(config_root, "zeroMeterStartEnable", chargerConfigMQTT.ZeroMeterStartEnable);
            flagConfig.ZeroMeterStartEnableFlag = 0;
        }

        if(flagConfig.ocppURLFlag == 1) {
            cJSON_AddStringToObject(config_root, "ocppConnectionUrl", chargerConfigMQTT.ocppURL);
            flagConfig.ocppURLFlag = 0;
        }

        if(flagConfig.configKeyValueFlag == 1) {
            cJSON_AddNumberToObject(config_root, "chargerMode", chargerConfigMQTT.configKeyValue);
            flagConfig.configKeyValueFlag = 0;
        }

        if(flagConfig.tarrifAmountFlag == 1) {
            cJSON_AddNumberToObject(config_root, "tariff", roundToDecimals(chargerConfigMQTT.tarrifAmount,2));
            flagConfig.tarrifAmountFlag = 0;
        }
        if(flagConfig.eepromCommStatusFlag == 1){
            cJSON_AddNumberToObject(config_root, "eepromCommStatus", chargerConfigMQTT.eepromCommStatus);
            flagConfig.eepromCommStatusFlag = 0;
        }

        if(flagConfig.rtcCommStatusFlag == 1){
            cJSON_AddNumberToObject(config_root, "rtcCommStatus", chargerConfigMQTT.rtcCommStatus);
            flagConfig.rtcCommStatusFlag = 0;
        }
    }  
    char *json_payload = create_json_call_string("ConfigurationData",config_root);
    //ESP_LOGI(MQTT_TAG, "payloadChargerConfig: %s", json_payload);    //Created [] vased payload
    save_payload_to_spiffs(FILEPATH, json_payload);
    free(json_payload);            //Free the payload string after use
    check_memory_status();
    cJSON_Delete(config_root);
}

void create_chargingSession_message()
{
    cJSON *json = cJSON_CreateObject();
    if (json == NULL){
        ESP_LOGE(MQTT_TAG, "Error: Memory allocation failed for json\n");
        return;
    }

    // Add timestamp in ISO 8601 format
    struct timeval tv;
    gettimeofday(&tv, NULL); // Get the current time
    struct tm timeinfo;
    gmtime_r(&tv.tv_sec, &timeinfo); // Convert to UTC time

    char timestamp[30]; // Buffer to hold the timestamp string
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
    
    //Compulsory field
    cJSON_AddStringToObject(json, "timestamp", timestamp); // Add formatted timestamp
    //Compulsory field
    cJSON_AddNumberToObject(json, "sessionType", sessionInfoMQTT.sessionType);
    //Compulsory field
    cJSON_AddNumberToObject(json, "connector", sessionInfoMQTT.connector);
    //All the fields below are optional
    if (sessionInfoMQTT.sessionType == initiatedButNotStarted ||  sessionInfoMQTT.sessionType == initiatedStartedNormalTermination || sessionInfoMQTT.sessionType == initiatedStartedFaultyTermination)
        cJSON_AddNumberToObject(json, "stopReason", sessionInfoMQTT.stopReason);

    if(sessionInfoMQTT.sessionType == initiatedStartedNormalTermination || sessionInfoMQTT.sessionType == initiatedStartedFaultyTermination)
    {
        cJSON_AddStringToObject(json, "startDateTime", sessionInfoMQTT.startDateTime);
        cJSON_AddStringToObject(json, "endDateTime", sessionInfoMQTT.endDateTime);
        cJSON_AddNumberToObject(json, "energyConsumed", sessionInfoMQTT.energyConsumed);
        cJSON_AddNumberToObject(json, "estimatedCost", roundToDecimals(sessionInfoMQTT.estimatedCost,2));
        cJSON_AddNumberToObject(json, "chargePercentage", sessionInfoMQTT.chargePercentage);
        cJSON_AddNumberToObject(json, "chargeDuration", sessionInfoMQTT.chargeDuration);
        cJSON_AddNumberToObject(json, "startSoC", sessionInfoMQTT.startSoC);
        cJSON_AddNumberToObject(json, "endSoC", sessionInfoMQTT.endSoC);
    }
    //if(sessionInfoMQTT.sessionType == initiatedStartedPowerOutage)
        // cJSON_AddStringToObject(json, "startDateTime", sessionInfoMQTT.startDateTime);
    //call form message function here
    char *json_array_string = create_json_call_string("ChargingSession", json);

    // Cleanup: Ensure objects are deleted only once
    cJSON_Delete(json);

    ESP_LOGI(OCPP_TAG,"json_string_chargingSession:%s", json_array_string);
    save_payload_to_spiffs(FILEPATH, json_array_string);
    free(json_array_string);
    json_array_string = NULL;
}

void check_memory_status() {
    size_t free_heap = esp_get_free_heap_size();
    ESP_LOGI("MEMORY_CHECK", "Free heap size: %d bytes", free_heap);
}

//Function to increment uptime/downtime by 30 seconds
void increment_time(timeDuration_t *timeStatus, uint8_t seconds_to_add) {
    timeStatus->seconds += seconds_to_add;

    if (timeStatus->seconds >= 60) {
        timeStatus->minutes += timeStatus->seconds / 60;
        timeStatus->seconds %= 60;
    }
    if (timeStatus->minutes >= 60) {
        timeStatus->hours += timeStatus->minutes / 60;
        timeStatus->minutes %= 60;
    }
    if (timeStatus->hours >= 24) {
        timeStatus->days += timeStatus->hours / 24;
        timeStatus->hours %= 24;
    }
}

void convertToTimeDuration(uint32_t totalSeconds, timeDuration_t *timeDuration) {
    if (timeDuration == NULL) {
        return;  // Prevent null pointer dereference
    }
    timeDuration->days    = totalSeconds / (24 * 3600);
    timeDuration->hours   = (totalSeconds % (24 * 3600)) / 3600;
    timeDuration->minutes = (totalSeconds % 3600) / 60;
    timeDuration->seconds = totalSeconds % 60;
}

void create_faultLog_message(){
    cJSON *json = cJSON_CreateObject();
    if (json == NULL){
        ESP_LOGE(MQTT_TAG, "Error: Memory allocation failed for json\n");
        return;
    }

    // Add timestamp in ISO 8601 format
    struct timeval tv;
    gettimeofday(&tv, NULL); // Get the current time
    struct tm timeinfo;
    gmtime_r(&tv.tv_sec, &timeinfo); // Convert to UTC time

    char timestamp[30]; // Buffer to hold the timestamp string
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
    
    cJSON_AddStringToObject(json, "timestamp", timestamp); // Add formatted timestamp
    cJSON_AddNumberToObject(json, "type", faultLogMQTT.faultType);
    cJSON_AddNumberToObject(json, "connectorId", faultLogMQTT.player);
    cJSON_AddStringToObject(json, "occurrenceDateTime", faultLogMQTT.occuranceDateTime);
    cJSON_AddStringToObject(json, "clearanceDateTime", faultLogMQTT.clearanceDateTime); 
    cJSON_AddNumberToObject(json, "faultCode", faultLogMQTT.faultCode);

    //call form message function here
    char *json_array_string = create_json_call_string("FaultInformation", json);

    // Cleanup: Ensure objects are deleted only once
    cJSON_Delete(json);

    ESP_LOGI(OCPP_TAG,"json_string_faultLog:%s", json_array_string);

    // Decide the file path based on faultType and player
    if (faultLogMQTT.faultType == 0) {
        if (faultLogMQTT.player == 0) {
            ESP_LOGW("DEBUG", "INSIDE THE PAYLOAD CREATION");
            save_payload_to_spiffs(FAULT_LOG_FILEPATH_OfflineCharger, json_array_string);
        } 
        else if (faultLogMQTT.player > 0 && faultLogMQTT.player <= NO_OF_CONNECTORS) {
            // Connector-specific log (player is 1-based, array is 0-based)
            save_payload_to_spiffs(fileNameConnector[faultLogMQTT.player - 1].logFilePath, json_array_string);
        } 
    } 
    else 
    {
        save_payload_to_spiffs(FAULT_LOG_FILEPATH_OnlineCharger, json_array_string);
    }
    free(json_array_string);
    json_array_string = NULL;
}
