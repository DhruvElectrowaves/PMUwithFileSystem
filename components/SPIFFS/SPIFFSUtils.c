#include "../../main/PMUFinalmain.h"
#include "esp_spiffs.h"
#include "esp_log.h"
#include "string.h"
#include "sys/stat.h"

char *extract_action_from_json_response(const char *json_response) {
    cJSON *json = cJSON_Parse(json_response);
    if (!json) {
        ESP_LOGE(SPIFFS_TAG, "Invalid JSON format in response\n");
        return NULL;
    }

    // Extract the UUID, assuming it's the second element in the array
    cJSON *action_item = cJSON_GetArrayItem(json, 2);
    char *action = NULL;

    if (action_item && action_item->valuestring) {
        action = strdup(action_item->valuestring); // Duplicate the string
        if (!action) {
            ESP_LOGE(SPIFFS_TAG, "Memory allocation failed for Action\n");
        }
    } else {
        ESP_LOGE(SPIFFS_TAG, "Action not found in response\n");
    }

    cJSON_Delete(json); // Clean up JSON object
    return action; // Return the duplicated string (or NULL on failure)
}

/*
char* get_action_from_uuid(const char *uuid) {
    FILE *file = fopen(FILEPATH, "r");
    if (!file) {
        ESP_LOGE(SPIFFS_TAG, "Failed to open file for reading\n");
        return NULL;
    }

    char line[TOTAL_LINE];
    int found_match = 0;
    char *action = NULL;

    // Read through the file line-by-line
    while (fgets(line, sizeof(line), file) != NULL) {
        // Trim newline character from the line
        line[strcspn(line, "\n")] = 0;

        // Find the '|' character
        char *separator = strchr(line, '|');
        if (separator) {
            char message[TOTAL_LINE];
            size_t message_length = separator - line;
            strncpy(message, line, message_length);
            message[message_length] = '\0';

            char *request_uuid = extract_uuid_from_json_response(message);

            if (request_uuid) {
                if (strcmp(request_uuid, uuid) == 0) {
                    found_match = 1;
                    ESP_LOGI(SPIFFS_TAG, "Found matching request entry for UUID: %s, getting action name", uuid);
                    action = extract_action_from_json_response(message);
                    free(request_uuid);
                    break;
                }
                free(request_uuid);
            }
        }
    }

    fclose(file);

    if (found_match) {
        ESP_LOGI(SPIFFS_TAG, "Action is %s matching request entry with UUID: %s", action, uuid);
    } else {
        ESP_LOGE(SPIFFS_TAG, "No matching request entry found for UUID: %s", uuid);
    }

    return action;
}
*/

char* search_uuid_in_file(const char *uuid, const char *filePath) {
        FILE *file = fopen(filePath, "r");
        if (!file) {
            ESP_LOGE(SPIFFS_TAG, "Failed to open file: %s for reading", filePath);
            
            return NULL;
        }
        ESP_LOGI(SPIFFS_TAG, "Searching UUID in file: %s", filePath);

        char line[TOTAL_LINE];
        char *action = NULL;

        while (fgets(line, sizeof(line), file) != NULL) {
            line[strcspn(line, "\n")] = 0; // Remove trailing newline

            char *separator = strchr(line, '|');
            if (separator) {
                char message[TOTAL_LINE];
                size_t message_length = separator - line;
                strncpy(message, line, message_length);
                message[message_length] = '\0';

                char *request_uuid = extract_uuid_from_json_response(message);
                if (request_uuid) {
                    if (strcmp(request_uuid, uuid) == 0) {
                        ESP_LOGI(SPIFFS_TAG, "Found UUID in file: %s", filePath);
                        action = extract_action_from_json_response(message);
                        free(request_uuid);
                        fclose(file);
                        
                        return action;
                    }
                    free(request_uuid);
                }
            }
        }

        fclose(file);
    return NULL;
}

char* get_action_from_uuid(const char *uuid) {
    char *action = NULL;

    // Search static files first
    if (fileInfo.request_file_entries > 0) {
        action = search_uuid_in_file(uuid, FILEPATH);
        if (action) return action;
    }
        if (fileInfo.onlineCharger_file_entries > 0) {
        action = search_uuid_in_file(uuid, FAULT_LOG_FILEPATH_OnlineCharger);
        if (action) return action;
    }
    if (fileInfo.offlineCharger_file_entries > 0) {
        action = search_uuid_in_file(uuid, FAULT_LOG_FILEPATH_OfflineCharger);
        if (action) return action;
    }

    // Search dynamic files if available
    if (fileNameConnector != NULL) {
        for (int i = 0; i < chargerConfigMQTT.numConnectors; i++) {
            if (fileNameConnector[i].entries > 0) {
                action = search_uuid_in_file(uuid, fileNameConnector[i].logFilePath);
                if (action) return action;
            }
        }
    }

    ESP_LOGE(SPIFFS_TAG, "getActionFromUUID: No matching request entry found for UUID: %s", uuid);
    return NULL;
}

char* get_queued_request_messages(uint8_t *is_message_important) {
    char *message = malloc(TOTAL_LINE);  // Allocate memory for the message

    if (message == NULL) {
        ESP_LOGE(SPIFFS_TAG, "Failed to allocate memory for message");
        return NULL;  // Return NULL if memory allocation fails
    }

        FILE *file = fopen(FILEPATH, "r");
        if (!file) {
            // ESP_LOGE(SPIFFS_TAG, "No request file found");
            free(message);  // Free the allocated memory
            
            return NULL;    // Return NULL if the file cannot be opened
        }

        // Attempt to read the first line (message|timestamp) from the file
        if (fgets(message, TOTAL_LINE, file) == NULL) {
            fclose(file);  // Close the file before retrying
            //ESP_LOGE(SPIFFS_TAG, "No entries in request file");
            free(message);  // Free the allocated memory
            
            return NULL;  // Return NULL if reading fails
        }

        fclose(file);

    // Newly added code 
    // Trim trailing newline characters if present
    size_t len = strlen(message);
    if (len > 0 && message[len - 1] == '\n') {
        message[len - 1] = '\0';
    }
    
    // Split the message on the '|' character to extract only the message
    char *separator = strchr(message, '|');  // Find the pipe character
    if (separator != NULL) {
        *separator = '\0';  // Replace the pipe with a null terminator to truncate the string
    }

    // Successfully read and truncated the message
    // ESP_LOGI(SPIFFS_TAG, "Read message: %s", message);

    // Check if this is a chargerConfig message
    // if (strstr(message, "\"ConfigurationData\"") != NULL || strstr(message, "\"ChargingSession\"")){ 
    //     *is_message_important = true;
    // }
    // Check message type
    if (strstr(message, "\"ConfigurationData\"") != NULL) { 
        *is_message_important = 1;  // 1 = ConfigurationData
    } 
    else if (strstr(message, "\"ChargingSession\"") != NULL) { 
        *is_message_important = 2;  // 2 = ChargingSession
    } 
    else {
        *is_message_important = 0;  // 0 = Unknown/Other
    }

    return message;  // Return the read message
}

char* get_queued_fault_messages(int num_connectors) {
    if (num_connectors > NO_OF_CONNECTORS) num_connectors = NO_OF_CONNECTORS;

    // Define total files (Static + Dynamic)
    int total_files = 2 + num_connectors;  // Online, OfflineCharger + connectors
    const char *filepaths[total_files];

    // Assign static file paths
    filepaths[0] = FAULT_LOG_FILEPATH_OnlineCharger;
    filepaths[1] = FAULT_LOG_FILEPATH_OfflineCharger;

    // Assign pre-allocated connector file paths
    for (int i = 0; i < num_connectors; i++) {
        filepaths[2 + i] = fileNameConnector[i].logFilePath;
    }

    // Allocate buffer for message storage
    char *message = malloc(TOTAL_LINE);
    if (!message) {
        ESP_LOGE(SPIFFS_TAG, "Failed to allocate memory for messageFault");
        return NULL;
    }

        // Iterate through file paths and check if they have content
        for (int i = 0; i < total_files; i++) {
            struct stat file_stat;
            if (stat(filepaths[i], &file_stat) == 0 && file_stat.st_size > 0) {  
                FILE *file = fopen(filepaths[i], "r");
                if (file) {
                    if (fgets(message, TOTAL_LINE, file) != NULL) {
                        fclose(file);

                        // Trim newline characters
                        size_t len = strlen(message);
                        if (len > 0 && message[len - 1] == '\n') {
                            message[len - 1] = '\0';
                        }

                        // Extract message before timestamp
                        char *separator = strchr(message, '|');
                        if (separator) {
                            *separator = '\0';
                        }

                        // ESP_LOGI(SPIFFS_TAG, "Read message: %s from %s", message, filepaths[i]);
                        
                        return message;  // Return first available message
                    }
                    fclose(file);
                }
            } else {
                //ESP_LOGI(SPIFFS_TAG, "Skipping empty file: %s", filepaths[i]);
            }
        }

    // Cleanup if no message found
    free(message);
    return NULL;
}