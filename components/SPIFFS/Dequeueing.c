#include <string.h>
#include <stdio.h>
#include "esp_log.h"
#include "cJSON.h" 
#include "../../main/PMUFinalmain.h"
#include "sys/stat.h"
#define TAG "SPIFFS_DELETE"

fileStruct_t fileInfo = {0};
//basic function to delete a line from a file in SPIFFS
/*void delete_line_from_spiffs(const char *target_string) {
    FILE *src = fopen(FILE_PATH, "r");
    if (src == NULL) {
        ESP_LOGE(TAG, "Failed to open source file");
        return;
    }

    FILE *temp = fopen(TEMP_PATH, "w");
    if (temp == NULL) {
        ESP_LOGE(TAG, "Failed to open temp file");
        fclose(src);
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), src)) {
        // Trim newline for comparison
        line[strcspn(line, "\r\n")] = 0;

        if (strcmp(line, target_string) != 0) {
            fprintf(temp, "%s\n", line);  // Write back non-matching line
        } else {
            ESP_LOGI(TAG, "Deleted line: %s", line);
        }
    }

    fclose(src);
    fclose(temp);

    // Remove original file and rename temp
    remove(FILE_PATH);
    rename(TEMP_PATH, FILE_PATH);
}*/

/*bool delete_line_if_found(const char *filepath, const char *target) {
    char temp_path[128];
    if (strcmp(filepath, FILEPATH) == 0){
        strcpy(temp_path, TEMPFILEPATH);
    }
    else{
        strcpy(temp_path, TEMP_FAULT_LOG_FILEPATH);
    }
    // snprintf(temp_path, sizeof(temp_path), "%s.temp", filepath);

    FILE *src = fopen(filepath, "r");
    if (!src) {
        ESP_LOGW(TAG, "Cannot open file: %s", filepath);
        return NULL;
    }

    FILE *temp = fopen(temp_path, "w");
    if (!temp) {
        ESP_LOGE(TAG, "Cannot create temp file: %s", temp_path);
        fclose(src);
        return false;
    }

    char line[256];
    bool deleted = false;

    while (fgets(line, sizeof(line), src)) {
        // Strip newline
        line[strcspn(line, "\r\n")] = 0;

        if (strcmp(line, target) == 0) {
            ESP_LOGI(TAG, "Message deleted from: %s", filepath);
            deleted = true;
            continue;  // skip writing this line
        }

        fprintf(temp, "%s\n", line);
    }

    fclose(src);
    fclose(temp);

    if (deleted) {
        remove(filepath);
        rename(temp_path, filepath);
    } else {
        remove(temp_path);  // cleanup
    }

    return deleted;
}

bool search_and_delete_message_from_any_file(const char *target_message) {
    int num_connectors = chargerConfigMQTT.numConnectors;
    if (num_connectors > NO_OF_CONNECTORS) num_connectors = NO_OF_CONNECTORS;

    int total_files = 2 + num_connectors;
    const char *filepaths[total_files];

    // Static filepaths
    filepaths[0] = FAULT_LOG_FILEPATH_OnlineCharger;
    filepaths[1] = FAULT_LOG_FILEPATH_OfflineCharger;

    // Connector filepaths
    for (int i = 0; i < num_connectors; i++) {
        filepaths[2 + i] = fileNameConnector[i].logFilePath;
    }

    // Search and delete in each non-empty file
    bool found = false;
    for (int i = 0; i < total_files; i++) {
        struct stat file_stat;
        if (stat(filepaths[i], &file_stat) == 0 && file_stat.st_size > 0) {
            if (delete_line_if_found(filepaths[i], target_message)) {
                found = true;
            }
        } else {
            //ESP_LOGI(TAG, "Skipping empty or missing file: %s", filepaths[i]);
        }
    }

    if (!found) {
        ESP_LOGW(TAG, "Message not found in any file: %s", target_message);
    }

    return found;
}*/

void delete_entry(const char *msg){
    char *request_uuid = extract_uuid_from_json_response(msg);
    // if (request_uuid) {
    //     ESP_LOGI("msg uuid", "%s", request_uuid);
    //     int count = handle_response_message(request_uuid);
    //     if(count != -1){
    //         fileInfo.request_file_entries = count;
    //     }
    //     if(request_uuid){
    //         free(request_uuid);
    //         request_uuid = NULL;
    //     }
    // }
}

char *extract_uuid_from_json_response(const char *json_response) {
    // ESP_LOGW("DEBUG1", "Raw JSON: %s", json_response);
    cJSON *json = cJSON_Parse(json_response);
    if(critical_retry_count >= 3 && json == NULL) {
        critical_retry_count = 0; // Reset critical retry count if parsing fails
        search_and_delete_message_from_any_file(json_response);
        ESP_LOGE(SPIFFS_TAG, "Failed to parse JSON response: %s", cJSON_GetErrorPtr());
        return NULL; // Return NULL if parsing fails
    }
    else if(critical_retry_count >= 3 && json != NULL)
       critical_retry_count = 0; // Reset critical retry count if parsing is successful
    // if (!json) {
    //     ESP_LOGE(SPIFFS_TAG, "Invalid JSON format in response\n");
    //     return NULL;
    // }


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

/*bool delete_line_if_found(const char *filepath, const char *target) {
    char temp_path[128];

    // Choose temp file based on filepath
    if (strcmp(filepath, FILEPATH) == 0){
        strcpy(temp_path, TEMPFILEPATH);
        ESP_LOGI(TAG, "Using TEMPFILEPATH for %s", filepath);
    } else {
        strcpy(temp_path, TEMP_FAULT_LOG_FILEPATH);
        ESP_LOGI(TAG, "Using TEMP_FAULT_LOG_FILEPATH for %s", filepath);
    }

    ESP_LOGI(TAG, "Opening source file: %s", filepath);
    FILE *src = fopen(filepath, "r");
    if (!src) {
        ESP_LOGW(TAG, "Cannot open file: %s", filepath);
        return false;
    }

    ESP_LOGI(TAG, "Creating temp file: %s", temp_path);
    FILE *temp = fopen(temp_path, "w");
    if (!temp) {
        ESP_LOGE(TAG, "Cannot create temp file: %s", temp_path);
        fclose(src);
        return false;
    }

    char line[DEQUE_MSG_BUFFER];
    bool deleted = false;

    while (fgets(line, sizeof(line), src)) {
        // Strip newline
        line[strcspn(line, "\r\n")] = 0;
        ESP_LOGD(TAG, "Read line: '%s'", line);

        if (strstr(line, target) != NULL) {
            ESP_LOGI(TAG, "Match found and deleted from %s: %s", filepath, line);
            deleted = true;
            continue;  // Skip this line
        }

        fprintf(temp, "%s\n", line);  // Write valid line
    }

    fclose(src);
    fclose(temp);

    if (deleted) {
        ESP_LOGI(TAG, "Replacing original file with updated content: %s", filepath);
        remove(filepath);
        rename(temp_path, filepath);
    } else {
        ESP_LOGI(TAG, "No match found in file: %s. Removing temp file.", filepath);
        remove(temp_path);
    }

    return deleted;
}

bool search_and_delete_message_from_any_file(const char *target_message) {
    int num_connectors = chargerConfigMQTT.numConnectors;
    if (num_connectors > NO_OF_CONNECTORS) num_connectors = NO_OF_CONNECTORS;

    int total_files = 2 + num_connectors;
    const char *filepaths[total_files];

    // Static filepaths
    filepaths[0] = FAULT_LOG_FILEPATH_OnlineCharger;
    filepaths[1] = FAULT_LOG_FILEPATH_OfflineCharger;

    // Connector filepaths
    for (int i = 0; i < num_connectors; i++) {
        filepaths[2 + i] = fileNameConnector[i].logFilePath;
    }

    ESP_LOGI(TAG, "Searching for message across %d files: \"%s\"", total_files, target_message);

    bool found = false;
    for (int i = 0; i < total_files; i++) {
        struct stat file_stat;
        if (stat(filepaths[i], &file_stat) == 0) {
            if (file_stat.st_size > 0) {
                ESP_LOGI(TAG, "Checking file [%d]: %s (Size: %ld bytes)", i, filepaths[i], file_stat.st_size);

                if (delete_line_if_found(filepaths[i], target_message)) {
                    found = true;
                    ESP_LOGI(TAG, "Deleted target from file: %s", filepaths[i]);
                } else {
                    ESP_LOGI(TAG, "Message not found in file: %s", filepaths[i]);
                }
            } else {
                ESP_LOGI(TAG, "Skipping empty file: %s", filepaths[i]);
            }
        } else {
            ESP_LOGW(TAG, "stat() failed on file: %s. Skipping.", filepaths[i]);
        }
    }

    if (!found) {
        ESP_LOGW(TAG, "Message not found in any file: %s", target_message);
    } else {
        ESP_LOGI(TAG, "Message deleted from one or more files.");
    }

    return found;
}
*/

int delete_line_if_found(const char *filepath, const char *target) {
    char temp_path[128];

    if (strcmp(filepath, FILEPATH) == 0) {
        strcpy(temp_path, TEMPFILEPATH);
        ESP_LOGI(TAG, "[%s] Using TEMPFILEPATH: %s", filepath, TEMPFILEPATH);
    } 
    else {
        strcpy(temp_path, TEMP_FAULT_LOG_FILEPATH);
        ESP_LOGI(TAG, "[%s] Using TEMP_FAULT_LOG_FILEPATH: %s", filepath, TEMP_FAULT_LOG_FILEPATH);
    }

    ESP_LOGI(TAG, "[%s] Opening source file...", filepath);
    FILE *src = fopen(filepath, "r");
    if (!src) {
        ESP_LOGW(TAG, "[%s] Failed to open source file!", filepath);
        return -1;
    }

    ESP_LOGI(TAG, "[%s] Creating temp file: %s", filepath, temp_path);
    FILE *temp = fopen(temp_path, "w");
    if (!temp) {
        ESP_LOGE(TAG, "[%s] Failed to create temp file!", filepath);
        fclose(src);
        return -1;
    }

    char line[DEQUE_MSG_BUFFER];
    int total_lines = 0;
    int lines_kept = 0;
    int lines_deleted = 0;

    while (fgets(line, sizeof(line), src)) {
        total_lines++;
        line[strcspn(line, "\r\n")] = 0;  // Strip newline

        if (strstr(line, target) != NULL) {
            lines_deleted++;
            ESP_LOGI(TAG, "[%s] Deleting matching line: \"%s\"", filepath, line);
            continue;  // Don't write this line
        }

        fprintf(temp, "%s\n", line);
        lines_kept++;
    }

    fclose(src);
    fclose(temp);

    if (lines_deleted > 0) {
        remove(filepath);
        rename(temp_path, filepath);
        ESP_LOGI(TAG, "[%s] Deletion complete. Total: %d, Deleted: %d, Remaining: %d",
                 filepath, total_lines, lines_deleted, lines_kept);
    } else {
        remove(temp_path);
        ESP_LOGI(TAG, "[%s] No matching line found. Total entries: %d", filepath, total_lines);
    }

    return lines_kept;
}

int search_and_delete_message_from_any_file(const char *target_message) {
    int num_connectors = chargerConfigMQTT.numConnectors;
    if (num_connectors > NO_OF_CONNECTORS) num_connectors = NO_OF_CONNECTORS;

    int total_files = 3 + num_connectors;  // 1 for request log, 2 for fault logs, and connectors
    const char *filepaths[total_files];
    filepaths[0] = FILEPATH;  // Request log file
    filepaths[1] = FAULT_LOG_FILEPATH_OnlineCharger;
    filepaths[2] = FAULT_LOG_FILEPATH_OfflineCharger;

    for (int i = 0; i < num_connectors; i++) {
        filepaths[3 + i] = fileNameConnector[i].logFilePath;
    }

    ESP_LOGI(TAG, "Starting message search & deletion: \"%s\"", target_message);

    int total_remaining = 0;
    for (int i = 0; i < total_files; i++) {
        struct stat file_stat;
        if (stat(filepaths[i], &file_stat) == 0) {
            if (file_stat.st_size > 0) {
                ESP_LOGI(TAG, "Processing file [%d]: %s (Size: %ld bytes)",
                         i, filepaths[i], file_stat.st_size);

                int remaining = delete_line_if_found(filepaths[i], target_message);
                if (remaining >= 0) {
                    total_remaining += remaining;
                    if(i == 0)
                    {
                        // total_remaining += remaining;
                        // fileInfo.request_file_entries = 0;
                        fileInfo.request_file_entries = total_remaining;
                    }
                    else if(i == 1)
                    {
                        // total_remaining += remaining;
                        // fileInfo.onlineCharger_file_entries = 0;
                        fileInfo.onlineCharger_file_entries = total_remaining;
                    }
                    else if(i == 2)
                    {
                        // total_remaining += remaining;
                        // fileInfo.offlineCharger_file_entries = 0;
                        fileInfo.offlineCharger_file_entries = total_remaining;
                    }
                    else if(i == 3 || i == 4) // Assuming connectors file are at index 3 and 4
                    {
                        // total_remaining += remaining;
                        // fileNameConnector[i].entries = 0;
                        fileNameConnector[i].entries = total_remaining;
                    }
                } else {
                    ESP_LOGW(TAG, "Error processing file: %s", filepaths[i]);
                }
            } else {
                ESP_LOGI(TAG, "Skipping empty file [%d]: %s", i, filepaths[i]);
            }
        } else {
            ESP_LOGW(TAG, "stat() failed for file [%d]: %s", i, filepaths[i]);
        }
    }

    ESP_LOGI(TAG, "Deletion completed. Total remaining entries across all files: %d", total_remaining);
    return total_remaining;
}
