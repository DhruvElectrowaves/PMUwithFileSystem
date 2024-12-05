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

// This Tag will be used for Logging MQTT Information, Errors, and Warnings.
static const char *MQTT_TAG = "PMU_Charger";
esp_mqtt_client_handle_t mqtt_client;

// This are the variables used for storing Wi-fi Initialize credentials 

char wifi_ssid[32] = {0};
char wifi_password[64] = {0};

//+++++++++++++++++ For Testing Purpose we have set the Broker +++++++++++++++++++
char mqtt_broker_uri[] = "mqtt://test.mosquitto.org:1883";
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

char charger_Id[20];	// This Variable would Hold the charger Id received from MQTT Broker 

//+++++++++++++++++ For Testing Purpose we have set the Serial Number +++++++++++++++++++
char Serial_Number[25] = "PE110ACDC00602I02G2300001";  
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

uint16_t component_Id ;  //This would hold the value of component whose status i want to know 

//Defining Variables to Store Topics that Charger Has Subscribed to 
char sub_topic1[65];   //   "setChargerId/SerialNumber"
char sub_topic2[65];   //
char sub_topic3[65];
char sub_topic4[65];

//This structure is defined for Subscription and Checking.
typedef struct {
    char topic[60];
    char data[500];
} mqtt_message_t;

//The Declaration of variables to store data received from CAN 
FaultInfo_t faultInfoCAN;                                  // Store fault Info from MCU via CAN
SessionInfo_t sessionInfoCAN;							   // Store sessionInfo from MCU via CAN

powerModule_t powerModulesCAN[MAX_PCM_MODULE];			   // Store PCM module from PCM via CAN
powerModule_t powerModulesIPC[MAX_PCM_MODULE];
powerModule_t powerModulesMQTT[MAX_PCM_MODULE];

chargerConfig_t chargerConfigCAN;
ChargerPeriodicStatus_t chargerPeriodicStatusCAN;

publishMsgFlagType publishMsgFlag;											//Structure Variable for Flags
//The two structure for Sending CAN information to MCU
can_message_t PMUInformation;
CAN_Message AckPmu;

int timerCounter = 0;  // To keep the timer check on CAN task

//This is flag that would be raised whenever PMU received any message and will send an acknowlegment
uint8_t pmuAckFlag = 0;

//Variables Added from Niharika Ma'am Project
uint32_t counterPeriodicPCM = 0;
uint32_t counterPeriodicCharger = 0;
void publishMsgTask(void *client);
void mqtt_task(void *client);
int global_component_value = 0;

//For Publishing to Topic "chargerStatus/chargerId" ----- Dhruv Code 
ConnectorStatusEnum connectorStatus[3];
WebSocketStatusEnum webSocketStatus;
faultStatusEnum faultStatus[3];

//For Publishing to Topic "chargingSession/chargerId" --- Dhruv Code 
ConnectorEnum connector;
EventEnum event ;
FinishReasonEnum finishReason;

//For Publishing to Topic "sendConfigData/chargerId"  ---- Dhruv Code 
valueOfSettingEnum AutoCharge;
valueOfSettingEnum DynamicPowerSharing;
BrandEnum Brand;
energyMeterSettingEnum EnergyMeterSetting;

//+++++++++++++++++++++++ Variables to Store the topic and payload to send getChargerId/SerialNumber
char get_Charger_Id_topic[50];				
char get_Charger_Id_payload[256];
//+++++++++++++++++++++++ Variables to Store the topic and payload to send chargerStatus/chargerId
char status_topic[50];
char status_payload[256];
//++++++++++++++++++++++++Variables to Store the topic and payload to send chargingSession/chargerId
char charging_session_topic[50];
char charging_session_payload[256];
//++++++++++++++++++++++++Variables to Store the topic and payload to send sendConfigData/chargerId
char send_config_data_topic[50];
char send_config_data_payload[512];

// -----------------------------------WiFi Functionality ------------------------------------------------
void store_wifi_credentials(const char *ssid, const char *password) {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    if (err == ESP_OK) {
        nvs_set_str(nvs_handle, "wifi_ssid", ssid);
        nvs_set_str(nvs_handle, "wifi_password", password);
        nvs_commit(nvs_handle);
        nvs_close(nvs_handle);
        ESP_LOGI(MQTT_TAG, "Wi-Fi credentials stored in NVS");
    } else {
        ESP_LOGE(MQTT_TAG, "Error (%s) opening NVS handle", esp_err_to_name(err));
    }
}

// Function to read Wi-Fi credentials from NVS
bool read_wifi_credentials(char *ssid, size_t ssid_size, char *password, size_t password_size) {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("storage", NVS_READONLY, &nvs_handle);
    if (err == ESP_OK) {
        err = nvs_get_str(nvs_handle, "wifi_ssid", ssid, &ssid_size);
        if (err == ESP_OK) {
            err = nvs_get_str(nvs_handle, "wifi_password", password, &password_size);
            if (err == ESP_OK) {
                ESP_LOGI(MQTT_TAG, "Wi-Fi credentials read from NVS");
                nvs_close(nvs_handle);
                return true;
            }
        }
        nvs_close(nvs_handle);
    }
    ESP_LOGW(MQTT_TAG, "No Wi-Fi credentials found in NVS");
    return false;
}

// Wi-Fi event handler
void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(MQTT_TAG, "Disconnected from WiFi, attempting to reconnect...");
        PMUInformation.internet_status = 0;
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(MQTT_TAG, "Connected to WiFi with IP Address: " IPSTR, IP2STR(&event->ip_info.ip));
        PMUInformation.internet_status = 1;
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

    if (read_wifi_credentials(wifi_ssid, sizeof(wifi_ssid), wifi_password, sizeof(wifi_password))) {
        ESP_LOGI(MQTT_TAG, "Using stored Wi-Fi credentials: SSID=%s", wifi_ssid);
        ESP_LOGI(MQTT_TAG, "Using stored Wi-Fi credentials: Password=%s", wifi_password);
    } else {
        // Default values (or replace with dynamic input)
        strncpy(wifi_ssid, "ApnaBanda", sizeof(wifi_ssid));
        strncpy(wifi_password, "12345abc", sizeof(wifi_password));
        store_wifi_credentials(wifi_ssid, wifi_password);
    }

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


// ----------------------------------- MQTT Functionality ---------------------------------------------

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(MQTT_TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    
    int msg_id;
    
    switch (event_id) {
		
        case MQTT_EVENT_CONNECTED:
        
            ESP_LOGI(MQTT_TAG, "MQTT_EVENT_CONNECTED");
         	PMUInformation.broker_status=1;
            
          // Subscribe to the CMS topic (replace "setChargerId/SerialNumber" with the actual topic name from the CMS)
    		snprintf(sub_topic1, sizeof(sub_topic1), "setChargerId/%s", Serial_Number);
        	int msg_id = esp_mqtt_client_subscribe(mqtt_client, sub_topic1, 1);
     		ESP_LOGI(MQTT_TAG, "Subscribed to CMS topic with msg_id=%d", msg_id);
     		break;
            
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(MQTT_TAG, "MQTT_EVENT_DISCONNECTED");
            PMUInformation.broker_status=0;
            esp_mqtt_client_reconnect(mqtt_client);
            break;
             
        case MQTT_EVENT_DATA:
        
           ESP_LOGI(MQTT_TAG, "Received data from topic: %.*s", event->topic_len, event->topic);
		   ESP_LOGI(MQTT_TAG, "Data: %.*s", event->data_len, event->data);
		   
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
            
            if (strncmp(message.topic, sub_topic1, sizeof(message.topic)) == 0) {
				
				if (strlen(charger_Id) == 0){
                 ESP_LOGI(MQTT_TAG, "Processing setChargerId/%s logic", Serial_Number);
                
                 strncpy(charger_Id, message.data, sizeof(charger_Id) - 1);
                 charger_Id[sizeof(charger_Id) - 1] = '\0';
                 printf("The charger Id is %s\n", charger_Id);
                 publishMsgFlag.subscribeFlag = 1;
            	 
            	 nvs_handle_t nvs_handle;
            	 esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
            	 esp_err_t ret = nvs_set_str(nvs_handle, "charger_id", charger_Id);
            	 if (ret == ESP_OK) {
                          ESP_LOGI(MQTT_TAG, "Charger ID saved to NVS: %s", charger_Id);
                          if (nvs_commit(nvs_handle) == ESP_OK) {
                                 ESP_LOGI(MQTT_TAG, "NVS commit successful");
                        } else {
                                  ESP_LOGE(MQTT_TAG, "Failed to commit NVS");
                        }  
                 } else {
                     		ESP_LOGE(MQTT_TAG, "Failed to save Charger ID to NVS");
                }
            	nvs_close(nvs_handle);}
            } else if (strncmp(message.topic, sub_topic2, sizeof(message.topic)) == 0) {
				
				ESP_LOGI(MQTT_TAG, "Processing getComponentStatus/%s logic", charger_Id);
				component_Id = atoi(message.data);
				//publishMsgFlag.send_component_status = 1;
				//Raise A Flag to denote this condition has occured 
			}
            else if (strncmp(message.topic, sub_topic3, sizeof(message.topic)) == 0){
				
				ESP_LOGI(MQTT_TAG, "Processing OTA/%s", charger_Id);
				//Rest Logic to be Implemented
			}	
            else if(strncmp(message.topic, sub_topic4, sizeof(message.topic)) == 0)	{
				ESP_LOGI(MQTT_TAG, "receivedConfigData/%s", charger_Id);
				//Rest Logic to be Implemented
			}            
             break;
             
			
		case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(MQTT_TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
            
        default:
            ESP_LOGI(MQTT_TAG, "Other event id:%d", event->event_id);
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
	
		const TickType_t xDelay = pdMS_TO_TICKS(100);  //100 ms Delay 
	
		while(1) {
			esp_mqtt_client_handle_t client = (esp_mqtt_client_handle_t) pvParameters;
			char* json_string_allMsgs = NULL;
			cJSON* json_object_componentStatus = NULL;
			cJSON *PCMCommArray = cJSON_CreateArray();
			char topicName[50];
			
			if (strlen(charger_Id) == 0){
			prepare_mqtt_get_Charger_Id_data(Serial_Number, get_Charger_Id_topic, get_Charger_Id_payload);
			int msg_id0 = esp_mqtt_client_publish(mqtt_client, get_Charger_Id_topic, get_Charger_Id_payload, 0, 1, 0);
			ESP_LOGI(MQTT_TAG, "Published to topic %s with payload %s, msg_id=%d", get_Charger_Id_topic, get_Charger_Id_payload, msg_id0);
	     	}
			
			if(publishMsgFlag.subscribeFlag == 1){
				
				//Subscribe to the CMS topic "getComponentStatus/chargeId"
			   snprintf(sub_topic2, sizeof(sub_topic2), "getComponentStatus/%s" ,charger_Id);
         	   int msg_subscribe_id2 = esp_mqtt_client_subscribe(mqtt_client, sub_topic2, 1);
         	   ESP_LOGI(MQTT_TAG, "Subscribed to CMS topic %s with msg_id=%d",sub_topic2, msg_subscribe_id2);

         	   //Subscribe to the CMS topic "OTA/chargeId"
         	   snprintf(sub_topic3, sizeof(sub_topic3), "OTA/%s", charger_Id);
         	   int msg_subscribe_id3 = esp_mqtt_client_subscribe(mqtt_client, sub_topic3, 1);
         	   ESP_LOGI(MQTT_TAG, "Subscribed to CMS topic %s with msg_id=%d",sub_topic3, msg_subscribe_id3);
         	   
         	   snprintf(sub_topic4, sizeof(sub_topic4), "receivedConfigData/%s", charger_Id);
         	   int msg_subscribe_id4 = esp_mqtt_client_subscribe(mqtt_client, sub_topic4, 1);
         	   ESP_LOGI(MQTT_TAG, "Subscribed to CMS topic %s with msg_id=%d",sub_topic4, msg_subscribe_id4);
				
			   publishMsgFlag.subscribeFlag = 0;
			}

		//Counter for periodic messages
        counterPeriodicPCM++;
        if(counterPeriodicPCM >= 300)
        {
            publishMsgFlag.charger_periodic_data_pcm = 1;
            counterPeriodicPCM = 0;
        }
		
		counterPeriodicCharger++;
        if(counterPeriodicCharger >= 300)
        {
            publishMsgFlag.charger_periodic_data = 1;
            counterPeriodicCharger = 0;
        }	
			
		//checking for which topic has Flag as set
       if(publishMsgFlag.charger_status == 1){
		   publishMsgFlag.charger_status=0;
		   
		   prepare_mqtt_Status_data(charger_Id, status_topic, status_payload);
		   int msg_id2 = esp_mqtt_client_publish(mqtt_client, status_topic, status_payload, 0, 1, 0);
		   ESP_LOGI(MQTT_TAG, "Published to topic %s with payload %s, msg_id=%d", status_topic, status_payload, msg_id2);
	   }
	   else if(publishMsgFlag.charging_session == 1){
		   publishMsgFlag.charging_session =0;
		   
		   prepare_mqtt_charging_session_data(charger_Id, charging_session_topic, charging_session_payload);
		   int msg_id3 = esp_mqtt_client_publish(mqtt_client, charging_session_topic, charging_session_payload, 0, 1, 0);
		   ESP_LOGI(MQTT_TAG, "Published to topic %s with payload %s, msg_id=%d", charging_session_topic, charging_session_payload, msg_id3);  
	   }
/*	else if(publishMsgFlag.send_config_data == 1){	
		  publishMsgFlag.send_config_data = 0;
			
		  prepare_mqtt_send_config_data(charger_Id, send_config_data_topic , send_config_data_payload);
          int msg_id5 = esp_mqtt_client_publish(mqtt_client, send_config_data_topic, send_config_data_payload, 0, 1, 0);
		  ESP_LOGI(TAG, "Published to topic %s with payload %s, msg_id=%d", send_config_data_topic, send_config_data_payload, msg_id5);
		}
		*/	
		else if(publishMsgFlag.charger_periodic_data ==1){
			//Write The Code
		}
		else if(publishMsgFlag.charger_periodic_data_pcm == 1){
			//Write The Code
		}
		else if(publishMsgFlag.send_component_status == 1){
			//Write The Code
		}
			vTaskDelay(xDelay); 
		}		
}

//----------------------------------------------------CAN Function------------------ -----------------------
void receive_can_message() {
	twai_message_t rx_msg;
    esp_err_t err = twai_receive(&rx_msg, pdMS_TO_TICKS(100));
    
     if (err == ESP_OK) {
        ESP_LOGI(CAN_MCU_TAG, "Received CAN message with ID: 0x%" PRIx32 ".", rx_msg.identifier);
        
     switch (rx_msg.identifier) 
     {
            case FAULT_INFO_ID:
               handle_fault_info(rx_msg.data);
               pmuAckFlag=1;
               break;

            case SESSION_INFO_ID:
               handle_session_info(rx_msg.data);
               pmuAckFlag=1;
               break;

            case PERIODIC_DATA_ID:
                handle_periodic_data(rx_msg.data);
                break;

            case CHARGER_CONFIG_ID:
                handle_charger_config(rx_msg.data);
                pmuAckFlag=1;
                break;

            case ACK_MCU_ID:
                handle_ack_mcu(rx_msg.data);
                pmuAckFlag=1;
                break;

            default:
            
            	if (rx_msg.identifier >= 0x21 && rx_msg.identifier <= 0x30){
					handle_pcm_message(rx_msg.identifier, rx_msg.data);
				}
            	else {
                   ESP_LOGW(CAN_MCU_TAG, "Received unrecognized CAN message with ID: 0x%" PRIx32 ".", rx_msg.identifier);
                   pmuAckFlag=0;
                }
             break;
            }   
        }
        else {
	         ESP_LOGE(CAN_MCU_TAG, "Error receiving CAN message: %d", err);
        }
}


void can_task(void *pvParameters){
	
	PMUInformation.sequence_number_1 = 1;
	PMUInformation.pmu_major_version = PMU_MAJOR_VERSION;
	PMUInformation.pmu_minor_version = PMU_MINOR_VERSION;
	PMUInformation.reserved = 0x3F;	// Set Bit 3-8 as 111111
	PMUInformation.unused[0] = 0xFF; //Set Unused Bytes
	PMUInformation.unused[1] = 0xFF;
	PMUInformation.unused[2] = 0xFF;
	PMUInformation.unused[3] = 0xFF;
	
	while (true){
		
		timerCounter++;
		
		if(timerCounter == 300) {
		  
		  	// Prepare CAN Periodic message data
        	uint8_t message_data[8] = {0};
        	message_data[0] = PMUInformation.sequence_number_1;       																							// Byte 1: Sequence Number
		    message_data[1] = (PMUInformation.internet_status & 0x01) | ((PMUInformation.broker_status & 0x01) << 1) | (PMUInformation.reserved << 2);          // Byte 2: Connection Status
            message_data[2] = PMUInformation.pmu_major_version;       																							// Byte 3
            message_data[3] = PMUInformation.pmu_minor_version;       																							// Byte 4
            memcpy(&message_data[4], PMUInformation.unused, 4);       																							// Bytes 5-8

		    ESP_LOGI(CAN_MCU_TAG, "Transmitting CAN message: Seq=%d, Internet=%d, Broker=%d", PMUInformation.sequence_number_1, PMUInformation.internet_status, PMUInformation.broker_status);
            send_can_message(PMU_INFO_ID, message_data, 8);       
        	timerCounter = 0;
          }
        	receive_can_message();
 		
    //Acknowledgement will be changed as per modification 
    
        	if(pmuAckFlag == 1)
        	{
			  pmuAckFlag = 0;
			  uint8_t can_message_data[8] = {0};
	          can_message_data[0] = AckPmu.can_message_ack; 				// Byte 1 : CAN Message Name Acknowledgement
			  if(can_message_data[0] == 3)
			    can_message_data[1] = AckPmu.sequence_number; 			    //  Byte 2 : Sequence Number
			  else
				can_message_data[1] = 0xFF;
				
			  if(can_message_data[0] == 3 && can_message_data[1] == 5)	
				can_message_data[2] = AckPmu.parameter_number;				//Byte 3 : Parameter Number 
			  else
				can_message_data[2] = 0xFF ;
			
			  if(can_message_data[0] == 3 && can_message_data[1] == 5 && can_message_data[2] == 3)   
				can_message_data[3] = AckPmu.limit_setting_value;			//Byte 4 : Limit Setting Value
			  else 	 	
				can_message_data[3] = 0xFF;
				
			  memcpy(&can_message_data[4], AckPmu.unused, 4);              // Bytes 5-8
			
			  ESP_LOGI(CAN_MCU_TAG, "Transmitting CAN Acknowledgement message with AckId = %d , Seq No = %d" ,AckPmu.can_message_ack , AckPmu.sequence_number);
              send_can_message(ACK_PMU_ID, can_message_data, 8);			
		    }         
	  vTaskDelay(pdMS_TO_TICKS(10));  // 10ms delay	
    }
}

//-------------------------------------- ------------Main Function -------------------------------
void app_main(void)
{
	ESP_LOGI(MQTT_TAG, "[APP] Startup..");
    ESP_LOGI(MQTT_TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(MQTT_TAG, "[APP] IDF version: %s", esp_get_idf_version());
    ESP_LOGI(MQTT_TAG, "PMU Code Started with the Wifi Intialization");
    
     connectorStatus[0] = 3;
    connectorStatus[1] = 1;
    connectorStatus[2] = 3;
    
    webSocketStatus = 2;
    
    faultStatus[0] = 0;
    faultStatus[1] = 6;
    faultStatus[2] = 10;
    
    connector = 2;
    event = 3;
    finishReason = 46;
    
    Brand = 3;
    AutoCharge = 1;
    DynamicPowerSharing = 1;
    EnergyMeterSetting = 3;
    
    twai_setup();
    
    //CAN Task for Periodic Transmission of PMU information from PMU --> MCU 
    xTaskCreatePinnedToCore(can_task, "CAN_Task", 4096, NULL, 4, NULL, 1);
   
    // Intialize the WiFi Settings
    wifi_init();
    
    // Wait for WiFi connection
    vTaskDelay(pdMS_TO_TICKS(5000));
    
    // Initialize MQTT client
    init_mqtt();
    
    //Create MQTT Task 
    xTaskCreatePinnedToCore(mqtt_task, "MQTT_Task",4096, NULL, 6, NULL, 0);
    
}

