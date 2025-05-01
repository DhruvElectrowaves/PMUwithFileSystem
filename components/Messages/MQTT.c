#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../main/PMUFinalmain.h"
#include "../../main/MQTTFunctions.h"
#include "../Operation/FormMessage.h"
#include <cJSON.h>
#include <stdlib.h>
#include "esp_log.h"
#include "../Operation/ValidateMessage.h"
#include "MQTT.h" 
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sys/time.h"
#include "regex.h"

ErrorMessageCodeEnumType error;

//Notify task function
void process_MQTT_msg(void *pvParameters){
    // char msg_string[MSG_STRING_LEN];
    while(1){
        // Wait for the notification from the timer callback
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        char *error_uuid = NULL;
        error = validate_json_string(msg_string, &error_uuid);
        if(error_uuid){
            if(error){
                ESP_LOGE(OCPP_TAG,"Error in received MQTT msg");
                send_json_call_error(error_uuid, error);
            }    
            free(error_uuid);
            error_uuid = NULL;
        }
    }
}

//payload validation for ComponentStatus Request
uint8_t process_ComponentStatus(cJSON *payload, const char *messageId) {
    
    if (payload == NULL) {
        ESP_LOGI(OCPP_TAG, "payload is NULL");
        return COMPONENT_STATUS_PAYLOAD_ERROR; // Invalid payload
    }

    /* Validation */
    {
        // Extract componentId
        cJSON *componentId = cJSON_GetObjectItem(payload, "component");
        if (componentId != NULL && cJSON_IsNumber(componentId)) {
            global_component_value = componentId->valueint;
            printf("Extracted component value: %d\n", global_component_value);
        } else {
            return COMPONENT_STATUS_PAYLOAD_ERROR;
        }

        // Extract id
        cJSON *idItem = cJSON_GetObjectItem(payload, "id");
        if (idItem != NULL && cJSON_IsNumber(idItem)) {
            id_requested = idItem->valueint; // Assuming global_id_value is defined globally
            printf("Extracted id value: %d\n", id_requested);
        } else {
            return COMPONENT_STATUS_PAYLOAD_ERROR;
        }
    }

    create_componentStatus_response(messageId, global_component_value, id_requested);

    return 0; // Success
}

// Function to check if a timestamp is in valid UTC format
uint8_t is_valid_utc_timestamp(const char *timestamp) {
    if (timestamp == NULL) {
        return 0;
    }

    // Regex pattern for UTC format: "YYYY-MM-DDTHH:MM:SSZ"
    const char *pattern = "^[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}Z$";

    regex_t regex;
    if (regcomp(&regex, pattern, REG_EXTENDED) != 0) {
        return 0;  // Regex compilation failed
    }

    int match = regexec(&regex, timestamp, 0, NULL, 0);
    regfree(&regex);

    if (match != 0) {
        return 0;  // Regex validation failed
    }

    //Validate date-time values using strptime()
    struct tm time_info;
    if (strptime(timestamp, "%Y-%m-%dT%H:%M:%SZ", &time_info) == NULL) {
        return 0;
    }
    return 1;
}

//Payload Validation for Configuration Response
uint8_t process_configurationData_payload(cJSON *payload, const char *uuid){
    if (payload == NULL) {
        ESP_LOGE(OCPP_TAG,"payload is NULL");
        return CONFIGURATION_DATA_PAYLOAD_ERROR; // Invalid payload
    }

    // Extract "timestamp"
    cJSON *timestamp = cJSON_GetObjectItem(payload, "timestamp");
    if (!timestamp || !cJSON_IsString(timestamp)) {
        ESP_LOGI("Timestamp","Timestamp key missing or not a string.");
        return CONFIGURATION_DATA_PAYLOAD_ERROR;
    }

   //printf("Timestamp received: %s\n", timestamp->valuestring);

    // Validate timestamp format
    if (is_valid_utc_timestamp(timestamp->valuestring)) {
        ESP_LOGI("Timestamp","Valid TimeStamp");
    } else {
        ESP_LOGI("Timestamp","InValid TimeStamp");
        return CONFIGURATION_DATA_PAYLOAD_ERROR;
    }
    
    // Action - SaveData to NVS 
    if(publishMsgFlag.configDataExchanged != 1 && strcmp(configDataBootUUID, uuid) == 0){
        // uint8_t flag = 1;
        // save_data_to_NVS(CHARGERCONFIGKEYS, SEND_CONFIG_FLAG_KEY, &flag, TYPE_U8);
        publishMsgFlag.configDataExchanged = 1;
    }
    valueofChangedParameter = 0;  
    return 0;
}

//Payload Validation for periodicData Response
uint8_t process_periodicData_payload(cJSON *payload){
    if (payload == NULL) {
        ESP_LOGE(OCPP_TAG,"payload is NULL");
        return PERIODIC_DATA_PAYLOAD_ERROR; // Invalid payload
    }

    // Extract "timestamp"
    cJSON *timestamp = cJSON_GetObjectItem(payload, "timestamp");
    if (!timestamp || !cJSON_IsString(timestamp)) {
        ESP_LOGI("Timestamp","Timestamp key missing or not a string.");
        return PERIODIC_DATA_PAYLOAD_ERROR;
    }

    // Validate timestamp format
    if (is_valid_utc_timestamp(timestamp->valuestring)) {
        ESP_LOGI("Timestamp","Valid TimeStamp");
    } else {
        ESP_LOGI("Timestamp","InValid TimeStamp");
        return PERIODIC_DATA_PAYLOAD_ERROR;
    } 
    return 0;
}

//Payload Validation for chargingSession Response
uint8_t process_chargingSession_payload(cJSON *payload){
    if (payload == NULL) {
        ESP_LOGE(OCPP_TAG,"payload is NULL");
        return CHARGING_SESSION_PAYLOAD_ERROR; // Invalid payload
    }

    // Extract "timestamp"
    cJSON *timestamp = cJSON_GetObjectItem(payload, "timestamp");
    if (!timestamp || !cJSON_IsString(timestamp)) {
        ESP_LOGI("Timestamp","Timestamp key missing or not a string.");
        return CHARGING_SESSION_PAYLOAD_ERROR;
    }

    // Validate timestamp format
    if (is_valid_utc_timestamp(timestamp->valuestring)) {
        ESP_LOGI("Timestamp","Valid TimeStamp");
    } else {
        ESP_LOGI("Timestamp","InValid TimeStamp");
        return CHARGING_SESSION_PAYLOAD_ERROR;
    } 
    return 0;
}

//Payload Validation for FaultLog Response

uint8_t process_faultLog_payload(cJSON *payload){
    if (payload == NULL) {
        ESP_LOGE(OCPP_TAG,"payload is NULL");
        return FAULTLOG_PAYLOAD_ERROR; // Invalid payload
    }

    // Extract "timestamp"
    cJSON *timestamp = cJSON_GetObjectItem(payload, "timestamp");
    if (!timestamp || !cJSON_IsString(timestamp)) {
        ESP_LOGI("Timestamp","Timestamp key missing or not a string.");
        return FAULTLOG_PAYLOAD_ERROR;
    }

    // Validate timestamp format
    if (is_valid_utc_timestamp(timestamp->valuestring)) {
        ESP_LOGI("Timestamp","Valid TimeStamp");
    } else {
        ESP_LOGI("Timestamp","InValid TimeStamp");
        return FAULTLOG_PAYLOAD_ERROR;
    } 
    return 0;
}