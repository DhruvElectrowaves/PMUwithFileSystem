#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "cJSON.h"
#include "nvs_flash.h"
#include "nvs.h"

#include "driver/gpio.h"
#include "driver/twai.h"

#include <stdint.h>
#include <time.h>
#include "PMUFinalmain.h"
#include "CAN.h"
#include "MqttFunctions.h"
#include "../components/Operation/ValidateMessage.h"
#include "../components/Messages/MQTT.h"
#include <inttypes.h>

#include "driver/gpio.h"
#include "esp32/rom/gpio.h" // Include this header for gpio_pad_select_gpio()
#include "esp_spiffs.h"
#include <stdbool.h>
#include <esp_http_client.h>
#include <esp_timer.h>
#include <float.h>  

#define SPIFFS_TAG "SPIFFS"
const char *MQTT_TAG = "PMU_Charger";

uint8_t internet_counter = 0;
bool wifiConnected = false;
uint8_t valueofChangedParameter=0;

//Inter-Processor Call
SemaphoreHandle_t mutex;
SemaphoreHandle_t mutex_spiffs;
uint8_t pcm_index;

// Queue to hold filenames
QueueHandle_t payload_queue;
typedef struct {
    char filename[64];  // Adjust the size as necessary
} payload_t;

uint8_t messageCount=0;
int last_processed_entry =0 ;

//Test Variables
uint64_t countPerioidicCAN;
uint8_t printChargerConfigData = 0;
int counterIncomingMsg = 0;
int counterAck = 0;
int prevSequenceNo;
char topicName[64];

esp_mqtt_client_handle_t mqtt_client;

// This are the variables used for storing Wi-fi Initialize credentials 
char wifi_ssid[32] = {0};
char wifi_password[64] = {0};
char pubTopic[40];

//+++++++++++++++++ For Testing Purpose we have set the Broker +++++++++++++++++++
//char mqtt_broker_uri[] = "mqtt://13.127.194.179:1883";
char mqtt_broker_uri[] = "mqtt://broker.emqx.io:1883";
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

uint64_t lastCheckTime = 0;
bool isTimeSet = false;

uint16_t component_Id ;  //This would hold the value of component whose status i want to know 

//Defining Variables to Store Topics that Charger Has Subscribed to 
char sub_topic1[65];

//This structure is defined for Subscription and Checking.
typedef struct {
    char topic[60];
    char data[500];
} mqtt_message_t;

//The Declaration of variables to store data received from CAN 
FaultInfo_t faultInfoCAN;                                  // Store fault Info from MCU via CAN
FaultInfo_t faultInfoIPC;                                  // Will be used for IPC 
FaultInfo_t faultInfoMQTT;                                 // Will be used for MQTT Transmission

SessionInfo_t sessionInfoCAN /*= { .chargingSessionOccured = 1 }*/;							   // Store sessionInfo from MCU via CAN
SessionInfo_t sessionInfoIPC;                              // Will be used for IPC 
SessionInfo_t sessionInfoMQTT;                             // Will be used for MQTT Transmission

ChargerPeriodicStatus_t chargerPeriodicStatusCAN = {.trackNoofFrames = 0};           // Store chargerPeriodicStatus from MCU via CAN
ChargerPeriodicStatus_t chargerPeriodicStatusIPC= {.trackNoofFrames = 0};           // Will be used for IPC
ChargerPeriodicStatus_t chargerPeriodicStatusMQTT= {.trackNoofFrames = 0};          // Will be used for MQTT Transmission

powerModule_t powerModulesCAN[MAX_PCM_MODULE];			   // Store PCM module from PCM via CAN
powerModule_t powerModulesIPC[MAX_PCM_MODULE];             //Will be used for IPC
powerModule_t powerModulesMQTT[MAX_PCM_MODULE];            //Will be used for MQTT transmission

chargerConfig_t chargerConfigCAN;
chargerConfig_t chargerConfigMQTT;
chargerConfig_t chargerConfigIPC;

faultLog_t faultLogCAN;
faultLog_t faultLogIPC;
faultLog_t faultLogMQTT;

publishMsgFlagType publishMsgFlag ={
    .send_component_status =0,
    .config_data = 1,
    .configDataExchanged = 0,
    .charging_session = 1
};


//Structure Variable for Flags
//The two structure for Sending CAN information to MCU
can_message_t PMUInformation;
CAN_Message AckPmu;

int timerCounter = 0;  // To keep the timer check on CAN task
uint16_t configRequestCounter = 0;
//This is flag that would be raised whenever PMU received any message and will send an acknowlegment
uint8_t pmuAckFlag = 0;

//flag for chargerConfigdata
uint8_t Not_send_config_data_flag = 255;

//Variables Added from Niharika Project
uint32_t counterPeriodic = 0;
uint32_t counterUptime = 0;
uint32_t dhruvCounter = 0;
uint32_t counterPeriodicCharger = 0;
uint8_t validate_responseChargerConfig = 0;
uint32_t validate_responseChargerConfigCounter = 0;
uint32_t chargerIdCounter = 0;
uint8_t firstTimeBootUpFlag = 1;

uint8_t LCUInternetUp = 0;
uint8_t LCUInternetDown = 0;
uint8_t LCUWebsocketUp = 0;
uint8_t LCUWebsocketDown = 0;
uint32_t internetCountDown = 0;

void mqtt_task(void *client);
int global_component_value = 255;
int global_id_value = 255;
// int componentId = 255;
int pmuIndex=0;
char msg_string[MSG_STRING_LEN];
gridSupplyEnum gridStatus;


//handler for MQTT notify task
TaskHandle_t process_MQTT_msg_handler = NULL;

/* message retrying functionality*/
char *last_sent_message = NULL;
uint8_t count_retry_interval = 0;
uint8_t retry_count = 0;
uint8_t critical_retry_count = 0;
char* msg = NULL;

//Date Added 4/2/2025
int id_requested = 255; //Modified by Shikha
bool isConfigurationChanged = false;
flags_t flagConfig;
uint8_t configDataExchangedDetectedFromNVS = 0 ;
uint8_t ntpInitFlag = 0;

//19/02/2025 
internetStatus_t LCUinternetStatus;
internetStatus_t LCUinternetStatusComponent;
websocketStatus_t websocketStatus;
websocketStatus_t websocketStatusComponent;

//24/02/2025
uint8_t configRequestFlag = 0;
void request_config_data_from_MCU();
bool wifiInitialised = false;

//26/02/2025
uint32_t prev_LCUInternetUp = 0;
uint32_t prev_LCUInternetDown= 0;
uint32_t prev_LCUWebsocketUp = 0;
uint32_t prev_LCUWebsocketDown = 0;
uint64_t lastUptimeUpdate = 0 ;
uint64_t last30SecUpdate = 0;

//Variable to Read Size of Queque
bool fileEntryReceived = false;

//faultLog variables
connectorFileName_t *fileNameConnector = NULL;  // Pointer to dynamically allocate memory
// -----------------------------------WiFi Functionality ------------------------------------------------

void check_internet_status() {
    esp_err_t err = ESP_OK;
    esp_http_client_config_t config = {
        .url = "http://www.google.com",
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_method(client, HTTP_METHOD_GET);
    err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        PMUInformation.internet_status = 1;
        //printf("Internet Status %d \n",PMUInformation.internet_status);
        internet_counter = 0;
    } else {
        internet_counter++;
    }
    if(internet_counter >= 5){
        internet_counter=0;
        PMUInformation.internet_status = 0;
        //printf("Internet Status %d \n",PMUInformation.internet_status);
    }
    esp_http_client_cleanup(client);
}

// Wi-Fi event handler
void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
        wifiInitialised = true;
        ESP_LOGI("Wifi", "The wifi Initailsed ");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(MQTT_TAG, "Disconnected from WiFi, attempting to reconnect...");
        wifiConnected = false;
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(MQTT_TAG, "Connected to WiFi with IP Address: " IPSTR, IP2STR(&event->ip_info.ip));
        wifiConnected = true;
    }
}

// Function to initialize Wi-Fi
void wifi_init(void) {
	//Initialization of NVS at the beginning of the code
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config));
    strncpy((char *)wifi_config.sta.ssid, wifi_ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, wifi_password, sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(MQTT_TAG, "Connecting to WiFi...");
}

// Function to print the contents of a file entry by entry
void print_file_entries(const char* file_path) {
    // // Check if SPIFFSMutex is available and acquire it for thread safety
    // FILE *file = fopen(file_path, "r");
    // if (!file) {
    //     ESP_LOGE(SPIFFS_TAG, "Failed to open file: %s", file_path);
    //     return;
    // }

    // char buffer[TOTAL_LINE];  // Buffer to store each line from the file
    // int entry_number = 1;

    // //ESP_LOGI("log", "Printing file entries from: %s", file_path);
    // while (fgets(buffer, sizeof(buffer), file) != NULL) {
    //     // Trim newline character from the buffer
    //     buffer[strcspn(buffer, "\n")] = '\0';
    //     //ESP_LOGI("log", "Entry %d: %s", entry_number++, buffer);
    // }

    // fclose(file);  // Close the file after reading

}

// ----------------------------------- MQTT Functionality ---------------------------------------------
void initialize_file_names(int numConnectors) {
    // Allocate memory dynamically for the required number of connectors
    fileNameConnector = (connectorFileName_t *)malloc(numConnectors * sizeof(connectorFileName_t));

    if (!fileNameConnector) {
        printf("Memory allocation failed for fileNameConnector\n");
        return;
    }

    for (int i = 0; i < numConnectors; i++) {
        snprintf(fileNameConnector[i].logFilePath, sizeof(fileNameConnector[i].logFilePath), "/spiffs/fault_log_connector%d.txt", i+1);
        // fileNameConnector[i].entries = 0; // Initialize entries count
    }
}

uint8_t count_file_entries(const char *file_path)
{
    FILE *f = fopen(file_path, "r"); // Open file in read mode
    if (f == NULL)
    {
        ESP_LOGE("TAG", "File open failed: %s", file_path);
        return 0; // Return -1 to indicate failure
    }

    int count = 0;
    char line[DEQUE_MSG_BUFFER]; // Buffer to store each line

    while (fgets(line, sizeof(line), f)) // Read file line by line
    {
        count++; // Increment count for each line
    }

    fclose(f); // Close file after reading
    //ESP_LOGI("TAG", "Total Entries in File %s: %d", file_path, count);
    return count; // Return total count
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(MQTT_TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    
    esp_mqtt_event_handle_t event = event_data;
    
    switch (event_id) {
		
        case MQTT_EVENT_CONNECTED:
        
            ESP_LOGI(MQTT_TAG, "MQTT_EVENT_CONNECTED");
         	PMUInformation.broker_status = 1;

            snprintf(sub_topic1, sizeof(sub_topic1), "serverData/%s", chargerConfigMQTT.serialNumber);
        	ESP_LOGI(MQTT_TAG, "sub_topic1: %s", sub_topic1);
            int msg_id = esp_mqtt_client_subscribe(mqtt_client, sub_topic1, 1);
     		ESP_LOGI(MQTT_TAG, "Subscribed to CMS topic with msg_id=%d", msg_id);
     		break;
            
        case MQTT_EVENT_DISCONNECTED:
            //ESP_LOGI(MQTT_TAG, "MQTT_EVENT_DISCONNECTED");
            PMUInformation.broker_status = 0;
            if(wifiConnected==true && PMUInformation.internet_status == 1)
                esp_mqtt_client_reconnect(mqtt_client);
            break;
             
        case MQTT_EVENT_DATA:
        
           ESP_LOGI(MQTT_TAG, "Received data from topic: %.*s", event->topic_len, event->topic);
		   
    		// Create a structure to hold both the topic and the data
    		mqtt_message_t message;
    		
        	// Copy the topic and data into the structure
			strncpy(message.topic, event->topic, event->topic_len);
			message.topic[event->topic_len] = '\0';  // Null-terminate
    
            // Convert data to a null-terminated string
            if (event->data_len < sizeof(message.data)) {
                strncpy(message.data, event->data, event->data_len);
                message.data[event->data_len] = '\0'; // Null-terminate
            } else {
                ESP_LOGE(MQTT_TAG, "Data too long, truncating");
                strncpy(message.data, event->data, sizeof(message.data) - 1);
                message.data[sizeof(message.data) - 1] = '\0'; // Ensure null-termination
            }
            
            // Copy the data to the global variable
            if (event->data_len < MSG_STRING_LEN) {
                strncpy(msg_string, message.data, event->data_len);
                msg_string[event->data_len] = '\0';  // Null-terminate

                if (process_MQTT_msg_handler != NULL) {
                    xTaskNotifyGive(process_MQTT_msg_handler);
                }
            }else {
                ESP_LOGE(MQTT_TAG, "Received data length (%d) exceeds buffer size (%zu)", event->data_len, sizeof(msg_string));
                strncpy(msg_string, message.data, MSG_STRING_LEN - 1);
                msg_string[MSG_STRING_LEN - 1] = '\0';  // Null-terminate
            }
             break;
			
		case MQTT_EVENT_PUBLISHED:
            //ESP_LOGI(MQTT_TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
            
        default:
            //ESP_LOGI(MQTT_TAG, "Other event id:%d", event->event_id);
            break;
    }
}

void init_mqtt() 
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = mqtt_broker_uri,
        .session.keepalive = 60
    };
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
}

void mqtt_task(void *pvParameters){

    const TickType_t xDelay = pdMS_TO_TICKS(50); 
    snprintf(pubTopic, sizeof(pubTopic), "chargerData/%s", chargerConfigMQTT.serialNumber2);
    ESP_LOGI(MQTT_TAG, "pubTopic: %s", pubTopic);

    while(1)
    {
            if(xSemaphoreTake(mutex, (TickType_t)5) == pdTRUE){      
                memcpy(&sessionInfoMQTT,&sessionInfoIPC,sizeof(sessionInfoMQTT));
                memcpy(&faultLogMQTT, &faultLogIPC , sizeof(faultLogMQTT));
                //ESP_LOGI("FAULT_LOG", LOG_COLOR_CYAN "faultLogDataExchangedMQTT: %d" LOG_RESET_COLOR, faultLogMQTT.faultLogDataExchanged);
                xSemaphoreGive(mutex);
            }

            //1)Case 1 is that for the "ConfigurationData"
            if(publishMsgFlag.config_data == 1 && (firstTimeBootUpFlag == 3 || firstTimeBootUpFlag == 0) && chargerConfigMQTT.numConnectors != 0)
            {   
                prepare_mqtt_send_config_data();

                // Step 1: Initialize file names
                initialize_file_names(chargerConfigMQTT.numConnectors);

                // Step 2: Count entries after names are formed
                for (int i = 0; i < chargerConfigMQTT.numConnectors; i++)
                {
                    fileNameConnector[i].entries = count_file_entries(fileNameConnector[i].logFilePath);
                    ESP_LOGI("TAG", "Entries in Offline Connector Log %d: %d", i + 1, fileNameConnector[i].entries);
                }
                publishMsgFlag.config_data = 0;
                fileEntryReceived = true;
            }

            if(counterPeriodic++ >= 600) // counter will reach to 30 sec
                {
                    counterPeriodic = 0;

                    //ESP_LOGW("COUNTERS", "Before Reset: LCUInternetUp: %d, LCUInternetDown: %d, LCUWebsocketUp: %d, LCUWebsocketDown: %d", LCUInternetUp, LCUInternetDown, LCUWebsocketUp, LCUWebsocketDown);
                    
                    LCUinternetStatus.totalDownTime += LCUInternetDown;
                    LCUinternetStatus.totalUpTime += LCUInternetUp;
                    websocketStatus.totalUpTime += LCUWebsocketUp;
                    websocketStatus.totalDownTime += LCUWebsocketDown;
    
                    write_nvs_value(NVS_INTERNET_TOTAL_DOWN_TIME,LCUinternetStatus.totalDownTime);
                    write_nvs_value(NVS_INTERNET_TOTAL_UP_TIME,LCUinternetStatus.totalUpTime);
                    write_nvs_value(NVS_WEBSOCKET_TOTAL_UP_TIME,websocketStatus.totalUpTime);
                    write_nvs_value(NVS_WEBSOCKET_TOTAL_DOWN_TIME,websocketStatus.totalDownTime);
    
                    // Reset session counters
                    LCUInternetUp = 0;
                    LCUInternetDown = 0;
                    LCUWebsocketUp = 0;
                    LCUWebsocketDown = 0;

                    //Condition InternetStatus Condition
                    if(IS_MQTT_NETWORK_ONLINE && valueofChangedParameter == 0 && publishMsgFlag.configDataExchanged == 1 && chargerConfigMQTT.pmuEnable == 1)
                    {
                        publishMsgFlag.pcm_periodic_data = 1;
                        //printf("The value of trackNoOfFrsme %d \n",chargerPeriodicStatusMQTT.trackNoofFrames);
                        if(((chargerPeriodicStatusMQTT.trackNoofFrames & 0x01) && (chargerPeriodicStatusMQTT.trackNoofFrames & (1 << 4)) && (chargerPeriodicStatusMQTT.trackNoofFrames & (1 << 8)) && (chargerPeriodicStatusMQTT.trackNoofFrames & (1 << 9))) || chargerPeriodicStatusMQTT.MCUComm == 0)
                            publishMsgFlag.charger_periodic_data = 1;
                    }
                } 
                              
            //2) Case for the "PCMPeriodicData"
            if(publishMsgFlag.pcm_periodic_data)
            {
                ESP_LOGI(MQTT_TAG, "Processing PCM periodic data...");
                publishMsgFlag.pcm_periodic_data = 0;
                // int startIndex = 0;
                // int endIndex = chargerConfigMQTT.numPCM;
                create_PCM_Periodic_message(PCMPeriodicData);
            }
            
            //3) case for the "chargerPeriodicData"
            if(publishMsgFlag.charger_periodic_data)
            {
                ESP_LOGI(MQTT_TAG, "Processing charger periodic data...");
                publishMsgFlag.charger_periodic_data = 0;
                create_chargerPeriodicData_message();
                ESP_LOGW("Buffer","Perioidc Wala WALA");

                check_memory_status();
            }

            //4)case for the "ChargingSession"
            if(sessionInfoMQTT.chargingSessionOccured != publishMsgFlag.charging_session && chargerConfigMQTT.pmuEnable == 1)
            {
                ESP_LOGI("FLAG", LOG_COLOR_PINK "chargingSessionOccured:%d" LOG_RESET_COLOR, sessionInfoMQTT.chargingSessionOccured);
                ESP_LOGI("FLAG", LOG_COLOR_PINK "charging_sessionFlag:%d" LOG_RESET_COLOR, publishMsgFlag.charging_session);
                publishMsgFlag.charging_session = sessionInfoMQTT.chargingSessionOccured;  // Update last state
                ESP_LOGI("FLAG", LOG_COLOR_PINK "Last chargingSessionOccured:%d" LOG_RESET_COLOR, sessionInfoMQTT.chargingSessionOccured);
                ESP_LOGI("FLAG", LOG_COLOR_PINK "Last charging_sessionFlag:%d" LOG_RESET_COLOR, publishMsgFlag.charging_session);
                create_chargingSession_message();
            }
            
            if(chargerPeriodicStatusMQTT.MCUComm == FINE){
                if (chargerConfigMQTT.configKeyValue == ONLINE && chargerPeriodicStatusMQTT.receivedDataChargerPeriodic == 1) {
                    if (++counterUptime >= 20) {  // Increment only when condition is met
                        counterUptime = 0;  // Reset after reaching threshold
                        ESP_LOGW("DEBUG", "Internet Status: %d", chargerPeriodicStatusMQTT.LCUInternetStatus);
                        ESP_LOGW("DEBUG", "Websocket Status MQTT: %d", chargerPeriodicStatusMQTT.websocketConnection);
                        if (chargerPeriodicStatusMQTT.LCUInternetStatus == FAILED) {
                            LCUInternetDown++;
                        //internetCountDown++;
                        } else {
                            LCUInternetUp++;
                            if (chargerPeriodicStatusMQTT.websocketConnection == FINE) {
                                LCUWebsocketUp++;
                            } else {
                                LCUWebsocketDown++;
                            }
                        }
                    }
                }
            }

            
            if((faultLogMQTT.faultLogDataExchanged != publishMsgFlag.toggleFaultLog) && chargerConfigMQTT.receivedConfigDataMQTT && chargerConfigMQTT.pmuEnable == 1){
                ESP_LOGI("FLAG", LOG_COLOR_PINK "faultLogDataExchanged:%d" LOG_RESET_COLOR, faultLogMQTT.faultLogDataExchanged);
                ESP_LOGI("FLAG", LOG_COLOR_PINK "toggleFaultLog:%d" LOG_RESET_COLOR, publishMsgFlag.toggleFaultLog);
                publishMsgFlag.toggleFaultLog = faultLogMQTT.faultLogDataExchanged;
                ESP_LOGI("FLAG", LOG_COLOR_PINK "MQTT After faultLogDataExchanged:%d" LOG_RESET_COLOR, faultLogMQTT.faultLogDataExchanged);
                ESP_LOGI("FLAG", LOG_COLOR_PINK "MQTT After toggleFaultLog:%d" LOG_RESET_COLOR, publishMsgFlag.toggleFaultLog);
                create_faultLog_message();
            }   

            vTaskDelay(xDelay); 
    }  
}

//----------------------------------------------------CAN Function---------------------------------------------------

void check_config_data_changes() {

    // ESP_LOGW("ChangeConfig", "The value of isConfigChanged %d", isConfigurationChanged);

    //Check Wifi_ssid
    if (strcmp(chargerConfigIPC.wifi_ssid, chargerConfigMQTT.wifi_ssid) != 0 ) {
        //Copy Kardo 
        isConfigurationChanged = true;
        valueofChangedParameter = 1;
        flagConfig.wifi_ssidFlag = 1; 
        ESP_LOGI("DEBUG", "Value of parameter %d",valueofChangedParameter );
        ESP_LOGI("Config Change", "[MQTT] WiFi SSID changed: %s -> %s", chargerConfigIPC.wifi_ssid, chargerConfigMQTT.wifi_ssid);
    }

    // Check WiFi Password
    if (strcmp(chargerConfigIPC.wifi_password, chargerConfigMQTT.wifi_password) != 0) {
        isConfigurationChanged = true;
        valueofChangedParameter = 2;
        flagConfig.wifi_passwordFlag = 1; 
        ESP_LOGI("Config Change", "[MQTT] WiFi Password changed");
    }

    // Check MQTT Broker
    if (strcmp(chargerConfigIPC.mqtt_broker, chargerConfigMQTT.mqtt_broker) != 0) {
        isConfigurationChanged = true;
        valueofChangedParameter = 3;
        flagConfig.mqtt_brokerFlag =1;          
        ESP_LOGI("DEBUG", "Value of parameter %d",valueofChangedParameter );
        ESP_LOGI("Config Change", "[MQTT] MQTT Broker changed: %s -> %s", chargerConfigIPC.mqtt_broker, chargerConfigMQTT.mqtt_broker);
    }

    // Check OCPP URL
    if (strcmp(chargerConfigIPC.ocppURL, chargerConfigMQTT.ocppURL) != 0) {
        isConfigurationChanged = true;
        valueofChangedParameter = 4;
        flagConfig.ocppURLFlag = 1;
        ESP_LOGI("DEBUG", "Value of parameter %d",valueofChangedParameter );
        ESP_LOGI("Config Change", "[MQTT] OCPP URL changed: %s -> %s", chargerConfigIPC.ocppURL, chargerConfigMQTT.ocppURL);
    }

    // Check Serial Number
    if (strcmp(chargerConfigIPC.serialNumber, chargerConfigMQTT.serialNumber) != 0) {
        isConfigurationChanged = true;
        valueofChangedParameter = 5;
        flagConfig.serialNumberFlag = 1;
        ESP_LOGI("DEBUG", "Value of parameter %d",valueofChangedParameter );
        ESP_LOGI("Config Change", "[MQTT] Serial Number changed: %s -> %s", chargerConfigIPC.serialNumber, chargerConfigMQTT.serialNumber);
    }

    if (chargerConfigIPC.AutoCharge != chargerConfigMQTT.AutoCharge) {
        isConfigurationChanged = true;
        valueofChangedParameter = 6;
        flagConfig.AutoChargeFlag = 1;
        ESP_LOGI("DEBUG", "Value of parameter %d",valueofChangedParameter );
        ESP_LOGI("Config Change", "[MQTT] AutoCharge changed: %d -> %d", chargerConfigIPC.AutoCharge, chargerConfigMQTT.AutoCharge);
    }

    if(chargerConfigIPC.Brand != chargerConfigMQTT.Brand){
        isConfigurationChanged = true;
        valueofChangedParameter = 7;
        flagConfig.BrandFlag = 1;
        ESP_LOGI("DEBUG", "Value of parameter %d",valueofChangedParameter );
        ESP_LOGI("Config Change", "[MQTT] Brand changed: %d -> %d", chargerConfigIPC.Brand, chargerConfigMQTT.Brand);
    }

    if(chargerConfigIPC.limitSetting.chargerPower != chargerConfigMQTT.limitSetting.chargerPower){
        isConfigurationChanged = true;
        valueofChangedParameter = 8;
        flagConfig.chargerPowerFlag = 1;
        ESP_LOGI("DEBUG", "Value of parameter %d",valueofChangedParameter );
        ESP_LOGI("Config Change", "[MQTT] charger Power changed: %f -> %f", chargerConfigIPC.limitSetting.chargerPower, chargerConfigMQTT.limitSetting.chargerPower);
    }

    if(strcmp(chargerConfigIPC.versionInfo.evseCommContoller[0],chargerConfigMQTT.versionInfo.evseCommContoller[0]) != 0){
        isConfigurationChanged = true;
        valueofChangedParameter = 9;
        flagConfig.numConnectorsFlag = 1;
        flagConfig.numConnector1 = 1;
        flagConfig.commController1 = 1;
        ESP_LOGI("DEBUG", "Value of parameter %d",valueofChangedParameter );
        ESP_LOGD("Config Change", "[MQTT] evseCommController of 1 changed: %s -> %s", chargerConfigIPC.versionInfo.evseCommContoller[0], chargerConfigMQTT.versionInfo.evseCommContoller[0]);
    }

    if(chargerConfigIPC.limitSetting.gunAVoltage != chargerConfigMQTT.limitSetting.gunAVoltage){
        isConfigurationChanged = true;
        valueofChangedParameter = 10;
        flagConfig.numConnectorsFlag = 1;
        flagConfig.numConnector1 = 1;
        flagConfig.gunAVoltageFlag = 1;
        ESP_LOGI("DEBUG", "Value of parameter %d",valueofChangedParameter );
        ESP_LOGI("Config Change", "[MQTT] gunAvoltage 1 changed: %f -> %f", chargerConfigMQTT.limitSetting.gunAVoltage, chargerConfigIPC.limitSetting.gunAVoltage);
    }

    if(chargerConfigIPC.limitSetting.gunACurrent != chargerConfigMQTT.limitSetting.gunACurrent){
        isConfigurationChanged = true;
        valueofChangedParameter = 11;
        flagConfig.numConnectorsFlag = 1;
        flagConfig.numConnector1 = 1;
        flagConfig.gunACurrentFlag = 1;
        ESP_LOGI("DEBUG", "Value of parameter %d",valueofChangedParameter );
        ESP_LOGI("Config Change", "[MQTT] gunACurrent 1 changed: %f -> %f", chargerConfigMQTT.limitSetting.gunACurrent, chargerConfigIPC.limitSetting.gunACurrent);
    }

    if(chargerConfigIPC.limitSetting.gunAPower != chargerConfigMQTT.limitSetting.gunAPower){
        isConfigurationChanged = true;
        valueofChangedParameter = 12;
        flagConfig.numConnectorsFlag = 1;
        flagConfig.numConnector1 = 1;
        flagConfig.gunAPowerFlag = 1;
        ESP_LOGI("DEBUG", "Value of parameter %d",valueofChangedParameter );
        ESP_LOGI("Config Change", "[MQTT] gunAPower 1 changed: %f -> %f", chargerConfigMQTT.limitSetting.gunAPower, chargerConfigIPC.limitSetting.gunAPower);
    }

    if(strcmp(chargerConfigIPC.versionInfo.evseCommContoller[1], chargerConfigMQTT.versionInfo.evseCommContoller[1])!=0){
        isConfigurationChanged = true;
        valueofChangedParameter = 13;
        flagConfig.numConnectorsFlag = 1;
        flagConfig.numConnector2 = 1;
        flagConfig.commController2 = 1;
        ESP_LOGI("DEBUG", "Value of parameter %d",valueofChangedParameter );
        //ESP_LOGD("Config Change", "[MQTT] evseCommController of 2 changed: %d -> %d", chargerConfigIPC.versionInfo.evseCommContoller[1], chargerConfigMQTT.versionInfo.evseCommContoller[1]);
    }

    if(chargerConfigIPC.limitSetting.gunBVoltage != chargerConfigMQTT.limitSetting.gunBVoltage){
        isConfigurationChanged = true;
        valueofChangedParameter = 14;
        flagConfig.numConnectorsFlag = 1;
        flagConfig.numConnector2 = 1;
        flagConfig.gunBVoltageFlag = 1;
        ESP_LOGI("DEBUG", "Value of parameter %d",valueofChangedParameter );
        ESP_LOGI("Config Change", "[MQTT] gunBvoltage 2 changed: %f -> %f", chargerConfigMQTT.limitSetting.gunBVoltage, chargerConfigIPC.limitSetting.gunBVoltage);
    }

    if(chargerConfigIPC.limitSetting.gunBCurrent != chargerConfigMQTT.limitSetting.gunBCurrent){
        isConfigurationChanged = true;
        valueofChangedParameter = 15;
        flagConfig.numConnectorsFlag = 1;
        flagConfig.numConnector2 = 1;
        flagConfig.gunBCurrentFlag = 1;
        ESP_LOGI("DEBUG", "Value of parameter %d",valueofChangedParameter );
        ESP_LOGI("Config Change", "[MQTT] gunBCurrent 2 changed: %f -> %f", chargerConfigMQTT.limitSetting.gunBCurrent, chargerConfigIPC.limitSetting.gunBCurrent);
    }

    if(chargerConfigIPC.limitSetting.gunBPower != chargerConfigMQTT.limitSetting.gunBPower){
        isConfigurationChanged = true;
        valueofChangedParameter = 16;
        flagConfig.numConnectorsFlag = 1;
        flagConfig.numConnector2 = 1;
        flagConfig.gunBPowerFlag = 1;
        ESP_LOGI("DEBUG", "Value of parameter %d",valueofChangedParameter );
        ESP_LOGI("Config Change", "[MQTT] gunBPower 2 changed: %f -> %f", chargerConfigMQTT.limitSetting.gunBPower, chargerConfigIPC.limitSetting.gunBPower);
    }

    if(chargerConfigIPC.DynamicPowerSharing != chargerConfigMQTT.DynamicPowerSharing){
        isConfigurationChanged = true;
        valueofChangedParameter = 17;
        flagConfig.DynamicPowerSharingFlag = 1;
        ESP_LOGI("DEBUG", "Value of parameter %d",valueofChangedParameter );
        ESP_LOGI("Config Change", "[MQTT] DynamicPowerSharing changed: %d -> %d", chargerConfigIPC.DynamicPowerSharing, chargerConfigMQTT.DynamicPowerSharing);
    }

    if(chargerConfigIPC.EnergyMeterSetting != chargerConfigMQTT.EnergyMeterSetting){
        isConfigurationChanged = true;
        valueofChangedParameter = 18;
        flagConfig.EnergyMeterSettingFlag = 1;
        ESP_LOGI("DEBUG", "Value of parameter %d",valueofChangedParameter );
        ESP_LOGI("Config Change", "[MQTT] EnergyMeterSetting changed: %d -> %d", chargerConfigIPC.EnergyMeterSetting, chargerConfigMQTT.EnergyMeterSetting);
    }

    if(chargerConfigIPC.ZeroMeterStartEnable != chargerConfigMQTT.ZeroMeterStartEnable){
        isConfigurationChanged = true;
        valueofChangedParameter = 19;
        flagConfig.ZeroMeterStartEnableFlag = 1;
        ESP_LOGI("DEBUG", "Value of parameter %d",valueofChangedParameter );
        ESP_LOGI("Config Change", "[MQTT] ZeroMeterStartEnable changed: %d -> %d", chargerConfigIPC.ZeroMeterStartEnable, chargerConfigMQTT.ZeroMeterStartEnable);
    }

    if(chargerConfigIPC.configKeyValue != chargerConfigMQTT.configKeyValue){
        isConfigurationChanged = true;
        valueofChangedParameter = 20;
        flagConfig.configKeyValueFlag = 1;
        ESP_LOGI("DEBUG", "Value of parameter %d",valueofChangedParameter );
        ESP_LOGI("Config Change", "[MQTT] configKeyValue changed: %d -> %d", chargerConfigIPC.configKeyValue, chargerConfigMQTT.configKeyValue);
    }

    if(chargerConfigIPC.tarrifAmount != chargerConfigMQTT.tarrifAmount){
        isConfigurationChanged = true;
        valueofChangedParameter = 21;
        flagConfig.tarrifAmountFlag = 1;
        ESP_LOGI("DEBUG", "Value of parameter %d",valueofChangedParameter );
        ESP_LOGI("Config Change", "[MQTT] tarrifAmount changed: %f -> %f", chargerConfigIPC.tarrifAmount, chargerConfigMQTT.tarrifAmount);
    }

    if(strcmp(chargerConfigIPC.versionInfo.hmiVersion,chargerConfigMQTT.versionInfo.hmiVersion)!= 0){
        isConfigurationChanged = true;
        valueofChangedParameter = 22;
        flagConfig.versionInfoFlag = 1;
        flagConfig.hmiVersionFlag = 1;
        ESP_LOGI("DEBUG", "Value of parameter %d",valueofChangedParameter);
        ESP_LOGI("Config Change", "[MQTT] hmiVersion changed: %s -> %s", chargerConfigIPC.versionInfo.hmiVersion, chargerConfigMQTT.versionInfo.hmiVersion);
    }

    if(strcmp(chargerConfigIPC.versionInfo.lcuVersion,chargerConfigMQTT.versionInfo.lcuVersion)!= 0){
        isConfigurationChanged = true;
        valueofChangedParameter = 23;
        flagConfig.versionInfoFlag = 1;
        flagConfig.lcuVersionFlag = 1;
        ESP_LOGI("DEBUG", "Value of parameter %d",valueofChangedParameter);
        ESP_LOGI("Config Change", "[MQTT] lcuVersion changed: %s -> %s", chargerConfigIPC.versionInfo.lcuVersion, chargerConfigMQTT.versionInfo.lcuVersion);
    }

    if(strcmp(chargerConfigIPC.versionInfo.mcuVersion,chargerConfigMQTT.versionInfo.mcuVersion)!= 0){
        isConfigurationChanged = true;
        valueofChangedParameter = 24;
        flagConfig.versionInfoFlag = 1;
        flagConfig.mcuVersionFlag = 1;
        ESP_LOGI("DEBUG", "Value of parameter %d",valueofChangedParameter);
        ESP_LOGI("Config Change", "[MQTT] mcuVersion changed: %s -> %s", chargerConfigIPC.versionInfo.mcuVersion, chargerConfigMQTT.versionInfo.mcuVersion);
    }

    if(strcmp(chargerConfigIPC.versionInfo.PMU,chargerConfigMQTT.versionInfo.PMU)!= 0){
        isConfigurationChanged = true;
        valueofChangedParameter = 25;
        flagConfig.versionInfoFlag = 1;
        flagConfig.pmuVersionFlag = 1;
        ESP_LOGI("DEBUG", "Value of parameter %d",valueofChangedParameter);
        ESP_LOGI("Config Change", "[MQTT] PMU changed: %s -> %s", chargerConfigIPC.versionInfo.PMU, chargerConfigMQTT.versionInfo.PMU);
    }

    for(int i=0;i<chargerConfigMQTT.numPCM; i++){
        if(powerModulesIPC[i].versionMajorNumber != powerModulesMQTT[i].versionMajorNumber ||
         powerModulesIPC[i].versionMinorNumber != powerModulesMQTT[i].versionMinorNumber ){
            isConfigurationChanged = true;
            valueofChangedParameter = 25 +(i+1);
            flagConfig.versionInfoFlag = 1;
            flagConfig.numPCMFlag = 1;
            ESP_LOGI("DEBUG", "Value of parameter %d",valueofChangedParameter);
            ESP_LOGI("Config Change", "[MQTT] PMU changed: %s -> %s", chargerConfigIPC.versionInfo.PMU, chargerConfigMQTT.versionInfo.PMU);
        }
    }

    //Date - 10/02/2025 - Dhruv Addition
    if(chargerConfigIPC.limitSetting.serverChargerPower != chargerConfigMQTT.limitSetting.serverChargerPower){
        isConfigurationChanged = true;
        valueofChangedParameter = 36;
        flagConfig.serverChargerPowerFlag = 1;
        ESP_LOGI("DEBUG" , "Value of parameter %d",valueofChangedParameter);
        ESP_LOGI("Config Change", "[MQTT] serverChargerPower changed: %f -> %f", chargerConfigIPC.limitSetting.serverChargerPower, chargerConfigMQTT.limitSetting.serverChargerPower);
    }

    if(chargerConfigIPC.limitSetting.serverConnectorAPower != chargerConfigMQTT.limitSetting.serverConnectorAPower){
        isConfigurationChanged = true;
        valueofChangedParameter = 37;
        flagConfig.numConnectorsFlag = 1;
        flagConfig.numConnector1 = 1;
        flagConfig.serverConnectorAPowerFlag = 1;
        ESP_LOGI("DEBUG" , "Value of parameter %d",valueofChangedParameter);
        ESP_LOGI("Config Change", "[MQTT] serverConnectorAPower changed: %f -> %f", chargerConfigIPC.limitSetting.serverConnectorAPower, chargerConfigMQTT.limitSetting.serverConnectorAPower);
    }

    if(chargerConfigIPC.limitSetting.serverConnectorACurrent != chargerConfigMQTT.limitSetting.serverConnectorACurrent){
        isConfigurationChanged = true;
        valueofChangedParameter = 38;
        flagConfig.numConnectorsFlag = 1;
        flagConfig.numConnector1 = 1;
        flagConfig.serverConnectorACurrentFlag = 1;
        ESP_LOGI("DEBUG" , "Value of parameter %d",valueofChangedParameter);
        ESP_LOGI("Config Change", "[MQTT] serverConnectorACurrent changed: %f -> %f", chargerConfigIPC.limitSetting.serverConnectorACurrent, chargerConfigMQTT.limitSetting.serverConnectorACurrent);
    }

    if(chargerConfigIPC.limitSetting.serverConnectorBPower != chargerConfigMQTT.limitSetting.serverConnectorBPower){
        isConfigurationChanged = true;
        valueofChangedParameter = 37;
        flagConfig.numConnectorsFlag = 1;
        flagConfig.numConnector1 = 1;
        flagConfig.serverConnectorBPowerFlag = 1;
        ESP_LOGI("DEBUG" , "Value of parameter %d",valueofChangedParameter);
        ESP_LOGI("Config Change", "[MQTT] serverConnectorBPower changed: %f -> %f", chargerConfigIPC.limitSetting.serverConnectorBPower, chargerConfigMQTT.limitSetting.serverConnectorBPower);
    }

    if(chargerConfigIPC.limitSetting.serverConnectorBCurrent != chargerConfigMQTT.limitSetting.serverConnectorBCurrent){
        isConfigurationChanged = true;
        valueofChangedParameter = 37;
        flagConfig.numConnectorsFlag = 1;
        flagConfig.numConnector1 = 1;
        flagConfig.serverConnectorBCurrentFlag = 1;
        ESP_LOGI("DEBUG" , "Value of parameter %d",valueofChangedParameter);
        ESP_LOGI("Config Change", "[MQTT] serverConnectorBCurrent changed: %f -> %f", chargerConfigIPC.limitSetting.serverConnectorBCurrent, chargerConfigMQTT.limitSetting.serverConnectorBCurrent);
    }

    if(chargerConfigIPC.pmuEnable != chargerConfigMQTT.pmuEnable){
        isConfigurationChanged = true;
        valueofChangedParameter = 38;
        flagConfig.pmuEnabled = 1;
        ESP_LOGI("DEBUG" , "Value of parameter %d",valueofChangedParameter);
        ESP_LOGI("Config Change", "[MQTT] pmuEnable changed: %d -> %d", chargerConfigIPC.pmuEnable, chargerConfigMQTT.pmuEnable);
    }
}

void receive_can_message() {
    
	twai_message_t rx_msg;
    esp_err_t err = twai_receive(&rx_msg, pdMS_TO_TICKS(0));
    
     if (err == ESP_OK) {
       //ESP_LOGI(CAN_MCU_TAG, "Received CAN message with ID: 0x%" PRIx32 ".", rx_msg.identifier);
        
     switch (rx_msg.identifier) 
     {
            
            case PERIODIC_DATA_ID:
                chargerPeriodicStatusCAN.CANSuccess = 1;    // Modified by Shikha
                handle_periodic_data(rx_msg.data);
                break;

            case FAULT_INFO_ID:
               chargerPeriodicStatusCAN.CANSuccess = 1;    // Modified by Shikha
               handle_fault_info(rx_msg.data);
               break;
            
            case SESSION_INFO_ID:
               chargerPeriodicStatusCAN.CANSuccess = 1;    // Modified by Shikha
               handle_session_info(rx_msg.data);
               break;

            
            case CHARGER_CONFIG_ID:
                chargerPeriodicStatusCAN.CANSuccess = 1;
                if((chargerConfigCAN.numConnectors != 0 && strlen(chargerConfigCAN.serialNumber) > 0) || (eepromFaultedFlag == 1 && chargerConfigCAN.eepromCommStatus == 0))
                    configRequestFlag++;   
                handle_charger_config(rx_msg.data);
                // if(rx_msg.data[0] == 5){
                // for (int i = 0; i < 8; i++)
                // {
                //     //printf("%u ", rx_msg.data[i]); 
                // }
                // // }
                // //printf("\n");  
                // fflush(stdout);

                //counterIncomingMsg++;
                break;

            case FAULT_LOG:
                // ESP_LOGW("FAULT_LOG", LOG_COLOR_CYAN "Inside SwitchCAN faultLogDataExchangedCAN = %d" LOG_RESET_COLORCYAN, faultLogCAN.faultLogDataExchanged);
                handle_fault_log(rx_msg.data);
                break;
            
            default:
                
            	if (rx_msg.identifier >= 0x21 && rx_msg.identifier <= 0x30){
				   handle_pcm_message(rx_msg.identifier, rx_msg.data);
				}
            	else {
                    //ESP_LOGW(CAN_MCU_TAG, "Received unrecognized CAN message with ID: 0x%" PRIx32 ".", rx_msg.identifier);
                   pmuAckFlag=0;
                }
             break;
            }   
        }
}

void can_task(void *pvParameters)
{
    uint16_t sessionInfoCounter = 0;
    while(true)
    {
        // Modifications for CAN Communication Health : START
        // Modified by Shikha
        uint8_t k=0;
        // MCU CAN Health
        static uint16_t MCUCANFailureCount = 0;

        if(chargerPeriodicStatusCAN.CANSuccess == 0){
            MCUCANFailureCount++;
            if(MCUCANFailureCount > 3000){
                // Data not received until three seconds
                MCUCANFailureCount = 0;
                chargerPeriodicStatusCAN.MCUComm = FAILED;
            }
        }
        else{
            MCUCANFailureCount = 0;
            chargerPeriodicStatusCAN.MCUComm = FINE;
            chargerPeriodicStatusCAN.CANSuccess = 0;
        }

        // PCM CAN health
        for(k=0;k<MAX_PCM_MODULE;k++){
            if(powerModulesCAN[k].CANSuccess == 0){
                powerModulesCAN[k].CANFailureCount++;
                if(powerModulesCAN[k].CANFailureCount > 3000){
                    // Data not received until three seconds
                    powerModulesCAN[k].CANFailureCount = 0;
                    powerModulesCAN[k].CANFailure = 1;
                }
            }
            else{
                powerModulesCAN[k].CANFailureCount = 0;
                powerModulesCAN[k].CANFailure = 0;
                powerModulesCAN[k].CANSuccess = 0;
            }
        }
        // Modifications for CAN Communication Health : END

        timerCounter++;
        configRequestCounter++;

        if(timerCounter == 300) 
        {
            //Prepare CAN Periodic message data
            uint8_t message_data[8] = {0};
            message_data[0] = PMUInformation.sequence_number_1;                                                                                                 // Byte 1: Sequence Number
            message_data[1] = (PMUInformation.internet_status & 0x01) | ((PMUInformation.broker_status & 0x01) << 1) | (PMUInformation.reserved << 2);          // Byte 2: Connection Status
            message_data[2] = PMUInformation.pmu_major_version;                                                                                                 // Byte 3
            message_data[3] = PMUInformation.pmu_minor_version;
            message_data[4] = (PMUInformation.quequeSizeCAN & 0xFF);                                                                                           // Byte 4
            message_data[5] = (PMUInformation.quequeSizeCAN >> 8) & 0xFF;  
            memcpy(&message_data[6], PMUInformation.unused, 3);                                                                                                 // Bytes 5-8
            send_can_message(PMU_INFO_ID, message_data, 8);       
            timerCounter = 0;
        }

        //Send CAN Request for Config Data Only If Not Received Yet
        if(configRequestFlag == 0 && configRequestCounter >= 300) {
            uint8_t message_data[8] = {0};
            message_data[0] = 0x01;                                                                                               // Byte 1: Sequence Number
            message_data[1] = 0x01;
            message_data[2] = 0xFF;                                                                                                 // Byte 3
            message_data[3] = 0xFF;                                                                                                 // Byte 4
            memset(&message_data[4], 0xFF, 4);                                                                                               // Bytes 5-8
            send_can_message(PMU_REQUEST, message_data, 8);  
            //ESP_LOGI("CONFIG", "Requesting Configuration Data from MCU via CAN...");
            configRequestCounter=0;
        }

        receive_can_message();

        //+++++++++++++++++++ IPC code of Reading from CAN to IPC variables be written here ++++++++++++++++
        if(chargerConfigReceived == 1 /*&& chargerPeriodicReceived == 1*/){
             if(xSemaphoreTake(mutex, 0) == pdTRUE){
                // Copy the received CAN message to shared memory
                PMUInformation.quequeSizeCAN = PMUInformation.quequeSizeIPC;
                memcpy(&chargerPeriodicStatusIPC, &chargerPeriodicStatusCAN, sizeof(chargerPeriodicStatusIPC));
                memcpy(&chargerConfigIPC, &chargerConfigCAN, sizeof(chargerConfigIPC));
                memcpy(&powerModulesIPC, &powerModulesCAN, sizeof(powerModulesIPC));
                memcpy(&sessionInfoIPC,&sessionInfoCAN,sizeof(sessionInfoIPC));
                memcpy(&faultLogIPC,&faultLogCAN,sizeof(faultLogIPC));
                // Unlock the mutex after writing to shared memory
                xSemaphoreGive(mutex);
            }else{
                //ESP_LOGE(CAN_MCU_TAG, "Failed to take mutex for storing IPC message.");
            }
        }

        //if(faultLogCAN.messageReceived == 1)

        sessionInfoCounter++;

        vTaskDelay(pdMS_TO_TICKS(1));      // 1ms delay 
    }
}

void print_uint8_string(const uint8_t* array) {
    for (size_t i = 0; array[i] != '\0'; i++) {
        //printf("%c", array[i]);  // Print the character
    }
    //printf("\\0 ");  // Print null character explicitly
}

void request_config_data_from_MCU() {
    ESP_LOGI("CONFIG", "Requesting Configuration Data from MCU via CAN...");
    uint8_t message_data[8] = {0};
    message_data[0] = 0x01;                                                                                               // Byte 1: Sequence Number
    message_data[1] = 0x01;
    message_data[2] = 0xFF;                                                                                                 // Byte 3
    message_data[3] = 0xFF;                                                                                                 // Byte 4
    memset(&message_data[4], 0xFF, 4);                                                                                               // Bytes 5-8
    send_can_message(0x43, message_data, 8);       
}

//--------------------------------------------------Main Function -------------------------------
void write_nvs_value(const char *key, uint32_t new_value) {
    nvs_handle_t my_handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &my_handle) == ESP_OK) {
        nvs_set_u32(my_handle, key, new_value);
        nvs_commit(my_handle);
        nvs_close(my_handle);
    }
}

//Function to read a stored value from NVS
uint32_t read_nvs_value(const char *key) {
    nvs_handle_t my_handle;
    uint32_t value = 0; // Default to 0 if key is not found

    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &my_handle) == ESP_OK) {
        nvs_get_u32(my_handle, key, &value);
        nvs_close(my_handle);
    }
    return value;
}

void initialize_logging() {
    esp_log_level_set("transport_base", ESP_LOG_WARN);  // Hide errors from transport_base
    esp_log_level_set("mqtt_client", ESP_LOG_WARN);  // Hide errors from mqtt_client
    esp_log_level_set("esp-tls", ESP_LOG_WARN);  // Hide errors from esp-tls
    esp_log_level_set("*", ESP_LOG_WARN);
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set(MQTT_TAG, ESP_LOG_INFO);
    esp_log_level_set(SPIFFS_TAG, ESP_LOG_INFO);
    esp_log_level_set(NTP_TAG, ESP_LOG_INFO);
    esp_log_level_set("DEBUG", ESP_LOG_ERROR);
}

char* read_esp32_time() {
    struct timeval now;
    gettimeofday(&now, NULL); // Get the current time

    // Declare a local `struct tm` to hold the result
    struct tm timeinfo;
    
    // Use `gmtime_r()` to write the result into `timeinfo`
    gmtime_r(&now.tv_sec, &timeinfo);  // Thread-safe version of `gmtime()`

    // Allocate memory for the formatted string (30 characters including null-terminator)
    char *time_str = malloc(30);
    if (time_str == NULL) {
        return NULL;  // Return NULL if allocation fails
    }

    // Format the time (without milliseconds)
    size_t strftime_result = strftime(time_str, 20, "%Y-%m-%dT%H:%M:%S", &timeinfo);  // First 19 characters

    // Check if strftime was successful
    if (strftime_result == 0) {
        free(time_str);  // Free allocated memory in case of failure
        return NULL;     // Return NULL to indicate an error
    }

    // Append milliseconds and 'Z' for UTC
    snprintf(time_str + 19, 11, ".%03ldZ", now.tv_usec / 1000);  // Adds '.xxxZ' for milliseconds

    return time_str;  // Return the dynamically allocated string
}

void init_spiffs() {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = "spiffs",
        .max_files = 4,
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        //printf("SPIFFS init failed!\n");
    } else {
        //printf("SPIFFS mounted successfully!\n");
    }
}

//Function to compare timestamps in ISO 8601 format (excluding milliseconds)
int compare_timestamps(const char *ts1, const char *ts2) {
    struct tm tm1 = {0}, tm2 = {0};

    // Adjust format to ignore milliseconds and parse correctly
    strptime(ts1, "%Y-%m-%dT%H:%M:%S", &tm1);
    strptime(ts2, "%Y-%m-%dT%H:%M:%S", &tm2);

    time_t time1 = mktime(&tm1);
    time_t time2 = mktime(&tm2);

    return difftime(time1, time2);
}

int remove_oldest_message(const char *file_path) {
    ESP_LOGI("SPIFFS", "Opening file: %s", file_path);
    FILE *file = fopen(file_path, "r");
    if (!file) {
        ESP_LOGE("SPIFFS", "Failed to open file for reading");
        return -1;
    }
    
    FILE *temp_file = NULL;
    if (strcmp(file_path, FILEPATH) == 0)
        temp_file = fopen(TEMPFILEPATH, "w");
    else
        temp_file = fopen(TEMP_FAULT_LOG_FILEPATH, "w");

    if (!temp_file) {
        ESP_LOGE("SPIFFS", "Failed to open temp file for writing");
        fclose(file);
        return -1;
    }

    ESP_LOGI("SPIFFS", "Temp file opened successfully");

    //check_memory_status();
    char line[TOTAL_LINE];
    char oldest_line[TOTAL_LINE] = "";
    char oldest_timestamp[30] = "9999-99-99T99:99:99Z"; // Set to a future timestamp
    uint8_t found_oldest = 0;

    ESP_LOGI("SPIFFS", "Starting first pass to find the oldest message");
    while (fgets(line, sizeof(line), file) != NULL) {
        line[strcspn(line, "\n")] = 0;
        char *timestamp = strrchr(line, '|');
        if (timestamp) {
            *timestamp = '\0';
            timestamp++;
            ESP_LOGI("SPIFFS", "Comparing timestamps: %s with %s", timestamp, oldest_timestamp);
            if (compare_timestamps(timestamp, oldest_timestamp) < 0) {
                strcpy(oldest_timestamp, timestamp);
                strcpy(oldest_line, line);
                found_oldest = 1;
                ESP_LOGI("SPIFFS", "Found the oldest entry: %s|%s", oldest_line, oldest_timestamp);
            }
        } else {
            ESP_LOGW("SPIFFS", "Line does not contain a timestamp: %s", line);
        }
    }

    char oldest_entry[TOTAL_LINE];
    strcpy(oldest_entry, oldest_line);
    strcat(oldest_entry, "|");
    strcat(oldest_entry, oldest_timestamp);
    ESP_LOGI("SPIFFS", "Oldest entry to be deleted: %s", oldest_entry);
    ESP_LOGW("Buffer","Remove Oldest WALA");
    check_memory_status();
    rewind(file);
    int entry_count = 0;
    ESP_LOGI("SPIFFS", "Starting second pass to rewrite file excluding oldest message");
    while (fgets(line, sizeof(line), file) != NULL) {
        line[strcspn(line, "\n")] = 0;
        if (strcmp(line, oldest_entry) != 0) {
            fputs(line, temp_file);
            fputs("\n", temp_file);
            entry_count++;
            ESP_LOGI("SPIFFS", "Written to temp file: %s", line);
        } else {
            ESP_LOGI("SPIFFS", "Skipping oldest message: %s", line);
        }
    }

    fclose(file);
    fclose(temp_file);
    ESP_LOGI("SPIFFS", "File processing completed, replacing old log file");
    // remove(file_path);    //remove() fails (permission error).
    if (remove(file_path) == 0) {
        ESP_LOGI("SPIFFS", "File deleted successfully: %s", file_path);
    } else {
        ESP_LOGE("SPIFFS", "Error deleting file: %s", file_path);
    }

    if (strcmp(file_path, FILEPATH) == 0) {
        // rename(TEMPFILEPATH, file_path);
        rename(TEMPFILEPATH, FILEPATH);
    } else {
        rename(TEMP_FAULT_LOG_FILEPATH, file_path);
    }

    if (found_oldest) {
        ESP_LOGI("SPIFFS", "Removed oldest message: %s|%s", oldest_line, oldest_timestamp);
    } else {
        ESP_LOGE("SPIFFS", "No messages found to remove.");
    }

    ESP_LOGI("SPIFFS", "Remaining entries count: %d", entry_count);
    return entry_count;
}

//Function to append the payload to file 
void save_payload_to_spiffs(const char *filename, const char *payload) {  
        //ESP_LOGI("SPIFFS", "Entering save_payload_to_spiffs with filename: %s", filename);
        uint8_t file_select = 0;
        
        if(strcmp(filename, FILEPATH) == 0) {
            file_select = FILE_SELECT_REQUEST;
            ESP_LOGI("SPIFFS", "File identified as request file");
            if(fileInfo.request_file_entries >= MAX_QUEUE_SIZE) {
                ESP_LOGE("SPIFFS", "Max messages limit reached, removing oldest message for messages other than fault messages...");
                int count = remove_oldest_message(filename);
                if(count != -1) {
                    fileInfo.request_file_entries = count;
                    ESP_LOGW("SPIFFS", "Remaining Entries for messages other than fault: %d", fileInfo.request_file_entries);
                } else {
                    ESP_LOGE("SPIFFS", "Failed to remove the oldest message. Skipping new log entry.");
                    return;
                }
            }
        } else if(strcmp(filename, FAULT_LOG_FILEPATH_OnlineCharger) == 0) {
            file_select = FILE_SELECT_ONLINE_CHARGER;
            ESP_LOGI("SPIFFS", "File identified as OnlineCharger fault log");
            if(fileInfo.onlineCharger_file_entries >= MAX_QUEUE_SIZE_Fault) {
                ESP_LOGE("SPIFFS", "Max messages limit reached, removing oldest message for fault in OnlineCharger...");
                int count = remove_oldest_message(filename);
                if(count != -1) {
                    fileInfo.onlineCharger_file_entries = count;
                    ESP_LOGW("SPIFFS", "Remaining Entries for onlineChargerFaults: %d", fileInfo.onlineCharger_file_entries);
                } else {
                    ESP_LOGE("SPIFFS", "Failed to remove the oldest message. Skipping new log entry.");
                    
                    return;
                }
            }
        } else if(strcmp(filename, FAULT_LOG_FILEPATH_OfflineCharger) == 0) {
            file_select = FILE_SELECT_OFFLINE_CHARGER;
            ESP_LOGI("SPIFFS", "File identified as OfflineCharger fault log");
            if(fileInfo.offlineCharger_file_entries >= MAX_QUEUE_SIZE_Fault) {
                ESP_LOGE("SPIFFS", "Max messages limit reached, removing oldest message for fault in OfflineCharger...");
                int count = remove_oldest_message(filename);
                if(count != -1) {
                    fileInfo.offlineCharger_file_entries = count;
                    ESP_LOGW("SPIFFS", "Remaining Entries for offlineChargerFaults: %d", fileInfo.offlineCharger_file_entries);
                } else {
                    ESP_LOGE("SPIFFS", "Failed to remove the oldest message. Skipping new log entry.");
                    
                    return;
                }
            }
        } else {
            for (int i = 0; i < chargerConfigMQTT.numConnectors; i++) {
                if (strcmp(filename, fileNameConnector[i].logFilePath) == 0) {
                    file_select = FILE_SELECT_CONNECTOR_BASE + i;
                    ESP_LOGI("SPIFFS", "File identified as Connector %d fault log", i + 1);
                    if(fileNameConnector[i].entries >= MAX_QUEUE_SIZE_Fault) {
                        ESP_LOGE("SPIFFS", "Max messages limit reached, removing oldest message for fault in OfflineConnector: %d", i + 1);
                        int count = remove_oldest_message(filename);
                        if(count != -1) {
                            fileNameConnector[i].entries = count;
                            ESP_LOGW("SPIFFS", "Remaining Entries for offlineConnectorFaults: %d", fileNameConnector[i].entries);
                        } else {
                            ESP_LOGE("SPIFFS", "Failed to remove the oldest message. Skipping new log entry.");
                               
                            return;
                        }
                    }
                }
            }
        }
        
        ESP_LOGI("SPIFFS", "Generating timestamp");
        char timestamp[64];
        char *time_str = read_esp32_time();
        if(time_str != NULL) {
            strncpy(timestamp, time_str, sizeof(timestamp) - 1);
            timestamp[sizeof(timestamp) - 1] = '\0';
            free(time_str);
        } else {
            ESP_LOGE("TIME", "Failed to allocate memory for timestamp");
            
            return;
        }

        ESP_LOGI("SPIFFS", "Opening file for appending");
        FILE *f = fopen(filename, "a");
        if (f == NULL) {
            ESP_LOGE("SPIFFS", "Failed to open file for appending");
            
            return;
        }
        
        // ESP_LOGI("SPIFFS", "Writing payload to file");
        fprintf(f, "%s|%s\n", payload, timestamp);
        fclose(f);
        
        ESP_LOGI("SPIFFS", "Updating file entry counts");
        if(file_select == FILE_SELECT_REQUEST) {
            fileInfo.request_file_entries++;
        } else if(file_select == FILE_SELECT_ONLINE_CHARGER) {
            fileInfo.onlineCharger_file_entries++;
        } else if(file_select == FILE_SELECT_OFFLINE_CHARGER) {
            fileInfo.offlineCharger_file_entries++;
        } else if(file_select >= FILE_SELECT_CONNECTOR_BASE) {
            int index = file_select - FILE_SELECT_CONNECTOR_BASE;
            if(index < chargerConfigMQTT.numConnectors) { 
                fileNameConnector[index].entries++;
            }
        }
        //fileInfo.totalEntries = fileInfo.request_file_entries+fileInfo.onlineCharger_file_entries+fileInfo.offlineCharger_file_entries+fileNameConnector[0].entries+fileNameConnector[1].entries; 
        // print_file_entries(filename);

        // if (xSemaphoreTake(mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
        //     fileInfo.totalEntries = fileInfo.request_file_entries 
        //                            + fileInfo.onlineCharger_file_entries 
        //                            + fileInfo.offlineCharger_file_entries 
        //                            + fileNameConnector[0].entries 
        //                            + fileNameConnector[1].entries;
        //     xSemaphoreGive(mutex);                          // Very important!
        // }
        // else {
        //     ESP_LOGE("SPIFFS", "Failed to take Mutex for totalEntries update (in save_payload_to_spiffs)");
        // }
        
        //ESP_LOGI("SPIFFS", "Stored message: %s | Timestamp: %s | in file: %s", payload, timestamp, filename);
}

void removeMillisecondsAndZ(char *dateTime) {
    char *dotPos = strchr(dateTime, '.'); // Find first dot (.)
    if (dotPos) {
        *dotPos = '\0'; // Cut the string at the dot
    }
}

//Function to Initialise Grid Variables in chargerPeriodic
void initChargerGridValues(ChargerPeriodicStatus_t *charger) {
    for (int i = 0; i < MAX_GRID_PHASES; i++) {
        charger->gridVoltages[i] = FLT_MAX;
        charger->gridCurrents[i] = FLT_MAX;
        charger->acMeterLineVoltages[i] = FLT_MAX;
        charger->RealPowerFactor = FLT_MAX;
        charger->AvgPowerFactor = FLT_MAX;
        charger->dcVoltage[i] = FLT_MAX;
        charger->dcCurrent[i] = FLT_MAX;
    }
}

void app_main(void)
{   
    ESP_LOGI(MQTT_TAG, "[APP] Startup..");
    ESP_LOGI(MQTT_TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());

    //Initialize All CAN Members by Zero or Default
    memset(&chargerConfigCAN, 0, sizeof(chargerConfigCAN));
    chargerConfigCAN.configKeyValue = 0xFF;
    for(int i=0; i<MAX_PCM_MODULE; i++){
        strncpy(chargerConfigCAN.versionInfo.PCM[i], "CommunicationFault", sizeof(chargerConfigCAN.versionInfo.PCM[i]) - 1);
        chargerConfigCAN.versionInfo.PCM[i][sizeof(chargerConfigCAN.versionInfo.PCM[i]) - 1] = '\0';
        memset(&powerModulesCAN[i], 0xFF, sizeof(powerModulesCAN[i])); 
    }
    memset(&chargerPeriodicStatusCAN, 0, sizeof(chargerPeriodicStatusCAN));
    initChargerGridValues(&chargerPeriodicStatusCAN);
    memset(&sessionInfoCAN, 0xFF, sizeof(sessionInfoCAN));
    sessionInfoCAN.startDateTime[sizeof(sessionInfoCAN.startDateTime) - 1] = '\0';
    sessionInfoCAN.endDateTime[sizeof(sessionInfoCAN.endDateTime) - 1] = '\0';
    faultLogCAN.faultType = 0xF;  // Max 4-bit value (15)
    faultLogCAN.player = 0xF;     // Max 4-bit value (15)
    faultLogCAN.faultCode = 0xFF; // Max 8-bit value (255)
    faultLogCAN.occuranceDateTime[0] = '\0';
    faultLogCAN.clearanceDateTime[0] = '\0';
    chargerConfigCAN.receivedConfigDataMQTT = 0;
    chargerConfigCAN.limitSetting.chargerPower = FLT_MAX;
    chargerConfigCAN.limitSetting.gunACurrent = FLT_MAX;
    chargerConfigCAN.limitSetting.gunAPower = FLT_MAX;
    chargerConfigCAN.limitSetting.gunAVoltage = FLT_MAX;
    chargerConfigCAN.limitSetting.gunBCurrent = FLT_MAX;
    chargerConfigCAN.limitSetting.gunBPower = FLT_MAX;
    chargerConfigCAN.limitSetting.gunBVoltage = FLT_MAX;
    sessionInfoCAN.chargingSessionOccured = 1;
    sessionInfoCAN.prevChargingSession = 1;
    faultLogCAN.faultLogDataExchanged = 0;
    powerModulesCAN->dcCurrents.iTrf = FLT_MAX;
    powerModulesCAN->dcVoltages.fanVoltageMaster = FLT_MAX;
    powerModulesCAN->dcVoltages.fanVoltageSlave = FLT_MAX;
    PMUInformation.quequeSizeCAN = 0xFFFF;
    //ESP_LOGW("FAULT_LOG", LOG_COLOR_CYAN "Init faultLogDataExchangedCAN = %d" LOG_RESET_COLORCYAN, faultLogCAN.faultLogDataExchanged);

    //Initialize All IPC Members by Zero or Default 
    memset(&chargerConfigIPC, 0, sizeof(chargerConfigIPC));
    chargerConfigIPC.configKeyValue = 0xFF;
    for(int i=0; i<MAX_PCM_MODULE; i++){
        strncpy(chargerConfigIPC.versionInfo.PCM[i], "CommunicationFault", sizeof(chargerConfigIPC.versionInfo.PCM[i]) - 1);
        chargerConfigIPC.versionInfo.PCM[i][sizeof(chargerConfigIPC.versionInfo.PCM[i]) - 1] = '\0';
        memset(&powerModulesIPC[i], 0xFF, sizeof(powerModulesIPC[i])); 
    }
    memset(&chargerPeriodicStatusIPC, 0, sizeof(chargerPeriodicStatusIPC));
    initChargerGridValues(&chargerPeriodicStatusIPC);
    memset(&sessionInfoIPC, 0xFF, sizeof(sessionInfoIPC));
    sessionInfoIPC.startDateTime[sizeof(sessionInfoIPC.startDateTime) - 1] = '\0';
    sessionInfoIPC.endDateTime[sizeof(sessionInfoIPC.endDateTime) - 1] = '\0';
    faultLogIPC.faultType = 0xF;
    faultLogIPC.player = 0xF;
    faultLogIPC.faultCode = 0xFF;
    faultLogIPC.occuranceDateTime[0] = '\0';
    faultLogIPC.clearanceDateTime[0] = '\0';
    chargerConfigIPC.receivedConfigDataMQTT = 0;
    chargerConfigIPC.limitSetting.chargerPower = FLT_MAX;
    chargerConfigIPC.limitSetting.gunACurrent = FLT_MAX;
    chargerConfigIPC.limitSetting.gunAPower = FLT_MAX;
    chargerConfigIPC.limitSetting.gunAVoltage = FLT_MAX;
    chargerConfigIPC.limitSetting.gunBCurrent = FLT_MAX;
    chargerConfigIPC.limitSetting.gunBPower = FLT_MAX;
    chargerConfigIPC.limitSetting.gunBVoltage = FLT_MAX;
    sessionInfoIPC.chargingSessionOccured = 1;
    faultLogIPC.faultLogDataExchanged = 0;
    powerModulesIPC->dcCurrents.iTrf = FLT_MAX;
    powerModulesIPC->dcVoltages.fanVoltageMaster = FLT_MAX;
    powerModulesIPC->dcVoltages.fanVoltageSlave = FLT_MAX;
    PMUInformation.quequeSizeIPC = 0xFFFF;
    // ESP_LOGW("FAULT_LOG", LOG_COLOR_CYAN "Init faultLogDataExchangedIPC = %d" LOG_RESET_COLORCYAN, faultLogIPC.faultLogDataExchanged);

    //Initialize All MQTT Members by Zero or Default 
    memset(&chargerConfigMQTT, 0, sizeof(chargerConfigMQTT));
    chargerConfigMQTT.configKeyValue = 0xFF;
    for(int i=0; i<MAX_PCM_MODULE; i++){
        strncpy(chargerConfigMQTT.versionInfo.PCM[i], "CommunicationFault", sizeof(chargerConfigMQTT.versionInfo.PCM[i]) - 1);
        chargerConfigMQTT.versionInfo.PCM[i][sizeof(chargerConfigMQTT.versionInfo.PCM[i]) - 1] = '\0';
        memset(&powerModulesMQTT[i], 0xFF, sizeof(powerModulesMQTT[i])); 
    }
    memset(&chargerPeriodicStatusMQTT, 0, sizeof(chargerPeriodicStatusMQTT));
    initChargerGridValues(&chargerPeriodicStatusMQTT);
    memset(&sessionInfoMQTT, 0xFF, sizeof(sessionInfoMQTT));
    sessionInfoMQTT.startDateTime[sizeof(sessionInfoMQTT.startDateTime) - 1] = '\0';
    sessionInfoMQTT.endDateTime[sizeof(sessionInfoMQTT.endDateTime) - 1] = '\0';
    faultLogMQTT.faultType = 0xF;
    faultLogMQTT.player = 0xF;
    faultLogMQTT.faultCode = 0xFF;
    faultLogMQTT.occuranceDateTime[0] = '\0';
    faultLogMQTT.clearanceDateTime[0] = '\0';
    chargerConfigMQTT.receivedConfigDataMQTT = 0;
    chargerConfigMQTT.limitSetting.chargerPower = FLT_MAX;
    chargerConfigMQTT.limitSetting.gunACurrent = FLT_MAX;
    chargerConfigMQTT.limitSetting.gunAPower = FLT_MAX;
    chargerConfigMQTT.limitSetting.gunAVoltage = FLT_MAX;
    chargerConfigMQTT.limitSetting.gunBCurrent = FLT_MAX;
    chargerConfigMQTT.limitSetting.gunBPower = FLT_MAX;
    chargerConfigMQTT.limitSetting.gunBVoltage = FLT_MAX;
    sessionInfoMQTT.chargingSessionOccured = 1;
    faultLogMQTT.faultLogDataExchanged = 0;
    publishMsgFlag.charging_session = 1;
    publishMsgFlag.toggleFaultLog = 0;
    powerModulesMQTT->dcCurrents.iTrf = FLT_MAX;
    powerModulesMQTT->dcVoltages.fanVoltageMaster = FLT_MAX;
    powerModulesMQTT->dcVoltages.fanVoltageSlave = FLT_MAX;
    fileInfo.totalEntries = 0xFFFF;

    // Modified by Shikha
    uint8_t k=0;
    for(k=0;k<MAX_PCM_MODULE;k++){
        powerModulesCAN[k].CANFailure = 1;
        powerModulesCAN[k].CANSuccess = 0;
        powerModulesCAN[k].CANFailureCount = 0;

        powerModulesMQTT[k].CANFailure = 1;
        powerModulesMQTT[k].CANSuccess = 0;
        powerModulesMQTT[k].CANFailureCount = 0;
    }
 
    //Formation of PMU Information  
    PMUInformation.sequence_number_1 = 1;
    PMUInformation.pmu_major_version = PMU_MAJOR_VERSION;
    PMUInformation.pmu_minor_version = PMU_MINOR_VERSION;
    PMUInformation.reserved = 0x3F; // Set Bit 3-8 as 111111
    PMUInformation.unused[0] = 0xFF; //Set Unused Bytes
    PMUInformation.unused[1] = 0xFF;
    PMUInformation.unused[2] = 0xFF;
    PMUInformation.unused[3] = 0xFF;

    //Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    
    memset(&flagConfig, 0xFF, sizeof(flagConfig));

    //To Fetch Uptime and Downtime @ Startup 
    open_ccg_namespace();
    fetch_uptime_keys();  //Boot Up fetch of keys to know if Config data has been sent once.

    //Initialize the SPIFFS 
    init_spiffs();

    //Count entries in FILEPATH
    fileInfo.request_file_entries = count_file_entries(FILEPATH);
    ESP_LOGI("TAG", "Entries in FILEPATH: %d", fileInfo.request_file_entries);

    //Count entries in FAULT_LOG_FILEPATH_OnlineCharger
    fileInfo.onlineCharger_file_entries = count_file_entries(FAULT_LOG_FILEPATH_OnlineCharger);
    ESP_LOGI("TAG", "Entries in Online Charger Log: %d", fileInfo.onlineCharger_file_entries);

    //Count entries in FAULT_LOG_FILEPATH_OfflineCharger
    fileInfo.offlineCharger_file_entries = count_file_entries(FAULT_LOG_FILEPATH_OfflineCharger);
    ESP_LOGI("TAG", "Entries in Offline Charger Log: %d", fileInfo.offlineCharger_file_entries);

    //Initialize the mutex
    mutex = xSemaphoreCreateBinary();
    xSemaphoreGive(mutex);
    mutex_spiffs = xSemaphoreCreateBinary();
    xSemaphoreGive(mutex_spiffs);

    //CAN Initialize
    twai_setup();

    //CAN Task for Periodic Transmission of PMU information from PMU --> MCU 
    xTaskCreatePinnedToCore(can_task, "CAN_Task", 4096, NULL, 4, NULL, 1);
   
    //Create Notify Task on Core-0
    BaseType_t result = xTaskCreatePinnedToCore(process_MQTT_msg, "process_MQTT_msg", 8192, NULL, 10, &process_MQTT_msg_handler, 1);

    // Check if Notify task creation was successful
    if (result == pdPASS) {
        ESP_LOGI("Main", "Successfully created process_MQTT_msg task");
    }else{
        ESP_LOGE("Main", "Failed to create process_MQTT_msg task, error code: %d", result);
    }

    initialize_logging();

    while(1){

        vTaskDelay(pdMS_TO_TICKS(100));
        uint8_t is_message_important = 0;
        
        if(fileEntryReceived){
            fileInfo.totalEntries = fileInfo.request_file_entries + fileInfo.onlineCharger_file_entries + fileInfo.offlineCharger_file_entries;
            for (int i = 0; i < chargerConfigMQTT.numConnectors; i++)
            {
                fileInfo.totalEntries += fileNameConnector[i].entries;
            }
            printf(" The Queque Size in PMU is: %d \n", fileInfo.totalEntries);
        }
        
        if(wifiConnected){
            check_internet_status();
        }

         //Initialization Wifi Connectivity 
        if(strlen(chargerConfigMQTT.wifi_ssid) != 0 && strlen(chargerConfigMQTT.wifi_password) != 0 && wifiConnected == false && chargerConfigMQTT.receivedConfigDataMQTT == 1)
        {
            if(strcmp(wifi_ssid,chargerConfigMQTT.wifi_ssid) != 0 || strcmp(wifi_password,chargerConfigMQTT.wifi_password) != 0)
            {
                ESP_LOGI("WiFi -- Main", "1st Time Received SSID: %s, Password: %s", chargerConfigMQTT.wifi_ssid, chargerConfigMQTT.wifi_password);

                strncpy(wifi_ssid, chargerConfigMQTT.wifi_ssid, sizeof(wifi_ssid) - 1);
                wifi_ssid[sizeof(wifi_ssid) - 1] = '\0'; // Ensure null termination

                strncpy(wifi_password, chargerConfigMQTT.wifi_password, sizeof(wifi_password) - 1);
                wifi_password[sizeof(wifi_password) - 1] = '\0'; // Ensure null termination

                //Intialize the WiFi Settings
                if(!wifiInitialised){
                    ESP_LOGI("DEBUG", "1st time running to init wifi");
                    wifi_init();
                }
            }
        }
        else if(strlen(chargerConfigMQTT.wifi_ssid) != 0 && strlen(chargerConfigMQTT.wifi_password) != 0 && wifiConnected == true)
        {
            if(strcmp(wifi_ssid,chargerConfigMQTT.wifi_ssid) != 0 || strcmp(wifi_password,chargerConfigMQTT.wifi_password) != 0)
            {
                ESP_LOGI("WiFi -- Main", "Updated SSID: %s, Password: %s", chargerConfigMQTT.wifi_ssid, chargerConfigMQTT.wifi_password);

                //Disconnect from the current Wi-Fi connection
                esp_wifi_disconnect();    

                strncpy(wifi_ssid, chargerConfigMQTT.wifi_ssid, sizeof(wifi_ssid) - 1);
                wifi_ssid[sizeof(wifi_ssid) - 1] = '\0'; // Ensure null termination

                strncpy(wifi_password, chargerConfigMQTT.wifi_password, sizeof(wifi_password) - 1);
                wifi_password[sizeof(wifi_password) - 1] = '\0'; // Ensure null termination

                //Intialize the WiFi Settings
                esp_wifi_connect();
            }
        } 

        //Create MQTT Task on Core-0
        if(firstTimeBootUpFlag == 1 && chargerConfigMQTT.receivedConfigDataMQTT == 1 && wifiInitialised)
        {   
            init_mqtt();
            xTaskCreatePinnedToCore(mqtt_task, "MQTT_Task", 9126, NULL, 6, NULL, 0);
            firstTimeBootUpFlag = 2;
        }

        if(firstTimeBootUpFlag == 2){
            char receivedDateTime[25];
            //Create a copy by allocating new memory , and using strcpy
            strcpy(receivedDateTime,chargerConfigMQTT.dateTime);
            removeMillisecondsAndZ(receivedDateTime);

            struct tm timeinfo = { 0 };

            // Parse the copied datetime string
            if(strptime(receivedDateTime, "%Y-%m-%dT%H:%M:%S", &timeinfo) != NULL) {
                time_t epoch_time = mktime(&timeinfo); // Convert struct tm to epoch time
                struct timeval tv = { .tv_sec = epoch_time, .tv_usec = 0 };

                if (settimeofday(&tv, NULL) == 0) {
                    ESP_LOGI("RTC", "RTC successfully set from CAN: %s", asctime(&timeinfo));
                } else {
                    ESP_LOGE("RTC", "Failed to set RTC from CAN");
                }
            }else {
                ESP_LOGE("RTC", "Invalid datetime format received from CAN: %s", receivedDateTime);
            }
            firstTimeBootUpFlag = 3;
            ntpInitFlag = 1;
        }

        if(ntpInitFlag == 1 && wifiConnected && PMUInformation.internet_status ){
            ntp_init();
            ntpInitFlag = 0;
        }
        
        if(IS_MQTT_NETWORK_ONLINE && isTimeSet) {
            uint64_t currentTime = esp_timer_get_time()/1000;
            if(msg == NULL && publishMsgFlag.configDataExchanged){
                msg = get_queued_fault_messages(chargerConfigMQTT.numConnectors);
                is_message_important = 3;
                if(msg == NULL)
                {
                    is_message_important = 0;
                }
            }
            if(msg == NULL)
            {
                msg = get_queued_request_messages(&is_message_important);
                if(is_message_important == 2 && !publishMsgFlag.configDataExchanged)
                {
                    free(msg);  // Free the dynamically allocated message
                    msg = NULL;
                }
            }
            if(msg){
                //Check if the message is the same as the last sent message
                if (last_sent_message != NULL && strcmp(last_sent_message, msg) == 0 && currentTime - lastCheckTime < RETRY_INTERVAL) {
                    //ESP_LOGI(SPIFFS_TAG, "Message is identical to the last sent message, delaying before retry...// Same Entry Hai");
                }
                else{
                    lastCheckTime = currentTime;
                    // Attempt to send the message over WebSocket
                    // Initialize WebSocket client and send the message
                    if (esp_mqtt_client_publish(mqtt_client, pubTopic, msg, 0, 1, 0) != -1)
                    {
                        ESP_LOGW(SPIFFS_TAG, "Message sent: %s", msg);
                        if(last_sent_message != NULL){
                            if(strcmp(last_sent_message, msg)==0)
                            {
                                if(!is_message_important){
                                    retry_count++;
                                    ESP_LOGW(SPIFFS_TAG, "Retry Count: %u", retry_count);
                                }
                                else{
                                    critical_retry_count++; // retry count for important messages
                                    ESP_LOGW(SPIFFS_TAG, "Retry Count: %u", critical_retry_count);
                                    if(critical_retry_count >= 3){
                                        // critical_retry_count = 0;
                                        ESP_LOGW(SPIFFS_TAG, "Deleting critical Entry");
                                        ESP_LOGW("DEBUG1","last critical msg sent : %s", last_sent_message);
                                        delete_entry(last_sent_message);
                                    }
                                }
                                
                                if(retry_count >= 3){
                                    retry_count = 0;
                                    //Add a condition to not delete 'ConfigurationData'
                                    ESP_LOGW(SPIFFS_TAG, "Deleting Entry");
                                    ESP_LOGW("DEBUG1","last msg sent : %s", last_sent_message);
                                    int count = search_and_delete_message_from_any_file(last_sent_message);
                                    // if(count)
                                    // {
                                    //     fileInfo.request_file_entries = count;
                                    // }
                                    // delete_entry(last_sent_message);
                                }
                            }
                            else
                                retry_count = 0;
                        }
                        // Update the last_sent_message
                        free(last_sent_message);  // Free the old message
                        last_sent_message = strdup(msg);  // Store the new message
                    }
                }
                free(msg);
                ESP_LOGW("Buffer","RETRY WALA");
                check_memory_status();
                msg = NULL;
            } 
        }

        //+++++++++++++++++++ IPC code of Reading to be written here ++++++++++++++++
        if(xSemaphoreTake(mutex, (TickType_t)10) == pdTRUE){
            // Copy the received IPC message to MQTT variables
            if(publishMsgFlag.configDataExchanged == 1){
                check_config_data_changes();
            }
            PMUInformation.quequeSizeIPC = fileInfo.totalEntries;               //IPC <-- MQTT
            memcpy(&chargerPeriodicStatusMQTT, &chargerPeriodicStatusIPC, sizeof(chargerPeriodicStatusMQTT));
            memcpy(&chargerConfigMQTT, &chargerConfigIPC, sizeof(chargerConfigMQTT));
            memcpy(&powerModulesMQTT, &powerModulesIPC, sizeof(powerModulesMQTT));
            if(isConfigurationChanged == true){
                isConfigurationChanged = false;
                publishMsgFlag.config_data = 1;
            }
            xSemaphoreGive(mutex);
        }
    
        /*
        if(countPerioidicCAN > 10 && countPerioidicCAN <= 200 && reminder == 0)
        {

        printf("\n----------------- Charger Periodic Status -----------------");

        printf("Counter Periodic CAN: %" PRIu64 "\n", countPerioidicCAN);

        printf("\nGrid Voltages: [%.1f V, %.1f V, %.1f V]\n",
           chargerPeriodicStatusCAN.gridVoltages[0],
           chargerPeriodicStatusCAN.gridVoltages[1],
           chargerPeriodicStatusCAN.gridVoltages[2]);

    printf("AC Meter ModBus: %d\n", (int)chargerPeriodicStatusCAN.energyMeterComm[0]);

    printf("\nGrid Currents (A):\n");
    printf("Phase A: %.1f A\n", chargerPeriodicStatusCAN.gridCurrents[0]);
    printf("Phase B: %.1f A\n", chargerPeriodicStatusCAN.gridCurrents[1]);
    printf("Phase C: %.1f A\n", chargerPeriodicStatusCAN.gridCurrents[2]);

    printf("\nAC Meter Line Voltages (V):\n");
    printf("Line A: %.1f V\n", chargerPeriodicStatusCAN.acMeterLineVoltages[0]);
    printf("Line B: %.1f V\n", chargerPeriodicStatusCAN.acMeterLineVoltages[1]);
    printf("Line C: %.1f V\n", chargerPeriodicStatusCAN.acMeterLineVoltages[2]);

    //printf("\nSequence Number: 4\n");
    printf("The Active Energy: %" PRIu32 "Wh\n", chargerPeriodicStatusCAN.energyMeter[0]);
    printf("Real Power Factor: %.2f\n", chargerPeriodicStatusCAN.RealPowerFactor);
    printf("Average Power Factor: %.2f\n", chargerPeriodicStatusCAN.AvgPowerFactor);

    printf("\nEnergy Meter (Wh):\n");
    printf("Energy Meter 1:%" PRIu32 "Wh\n", chargerPeriodicStatusCAN.energyMeter[1]);
    printf("DC Meter ModBus Comm: %d\n", (int)chargerPeriodicStatusCAN.energyMeterComm[1]);

    printf("\nDC Meter 1:\n");
    printf("Voltage: %.1f V\n", chargerPeriodicStatusCAN.dcVoltage[0]);
    printf("Current: %.1f A\n", chargerPeriodicStatusCAN.dcCurrent[0]);
    printf("Power: %d W\n", (int)chargerPeriodicStatusCAN.dcMeterPower[0]);

    printf("\nEnergy Meter in Wh:%" PRIu32 "Wh\n", chargerPeriodicStatusCAN.energyMeter[2]);
    printf("Energy Meter Comm: %d\n", (int)chargerPeriodicStatusCAN.energyMeterComm[2]);

    printf("\nDC Voltage and Current (A):\n");
    printf("DC Voltage A: %.1f V\n", chargerPeriodicStatusCAN.dcVoltage[1]);
    printf("DC Current A: %.1f A\n", chargerPeriodicStatusCAN.dcCurrent[1]);
    printf("DC Power A: %d W\n", (int)chargerPeriodicStatusCAN.dcMeterPower[1]);

    printf("\nLCU Communication Status:\n");
    printf("LCU Comm Status: %d\n", chargerPeriodicStatusCAN.LCUCommStatus);
    printf("LCU Internet Status: %d\n", chargerPeriodicStatusCAN.LCUInternetStatus);
    printf("LCU WebSocket Status: %d\n", chargerPeriodicStatusCAN.LCUWebsocketStatus);

    printf("\nEV-EVSE Communication:\n");
    printf("EV-EVSE Comm 1 Recv: %d\n", (int)chargerPeriodicStatusCAN.ev_evseCommRecv[0]);
    printf("EV-EVSE Comm 1 Error: %d\n", (int)chargerPeriodicStatusCAN.ev_evseCommControllers[0].ErrorCode0Recv);
    printf("EV-EVSE Comm 2 Recv: %d\n", (int)chargerPeriodicStatusCAN.ev_evseCommRecv[1]);
    printf("EV-EVSE Comm 2 Error: %d\n", (int)chargerPeriodicStatusCAN.ev_evseCommControllers[1].ErrorCode0Recv);

    printf("\nVirtual Energy Meter Reading:%" PRIu32 "Wh\n",chargerPeriodicStatusCAN.virtualEnergyMeter[2]);

    printf("---------------------------------------------------------\n");
      }*/ 
        
        /*if(printChargerConfigData == 1){
            printChargerConfigData = 0;
            printf("The Squence Number is 1 \n");
            printf("Num Connector in CAN : %d \n",chargerConfigCAN.numConnectors);
            printf("Num PCMs CAN: %d \n",chargerConfigCAN.numPCM);
            printf("power Input Type CAN: %d \n",chargerConfigCAN.powerInputType);
            printf("Dynamic Power Sharing CAN: %d \n",chargerConfigCAN.DynamicPowerSharing);
            printf("Energy Meter Setting CAN: %d \n",chargerConfigCAN.EnergyMeterSetting);
            printf("Project Info CAN: %d \n",chargerConfigCAN.projectInfo);
            printf("Brand CAN: %d \n",chargerConfigCAN.Brand);
            printf("%c ",chargerConfigCAN.serialNumber[0]);
            printf("%c ",chargerConfigCAN.serialNumber[1]);
            printf("%c ",chargerConfigCAN.serialNumber[2]);
            printf("%c ",chargerConfigCAN.serialNumber[3]);

            printf("The Squence Number is 2 \n");
            printf("%c ",chargerConfigCAN.serialNumber[4]);
            printf("%c ",chargerConfigCAN.serialNumber[5]);
            printf("%c",chargerConfigCAN.serialNumber[6]);
            printf("%c ",chargerConfigCAN.serialNumber[7]);
            printf("%c ",chargerConfigCAN.serialNumber[8]);
            printf("%c ",chargerConfigCAN.serialNumber[9]);
            printf("%c ",chargerConfigCAN.serialNumber[10]);

            printf("The Squence Number is 3 \n");
            printf("%c ",chargerConfigCAN.serialNumber[11]);
            printf("%c ",chargerConfigCAN.serialNumber[12]);
            printf("%c ",chargerConfigCAN.serialNumber[13]);
            printf("%c ",chargerConfigCAN.serialNumber[14]);
            printf("%c ",chargerConfigCAN.serialNumber[15]);
            printf("%c ",chargerConfigCAN.serialNumber[16]);
            printf("%c ",chargerConfigCAN.serialNumber[17]);

            printf("The Squence Number is 4 \n");
            printf("%c ",chargerConfigCAN.serialNumber[18]);
            printf("%c ",chargerConfigCAN.serialNumber[19]);
            printf(" %c ",chargerConfigCAN.serialNumber[20]);
            printf(" %c ",chargerConfigCAN.serialNumber[21]);
            printf(" %c ",chargerConfigCAN.serialNumber[22]);
            printf(" %c ",chargerConfigCAN.serialNumber[23]);
            printf(" %c ",chargerConfigCAN.serialNumber[24]);
            // printf("\n Serial Number at the Startup : %s",chargerConfigCAN.serialNumber);

            printf("The Squence Number is 5 \n");
            printf("The parameter Number is : %d \n",chargerConfigCAN.parameterName);

            printf("Nothing to be Changed \n");
            printf("Value is AutoCharge CAN: %d \n",chargerConfigCAN.AutoCharge);  
            printf("Value is configKeyValue CAN: %d \n",chargerConfigCAN.configKeyValue);
            printf("The value of chargePower is CAN: %.2f \n",chargerConfigCAN.limitSetting.chargerPower);                                                  
            printf("The value of gunAVoltage is CAN: %.2f \n",chargerConfigCAN.limitSetting.gunAVoltage);                                                   
            printf("The value of gunACurrent is CAN: %.2f \n ",chargerConfigCAN.limitSetting.gunACurrent);
            printf("The value of gunBPower is CAN: %.2f \n",chargerConfigCAN.limitSetting.gunAPower);                                   
            printf("The value of gunBVoltage is CAN: %.2f \n",chargerConfigCAN.limitSetting.gunBVoltage);                                       
            printf("The value of gunBCurrent is CAN: %.2f \n",chargerConfigCAN.limitSetting.gunBCurrent);                                   
            printf("The value of gunBPower is CAN: %.2f \n",chargerConfigCAN.limitSetting.gunBPower );
            printf("The value of serverChargerPower is CAN: %.2f \n",chargerConfigCAN.limitSetting.serverChargerPower);
            printf("The value of serverConnectorACurrent is CAN: %.2f \n",chargerConfigCAN.limitSetting.serverConnectorACurrent);
            printf("The value of serverConnectorAPower is CAN: %.2f \n",chargerConfigCAN.limitSetting.serverConnectorAPower );
            printf("The value of serverConnectorBPower is CAN: %.2f \n",chargerConfigCAN.limitSetting.serverConnectorBPower );
            printf("The value of serverConnectorBCurrent is CAN: %.2f \n",chargerConfigCAN.limitSetting.serverConnectorBCurrent );
            printf("The value of dateTime is CAN: %s \n",chargerConfigCAN.dateTime );
            
            printf("String Length of Version Information is CAN %d \n",chargerConfigCAN.stringLength);
            printf("Broker : %s\n", chargerConfigCAN.mqtt_broker);
            print_uint8_string((const uint8_t*)chargerConfigCAN.versionInfo.hmiVersion);
            print_uint8_string((const uint8_t*)chargerConfigCAN.versionInfo.lcuVersion);
            print_uint8_string((const uint8_t*)chargerConfigCAN.versionInfo.mcuVersion);
            print_uint8_string((const uint8_t*)chargerConfigCAN.versionInfo.evseCommContoller[0]);
            print_uint8_string((const uint8_t*)chargerConfigCAN.versionInfo.evseCommContoller[1]);
            print_uint8_string((const uint8_t*)chargerConfigCAN.versionInfo.PCM[0]);
            print_uint8_string((const uint8_t*)chargerConfigCAN.ocppURL);
            print_uint8_string((const uint8_t*)chargerConfigCAN.wifi_ssid);
            print_uint8_string((const uint8_t*)chargerConfigCAN.wifi_password);
            print_uint8_string((const uint8_t*)chargerConfigCAN.serialNumber2);
            //printf("Tarrif Plan for 9 is %.1f \n", chargerConfigCAN.tarrifAmount);

            printf("The value of counterAck %d \n", counterAck);
            printf("The value of counterIncoming %d \n", counterIncomingMsg);
            
    }  */    
}
}