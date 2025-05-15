#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ValidateMessage.h"
#include <cJSON.h>
#include <stdlib.h>
#include "esp_log.h"
#include "../Messages/MQTT.h"
#include "../../main/PMUFinalmain.h"
#include "../Messages/MQTT.h"

ErrorMessageCodeEnumType validate_json_string(char *msg, char **error_uuid){ 
    
    ESP_LOGI("Response", LOG_COLOR_PINK "The String of response %s" LOG_RESET_COLOR, msg);
    // Parse the JSON string
    cJSON *json_array = cJSON_Parse(msg);
    if (json_array == NULL) {
        ESP_LOGE(OCPP_TAG,"Invalid JSON format.\n");
        return INVALID_JSON_FORMAT;
    }
    else {
        char *printed_json = cJSON_Print(json_array);
        free(printed_json);  // Free memory allocated by cJSON_Print
    }

    // Check if the parsed JSON is an array
    if (!cJSON_IsArray(json_array)) {
        ESP_LOGE(OCPP_TAG,"The JSON object is not an array.\n");
        cJSON_Delete(json_array);
        return JSON_ARRAY_NOT_FOUND; // Return 0 if not an array
    }

    // Count the number of elements in the array
    uint8_t count = cJSON_GetArraySize(json_array);

    // Validate the count
    if (count < 3 || count > 5) {
        ESP_LOGI(OCPP_TAG,"The number of elements is out of the valid range (3 to 5).\n");
        cJSON_Delete(json_array);
        return JSON_ARRAY_ELEMENTS_OUT_OF_RANGE; // Return 0 if count is not in range
    }
    ErrorMessageCodeEnumType result = INVALID_MSG;

    switch(count){
        case CALL_ : {
            // Get elements from the JSON array
            cJSON *message_type_id = cJSON_GetArrayItem(json_array, 0);
            cJSON *message_id = cJSON_GetArrayItem(json_array, 1);
            cJSON *action = cJSON_GetArrayItem(json_array, 2);
            cJSON *payload = cJSON_GetArrayItem(json_array, 3);

            // Check if MessageTypeId matches CALL
            if (!cJSON_IsNumber(message_type_id) || message_type_id->valueint != CALL) {
                ESP_LOGE(OCPP_TAG,"The MessageTypeId must be %d for a CALL message.\n", CALL);
                cJSON_Delete(json_array);
                return INVALID_MESSAGE_TYPE_ID;
            }

            // Validate MessageId
            if (!cJSON_IsString(message_id) || (strlen(message_id->valuestring) > 36)) {
                ESP_LOGE(OCPP_TAG,"The second element must be a string (MessageId) with a maximum length of 36 characters.\n");
                cJSON_Delete(json_array);
                return INVALID_MESSAGE_ID;
            } else {
                *error_uuid = strdup(message_id->valuestring);
            }

            // Validate Action
            if (!cJSON_IsString(action) || (strlen(action->valuestring) == 0)) {
                ESP_LOGE(OCPP_TAG,"The third element must be a non-empty string (Action).\n");
                cJSON_Delete(json_array);
                return INVALID_ACTION;
            }

            // Validate Payload
            if (!cJSON_IsObject(payload)) {
                ESP_LOGE(OCPP_TAG,"The fourth element must be a JSON object (Payload).\n");
                cJSON_Delete(json_array);
                return INVALID_PAYLOAD;
            }

            // Additional logic for CALL can be implemented here based on Action and Payload if needed
            ESP_LOGI(OCPP_TAG,"Valid CALL message received: %s\n", action->valuestring);

           // if(lcu_config.ocpp_protocol == V_16){
                if(!(strcmp(action->valuestring,"ComponentStatus")))
                {
                    printf("In componentStatus next step validation payload\n");
                    result = process_ComponentStatus(payload, message_id->valuestring);
                }
            break;
        }
      case CALL_RESULT_ : {
            // Get elements from the JSON array
            cJSON *message_type_id = cJSON_GetArrayItem(json_array, 0);
            cJSON *message_id = cJSON_GetArrayItem(json_array, 1);
            cJSON *payload = cJSON_GetArrayItem(json_array, 2);

            // Check if MessageTypeId matches CALL
            if (!cJSON_IsNumber(message_type_id) || message_type_id->valueint != CALL_RESULT) {
                ESP_LOGE(OCPP_TAG,"The MessageTypeId must be %d for a CALL message.\n", CALL_RESULT);
                cJSON_Delete(json_array);
                return INVALID_MESSAGE_TYPE_ID;
            }

            // Validate MessageId
            if (!cJSON_IsString(message_id) || (strlen(message_id->valuestring) > 36)) {
                ESP_LOGE(OCPP_TAG,"The second element must be a string (MessageId) with a maximum length of 36 characters.\n");
                cJSON_Delete(json_array);
                return INVALID_MESSAGE_ID;
            } else {
                *error_uuid = strdup(message_id->valuestring);
            }
            
            char *action = get_action_from_uuid(message_id->valuestring); //to be decided the implementation later on, current it is anylsed from file in ocpp
            if(action != NULL)
                ESP_LOGW("ACTION", "Action is %s", action);
            else 
                ESP_LOGW("ACTION", "Action is NULL");
                
            if(action == NULL){
                ESP_LOGE(OCPP_TAG,"No matching Request found for this Response");
                cJSON_Delete(json_array);
                return 0;
            }

            // Validate Payload
            if (!cJSON_IsObject(payload)) {
                ESP_LOGE(OCPP_TAG,"The fourth element must be a JSON object (Payload).\n");
                cJSON_Delete(json_array);
                return INVALID_PAYLOAD;
            }

            if(!(strcmp(action,"ConfigurationData")))
            {
                ESP_LOGI(OCPP_TAG, "In ConfigurationData next step validation payload call Result\n");
                result = process_configurationData_payload(payload, message_id->valuestring);
            }
            else if(!(strcmp(action,"ChargerPeriodicData")))
            {
                ESP_LOGI(OCPP_TAG, "In ChargerPeriodicData next step validation payload call Result\n");
                result = process_periodicData_payload(payload);
            }
            else if(!(strcmp(action,"PCMPeriodicData")))
            {
                ESP_LOGI(OCPP_TAG, "In PCMPeriodicData next step validation payload call Result\n");
                result = process_periodicData_payload(payload);
            }
            else if(!(strcmp(action,"ChargingSession")))
            {
                ESP_LOGI(OCPP_TAG, "In ChargingSession next step validation payload call Result\n");
                result = process_chargingSession_payload(payload);
                // responseReceived.chargingSessionResponse = true;
            }
            else if(!(strcmp(action,"FaultInformation")))
            {
                result = process_faultLog_payload(payload);
                // responseReceived.faultInfoResponse = true;
            }
            // Deleting entry in req.txt file corresponding to the Response message UUID
            // if(!(strcmp(action,"ConfigurationData") == 0 && result != 0) || !(strcmp(action,"ChargingSession") == 0 && result != 0))
            if (!((strcmp(action, "ConfigurationData") == 0 && result != 0) || (strcmp(action, "ChargingSession") == 0 && result != 0) || (strcmp(action, "FaultInformation") == 0 && result != 0)))
            {
                if(strcmp(action, "FaultInformation") != 0)
                {
                    // responseReceived.chargingSessionResponse = false;
                    int count = handle_response_message(message_id->valuestring);
                    if(count != -1)
                        fileInfo.request_file_entries = count;
                }
                else{
                    // responseReceived.faultInfoResponse = false;
                    handle_fault_response_message(message_id->valuestring);
                }   
            }
            break;
        }

        case CALL_ERROR_ : {
            // Expected format: [4, "<UniqueId>", "<errorCode>", "<errorDescription>", { <errorDetails> }]
            cJSON *message_type_id = cJSON_GetArrayItem(json_array, 0);
            cJSON *message_id = cJSON_GetArrayItem(json_array, 1);
            cJSON *error_code = cJSON_GetArrayItem(json_array, 2);
            cJSON *error_description = cJSON_GetArrayItem(json_array, 3);
            cJSON *error_details = cJSON_GetArrayItem(json_array, 4);

            // Validate MessageTypeId
            if (!cJSON_IsNumber(message_type_id) || message_type_id->valueint != CALL_ERROR) {
                ESP_LOGE(OCPP_TAG, "The MessageTypeId must be %d for a CALL_ERROR message.\n", CALL_ERROR);
                cJSON_Delete(json_array);
                return INVALID_MESSAGE_TYPE_ID;
            }

            // Validate MessageId
            if (!cJSON_IsString(message_id) || (strlen(message_id->valuestring) > 36)) {
                ESP_LOGE(OCPP_TAG, "The second element must be a string (MessageId) with a maximum length of 36 characters.\n");
                cJSON_Delete(json_array);
                return INVALID_MESSAGE_ID;
            } else {
                *error_uuid = strdup(message_id->valuestring);
            }

            // Validate ErrorCode and Description
            if (!cJSON_IsString(error_code) || !cJSON_IsString(error_description)) {
                ESP_LOGE(OCPP_TAG, "ErrorCode and ErrorDescription must be strings.\n");
                cJSON_Delete(json_array);
                return INVALID_CALL_ERROR_FORMAT;
            }

            char *action = get_action_from_uuid(message_id->valuestring); //to be decided the implementation later on, current it is anylsed from file in ocpp
            if(action != NULL)
                ESP_LOGW("ACTION", "Action is %s", action);
            else 
                ESP_LOGW("ACTION", "Action is NULL");
                
            if(action == NULL){
                ESP_LOGE(OCPP_TAG,"No matching Request found for this Response");
                cJSON_Delete(json_array);
                return 0;
            }

            ESP_LOGE(OCPP_TAG, "CALL_ERROR received:\n  UUID: %s\n  Code: %s\n  Description: %s",
                    message_id->valuestring,
                    error_code->valuestring,
                    error_description->valuestring);

            if (cJSON_IsObject(error_details)) {
                char *error_detail_str = cJSON_PrintUnformatted(error_details);
                if (error_detail_str) {
                    ESP_LOGE(OCPP_TAG, "  Details: %s", error_detail_str);
                    free(error_detail_str);
                }
            }

            // Optional: Clear request from tracking file if needed
            if((strcmp(action,"FaultInformation")) == 0)
            {
                handle_fault_response_message(message_id->valuestring);
                // if (count != -1)
                //     fileInfo.request_file_entries = count;
            }
            else if((strcmp(action,"ChargingSession")) == 0)
            {
                handle_response_message(message_id->valuestring);
                if (count != -1)
                    fileInfo.request_file_entries = count;
            }
            break;
        }

        default : 
        {
            return INVALID_MESSAGE_TYPE_ID;
            break;
        }
    }

    // Cleanup and return success
    cJSON_Delete(json_array);
    return result; 
}