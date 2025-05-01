#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "FormMessage.h"
#include <cJSON.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h> // Required for gettimeofday
#include "esp_log.h"
#include "ValidateMessage.h"
#include "../../main/PMUFinalmain.h"
#include "mqtt_client.h"

//#include "ChargingSessionService.h"
// #define OCPP_TAG "Hi"
#define V_16 1

char configDataBootUUID[37];
// Function to seed only once with millisecond precision
void seedOnce() {
    static int seeded = 0;
    if (!seeded) {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        // Combine seconds and microseconds for a more precise seed
        srand((unsigned int)(tv.tv_sec ^ tv.tv_usec));
        seeded = 1;
    }
}

void generateUUID(char *uuid) {
    seedOnce();  // Call seed function to ensure it seeds only once

    const char *chars = "0123456789abcdef";

    // Fill in the uuid string
    for (int i = 0; i < 36; i++) {
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            uuid[i] = '-';  // Hyphen separators
        } else if (i == 14) {
            uuid[i] = '4';  // Version 4
        } else if (i == 19) {
            uuid[i] = "89ab"[rand() % 4];  // Variant (8, 9, A, or B)
        } else {
            uuid[i] = chars[rand() % 16];  // Random hex digit
        }
    }

    // Null-terminate the string
    uuid[36] = '\0';
}

char* create_json_call_string(const char *action,  cJSON *payload) {
    // Create a new JSON array
    cJSON *jsonArray = cJSON_CreateArray();
    if (!jsonArray) {
        return NULL;  // Memory allocation failed
    }

    //Add the message type ID as an array element
    cJSON_AddItemToArray(jsonArray, cJSON_CreateNumber(2));

    //Add the message ID as an array element
    char uuid[37];
    generateUUID(uuid);
    cJSON_AddItemToArray(jsonArray, cJSON_CreateString(uuid)); // Here, you can implement your message ID logic
    ESP_LOGI(OCPP_TAG,"Generated UUID: %s\n", uuid);

    //if using for other actions then add action in if condition as well
    if (firstTimeBootUpFlag == 3 && strcmp(action, "ConfigurationData") == 0)
    {
        strcpy(configDataBootUUID, uuid);
        firstTimeBootUpFlag = 0;
    }
    //Add the action as an array element
    cJSON_AddItemToArray(jsonArray, cJSON_CreateString(action));
    
    //Compare Action -- can be added 
    
    // Add the JSON payload object directly as an array element
    if (payload) {
        // Duplicate the payload to ensure the caller retains ownership of the original object
        cJSON_AddItemToArray(jsonArray, cJSON_Duplicate(payload, 1));
    } else {
        cJSON_Delete(jsonArray);  // Cleanup if payload is NULL
        return NULL;
    }
    // Convert the JSON array to a string
    char *jsonString = cJSON_PrintUnformatted(jsonArray);
    cJSON_Delete(jsonArray);  // Cleanup the JSON array after printing
    
    if (jsonString != NULL) {
        //ESP_LOGI(OCPP_TAG, "Processing message: %s", jsonString);
    }
    return jsonString;         // Return the JSON string
}

/*char* create_pcmPeriodic_payload_ocpp_string(const char *action,  cJSON *payload) {
    // Create a new JSON array
    cJSON *jsonArray = cJSON_CreateArray();
    if (!jsonArray) {
        return NULL;  // Memory allocation failed
    }

    //Add the message type ID as an array element
    cJSON_AddItemToArray(jsonArray, cJSON_CreateNumber(2));

    //Add the message ID as an array element
    char uuid[37];
    generateUUID(uuid);
    cJSON_AddItemToArray(jsonArray, cJSON_CreateString(uuid)); // Here, you can implement your message ID logic
    ESP_LOGI(OCPP_TAG,"Generated UUID: %s\n", uuid);

    //Add the action as an array element
    cJSON_AddItemToArray(jsonArray, cJSON_CreateString(action));
    
    //Compare Action -- can be added 
    
    if (payload) {
        // Duplicate the payload to ensure the caller retains ownership of the original object
        cJSON_AddItemToArray(jsonArray, cJSON_Duplicate(payload, 1));
    } else {
        cJSON_Delete(jsonArray);  // Cleanup if payload is NULL
        return NULL;
    }
    // Convert the JSON array to a string
    char *jsonPCMString = cJSON_PrintUnformatted(jsonArray);
    cJSON_Delete(jsonArray);  // Cleanup the JSON array after printing
    

    if (jsonPCMString != NULL) {
        ESP_LOGI(OCPP_TAG, "Processing your message:");
    }

    //Storing the File in SPIFFS System 
    //save_payload_to_spiffs("/spiffs/PCM_Periodic.json", jsonPCMString);
    return jsonPCMString;
}*/

/*char* create_config_payload_ocpp_string(const char *action,  cJSON *payload) {
    // Create a new JSON array
    cJSON *jsonArray = cJSON_CreateArray();
    if (!jsonArray) {
        return NULL;  // Memory allocation failed
    }

    //Add the message type ID as an array element
    cJSON_AddItemToArray(jsonArray, cJSON_CreateNumber(2));

    //Add the message ID as an array element
    char uuid[37];
    generateUUID(uuid);
    cJSON_AddItemToArray(jsonArray, cJSON_CreateString(uuid)); // Here, you can implement your message ID logic
    ESP_LOGI(OCPP_TAG,"Generated UUID: %s\n", uuid);

    //if using for other actions then add action in if condition as well
    if(firstTimeBootUpFlag == 3)
    {
        strcpy(configDataBootUUID, uuid);
        firstTimeBootUpFlag = 0;
    }
    //Add the action as an array element
    cJSON_AddItemToArray(jsonArray, cJSON_CreateString(action));
    
    //Compare Action -- can be added 
    
    //Add the payload string directly as an array element
    if (payload) {
        // Duplicate the payload to ensure the caller retains ownership of the original object
        cJSON_AddItemToArray(jsonArray, cJSON_Duplicate(payload, 1));
    } else {
        cJSON_Delete(jsonArray);  // Cleanup if payload is NULL
        return NULL;
    }
    // Convert the JSON array to a string
    char *jsonString = cJSON_PrintUnformatted(jsonArray);
    cJSON_Delete(jsonArray);  // Cleanup the JSON array after printing
    

    if (jsonString != NULL) {
        ESP_LOGI(OCPP_TAG, "Processing your message:");
    }
    return jsonString;
}*/

//----------------------for response--------------------
char* create_json_call_result_string(const char * messageId, cJSON *payload) {
    // Create a new JSON array
    cJSON *jsonArray = cJSON_CreateArray();
    if (!jsonArray) {
        return NULL;  // Memory allocation failed
    }

    // Add the message type ID as an array element
    cJSON_AddItemToArray(jsonArray, cJSON_CreateNumber(3));

    // Add the message type ID as an array element
    cJSON_AddItemToArray(jsonArray, cJSON_CreateString(messageId));

    // Add the payload string directly as an array element
    
    
    cJSON_AddItemToArray(jsonArray, payload); // Add the parsed payload as an element
   

    // Convert the JSON array to a string
    char *jsonString = cJSON_PrintUnformatted(jsonArray);
    cJSON_Delete(jsonArray);  // Cleanup the JSON array after printing
    return jsonString;         // Return the JSON string
}

const char* get_error(ErrorMessageCodeEnumType error_code) {
    switch (error_code) {
        case JSON_ARRAY_NOT_FOUND:
            return "ProtocolError";
        case INVALID_JSON_FORMAT:
            return "FormationViolation";
        case JSON_ARRAY_ELEMENTS_OUT_OF_RANGE:
            return "FormationViolation";
        case INVALID_MESSAGE_TYPE_ID:
            return "ProtocolError";
        case INVALID_MESSAGE_ID:
            return "ProtocolError";
        case INVALID_ACTION:
            return "InvalidAction";
        case INVALID_PAYLOAD:
            return "FormationViolation";
        case COMPONENT_STATUS_PAYLOAD_ERROR:
        case PERIODIC_DATA_PAYLOAD_ERROR:
        case CONFIGURATION_DATA_PAYLOAD_ERROR:
        case CHARGING_SESSION_PAYLOAD_ERROR:
            return "FormationViolation";
        case REQ_RES_PAIR_NOT_FOUND:
            return "NotSupported";
        case NOT_SUPPORTED:
            return "NotSupported";
        case ID_NOT_SUPPORTED:
            return "IDNotSupported";
        case COMPONENT_NOT_SUPPORTED:
            return "ComponentNotSupported";
        case INVALID_MSG:
            return "InvalidMsg";
        default:
            return "GenericError";
    }
}

void send_json_call_error(const char *messageId, ErrorMessageCodeEnumType error_code) {
    // Create a new JSON array
    cJSON *jsonArray = cJSON_CreateArray();
    if (!jsonArray) {
        return;  // Memory allocation failed
    }

    // Add the message type ID as an array element
    cJSON_AddItemToArray(jsonArray, cJSON_CreateNumber(4));

    // Add the message type ID as an array element
    cJSON_AddItemToArray(jsonArray, cJSON_CreateString(messageId));

    // Add the error code as an array element
    cJSON_AddItemToArray(jsonArray, cJSON_CreateString(get_error(error_code)));

    // Add the error code description empty as an array element
    cJSON_AddItemToArray(jsonArray, cJSON_CreateString(""));

    // Add the ErrorDetails empty object as an array element
    cJSON_AddItemToArray(jsonArray, cJSON_CreateObject());

    // Convert the JSON array to a string
    char *jsonString = cJSON_PrintUnformatted(jsonArray);
    if (jsonString) {

        //publish to broker
        if (IS_MQTT_NETWORK_ONLINE){
            // esp_mqtt_client_publish(mqtt_client, pubTopic, jsonString, 0, 1, 0);
            if (esp_mqtt_client_publish(mqtt_client, pubTopic, jsonString, 0, 1, 0) != -1) 
            {
                printf("Message send successfully");
            }
        }
        free(jsonString);
        jsonString = NULL;
    }
    cJSON_Delete(jsonArray);  // Cleanup the JSON array after printing
}