#include <string.h>
#include <stdio.h>
#include "esp_log.h"
#include "cJSON.h" 
#include "../../main/PMUFinalmain.h"


fileStruct_t fileInfo = {0};

void delete_entry(const char *msg){
    char *request_uuid = extract_uuid_from_json_response(msg);
    if (request_uuid) {
        //ESP_LOGI("msg uuid", "%s", request_uuid);
        int count = handle_response_message(request_uuid);
        if(count != -1){
            fileInfo.request_file_entries = count;
        }
        if(request_uuid){
            free(request_uuid);
            request_uuid = NULL;
        }
    }

    // if (xSemaphoreTake(mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
    //     fileInfo.totalEntries = fileInfo.request_file_entries 
    //                            + fileInfo.onlineCharger_file_entries 
    //                            + fileInfo.offlineCharger_file_entries 
    //                            + fileNameConnector[0].entries 
    //                            + fileNameConnector[1].entries;
    //     xSemaphoreGive(mutex);                          // Very important!
    // }
    // else {
    //     ESP_LOGE("SPIFFS", "Failed to take Mutex for totalEntries update (in delete_entry)");
    // }
}

char *extract_uuid_from_json_response(const char *json_response) {
    cJSON *json = cJSON_Parse(json_response);
    if (!json) {
        ESP_LOGE(SPIFFS_TAG, "Invalid JSON format in response\n");
        return NULL;
    }

    // Extract the UUID, assuming it's the second element in the array
    cJSON *uuid_item = cJSON_GetArrayItem(json, 1);
    char *request_uuid = NULL;

    if (uuid_item && uuid_item->valuestring) {
        request_uuid = strdup(uuid_item->valuestring); // Duplicate the string
        if (!request_uuid) {
            ESP_LOGE(SPIFFS_TAG, "Memory allocation failed for UUID\n");
        }
    } else {
        ESP_LOGE(SPIFFS_TAG, "UUID not found in response\n");
    }

    cJSON_Delete(json); // Clean up JSON object
    return request_uuid; // Return the duplicated string (or NULL on failure)
}

int handle_response_message(const char *response_uuid) {
    int entry_count = 0;  // Initialize the entry count
        FILE *file = fopen(FILEPATH, "r");
        if (!file) {
            ESP_LOGE(SPIFFS_TAG, "Failed to open file for reading\n");
            
            return -1;  // Indicate error
        }

        //create a temporary file
        FILE *temp_file = fopen(TEMPFILEPATH, "w");
        if (!temp_file) {
            ESP_LOGE(SPIFFS_TAG, "Failed to open temp file for writing\n");
            fclose(file);
            
            return -1;  // Indicate error
        }

        //create buffer according to the request message length
        char line[DEQUE_MSG_BUFFER];
        int found_match = 0;

        // Read through the file line-by-line
        while (fgets(line, sizeof(line), file) != NULL) {
            // Trim newline character from the line
            line[strcspn(line, "\n")] = 0;

            // ESP_LOGI(SPIFFS_TAG, "Line extracted from text file: %s", line);

            // Increment entry count for each line
            entry_count++;

            // Find the '|' character
            char *separator = strchr(line, '|');
            if (separator) {
                char message[DEQUE_MSG_BUFFER - 100];
                size_t message_length = separator - line;
                strncpy(message, line, message_length);
                message[message_length] = '\0'; 
                // Print the message
                // ESP_LOGI(SPIFFS_TAG, "Message in text file: %s", message);

                char *request_uuid = extract_uuid_from_json_response(message);
                if (request_uuid != NULL) {
                    // Ensure the string is null-terminated
                    size_t uuid_len = strlen(request_uuid);
                    request_uuid[uuid_len] = '\0'; // Add null terminator
                    // ESP_LOGI(SPIFFS_TAG, "Request UUID: %s", request_uuid);
                } else {
                    ESP_LOGE(SPIFFS_TAG, "Request UUID is NULL");
                }

                if (request_uuid) {
                    if (strcmp(request_uuid, response_uuid) == 0) {
                        found_match = 1;
                        // ESP_LOGI(SPIFFS_TAG, "Found matching request entry for UUID: %s, removing entry.", response_uuid);
                        free(request_uuid);
                        entry_count--;  // Decrement count since this entry is being removed
                        continue;  // Skip writing this line to temp file to delete it
                    }

                    free(request_uuid);
                    request_uuid = NULL;
                }
            }

            // Write the line to the temp file if no match
            fputs(line, temp_file);
            fputs("\n", temp_file);
        }

        fclose(file);
        fclose(temp_file);

        // Replace the original file with the temp file
        remove(FILEPATH);
        rename(TEMPFILEPATH, FILEPATH);

        if (found_match) {
            ESP_LOGI(SPIFFS_TAG, "Deleted matching request entry with UUID: %s", response_uuid);
        } else {
            ESP_LOGE(SPIFFS_TAG, "HandleResponseMessage: No matching request entry found for UUID: %s", response_uuid);
        }
        // print_file_entries(FILEPATH);
    return entry_count;  // Return the total number of entries
}

int delete_fault_file_entry_by_uuid(const char *filename, const char *response_uuid, bool *found_match){
    int entry_count = 0;  // Initialize the entry count
        FILE *file = fopen(filename, "r");
            if (!file) {
                ESP_LOGE(SPIFFS_TAG, "Failed to open file for reading\n");
                
                return -1;  // Indicate error
            }

            //create a temporary file
            FILE *temp_file = fopen(TEMP_FAULT_LOG_FILEPATH, "w");
            if (!temp_file) {
                ESP_LOGE(SPIFFS_TAG, "Failed to open temp file for writing\n");
                fclose(file);
                
                return -1;  // Indicate error
            }

            //create buffer according to the request message length
            char line[DEQUE_MSG_BUFFER_FAULT];
            *found_match = false;  // Ensure it's set to false initially

            // Read through the file line-by-line
            while (fgets(line, sizeof(line), file) != NULL) {
                // Trim newline character from the line
                line[strcspn(line, "\n")] = 0;

                // ESP_LOGI(SPIFFS_TAG, "Line extracted from text file: %s", line);

                // Increment entry count for each line
                entry_count++;

                // Find the '|' character
                char *separator = strchr(line, '|');
                if (separator) {
                    char message[DEQUE_MSG_BUFFER - 100];
                    size_t message_length = separator - line;
                    strncpy(message, line, message_length);
                    message[message_length] = '\0'; 
                    // Print the message
                    // ESP_LOGI(SPIFFS_TAG, "Message in text file: %s", message);

                    char *request_uuid = extract_uuid_from_json_response(message);
                    if (request_uuid != NULL) {
                        // Ensure the string is null-terminated
                        size_t uuid_len = strlen(request_uuid);
                        request_uuid[uuid_len] = '\0'; // Add null terminator
                        // ESP_LOGI(SPIFFS_TAG, "Request UUID: %s", request_uuid);
                    } else {
                        ESP_LOGE(SPIFFS_TAG, "Request UUID is NULL");
                    }

                    if (request_uuid) {
                        if (strcmp(request_uuid, response_uuid) == 0) {
                            *found_match = true; 
                            // ESP_LOGI(SPIFFS_TAG, "Found matching request entry for UUID: %s, removing entry.", response_uuid);
                            free(request_uuid);
                            entry_count--;  // Decrement count since this entry is being removed
                            continue;  // Skip writing this line to temp file to delete it
                        }

                        free(request_uuid);
                        request_uuid = NULL;
                    }
                }

                // Write the line to the temp file if no match
                fputs(line, temp_file);
                fputs("\n", temp_file);
            }

            fclose(file);
            fclose(temp_file);

            // Replace the original file with the temp file
            remove(filename);
            rename(TEMP_FAULT_LOG_FILEPATH, filename);

            if (found_match) {
                ESP_LOGI(SPIFFS_TAG, "Deleted matching request entry with UUID: %s", response_uuid);
            } else {
                ESP_LOGE(SPIFFS_TAG, "Inside Delete Fault No matching request entry found for UUID: %s", response_uuid);
            }
            print_file_entries(filename);
    return entry_count;  // Return the total number of entries
}

void handle_fault_response_message(const char *response_uuid) 
{
    bool found_match = false;

    if(fileInfo.onlineCharger_file_entries)
    {
        int count = delete_fault_file_entry_by_uuid(FAULT_LOG_FILEPATH_OnlineCharger, response_uuid, &found_match);
        if(count != -1){
            fileInfo.onlineCharger_file_entries = count;
            ESP_LOGW("FAULT_HANDLER", "Remaining Online Charger Fault Entries: %d", fileInfo.onlineCharger_file_entries);
        }   
    }
    if(fileInfo.offlineCharger_file_entries && !found_match)
    {
        int count = delete_fault_file_entry_by_uuid(FAULT_LOG_FILEPATH_OfflineCharger, response_uuid, &found_match);
        if(count != -1)
        {
            fileInfo.offlineCharger_file_entries = count;
            ESP_LOGW("FAULT_HANDLER", "Remaining Offline Charger Fault Entries: %d", fileInfo.offlineCharger_file_entries);
        }
    }
    for(int i=0; i<chargerConfigMQTT.numConnectors; i++)
    {
        if(fileNameConnector[i].entries && !found_match)
        {
            int count = delete_fault_file_entry_by_uuid(fileNameConnector[i].logFilePath, response_uuid, &found_match);
            if(count != -1)
            {
                fileNameConnector[i].entries = count;
                ESP_LOGW("FAULT_HANDLER", "Remaining Entries for Connector %d: %d", i + 1, fileNameConnector[i].entries);
            }
        }
    }
    //fileInfo.totalEntries = fileInfo.request_file_entries+fileInfo.onlineCharger_file_entries+fileInfo.offlineCharger_file_entries+fileNameConnector[0].entries+fileNameConnector[1].entries; 
    
    // if (xSemaphoreTake(mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
    //     fileInfo.totalEntries = fileInfo.request_file_entries 
    //                            + fileInfo.onlineCharger_file_entries 
    //                            + fileInfo.offlineCharger_file_entries 
    //                            + fileNameConnector[0].entries 
    //                            + fileNameConnector[1].entries;
    //     xSemaphoreGive(mutex);                          // Very important!
    // }
    // else {
    //     ESP_LOGE("SPIFFS", "Failed to take Mutex for totalEntries update (in handle_fault_response_message)");
    // }
}

