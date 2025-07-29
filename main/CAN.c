/*
 * CAN.c
 *
 *  Created on: 03-Dec-2024
 *      Author: Dhruv & Niharika
 *  File Description : Header that includes the declaration of the CAN for project 
 */

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include "driver/gpio.h"
#include "driver/twai.h"
#include "esp_log.h"
#include "PMUFinalmain.h"
#include "CAN.h"

#define MAROON "\x1B[38;2;128;0;0m"
#define RESET_COLOR "\x1B[0m"

twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(TX_GPIO_NUM, RX_GPIO_NUM, TWAI_MODE_NORMAL);
twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS(); // 500 kbps
twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

char *EXAMPLE_TAG = "CAN";
char *CAN_MCU_TAG = "PMU_MCU-CAN";

char arr[100];
int flagVersionInfo = 0;
int flagOcppUrl = 0;
int flagPmuInitial = 0;
int flagPmuBroker = 0;
int flagString = 0;
int i=0;
int giveEntrytoPrint = 0; // This flag is used to give entry to if conditions in Cases 
int flagDateTime = 0;


//Flags used for indicating the IPC 
bool chargerConfigReceived = false;
bool chargerPeriodicReceived = false;
bool pcmPeriodicReceived = false;
bool eepromFaultedFlag = false ;
void parseWiFiCredentials(const char *receivedString);

void twai_setup() {
	g_config.tx_queue_len = 50;
	g_config.rx_queue_len = 50;
    ESP_ERROR_CHECK(twai_driver_install(&g_config, &t_config, &f_config));
    ESP_ERROR_CHECK(twai_start());
    //ESP_LOGI(EXAMPLE_TAG, "TWAI driver installed and started.");
} 

void send_can_message(uint32_t id, uint8_t *data, size_t len){
	twai_message_t message;
    message.identifier = id;  
    message.data_length_code = len;
    message.rtr = 0;
    
    // Copy data into the message structure
    for (size_t i = 0; i < len; i++) {
        message.data[i] = data[i];
    }
    
    if (twai_transmit(&message, pdMS_TO_TICKS(0)) == ESP_OK) {

		if(data[0] == 3){
			counterAck++;
		}
        //ESP_LOGI(EXAMPLE_TAG, "Message Transmitted: ID=0x%" PRIx32 ", DLC=%d", message.identifier, message.data_length_code);
    } else {
        //ESP_LOGW(EXAMPLE_TAG, "Failed to transmit message.");
    }
}

void handle_fault_info(uint8_t *data){
	
	uint8_t can_message_data[8] = {0};
	AckPmu.can_message_ack = 1;

	// Populate the structure
	faultInfoCAN.ConnectorNumber = (data[0] & 0x03);	 // Bits 0.0 - 0.1 (2 bits)
	faultInfoCAN.FaultStatus = ((data[0] >> 2 ) & 0x03);        // Bits 0.2 - 0.3 (2 bits)
    faultInfoCAN.ConnectorStatus =  ((data[0] >> 4) & 0x07);   // Bits 0.4 - 0.6 (3 bits)
	if(faultInfoCAN.ConnectorStatus == 2){
		faultInfoCAN.finishReason = data[1];		//This is the finish Reason here
	}else{
	    faultInfoCAN.FaultCode = data[1];                        // Byte 2
	}

	if(faultInfoCAN.FaultStatus == 2) { 					// Fault Status: Power Outage
        faultInfoCAN.Date = 0;
        faultInfoCAN.Month = 0;
        faultInfoCAN.Year = 0;
        faultInfoCAN.Hour = 0;
        faultInfoCAN.Minute = 0;
        faultInfoCAN.Second = 0;
   }else {
		faultInfoCAN.Date = data[2];                             // Byte 3
		faultInfoCAN.Month = data[3];							 // Byte 4 
		faultInfoCAN.Year = data[4];                             // Byte 5
		faultInfoCAN.Hour = data[5]; 							 // Byte 6
		faultInfoCAN.Minute = data[6]; 							 // Byte 7
		faultInfoCAN.Second = data[7];                           // Byte 8 
   	}
  
	//printf("Connector Number: %d \n", faultInfoCAN.ConnectorNumber);
    //printf("Fault Status: %d \n", faultInfoCAN.FaultStatus);
    //printf("Connector Status: %d \n", faultInfoCAN.ConnectorStatus);
    //printf("Fault Code: %d \n", faultInfoCAN.FaultCode);
    
    if (faultInfoCAN.FaultStatus == 2) {
        //printf("Power Outage detected: No date or time information available.\n");
    } else {
        //printf("Date: %02d-%02d-20%02d \n", faultInfoCAN.Date, faultInfoCAN.Month, faultInfoCAN.Year);
        //printf("Time: %02d:%02d:%02d \n ", faultInfoCAN.Hour, faultInfoCAN.Minute, faultInfoCAN.Second);
    }

	AckPmu.sequence_number = faultInfoCAN.ConnectorNumber;
	AckPmu.parameter_number = 0xFF;
	AckPmu.limit_setting_value = 0xFF;	

	can_message_data[0] = AckPmu.can_message_ack; 
	can_message_data[1] = AckPmu.sequence_number;
	can_message_data[2] = AckPmu.parameter_number;
	can_message_data[3] = AckPmu.limit_setting_value;
	memcpy(&can_message_data[4], AckPmu.unused, 4);
	//ESP_LOGI(CAN_MCU_TAG, "Transmitting CAN Acknowledgement message for Fault Information with AckId = %d , Seq No = %d" ,AckPmu.can_message_ack , AckPmu.sequence_number);
	send_can_message(ACK_PMU_ID, can_message_data, 8);
}

void handle_periodic_data(uint8_t *data){
	uint8_t sequence_number = data[0];
	countPerioidicCAN++;
    chargerPeriodicStatusCAN.trackNoofFrames = chargerPeriodicStatusCAN.trackNoofFrames | (1 << (sequence_number-1));	
    //ESP_LOGW("bUFFER","The value of sequence_number %d",sequence_number);
    //printf("Hex Value: 0x%X\n", chargerPeriodicStatusCAN.trackNoofFrames);
	switch(sequence_number){
		case 1:
          
            if(!(data[1] == 0xFF && data[2] == 0xFF))
			    chargerPeriodicStatusCAN.gridVoltages[0] = ((data[1] << 8) | data[2])/10.0;    //Phase A Voltage 
            if(!(data[3] == 0xFF && data[4] == 0xFF))
                chargerPeriodicStatusCAN.gridVoltages[1] = ((data[3] << 8) | data[4])/10.0;    //Phase B Voltage
            if(!(data[5] == 0xFF && data[6] == 0xFF))
                chargerPeriodicStatusCAN.gridVoltages[2] = ((data[5] << 8) | data[6])/10.0;	 //Phase C Voltage
			
            chargerPeriodicStatusCAN.acMeterComm = data[7];						 //AC Meter Modbus Communication 
			//printf("This is sequence Number 1 \n");
			//printf("Grid Voltages: [%.1f, %.1f, %.1f]\n", chargerPeriodicStatusCAN.gridVoltages[0], chargerPeriodicStatusCAN.gridVoltages[1],  chargerPeriodicStatusCAN.gridVoltages[2]);
			//printf("AC Meter ModBus: %d",chargerPeriodicStatusCAN.acMeterComm);
			break;
			
		case 2:
           
            if(!(data[1] == 0xFF && data[2] == 0xFF))
                chargerPeriodicStatusCAN.gridCurrents[0] = ((data[1] << 8) | data[2])/10.0;		//Phase A Current
            if(!(data[3] == 0xFF && data[4] == 0xFF))
                chargerPeriodicStatusCAN.gridCurrents[1] = ((data[3] << 8) | data[4])/10.0;		//Phase B Current
            if(!(data[5] == 0xFF && data[6] == 0xFF))
                chargerPeriodicStatusCAN.gridCurrents[2] = ((data[5] << 8) | data[6])/10.0;		//Phase C Current
			//Last Byte would contain 0xFF
			//printf("This is sequence Number 2 \n");
			// printf("Grid Current A: %.1f \n",chargerPeriodicStatusCAN.gridCurrents[0]);
			// printf("Grid Current B: %.1f \n",chargerPeriodicStatusCAN.gridCurrents[1]);
			// printf("Grid Current C: %.1f \n",chargerPeriodicStatusCAN.gridCurrents[2]);
			break;
			
		case 3:
           
            if(!(data[1] == 0xFF && data[2] == 0xFF))                
                chargerPeriodicStatusCAN.acMeterLineVoltages[0] = ((data[1] << 8) | data[2])/10.0;     //V12 line Voltage
            if(!(data[3] == 0xFF && data[4] == 0xFF))
                chargerPeriodicStatusCAN.acMeterLineVoltages[1] = ((data[3] << 8) | data[4])/10.0;	 //V23 line Voltage
            if(!(data[5] == 0xFF && data[6] == 0xFF))
			    chargerPeriodicStatusCAN.acMeterLineVoltages[2] = ((data[5] << 8) | data[6])/10.0;     //V31 line Voltage
			//Last Byte would contain 0xFF
			//printf("This is sequence Number 3 \n");
			// printf("acMeterLineVoltages: %.1f \n",chargerPeriodicStatusCAN.acMeterLineVoltages[0]);
			// printf("acMeterLineVoltages: %.1f \n",chargerPeriodicStatusCAN.acMeterLineVoltages[1]);
			// printf("acMeterLineVoltages: %.1f \n",chargerPeriodicStatusCAN.acMeterLineVoltages[2]);
			break;
		
		case 4:
            
            if(!(data[1] == 0xFF && data[2] == 0xFF && data[3] == 0xFF))                
                chargerPeriodicStatusCAN.acEnergy =  (((data[1] << 16) | (data[2] << 8) | data[3]))/1000.0;       //Active Energy in Wh
            
            if(!(data[4] == 0xFF))                
                chargerPeriodicStatusCAN.RealPowerFactor = data[4]/100.0;
            else
                chargerPeriodicStatusCAN.RealPowerFactor = 0.1f; 									   //Real PF
            
            if(!(data[5] == 0xFF))                
                chargerPeriodicStatusCAN.AvgPowerFactor  = data[5]/100.0;                                        //Avg PF
			//Byte 6 & 7 have 0xFF
			//ESP_LOGI(CAN_MCU_TAG, "Energy Meter 0x%" PRIx32 ":",chargerPeriodicStatusCAN.energyMeter[0]);
			//printf("This is sequence Number 4 \n");
			//printf("The Active Energy is : %.1f Wh",chargerPeriodicStatusCAN.energyMeter[0] );
			// printf("RealPowerFactor: %f Wh \n",chargerPeriodicStatusCAN.RealPowerFactor);
			// printf("AvgPowerFactor: %f Wh \n",chargerPeriodicStatusCAN.AvgPowerFactor);
		 	break;
		
		case 5:

            if(!(data[1] == 0xFF && data[2] == 0xFF && data[3] == 0xFF))                	
                chargerPeriodicStatusCAN.dcEnergy[0] = (((data[1] << 16) | (data[2] << 8) | data[3]))/1000.0; 		//Energy (in Wh)
            if(!(data[4] == 0xFF))                
                chargerPeriodicStatusCAN.dcMeterComm[0] = data[4];	
            if(!(data[5] == 0xFF))                
                chargerPeriodicStatusCAN.chargerStatus = data[5];
            if(!(data[6] == 0xFF))                
                chargerPeriodicStatusCAN.connectorsStatus[0] = data[6];
            if(!(data[7] == 0xFF))                
                chargerPeriodicStatusCAN.connectorsStatus[1] = data[7];

            // printf("The value of chargerStatus is %d \n",chargerPeriodicStatusCAN.chargerStatus);
            // printf("The value of connectorsStatus0 is %d \n",chargerPeriodicStatusCAN.connectorsStatus[0]);
            // printf("The value of connectorsStatus1 is %d \n",chargerPeriodicStatusCAN.connectorsStatus[1]);
            //ESP_LOGI("CHARGER_STATUS", "chargerStatus: %d", chargerPeriodicStatusCAN.chargerStatus);
            
			break;
			
		case 6:
            if(!(data[1] == 0xFF && data[2] == 0xFF))                
                chargerPeriodicStatusCAN.dcVoltage[0] = ((data[1] << 8) | data[2])/10.0;         				//Dc Meter 1 : Dc Voltage 
            if(!(data[3] == 0xFF && data[4] == 0xFF))                
                chargerPeriodicStatusCAN.dcCurrent[0] = ((data[3] << 8) | data[4])/10.0;						//Dc Meter 2 : Dc Current
            if(!(data[5] == 0xFF && data[6] == 0xFF && data[7] == 0xFF))                
                chargerPeriodicStatusCAN.dcMeterPower[0] = ((data[5] << 16) | (data[6] << 8) | data[7])/1000.0; 		//Power in W
                //printf("This is sequence Number 6 \n");
            // printf("DC Meter 1 Voltage: %.1f\n",chargerPeriodicStatusCAN.dcVoltage[0]);
            // printf("DC Meter 1 Current: %.1f\n",chargerPeriodicStatusCAN.dcCurrent[0]);
                //printf("The DC Meter 1 Power : %d" ,(int)chargerPeriodicStatusCAN.dcMeterPower[0]);  //dj
            break;
			
		case 7:
            if(!(data[1] == 0xFF && data[2] == 0xFF && data[3] == 0xFF))                			
                chargerPeriodicStatusCAN.dcEnergy[1] = ((data[1] << 16) | (data[2] << 8) | data[3])/1000.0;			//Energy (in Wh)
            if(!(data[4] == 0xFF))                
                chargerPeriodicStatusCAN.dcMeterComm[1] = data[4];
                //Rest All Bytes are 0xFF
                //printf("This is sequence Number 7 \n");
                //printf("Enery Meter in Wh : %.1f" ,chargerPeriodicStatusCAN.energyMeter[2] );
                //printf("Energy Meter Comm: %d",(int)chargerPeriodicStatusCAN.energyMeterComm[2]);
            break;
            
		case 8:
            if(!(data[1] == 0xFF && data[2] == 0xFF))                
                chargerPeriodicStatusCAN.dcVoltage[1] = ((data[1] << 8) | data[2])/10.0;							//Dc Meter 2 : Dc Voltage 
            if(!(data[3] == 0xFF && data[4] == 0xFF))                
                chargerPeriodicStatusCAN.dcCurrent[1] = ((data[3] << 8) | data[4])/10.0;			    			//Dc Meter 2 : Dc Current
            if(!(data[5] == 0xFF && data[6] == 0xFF && data[7] == 0xFF))                
                chargerPeriodicStatusCAN.dcMeterPower[1] = ((data[5] << 16) | (data[6] << 8) | data[7])/1000.0; 	//Power in W
                //printf("This is sequence Number 8 \n");
            // printf("DC Voltage A : %.1f \n" ,chargerPeriodicStatusCAN.dcVoltage[1]);
            // printf("DC Current A : %.1f \n" ,chargerPeriodicStatusCAN.dcCurrent[1]);
                //printf("DC Current A : %d" ,(int)chargerPeriodicStatusCAN.dcMeterPower[1]);  // DJ
            break;	
		
		case 9:
			chargerPeriodicStatusCAN.LCUCommStatus = data[1];												//LCU Communication Status
			chargerPeriodicStatusCAN.ev_evseCommRecv[0] = data[2];
			chargerPeriodicStatusCAN.ErrorCode0Recv[0] = data[3];
			chargerPeriodicStatusCAN.ev_evseCommRecv[1] = data[4]; 
			chargerPeriodicStatusCAN.ErrorCode0Recv[1] = data[5];
			chargerPeriodicStatusCAN.LCUInternetStatus = data[6];											//LCU Internet Status
			chargerPeriodicStatusCAN.websocketConnection = data[7];											//LCU Websocket Status
			//Save only if values have changed
            //write_nvs_value("internetStatus", chargerPeriodicStatusCAN.LCUInternetStatus);
            //write_nvs_value("websocketStatus", chargerPeriodicStatusCAN.websocketConnection);

            
            //printf("This is sequence Number 9 \n");
			//printf("LCU Comm Status : %d",chargerPeriodicStatusCAN.LCUCommStatus);
			//printf("The ev Evse Comm 1 Recv Value : %d",(int)chargerPeriodicStatusCAN.ev_evseCommRecv[0] );
			//printf("The ev Evse Comm 1 Error Value : %d",(int)chargerPeriodicStatusCAN.ev_evseCommControllers[0].ErrorCode0Recv );
			//printf("The ev Evse Comm 2 Recv Value : %d",(int)chargerPeriodicStatusCAN.ev_evseCommRecv[1] );
			//printf("The ev Evse Comm 2 Error Value : %d",(int)chargerPeriodicStatusCAN.ev_evseCommControllers[1].ErrorCode0Recv );
            //ESP_LOGI("DEBUG", "\x1b[35mConnectionStatusInternetCAN: %d\x1b[0m", chargerPeriodicStatusCAN.LCUInternetStatus);
            //ESP_LOGI("DEBUG", "\x1b[35mConnectionStatusWebsocketCAN: %d\x1b[0m", chargerPeriodicStatusCAN.websocketConnection);

		    
			break;

		case 10:
			chargerPeriodicStatusCAN.virtualEnergyMeter	= ((data[1] << 16) | (data[2] << 8) | data[3]);
            chargerPeriodicStatusCAN.chargerFaultCode = data[4];
            chargerPeriodicStatusCAN.connectorFaultCode[0] = data[5];
            chargerPeriodicStatusCAN.connectorFaultCode[1] = data[6];
            chargerPeriodicStatusCAN.receivedDataChargerPeriodic = 1;
			chargerPeriodicReceived = 1;	
			//ESP_LOGI("CHARGER_STATUS", "Charger Fault Code: %d", chargerPeriodicStatusCAN.chargerFaultCode);
            //ESP_LOGI("CHARGER_STATUS", "Connector 1 Fault Code: %d", chargerPeriodicStatusCAN.connectorFaultCode[0]);
            //ESP_LOGI("CHARGER_STATUS", "Connector 2 Fault Code: %d", chargerPeriodicStatusCAN.connectorFaultCode[1]);
			//Rest are 0xFF.
			break;
			
		default:
			////ESP_LOGI(CAN_MCU_TAG,"Invalid Sequence Number");
			break;				
	}
}

void handle_charger_config(uint8_t *data){

    uint8_t sequence_number = data[0];
    uint8_t can_message_data[8] = {0};
    AckPmu.can_message_ack = 3;           

    if(sequence_number>5){

            if(sequence_number == prevSequenceNo){
                AckPmu.sequence_number = sequence_number;
                can_message_data[0] = AckPmu.can_message_ack; 
                can_message_data[1] = AckPmu.sequence_number;
                can_message_data[2] = AckPmu.parameter_number;
                can_message_data[3] = AckPmu.limit_setting_value;
                memcpy(&can_message_data[4], AckPmu.unused, 4);
                //ESP_LOGI(CAN_MCU_TAG, "Transmitting CAN Acknowledgement message with AckId = %d , Seq No = %d" ,AckPmu.can_message_ack , AckPmu.sequence_number);
                send_can_message(ACK_PMU_ID, can_message_data, 8);
                return;
            }
            else{
                prevSequenceNo = sequence_number;
            }
                int startIndex = i;
                //printf("The value of Sequence Number is %d \n",sequence_number);
                //printf("The value of i is %d \n", startIndex);
                while( i<startIndex+7 && i<chargerConfigCAN.stringLength)
                {
                     arr[i] = data[i-startIndex+1];
                     ////printf("Character : %c at index %d \n", arr[i],i);
                     i++;
                     if(i==chargerConfigCAN.stringLength){
                         arr[i] = '\0';
                         giveEntrytoPrint =1;
                        // printf("Array is %s \n" , arr);
                     }
                 }
                ////printf("Array is %s" , arr);
                 
                if(flagVersionInfo == 1 && giveEntrytoPrint ==1){

                    flagVersionInfo = 0;
                    giveEntrytoPrint = 0;
                     char *token;
                     int tokenIndex = 0;  // To track which value we are assigning
                    for (size_t i = 0; i<100; i++) {
                        //printf("%c", arr[i]);  // Print the character
                    }   
                     // Tokenize the string using ',' as a delimiter
                     token = strtok(arr, ",");
                     while(token != NULL){
                           switch (tokenIndex){
                               
                               case 0:
                                    tokenIndex++;
                                    strncpy(chargerConfigCAN.versionInfo.hmiVersion, token, sizeof(chargerConfigCAN.versionInfo.hmiVersion) - 1);
                                    chargerConfigCAN.versionInfo.hmiVersion[sizeof(chargerConfigCAN.versionInfo.hmiVersion) - 1] = '\0';
                                    break;
                                    
                               case 1:
                                    tokenIndex++;
                                    strncpy(chargerConfigCAN.versionInfo.lcuVersion, token, sizeof(chargerConfigCAN.versionInfo.lcuVersion) - 1);           
                                    chargerConfigCAN.versionInfo.lcuVersion[sizeof(chargerConfigCAN.versionInfo.lcuVersion) - 1] = '\0';
                                    break;
                               
                               case 2:
                                    tokenIndex++;
                                    strncpy(chargerConfigCAN.versionInfo.mcuVersion, token, sizeof(chargerConfigCAN.versionInfo.mcuVersion) - 1);
                                    chargerConfigCAN.versionInfo.mcuVersion[sizeof(chargerConfigCAN.versionInfo.mcuVersion) - 1] = '\0';
                                    break;
                                    
                               case 3:
                                    tokenIndex++;
                                    strncpy(chargerConfigCAN.versionInfo.evseCommContoller[0], token, sizeof(chargerConfigCAN.versionInfo.evseCommContoller[0]) - 1);
                                    chargerConfigCAN.versionInfo.evseCommContoller[0][sizeof(chargerConfigCAN.versionInfo.evseCommContoller[0]) - 1] = '\0';
                                    //printf("EVSE Comm Controller 1: %s\n", chargerConfigCAN.versionInfo.evseCommContoller[0]);

                                    break;
                                    
                               case 4:
                                    tokenIndex++;
                                    strncpy(chargerConfigCAN.versionInfo.evseCommContoller[1], token, sizeof(chargerConfigCAN.versionInfo.evseCommContoller[1]) - 1);
                                    chargerConfigCAN.versionInfo.evseCommContoller[1][sizeof(chargerConfigCAN.versionInfo.evseCommContoller[1]) - 1] = '\0';
                                    //printf("EVSE Comm Controller 2: %s\n", chargerConfigCAN.versionInfo.evseCommContoller[1]);
                                    break;      
                               
/*                             case 5:
                                    strncpy(chargerConfigCAN.versionInfo.PCM[0], token, sizeof(chargerConfigCAN.versionInfo.PCM[0]) - 1);
                                    chargerConfigCAN.versionInfo.PCM[0][sizeof(chargerConfigCAN.versionInfo.PCM[0]) - 1] = '\0';
                                    break;
 */                            
                               default:
                                    //printf("Unexpected data received\n");
                                    break;
                               
                            }
                         //tokenIndex++;
                         token = strtok(NULL, ",");                      
                    }//end of while 
                        
                         //printf("HMI Version: %s\n", chargerConfigCAN.versionInfo.hmiVersion);
                         //printf("LCU Version: %s\n", chargerConfigCAN.versionInfo.lcuVersion);
                         //printf("MCU Version: %s\n", chargerConfigCAN.versionInfo.mcuVersion);
                         //printf("EVSE Comm Controller 1: %s\n", chargerConfigCAN.versionInfo.evseCommContoller[0]);
                         //printf("EVSE Comm Controller 2: %s\n", chargerConfigCAN.versionInfo.evseCommContoller[1]);
                         //printf("PCM Module 1: %s\n", chargerConfigCAN.versionInfo.PCM[0]);
                 }//end of flagVersion       
                
                if(flagOcppUrl == 1 && giveEntrytoPrint ==1 ){
                    flagOcppUrl = 0;
                    giveEntrytoPrint = 0;
                    strncpy(chargerConfigCAN.ocppURL, arr, sizeof(chargerConfigCAN.ocppURL) - 1);
                    chargerConfigCAN.ocppURL[sizeof(chargerConfigCAN.ocppURL) - 1] = '\0';  // Ensure null-termination      
                    //printf("Ocpp URL: %s\n", chargerConfigCAN.ocppURL);           
                } 
                
                if(flagPmuInitial == 1 && giveEntrytoPrint ==1){
                    flagPmuInitial = 0;
                    giveEntrytoPrint = 0;
                    parseWiFiCredentials(arr);
                    //printf("SSID: %s\n", chargerConfigCAN.wifi_ssid);
                    //printf("Password: %s\n", chargerConfigCAN.wifi_password);
                }

                /*
                if(flagPmuBroker == 1 && giveEntrytoPrint ==1){
                    flagPmuBroker =0 ;
                    giveEntrytoPrint = 0;
                    strncpy(chargerConfigCAN.mqtt_broker, arr, sizeof(chargerConfigCAN.mqtt_broker) - 1);
                    chargerConfigCAN.mqtt_broker[sizeof(chargerConfigCAN.mqtt_broker) - 1] = '\0';  // Ensure null-termination  
                    //printf("Broker : %s\n", chargerConfigCAN.mqtt_broker);
                }*/
                
                if(flagString == 1 && giveEntrytoPrint ==1){
                    flagString = 0;
                    giveEntrytoPrint = 0;
                    strncpy(chargerConfigCAN.serialNumber2, arr, sizeof(chargerConfigCAN.serialNumber2) - 1);
                    chargerConfigCAN.serialNumber2[sizeof(chargerConfigCAN.serialNumber2)- 1] = '\0';
                    //printf("Serial No 2 : %s\n", chargerConfigCAN.serialNumber2);
                    //printChargerConfigData = 1;
                }
                
                if(flagDateTime == 1 && giveEntrytoPrint == 1){
                    flagDateTime = 0;
                    giveEntrytoPrint = 0;
                    ESP_LOGW("CONFIG", "CONFIG DATE TIME RECEIVED");
                    strncpy(chargerConfigCAN.dateTime, arr, sizeof(chargerConfigCAN.dateTime)-1);
                    chargerConfigCAN.dateTime[sizeof(chargerConfigCAN.dateTime)-1] = '\0';
                    //ESP_LOGI("CONFIG", LOG_COLOR_PINK " dateTime:%s" LOG_RESET_COLOR, chargerConfigCAN.dateTime);
                    printChargerConfigData = 1;
                }
                
                AckPmu.sequence_number = sequence_number;
                can_message_data[0] = AckPmu.can_message_ack; 
                can_message_data[1] = AckPmu.sequence_number;
                can_message_data[2] = AckPmu.parameter_number;
                can_message_data[3] = AckPmu.limit_setting_value;
                memcpy(&can_message_data[4], AckPmu.unused, 4);
                //ESP_LOGI(CAN_MCU_TAG, "Transmitting CAN Acknowledgement message with AckId = %d , Seq No = %d" ,AckPmu.can_message_ack , AckPmu.sequence_number);
                send_can_message(ACK_PMU_ID, can_message_data, 8);
    }
    else{
    switch (sequence_number) {
            
            case 1:
            
                chargerConfigCAN.numConnectors = data[1] & 0x0F;
                chargerConfigCAN.numPCM = (data[1] & 0xF0) >> 4;    //Maybe required check once     
                chargerConfigCAN.powerInputType = (data[2] >> 0) & 0x01;
                chargerConfigCAN.DynamicPowerSharing = (data[2] >> 1) & 0x01;
                chargerConfigCAN.EnergyMeterSetting = (data[2] >> 2) & 0x01;
                chargerConfigCAN.ZeroMeterStartEnable = (data[2] >> 3) & 0x01;
                chargerConfigCAN.projectInfo = (data[2] >> 4) & 0x0F;
                chargerConfigCAN.Brand = data[3];
                chargerConfigCAN.serialNumber[0] = data[4];
                chargerConfigCAN.serialNumber[1] = data[5];
                chargerConfigCAN.serialNumber[2] = data[6];
                chargerConfigCAN.serialNumber[3] = data[7];

                //printf("The Squence Number is 1 \n");
                // ESP_LOGI("CONFIG-CAN", MAROON "Num Connector : %d" RESET_COLOR, chargerConfigCAN.numConnectors);
                // ESP_LOGI("CONFIG-CAN", MAROON "Num PCMs : %d" RESET_COLOR, chargerConfigCAN.numPCM);
                // ESP_LOGI("CONFIG-CAN", MAROON "Power Input Type : %d" RESET_COLOR, chargerConfigCAN.powerInputType);
                // ESP_LOGI("CONFIG-CAN", MAROON "Dynamic Power Sharing : %d" RESET_COLOR, chargerConfigCAN.DynamicPowerSharing);
                //ESP_LOGI("CONFIG-CAN", MAROON "Energy Meter Setting : %d" RESET_COLOR, chargerConfigCAN.EnergyMeterSetting);
                // ESP_LOGI("CONFIG-CAN", MAROON "Project Info : %d" RESET_COLOR, chargerConfigCAN.projectInfo);
                // ESP_LOGI("CONFIG-CAN", MAROON "Brand : %d" RESET_COLOR, chargerConfigCAN.Brand);

                //Acknowedlegment to be sent 
                AckPmu.sequence_number = 1;
                AckPmu.parameter_number = 0xFF;
                AckPmu.limit_setting_value = 0xFF;

                can_message_data[0] = AckPmu.can_message_ack; 
                can_message_data[1] = AckPmu.sequence_number;
                can_message_data[2] = AckPmu.parameter_number;
                can_message_data[3] = AckPmu.limit_setting_value;
                memcpy(&can_message_data[4], AckPmu.unused, 4);
                //ESP_LOGI(CAN_MCU_TAG, "Transmitting CAN Acknowledgement message with AckId = %d , Seq No = %d" ,AckPmu.can_message_ack , AckPmu.sequence_number);
                send_can_message(ACK_PMU_ID, can_message_data, 8);
                break;
                
            case 2:
            
                chargerConfigCAN.serialNumber[4] = data[1];
                chargerConfigCAN.serialNumber[5] = data[2];
                chargerConfigCAN.serialNumber[6] = data[3];
                chargerConfigCAN.serialNumber[7] = data[4];
                chargerConfigCAN.serialNumber[8] = data[5];
                chargerConfigCAN.serialNumber[9] = data[6];
                chargerConfigCAN.serialNumber[10] = data[7];

                //printf("The Squence Number is 2 \n");
                // //printf("Serial Num : %s \n",chargerConfigCAN.serialNumber[4]);
                // //printf("Serial Num : %s \n",chargerConfigCAN.serialNumber[5]);
                // //printf("Serial Num : %s \n",chargerConfigCAN.serialNumber[6]);
                // //printf("Serial Num : %s \n",chargerConfigCAN.serialNumber[7]);
                // //printf("Serial Num : %s \n",chargerConfigCAN.serialNumber[8]);
                // //printf("Serial Num : %s \n",chargerConfigCAN.serialNumber[9]);
                // //printf("Serial Num : %s \n",chargerConfigCAN.serialNumber[10]);

                // Acknowedlegment to be sent 
                AckPmu.sequence_number = 2;
                AckPmu.parameter_number = 0xFF;
                AckPmu.limit_setting_value = 0xFF;
                
                can_message_data[0] = AckPmu.can_message_ack; 
                can_message_data[1] = AckPmu.sequence_number;
                can_message_data[2] = AckPmu.parameter_number;
                can_message_data[3] = AckPmu.limit_setting_value;
                memcpy(&can_message_data[4], AckPmu.unused, 4);
                //ESP_LOGI(CAN_MCU_TAG, "Transmitting CAN Acknowledgement message with AckId = %d , Seq No = %d" ,AckPmu.can_message_ack , AckPmu.sequence_number);
                send_can_message(ACK_PMU_ID, can_message_data, 8);
                break;
                
            case 3: 
            
                chargerConfigCAN.serialNumber[11] = data[1];
                chargerConfigCAN.serialNumber[12] = data[2];
                chargerConfigCAN.serialNumber[13] = data[3];
                chargerConfigCAN.serialNumber[14] = data[4];
                chargerConfigCAN.serialNumber[15] = data[5];
                chargerConfigCAN.serialNumber[16] = data[6];
                chargerConfigCAN.serialNumber[17] = data[7];

                 //printf("The Squence Number is 3 \n");
                // //printf("Serial Num : %s \n",chargerConfigCAN.serialNumber[11]);
                // //printf("Serial Num : %s \n",chargerConfigCAN.serialNumber[12]);
                // //printf("Serial Num : %s \n",chargerConfigCAN.serialNumber[13]);
                // //printf("Serial Num : %s \n",chargerConfigCAN.serialNumber[14]);
                // //printf("Serial Num : %s \n",chargerConfigCAN.serialNumber[15]);
                // //printf("Serial Num : %s \n",chargerConfigCAN.serialNumber[16]);
                // //printf("Serial Num : %s \n",chargerConfigCAN.serialNumber[17]);

                //Acknowedlegment to be sent 
                AckPmu.sequence_number = 3;
                AckPmu.parameter_number = 0xFF;
                AckPmu.limit_setting_value = 0xFF;
                
                can_message_data[0] = AckPmu.can_message_ack; 
                can_message_data[1] = AckPmu.sequence_number;
                can_message_data[2] = AckPmu.parameter_number;
                can_message_data[3] = AckPmu.limit_setting_value;
                memcpy(&can_message_data[4], AckPmu.unused, 4);
                //ESP_LOGI(CAN_MCU_TAG, "Transmitting CAN Acknowledgement message with AckId = %d , Seq No = %d" ,AckPmu.can_message_ack , AckPmu.sequence_number);
                send_can_message(ACK_PMU_ID, can_message_data, 8);
                break;
            
            case 4: 
                
                chargerConfigCAN.serialNumber[18] = data[1];
                chargerConfigCAN.serialNumber[19] = data[2];
                chargerConfigCAN.serialNumber[20] = data[3];
                chargerConfigCAN.serialNumber[21] = data[4];
                chargerConfigCAN.serialNumber[22] = data[5];
                chargerConfigCAN.serialNumber[23] = data[6];
                chargerConfigCAN.serialNumber[24] = data[7];

                // //printf("Serial Num : %s \n",chargerConfigCAN.serialNumber[18]);
                // //printf("Serial Num : %s \n",chargerConfigCAN.serialNumber[19]);
                // //printf("Serial Num : %s \n",chargerConfigCAN.serialNumber[20]);
                // //printf("Serial Num : %s \n",chargerConfigCAN.serialNumber[21]);
                // //printf("Serial Num : %s \n",chargerConfigCAN.serialNumber[22]);
                // //printf("Serial Num : %s \n",chargerConfigCAN.serialNumber[23]);
                // //printf("Serial Num : %s \n",chargerConfigCAN.serialNumber[24]);

                 //printf("The Squence Number is 4 \n");
                //ESP_LOGI("CONFIG-CAN", MAROON "Serial Number at the Startup : %s" RESET_COLOR, chargerConfigCAN.serialNumber);
                //Acknowedlegment to be sent
                AckPmu.sequence_number = 4;
                AckPmu.parameter_number = 0xFF;
                AckPmu.limit_setting_value = 0xFF;
                
                can_message_data[0] = AckPmu.can_message_ack; 
                can_message_data[1] = AckPmu.sequence_number;
                can_message_data[2] = AckPmu.parameter_number;
                can_message_data[3] = AckPmu.limit_setting_value;
                memcpy(&can_message_data[4], AckPmu.unused, 4);
                //ESP_LOGI(CAN_MCU_TAG, "Transmitting CAN Acknowledgement message with AckId = %d , Seq No = %d" ,AckPmu.can_message_ack , AckPmu.sequence_number);
                send_can_message(ACK_PMU_ID, can_message_data, 8);
                break;  
                
            case 5:
                
                /* Parameters Numbers are :     
                    
                    0-Nothing to be changed
                    1-Auto Charge Configuration value
                    2-Charger Mode Configuration Value
                    3 -Limit Setting Variable (check parameter name in Byte 3)
                    4-versionInfo(string)
                    5-ocppUrl (string)
                    6-pmu "ssid,password" (string)
                    7-pmu broker url (string)
                    8-SerialNumber(string)
                    9-Tarrif Plan
                */

                chargerConfigCAN.parameterName = data[1];
                //printf("The Squence Number is 5 \n");
                //printf("The parameter Number is : %d \n",chargerConfigCAN.parameterName);
                switch(chargerConfigCAN.parameterName){
                        
                        case 0:  
                           //printf("Nothing to be Changed \n");
                           AckPmu.parameter_number = 0;
                           break;
                        
                        case 1:
                            //AutoCharge Configuration Value
                            chargerConfigCAN.AutoCharge = data[2];     
                            AckPmu.parameter_number = 1;
                            //ESP_LOGI("CONFIG-CAN", MAROON "AutoCharge at the Startup : %d" RESET_COLOR,  chargerConfigCAN.AutoCharge);;  
                            break;
                        
                        case 2:
                            //ChargerMode Configuration Value
                            chargerConfigCAN.configKeyValue = data[2];
                            AckPmu.parameter_number = 2;  
                            //ESP_LOGI("CONFIG-CAN", MAROON "configKeyValue at the Startup : %d" RESET_COLOR,  chargerConfigCAN.configKeyValue);;  

                            break;
                        
                        case 3: 
                            //Limit Setting Variable
                            chargerConfigCAN.limitSettingParameterName = data[2];
                            AckPmu.parameter_number = 3;

                        switch(chargerConfigCAN.limitSettingParameterName){

                            
                                case 1:
                                    //ChargerPower
                                    if(!(data[3] == 0xFF && data[4] == 0xFF && data[5] == 0xFF)){
                                    chargerConfigCAN.limitSetting.chargerPower =  ((uint32_t)data[3] << 16) |  // Upper byte (Byte 4)
                                                                                    ((uint32_t)data[4] << 8)  |  // Middle byte (Byte 5)
                                                                                    (uint32_t)data[5]; 
                                    }          // Lower byte (Byte 6)
                                    AckPmu.limit_setting_value = 1;
                                    //ESP_LOGI("CONFIG-CAN", MAROON "chargerPower at the Startup : %f" RESET_COLOR,  chargerConfigCAN.limitSetting.chargerPower);;  
                                    break;
                                    
                                case 2:
                                    //gunAVoltage
                                    if(!(data[3] == 0xFF && data[4] == 0xFF && data[5] == 0xFF)){
                                    chargerConfigCAN.limitSetting.gunAVoltage =   ((uint32_t)data[3] << 16) |  // Upper byte (Byte 4)
                                                                                    ((uint32_t)data[4] << 8)  |  // Middle byte (Byte 5)
                                                                                    (uint32_t)data[5];           // Lower byte (Byte 6)
                                    chargerConfigCAN.limitSetting.gunAVoltage = (chargerConfigCAN.limitSetting.gunAVoltage)/10.0;
                                    }                           
                                    AckPmu.limit_setting_value = 2; 
                                    //printf("The value of gunAVoltage is : %.2f \n",chargerConfigCAN.limitSetting.gunAVoltage);                                                    
                                    break;
                                    
                                case 3:
                                    //gunACurrent
                                    if(!(data[3] == 0xFF && data[4] == 0xFF && data[5] == 0xFF)){
                                    chargerConfigCAN.limitSetting.gunACurrent = ((uint32_t)data[3] << 16) |  // Upper byte (Byte 4)
                                                                                ((uint32_t)data[4] << 8)  |  // Middle byte (Byte 5)
                                                                                (uint32_t)data[5];           // Lower byte (Byte 6)
                                    chargerConfigCAN.limitSetting.gunACurrent = (chargerConfigCAN.limitSetting.gunACurrent)/100.0;   
                                    }                                       
                                    AckPmu.limit_setting_value = 3;
                                    //printf("The value of gunACurrent is : %.2f \n ",chargerConfigCAN.limitSetting.gunACurrent);
                                    break;                                  
                                
                                case 4:
                                    //gunAPower
                                    if(!(data[3] == 0xFF && data[4] == 0xFF && data[5] == 0xFF)){
                                    chargerConfigCAN.limitSetting.gunAPower =  ((uint32_t)data[3] << 16) |  // Upper byte (Byte 4)
                                                                                ((uint32_t)data[4] << 8)  |  // Middle byte (Byte 5)
                                                                                (uint32_t)data[5];
                                    }           // Lower byte (Byte 6)
                                    AckPmu.limit_setting_value = 4;     
                                    //printf("The value of gunBPower is : %.2f \n",chargerConfigCAN.limitSetting.gunAPower);                                    
                                    break;
                                
                                case 5:
                                    //gunBVoltage 
                                    if(!(data[3] == 0xFF && data[4] == 0xFF && data[5] == 0xFF)){
                                    chargerConfigCAN.limitSetting.gunBVoltage = ((uint32_t)data[3] << 16) |  // Upper byte (Byte 4)
                                                                                ((uint32_t)data[4] << 8)  |  // Middle byte (Byte 5)
                                                                                (uint32_t)data[5];           // Lower byte (Byte 6)
                                    chargerConfigCAN.limitSetting.gunBVoltage = (chargerConfigCAN.limitSetting.gunBVoltage)/10.0; 
                                    }                                          
                                    AckPmu.limit_setting_value = 5;     
                                    //printf("The value of gunBVoltage is : %.2f \n",chargerConfigCAN.limitSetting.gunBVoltage);                                        
                                    break;      
                                
                                case 6: 
                                    //gunBCurrent
                                    if(!(data[3] == 0xFF && data[4] == 0xFF && data[5] == 0xFF)){
                                    chargerConfigCAN.limitSetting.gunBCurrent = ((uint32_t)data[3] << 16) |  // Upper byte (Byte 4)
                                                                                ((uint32_t)data[4] << 8)  |  // Middle byte (Byte 5)
                                                                                (uint32_t)data[5];           // Lower byte (Byte 6)
                                    chargerConfigCAN.limitSetting.gunBCurrent = (chargerConfigCAN.limitSetting.gunBCurrent)/100.0;
                                    }                                          
                                    AckPmu.limit_setting_value = 6;     
                                    //printf("The value of gunBCurrent is : %.2f \n",chargerConfigCAN.limitSetting.gunBCurrent);                                    
                                    break;      
                                
                                case 7:
                                    //gunBPower
                                    if(!(data[3] == 0xFF && data[4] == 0xFF && data[5] == 0xFF)){
                                    chargerConfigCAN.limitSetting.gunBPower = ((uint32_t)data[3] << 16) |  // Upper byte (Byte 4)
                                                                                ((uint32_t)data[4] << 8)  |  // Middle byte (Byte 5)
                                                                                (uint32_t)data[5];           // Lower byte (Byte 6)
                                    }
                                    AckPmu.limit_setting_value = 7;                                         
                                    //printf("The value of gunBPower is : %.2f \n",chargerConfigCAN.limitSetting.gunBPower );
                                    break;
                                
                                case 8:
                                    //serverChargerPower
                                    chargerConfigCAN.limitSetting.serverChargerPower = ((uint32_t)data[3] << 16) |  // Upper byte (Byte 4)
                                                                                            ((uint32_t)data[4] << 8)  |  // Middle byte (Byte 5)
                                                                                            (uint32_t)data[5];           // Lower byte (Byte 6)
                                    //ESP_LOGI("CAN_DATA", MAROON "serverChargerPower: %lf" RESET_COLOR, chargerConfigCAN.limitSetting.serverChargerPower);
                                    AckPmu.limit_setting_value = 8;
                                    break;

                                case 9:
                                    //serverConnectorAPower
                                    chargerConfigCAN.limitSetting.serverConnectorAPower = ((uint32_t)data[3] << 16) |  // Upper byte (Byte 4)
                                                                                            ((uint32_t)data[4] << 8)  |  // Middle byte (Byte 5)
                                                                                            (uint32_t)data[5];           // Lower byte (Byte 6)
                                    //ESP_LOGI("CAN_DATA", MAROON "serverConnectorAPower: %lf" RESET_COLOR, chargerConfigCAN.limitSetting.serverConnectorAPower);                                                        
                                    AckPmu.limit_setting_value = 9;
                                    break;

                                case 10:
                                    //serverConnectorACurrent
                                    chargerConfigCAN.limitSetting.serverConnectorACurrent = (((uint32_t)data[3] << 16) |  // Upper byte (Byte 4)
                                                                                              ((uint32_t)data[4] << 8)  |  // Middle byte (Byte 5)
                                                                                              (uint32_t)data[5])/100.0f ; 
                                    //ESP_LOGI("CAN_DATA", MAROON "serverConnectorACurrent: %lf" RESET_COLOR, chargerConfigCAN.limitSetting.serverConnectorACurrent);                                  
                                    AckPmu.limit_setting_value = 10;
                                    break;
                                    
                                case 11:
                                    //serverConnectorBPower
                                    chargerConfigCAN.limitSetting.serverConnectorBPower = ((uint32_t)data[3] << 16) |  // Upper byte (Byte 4)
                                                                                              ((uint32_t)data[4] << 8)  |  // Middle byte (Byte 5)
                                                                                              (uint32_t)data[5];
                                    //ESP_LOGI("CAN_DATA", MAROON "serverConnectorBPower: %lf" RESET_COLOR, chargerConfigCAN.limitSetting.serverConnectorBPower);                                                          
                                    AckPmu.limit_setting_value = 11;
                                    break;

                                case 12:
                                    //serverConnectorBCurrent
                                    chargerConfigCAN.limitSetting.serverConnectorBCurrent = (((uint32_t)data[3] << 16) |  // Upper byte (Byte 4)
                                                                                              ((uint32_t)data[4] << 8)  |  // Middle byte (Byte 5)
                                                                                              (uint32_t)data[5])/100.0f;
                                    //ESP_LOGI("CAN_DATA", MAROON "serverConnectorBCurrent: %lf" RESET_COLOR, chargerConfigCAN.limitSetting.serverConnectorBCurrent);
                                    AckPmu.limit_setting_value = 12;
                                    break;
                                
                                default:
                                    //Yet to be done
                                    break;      
                                }   
                            break;
                                  
                        case 4:
                            //Version Information
                            flagVersionInfo = 1;    
                            chargerConfigCAN.stringLength = ((uint16_t)data[2] << 8) | (uint16_t)data[3];
                            i=0;
                            while(i<4 && i<chargerConfigCAN.stringLength){
                                     arr[i] = data[i+4];
                                     i++;
                                 }
                            AckPmu.parameter_number = 4;
                            AckPmu.limit_setting_value = 0xFF;
                                     
                            break;
                            
                        case 5: 
                            //ocppUrl String
                            flagOcppUrl = 1;    
                            chargerConfigCAN.stringLength = ((uint16_t)data[2] << 8) | (uint16_t)data[3]; 
                            i=0;
                            while(i<4 && i<chargerConfigCAN.stringLength){
                                    arr[i] = data[i+4];
                                    i++;
                                }
                            AckPmu.parameter_number = 5;
                            AckPmu.limit_setting_value = 0xFF;  
                            break;
                            
                        case 6:
                            //PMU initials in the format "ssid,password"    
                            flagPmuInitial = 1; 
                            chargerConfigCAN.stringLength = ((uint16_t)data[2] << 8) | (uint16_t)data[3]; 
                            i=0;
                            while(i<4 && i<chargerConfigCAN.stringLength){
                                     arr[i] = data[i+4];
                                     i++;
                                 }
                            AckPmu.parameter_number = 6;
                            AckPmu.limit_setting_value = 0xFF;  
                            break;
                            
                        case 7:
                            //PMU Broker
                            //flagPmuBroker = 1;    
                            chargerConfigCAN.stringLength = ((uint16_t)data[2] << 8) | (uint16_t)data[3]; 
                            i=0;
                            while(i<4 && i<chargerConfigCAN.stringLength){
                                     arr[i] = data[i+4];
                                     i++;
                                     if(i == chargerConfigCAN.stringLength){
                                        arr[i] = '\0';
                                     }
                                 }
                            strncpy(chargerConfigCAN.mqtt_broker, arr, sizeof(chargerConfigCAN.mqtt_broker) - 1);
                            chargerConfigCAN.mqtt_broker[sizeof(chargerConfigCAN.mqtt_broker) - 1] = '\0';  // Ensure null-termination  
                            AckPmu.parameter_number = 7;
                            AckPmu.limit_setting_value = 0xFF;
                            break;  
                        
                        case 8:
                            //Serial Number     
                            flagString = 1; 
                            chargerConfigCAN.stringLength = ((uint16_t)data[2] << 8) | (uint16_t)data[3]; 
                            i=0;
                            while(i<4 && i<chargerConfigCAN.stringLength){
                                     arr[i] = data[i+4];
                                     i++;
                                 }
                            AckPmu.parameter_number = 8;
                            AckPmu.limit_setting_value = 0xFF;
                            break;

                        case 9:
                            //Tarrif Plan
                            chargerConfigCAN.tarrifAmount = (((uint32_t)data[2] << 16) |   //Upper byte (Byte 3)
                                                            ((uint32_t)data[3] << 8)  |    //Middle byte (Byte 4)
                                                            (uint32_t)data[4])/1000.0 ;      //Lower byte (Byte 5)
                            AckPmu.parameter_number = 9;
                            AckPmu.limit_setting_value = 0xFF;
                            //printf("Tarrif Plan for 9 is %.3f \n", chargerConfigCAN.tarrifAmount);
                            break;
                        
                        case 10:
                            //Datetime
                            flagDateTime = 1;   
                            chargerConfigCAN.stringLength = ((uint16_t)data[2] << 8) | (uint16_t)data[3];
                            //ESP_LOGI("CONFIG", LOG_COLOR_PINK "stringLength dateTime:%d" LOG_RESET_COLOR, chargerConfigCAN.stringLength);
                            i=0;
                            while(i<4 && i<chargerConfigCAN.stringLength){
                                arr[i] = data[i+4];
                                i++;
                            }
                            AckPmu.parameter_number = 10;
                            AckPmu.limit_setting_value = 0xFF;  
                            break;

                            case 11: 
                            //eepromCommStatus
                            chargerConfigCAN.eepromCommStatus = data[2];    
                            eepromFaultedFlag = true;
                            AckPmu.parameter_number = 11;
                            AckPmu.limit_setting_value = 0xFF;
                            //ESP_LOGI("EEPROM","eepromCommStatus is %d",chargerConfigCAN.eepromCommStatus);
                            break;
                        
                        case 12:
                            //rtcCommStatus
                            chargerConfigCAN.rtcCommStatus = data[2];
                            AckPmu.parameter_number = 12;
                            AckPmu.limit_setting_value = 0xFF;
                            //ESP_LOGW("EEPROM","rtcCommStatus is %d",chargerConfigCAN.rtcCommStatus);
                            //chargerConfigReceived = 1;
                            //chargerConfigCAN.receivedConfigDataMQTT = 1;
                            break;

                        case 13:
                            //pmuStatus
                            chargerConfigCAN.pmuEnable = data[2];
                            AckPmu.parameter_number = 13;
                            AckPmu.limit_setting_value = 0xFF;
                            //ESP_LOGW("EEPROM","pmuEnabled key is %d",chargerConfigCAN.pmuEnable);
                            chargerConfigReceived = 1;
                            chargerConfigCAN.receivedConfigDataMQTT = 1;
                            break;

                        default:
                            //Rare Occurence
                            break;
                        } 
                    

                    AckPmu.sequence_number = 5;

                    can_message_data[0] = AckPmu.can_message_ack; 
                    can_message_data[1] = AckPmu.sequence_number;
                    can_message_data[2] = AckPmu.parameter_number;
                    can_message_data[3] = AckPmu.limit_setting_value;
                    memcpy(&can_message_data[4], AckPmu.unused, 4);
                    //ESP_LOGI(CAN_MCU_TAG, "Transmitting CAN Acknowledgement message with AckId = %d , Seq No = %d \n" ,AckPmu.can_message_ack , AckPmu.sequence_number);
                    send_can_message(ACK_PMU_ID, can_message_data, 8);
                    break; // Case 5 ka Break 
                    
            case 6:
            case 7:
            case 8:
            case 9:
            case 10:
            case 11:
            case 12:
            case 13:
            {
                int startIndex = i;
                while( i<startIndex+7 && i<chargerConfigCAN.stringLength){
                     arr[i] = data[i-startIndex+1];
                     //printf("Character : %c at index %d \n", arr[i],i);
                     i++;
                     if(i==chargerConfigCAN.stringLength){
                         arr[i] = '\0';
                         giveEntrytoPrint =1;
                         //printf("Array is %s \n" , arr);
                     }
                 }
                ////printf("Array is %s" , arr);
                 
                if(flagVersionInfo == 1 && giveEntrytoPrint ==1){
                     char *token;
                     int tokenIndex = 0;  // To track which value we are assigning
                     
                     // Tokenize the string using ',' as a delimiter
                     token = strtok(arr, ",");
                     while(token != NULL){
                           switch (tokenIndex){
                               
                               case 0:
                                    strncpy(chargerConfigCAN.versionInfo.hmiVersion, token, sizeof(chargerConfigCAN.versionInfo.hmiVersion) - 1);
                                    chargerConfigCAN.versionInfo.hmiVersion[sizeof(chargerConfigCAN.versionInfo.hmiVersion) - 1] = '\0';
                                    break;
                                    
                               case 1:
                                    strncpy(chargerConfigCAN.versionInfo.lcuVersion, token, sizeof(chargerConfigCAN.versionInfo.lcuVersion) - 1);           
                                    chargerConfigCAN.versionInfo.lcuVersion[sizeof(chargerConfigCAN.versionInfo.lcuVersion) - 1] = '\0';
                                    break;
                               
                               case 2:
                                    strncpy(chargerConfigCAN.versionInfo.mcuVersion, token, sizeof(chargerConfigCAN.versionInfo.mcuVersion) - 1);
                                    chargerConfigCAN.versionInfo.mcuVersion[sizeof(chargerConfigCAN.versionInfo.mcuVersion) - 1] = '\0';
                                    break;
                                    
                               case 3:
                                    strncpy(chargerConfigCAN.versionInfo.evseCommContoller[0], token, sizeof(chargerConfigCAN.versionInfo.evseCommContoller[0]) - 1);
                                    chargerConfigCAN.versionInfo.evseCommContoller[0][sizeof(chargerConfigCAN.versionInfo.evseCommContoller[0]) - 1] = '\0';
                                    break;
                                    
                               case 4:
                                    strncpy(chargerConfigCAN.versionInfo.evseCommContoller[1], token, sizeof(chargerConfigCAN.versionInfo.evseCommContoller[1]) - 1);
                                    chargerConfigCAN.versionInfo.evseCommContoller[1][sizeof(chargerConfigCAN.versionInfo.evseCommContoller[1]) - 1] = '\0';
                                    break;      
                               
/*                             case 5:
                                    strncpy(chargerConfigCAN.versionInfo.PCM[0], token, sizeof(chargerConfigCAN.versionInfo.PCM[0]) - 1);
                                    chargerConfigCAN.versionInfo.PCM[0][sizeof(chargerConfigCAN.versionInfo.PCM[0]) - 1] = '\0';
                                    break;
 */                            
                               default:
                                    //printf("Unexpected data received\n");
                                    break;
                               
                            }
                         tokenIndex++;
                         token = strtok(NULL, ",");                      
                    }//end of while 
                        
                        //printf("HMI Version: %s\n", chargerConfigCAN.versionInfo.hmiVersion);
                        //printf("LCU Version: %s\n", chargerConfigCAN.versionInfo.lcuVersion);
                        //printf("MCU Version: %s\n", chargerConfigCAN.versionInfo.mcuVersion);
                        //printf("EVSE Comm Controller 1: %s\n", chargerConfigCAN.versionInfo.evseCommContoller[0]);
                        //printf("EVSE Comm Controller 2: %s\n", chargerConfigCAN.versionInfo.evseCommContoller[1]);
                //      //printf("PCM Module 1: %s\n", chargerConfigCAN.versionInfo.PCM[0]);
                 }//end of flagVersion       
                
                if(flagOcppUrl == 1 && giveEntrytoPrint ==1 ){
                    strncpy(chargerConfigCAN.ocppURL, arr, sizeof(chargerConfigCAN.ocppURL) - 1);
                    chargerConfigCAN.ocppURL[sizeof(chargerConfigCAN.ocppURL) - 1] = '\0';  // Ensure null-termination      
                    //printf("Ocpp URL: %s\n", chargerConfigCAN.ocppURL);           
                } 
                
                if(flagPmuInitial == 1 && giveEntrytoPrint ==1){
                    parseWiFiCredentials(arr);
                    //printf("SSID: %s\n", chargerConfigCAN.wifi_ssid);
                    //printf("Password: %s\n", chargerConfigCAN.wifi_password);
                }
                
                if(flagPmuBroker == 1 && giveEntrytoPrint ==1){
                    strncpy(chargerConfigCAN.mqtt_broker, arr, sizeof(chargerConfigCAN.mqtt_broker) - 1);
                    chargerConfigCAN.mqtt_broker[sizeof(chargerConfigCAN.mqtt_broker) - 1] = '\0';  // Ensure null-termination  
                    //printf("Broker : %s\n", chargerConfigCAN.mqtt_broker);
                }
                
                if(flagString == 1 && giveEntrytoPrint ==1){
                    strncpy(chargerConfigCAN.serialNumber, arr, sizeof(chargerConfigCAN.serialNumber) - 1);
                    chargerConfigCAN.serialNumber[sizeof(chargerConfigCAN.serialNumber)- 1] = '\0';
                    //printf("Serial: %s\n", chargerConfigCAN.serialNumber);
                } 
                
                break;
            }
        }
   
    }
}

void handle_pcm_message(uint32_t identifier, uint8_t *data){
	pcm_index = identifier - 0x21; // This would tell me which PCM module data has been received .(e.g PCM1 --> index 0 , PCM2 --> index 1)
        //printf("\n The PCM Number is : %d", (pcm_index+1));
	uint8_t sequence_number = data[0];
	//printf("\n The Sequence Number is : %d", sequence_number);
	uint8_t tempVar = 0;
	
	if (pcm_index < MAX_PCM_MODULE) //chargerConfigCAN.numPCM
	{
		//Modification for CAN Communication Health : START 
		powerModulesCAN[pcm_index].CANSuccess = 1;      // Modified by Shikha
		//Modification for CAN Communication Health : END
		switch (sequence_number) {
			
			case 1: 

				powerModulesCAN[pcm_index].gridAvailability = (data[1] & 0x80) >> 7;       
				powerModulesCAN[pcm_index].fault = (data[1] & 0x7F);
			    powerModulesCAN[pcm_index].acVoltages[0] = ((data[2] << 8) | data[3])/10.0f;
				powerModulesCAN[pcm_index].acVoltages[1] = ((data[4] << 8) | data[5])/10.0f;
				powerModulesCAN[pcm_index].acVoltages[2] = ((data[6] << 8) | data[7])/10.0f;
				//printf("\n The Sequence Number is : %d", sequence_number);
				// printf("\n The Grid Available state is : %d" ,powerModulesCAN[pcm_index].gridAvailability);
				// printf("\n The fault code is : %d" ,powerModulesCAN[pcm_index].fault);
				// printf("\n The AC Voltage Phase A is : %.1f" ,powerModulesCAN[pcm_index].acVoltages[0]);
				// printf("\n The AC Voltage Phase B is : %.1f" ,powerModulesCAN[pcm_index].acVoltages[1]);
				// printf("\n The AC Voltage Phase C is : %.1f" ,powerModulesCAN[pcm_index].acVoltages[2]);
				break;	
				
			case 2:
				
				powerModulesCAN[pcm_index].acCurrents[0] = ((data[1] << 8) | data[2])/10.0f;
				powerModulesCAN[pcm_index].acCurrents[1] = ((data[3] << 8) | data[4])/10.0f;
			    powerModulesCAN[pcm_index].acCurrents[2] = ((data[5] << 8) | data[6])/10.0f;
				//printf("\n The Sequence Number is : %d", sequence_number);
			    // printf("\n The AC Current Phase A is : %.1f" ,powerModulesCAN[pcm_index].acCurrents[0]);
			    // printf("\n The AC Current Phase B is : %.1f" ,powerModulesCAN[pcm_index].acCurrents[1]);
			    // printf("\n The AC Current Phase C is : %.1f" ,powerModulesCAN[pcm_index].acCurrents[2]);
				//In the last byte we will have 0xFF .*/
				break;	
				
			case 3:
			
				powerModulesCAN[pcm_index].dcVoltages.Vdcp = ((data[1] << 8) | data[2])/10.0f;
				powerModulesCAN[pcm_index].dcVoltages.Vdcn = ((data[3] << 8) | data[4])/10.0f;
				powerModulesCAN[pcm_index].dcVoltages.Vdc =  ((data[5] << 8) | data[6])/10.0f;
				//tempVar = data[7];
				//printf("\n The Sequence Number is : %d", sequence_number);
				// printf("\n The Vdcp is : %.1f" ,powerModulesCAN[pcm_index].dcVoltages.Vdcp);
				// printf("\n The Vdcn is : %.1f" ,powerModulesCAN[pcm_index].dcVoltages.Vdcn);
				// printf("\n The Vdc is : %.1f" ,powerModulesCAN[pcm_index].dcVoltages.Vdc);
				//powerModulesCAN[pcm_index].dcVoltages.Vdc2_1 = data[7];
				break;	
				
			case 4: 
						
				//powerModulesCAN[pcm_index].dcVoltages.Vdc2_1  = ((data[1] << 8) | tempVar)/10.0f;
				powerModulesCAN[pcm_index].dcVoltages.Vdc2_2  = ((data[2] << 8) | data[3])/10.0f;
				powerModulesCAN[pcm_index].dcVoltages.Vout1   = ((data[4] << 8) | data[5])/10.0f;
				powerModulesCAN[pcm_index].dcVoltages.Vout2   = ((data[6] << 8) | data[7])/10.0f;
				//printf("\n The Sequence Number is : %d", sequence_number);
				// printf("\n The Vdc2_1 is : %.1f" ,powerModulesCAN[pcm_index].dcVoltages.Vdc2_1);
				// printf("\n The Vout1 is : %.1f" ,powerModulesCAN[pcm_index].dcVoltages.Vout1);
				// printf("\n The Vout2 is : %.1f" ,powerModulesCAN[pcm_index].dcVoltages.Vout2);
				// printf("\n The Vdc2_2 is : %.1f",powerModulesCAN[pcm_index].dcVoltages.Vdc2_2);
				break;	
				
			case 5:
			
				powerModulesCAN[pcm_index].dcCurrents.iTrf = ((data[1] << 8) | data[2])/10.0f;
				powerModulesCAN[pcm_index].dcCurrents.Iout1 = ((data[3] << 8) | data[4])/10.0f;
				powerModulesCAN[pcm_index].dcCurrents.Iout2 = ((data[5] << 8) | data[6])/10.0f;
				//printf("\n The Sequence Number is : %d", sequence_number);
                //if(!(data[1] == 0xFF && data[2] == 0xFF))
				//     printf("\n The DC Current is : %.1f" ,powerModulesCAN[pcm_index].dcCurrents.iTrf);
				// printf("\n The DC Current is : %.1f" ,powerModulesCAN[pcm_index].dcCurrents.Iout1);
				// printf("\n The DC Current is : %.1f" ,powerModulesCAN[pcm_index].dcCurrents.Iout2);
				//In the last byte we will have 0xFF
				break;	
				
			case 6:
			
				powerModulesCAN[pcm_index].Temperatures.GSC = 	 ((data[1] << 8) | data[2])/10.0f;
				powerModulesCAN[pcm_index].Temperatures.HFIInv = ((data[3] << 8) | data[4])/10.0f;
				powerModulesCAN[pcm_index].Temperatures.HFRectifier = ((data[5] << 8) | data[6])/10.0f;
				//printf("\n The Sequence Number is : %d", sequence_number);
				// printf("\n The Temp GSC is : %.1f" ,powerModulesCAN[pcm_index].Temperatures.GSC);
				// printf("\n The Temp HFI Inv is : %.1f" ,powerModulesCAN[pcm_index].Temperatures.HFIInv);
				// printf("\n The Temp HFI Rectifier is : %.1f" ,powerModulesCAN[pcm_index].Temperatures.HFRectifier);
				//In the last byte we will have 0xFF
				break;	
				
			case 7:
			
				powerModulesCAN[pcm_index].Temperatures.Buck = ((data[1] << 8) | data[2])/10.0f;
				powerModulesCAN[pcm_index].versionMajorNumber = ((data[3] << 8) | data[4]);
				powerModulesCAN[pcm_index].versionMinorNumber = ((data[5] << 8) | data[6]);
				
				//printf("\n The Sequence Number is : %d", sequence_number);
				// printf("\n The Temp Buck is : %.1f" ,powerModulesCAN[pcm_index].Temperatures.Buck);
				//orNumber is : %d" ,powerModulesCAN[pcm_index].versionMajorNumber);
				//printf("\n The VersionMajorNumber is : %d \n" ,powerModulesCAN[pcm_index].versionMajorNumber);
				//printf("\n The VersionMinorNumber is : %d \n" ,powerModulesCAN[pcm_index].versionMinorNumber);
				//In the last byte we will have 0xFF
				break;		
				
			case 8:
				
				powerModulesCAN[pcm_index].dcVoltages.fanVoltageMaster = ((data[1] << 8) | data[2])/10.0f;
				powerModulesCAN[pcm_index].dcVoltages.fanVoltageSlave    = ((data[3] << 8) | data[4])/10.0f;
                powerModulesCAN[pcm_index].dcVoltages.Vdc2_1 = ((data[5] << 8) | data[6])/10.0f;
                //printf("The value of Vdc2_1 is %.1f", powerModulesCAN[pcm_index].dcVoltages.Vdc2_1);
				pcmPeriodicReceived = 1;
				//printf("\n The Sequence Number is : %d", sequence_number);
                //if(!(data[1] == 0xFF && data[2] == 0xFF))
				    //printf("The Value of FanVoltageMaster : %.1f",powerModulesCAN[pcm_index].dcVoltages.fanVoltageMaster);
				
                //printf("The Value of FanVoltageSlave : %.1f",powerModulesCAN[pcm_index].dcVoltages.fanVoltageSlave);
				//Rest of the Bytes are 0xFF	
				break;
				
			default:
				 //ESP_LOGW(CAN_MCU_TAG, "Invalid sequence number");		
				break;	
		}
	}	
}

void parseWiFiCredentials(const char *receivedString){
	
	char buffer[100];  // Temporary buffer to work with, ensure it's large enough
    strncpy(buffer, receivedString, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';  // Ensure null termination
    
    char *token;
    
    // Extract ssid
    token = strtok(buffer, ",");
    if (token != NULL) {
        strncpy(chargerConfigCAN.wifi_ssid, token, sizeof(chargerConfigCAN.wifi_ssid) - 1);
        chargerConfigCAN.wifi_ssid[sizeof(chargerConfigCAN.wifi_ssid) - 1] = '\0';  // Null-terminate
    }
    
    // Extract password
    token = strtok(NULL, ",");
    if (token != NULL) {
        strncpy(chargerConfigCAN.wifi_password, token, sizeof(chargerConfigCAN.wifi_password) - 1);
        chargerConfigCAN.wifi_password[sizeof(chargerConfigCAN.wifi_password) - 1] = '\0';  // Null-terminate
    }  
}

void handle_session_info(uint8_t *data){

    uint8_t sequence_number = data[0];
    uint8_t can_message_data[8] = {0}; // Will be used for sending Acknowledgement
    AckPmu.can_message_ack = 2;

    switch (sequence_number)
    {
    case 1:
        sessionInfoCAN.sessionType = data[1] & 0x0F;

        //-----------------conditions over sessionType: START-----------------------//
        if(sessionInfoCAN.sessionType == initiatedButNotStarted)
            publishMsgFlag.toggleChargingSession = 1;

        if(sessionInfoCAN.sessionType == initiatedStartedNormalTermination || sessionInfoCAN.sessionType == initiatedStartedFaultyTermination){
            publishMsgFlag.toggleChargingSession = 2;
        }
        if(sessionInfoCAN.sessionType == initiatedStartedPowerOutage)
            publishMsgFlag.toggleChargingSession = 4;

        //-----------------------conditions over sessionType: END-------------------------//

        sessionInfoCAN.connector = (data[1] & 0xF0) >> 4;
        sessionInfoCAN.stopReason = data[2];

        if(sessionInfoCAN.sessionType == initiatedButNotStarted || sessionInfoCAN.sessionType == initiatedStartedPowerOutage){
            memset(sessionInfoCAN.startDateTime, 0xFF, sizeof(sessionInfoCAN.startDateTime));
        }
        else{
            sessionInfoCAN.startDateTime[0] = data[3];
            sessionInfoCAN.startDateTime[1] = data[4];
            sessionInfoCAN.startDateTime[2] = data[5];
            sessionInfoCAN.startDateTime[3] = data[6];
            sessionInfoCAN.startDateTime[4] = data[7];
        }



        AckPmu.sequence_number = 1;
        if(publishMsgFlag.toggleChargingSession == 1 || publishMsgFlag.toggleChargingSession == 4)
            sessionInfoCAN.chargingSessionOccured ^= 1;
        // ESP_LOGW("SESSION_INFO", "Session Type: %d", sessionInfoCAN.sessionType);
        // ESP_LOGW("SESSION_INFO", "Connector: %d", sessionInfoCAN.connector);
        // ESP_LOGW("SESSION_INFO", "Stop Reason: %d", sessionInfoCAN.stopReason);

        //if(publishMsgFlag.toggleChargingSession == 1 || publishMsgFlag.toggleChargingSession == 4)
            // ESP_LOGW("SESSION_INFO", "sessionInfoCAN.chargingSessionOccured: %d", sessionInfoCAN.chargingSessionOccured);
        break;
    
    case 2:
        sessionInfoCAN.startDateTime[5] = data[1];
        sessionInfoCAN.startDateTime[6] = data[2];
        sessionInfoCAN.startDateTime[7] = data[3];
        sessionInfoCAN.startDateTime[8] = data[4];
        sessionInfoCAN.startDateTime[9] = data[5];
        sessionInfoCAN.startDateTime[10] = data[6];
        sessionInfoCAN.startDateTime[11] = data[7];

        AckPmu.sequence_number = 2;
        break;

    case 3:
        sessionInfoCAN.startDateTime[12] = data[1];
        sessionInfoCAN.startDateTime[13] = data[2];
        sessionInfoCAN.startDateTime[14] = data[3];
        sessionInfoCAN.startDateTime[15] = data[4];
        sessionInfoCAN.startDateTime[16] = data[5]; 
        sessionInfoCAN.startDateTime[17] = data[6];
        sessionInfoCAN.startDateTime[18] = data[7];

        AckPmu.sequence_number = 3;
        break;
    
    case 4: 
        sessionInfoCAN.startDateTime[19] = data[1];
        sessionInfoCAN.startDateTime[20] = data[2];
        sessionInfoCAN.startDateTime[21] = data[3];
        sessionInfoCAN.startDateTime[22] = data[4];
        sessionInfoCAN.startDateTime[23] = data[5];
        sessionInfoCAN.startDateTime[24] = data[6];
        sessionInfoCAN.chargePercentage = data[7];

        AckPmu.sequence_number = 4;
        break;
    
    case 5:
    
        sessionInfoCAN.endDateTime[0] = data[1];
        sessionInfoCAN.endDateTime[1] = data[2];
        sessionInfoCAN.endDateTime[2] = data[3];
        sessionInfoCAN.endDateTime[3] = data[4];
        sessionInfoCAN.endDateTime[4] = data[5];
        sessionInfoCAN.endDateTime[5] = data[6];
        sessionInfoCAN.endDateTime[6] = data[7];
        
        AckPmu.sequence_number = 5;
        break;
    
    case 6:
        
        sessionInfoCAN.endDateTime[7] = data[1];
        sessionInfoCAN.endDateTime[8] = data[2];
        sessionInfoCAN.endDateTime[9] = data[3];
        sessionInfoCAN.endDateTime[10] = data[4];
        sessionInfoCAN.endDateTime[11] = data[5];
        sessionInfoCAN.endDateTime[12] = data[6];
        sessionInfoCAN.endDateTime[13] = data[7];
  
        AckPmu.sequence_number = 6;
        break;
    
    case 7:
        sessionInfoCAN.endDateTime[14] = data[1];
        sessionInfoCAN.endDateTime[15] = data[2];
        sessionInfoCAN.endDateTime[16] = data[3];
        sessionInfoCAN.endDateTime[17] = data[4];
        sessionInfoCAN.endDateTime[18] = data[5];
        sessionInfoCAN.endDateTime[19] = data[6];
        sessionInfoCAN.endDateTime[20] = data[7];

        AckPmu.sequence_number = 7;
        break;

    case 8:
        sessionInfoCAN.endDateTime[21] = data[1];
        sessionInfoCAN.endDateTime[22] = data[2];
        sessionInfoCAN.endDateTime[23] = data[3];
        sessionInfoCAN.endDateTime[24] = data[4];
        sessionInfoCAN.energyConsumed = ((uint32_t)data[5] << 16) |  // Upper byte (Byte 6)
                                         ((uint32_t)data[6] << 8)  |  // Middle byte (Byte 7)
                                         (uint32_t)data[7];           // Lower byte (Byte 8)
        AckPmu.sequence_number = 8;
        break;
    
    case 9:
        sessionInfoCAN.estimatedCost = ((uint32_t)data[1] << 16) |  // Upper byte (Byte 6)
                                         ((uint32_t)data[2] << 8)  |  // Middle byte (Byte 7)
                                         (uint32_t)data[3];           // Lower byte (Byte 8)
        sessionInfoCAN.estimatedCost = sessionInfoCAN.estimatedCost/1000.0f;
        sessionInfoCAN.chargeDuration = data[4] << 8 | data[5]; // Upper byte(Byte 5) Lower Byte(Byte 6)
        sessionInfoCAN.startSoC = data[6];
        sessionInfoCAN.endSoC = data[7];
        if(publishMsgFlag.toggleChargingSession == 2){
            sessionInfoCAN.chargingSessionOccured ^= 1;
            // ESP_LOGW("SESSION_INFO", "sessionInfoCAN.chargingSessionOccured: %d", sessionInfoCAN.chargingSessionOccured);
        }
        AckPmu.sequence_number = 9;
        // ESP_LOGW("SESSION_INFO", "Estimated Cost: %.2f", sessionInfoCAN.estimatedCost);
        // ESP_LOGW("SESSION_INFO", "Charge Duration: %lu sec", sessionInfoCAN.chargeDuration);
        // ESP_LOGW("SESSION_INFO", "Start SoC: %d%%", sessionInfoCAN.startSoC);
        // ESP_LOGW("SESSION_INFO", "End SoC: %d%%", sessionInfoCAN.endSoC);
        break;

    default:
        break;
    }

    AckPmu.parameter_number = 0xFF;
	AckPmu.limit_setting_value = 0xFF;	

	can_message_data[0] = AckPmu.can_message_ack; 
	can_message_data[1] = AckPmu.sequence_number;
	can_message_data[2] = AckPmu.parameter_number;
	can_message_data[3] = AckPmu.limit_setting_value;
	memcpy(&can_message_data[4], AckPmu.unused, 4);
	//ESP_LOGI(CAN_MCU_TAG, "Transmitting CAN Acknowledgement message for Fault Information with AckId = %d , Seq No = %d" ,AckPmu.can_message_ack , AckPmu.sequence_number);
	send_can_message(ACK_PMU_ID, can_message_data, 8);
}

void handle_fault_log(uint8_t *data){
    uint8_t sequence_number = data[0];
    uint8_t can_message_data[8] = {0};
    AckPmu.can_message_ack = 4;
    switch(sequence_number){

        case 1:
            if(data[1]!= 0xFF || data[1] != 0xF)
                faultLogCAN.faultType = data[1] & 0x0F;
            if(data[1]!= 0xFF || data[1] != 0xF)
                faultLogCAN.player = (data[1] & 0xF0) >> 4;
            if(data[2] != 0xFF)
                faultLogCAN.faultCode = data[2] ;
            faultLogCAN.occuranceDateTime[0] = data[3];
            faultLogCAN.occuranceDateTime[1] = data[4];
            faultLogCAN.occuranceDateTime[2] = data[5];
            faultLogCAN.occuranceDateTime[3] = data[6];
            faultLogCAN.occuranceDateTime[4] = data[7];

            ESP_LOGW("FAULT_LOG", "Fault Type: %d", faultLogCAN.faultType);
            ESP_LOGW("FAULT_LOG", "Player: %d", faultLogCAN.player);
            ESP_LOGW("FAULT_LOG", "Fault Code: %d", faultLogCAN.faultCode);

            AckPmu.sequence_number = 1;
            AckPmu.parameter_number = 0xFF;
            AckPmu.limit_setting_value = 0xFF;

            can_message_data[0] = AckPmu.can_message_ack; 
            can_message_data[1] = AckPmu.sequence_number;
            can_message_data[2] = AckPmu.parameter_number;
            can_message_data[3] = AckPmu.limit_setting_value;
            memcpy(&can_message_data[4], AckPmu.unused, 4);
            send_can_message(ACK_PMU_ID, can_message_data, 8);
            break;
        
        case 2:

            faultLogCAN.occuranceDateTime[5] = data[1];
            faultLogCAN.occuranceDateTime[6] = data[2];
            faultLogCAN.occuranceDateTime[7] = data[3];
            faultLogCAN.occuranceDateTime[8] = data[4];
            faultLogCAN.occuranceDateTime[9] = data[5];
            faultLogCAN.occuranceDateTime[10] = data[6];
            faultLogCAN.occuranceDateTime[11] = data[7];

            AckPmu.sequence_number = 2;
            AckPmu.parameter_number = 0xFF;
            AckPmu.limit_setting_value = 0xFF;

            can_message_data[0] = AckPmu.can_message_ack; 
            can_message_data[1] = AckPmu.sequence_number;
            can_message_data[2] = AckPmu.parameter_number;
            can_message_data[3] = AckPmu.limit_setting_value;
            memcpy(&can_message_data[4], AckPmu.unused, 4);
            send_can_message(ACK_PMU_ID, can_message_data, 8);
            break;
        
        case 3:

            faultLogCAN.occuranceDateTime[12] = data[1];
            faultLogCAN.occuranceDateTime[13] = data[2];
            faultLogCAN.occuranceDateTime[14] = data[3];
            faultLogCAN.occuranceDateTime[15] = data[4];
            faultLogCAN.occuranceDateTime[16] = data[5];
            faultLogCAN.occuranceDateTime[17] = data[6];
            faultLogCAN.occuranceDateTime[18] = data[7];

            AckPmu.sequence_number = 3;
            AckPmu.parameter_number = 0xFF;
            AckPmu.limit_setting_value = 0xFF;

            can_message_data[0] = AckPmu.can_message_ack; 
            can_message_data[1] = AckPmu.sequence_number;
            can_message_data[2] = AckPmu.parameter_number;
            can_message_data[3] = AckPmu.limit_setting_value;
            memcpy(&can_message_data[4], AckPmu.unused, 4);
            send_can_message(ACK_PMU_ID, can_message_data, 8);
            
            break;
        
        case 4: 
            
            faultLogCAN.occuranceDateTime[19] = data[1];
            faultLogCAN.occuranceDateTime[20] = data[2];
            faultLogCAN.occuranceDateTime[21] = data[3];
            faultLogCAN.occuranceDateTime[22] = data[4];
            faultLogCAN.occuranceDateTime[23] = data[5];
            faultLogCAN.occuranceDateTime[24] = data[6];
            faultLogCAN.clearanceDateTime[0] = data[7];

            ESP_LOGW("FAULT_LOG", "Occurence Date Time %s", faultLogCAN.occuranceDateTime);
            AckPmu.sequence_number = 4;
            AckPmu.parameter_number = 0xFF;
            AckPmu.limit_setting_value = 0xFF;

            can_message_data[0] = AckPmu.can_message_ack; 
            can_message_data[1] = AckPmu.sequence_number;
            can_message_data[2] = AckPmu.parameter_number;
            can_message_data[3] = AckPmu.limit_setting_value;
            memcpy(&can_message_data[4], AckPmu.unused, 4);
            send_can_message(ACK_PMU_ID, can_message_data, 8);
            break;

        case 5:

            faultLogCAN.clearanceDateTime[1] = data[1];
            faultLogCAN.clearanceDateTime[2] = data[2];
            faultLogCAN.clearanceDateTime[3] = data[3];
            faultLogCAN.clearanceDateTime[4] = data[4];
            faultLogCAN.clearanceDateTime[5] = data[5];
            faultLogCAN.clearanceDateTime[6] = data[6];
            faultLogCAN.clearanceDateTime[7] = data[7];

            AckPmu.sequence_number = 5;
            AckPmu.parameter_number = 0xFF;
            AckPmu.limit_setting_value = 0xFF;

            can_message_data[0] = AckPmu.can_message_ack; 
            can_message_data[1] = AckPmu.sequence_number;
            can_message_data[2] = AckPmu.parameter_number;
            can_message_data[3] = AckPmu.limit_setting_value;
            memcpy(&can_message_data[4], AckPmu.unused, 4);
            send_can_message(ACK_PMU_ID, can_message_data, 8);
            break;

        case 6:

            faultLogCAN.clearanceDateTime[8] = data[1];
            faultLogCAN.clearanceDateTime[9] = data[2];
            faultLogCAN.clearanceDateTime[10] = data[3];
            faultLogCAN.clearanceDateTime[11] = data[4];
            faultLogCAN.clearanceDateTime[12] = data[5];
            faultLogCAN.clearanceDateTime[13] = data[6];
            faultLogCAN.clearanceDateTime[14] = data[7];

            AckPmu.sequence_number = 6;
            AckPmu.parameter_number = 0xFF;
            AckPmu.limit_setting_value = 0xFF;

            can_message_data[0] = AckPmu.can_message_ack; 
            can_message_data[1] = AckPmu.sequence_number;
            can_message_data[2] = AckPmu.parameter_number;
            can_message_data[3] = AckPmu.limit_setting_value;
            memcpy(&can_message_data[4], AckPmu.unused, 4);
            send_can_message(ACK_PMU_ID, can_message_data, 8);
            break;
        
        case 7:

            faultLogCAN.clearanceDateTime[15] = data[1];
            faultLogCAN.clearanceDateTime[16] = data[2];
            faultLogCAN.clearanceDateTime[17] = data[3];
            faultLogCAN.clearanceDateTime[18] = data[4];
            faultLogCAN.clearanceDateTime[19] = data[5];
            faultLogCAN.clearanceDateTime[20] = data[6];
            faultLogCAN.clearanceDateTime[21] = data[7];

            AckPmu.sequence_number = 7;
            AckPmu.parameter_number = 0xFF;
            AckPmu.limit_setting_value = 0xFF;

            can_message_data[0] = AckPmu.can_message_ack; 
            can_message_data[1] = AckPmu.sequence_number;
            can_message_data[2] = AckPmu.parameter_number;
            can_message_data[3] = AckPmu.limit_setting_value;
            memcpy(&can_message_data[4], AckPmu.unused, 4);
            send_can_message(ACK_PMU_ID, can_message_data, 8);
            break;

        case 8:

            faultLogCAN.clearanceDateTime[22] = data[1];
            faultLogCAN.clearanceDateTime[23] = data[2];
            faultLogCAN.clearanceDateTime[24] = data[3];
            // faultLogCAN.clearanceDateTime[25] = data[4];
            // faultLogCAN.messageReceived = 1;
            ESP_LOGW("FAULT_LOG", "Clerance Date Time %s", faultLogCAN.clearanceDateTime);
            // ESP_LOGW("FAULT_LOG", LOG_COLOR_CYAN "Inside case 8 SwitchCAN faultLogDataExchangedCAN = %d" LOG_RESET_COLORCYAN, faultLogCAN.faultLogDataExchanged);

            faultLogCAN.faultLogDataExchanged ^= 1;
            // ESP_LOGW("FAULT_LOG", LOG_COLOR_CYAN "Inside case 8 aftertoggle SwitchCAN faultLogDataExchangedCAN = %d" LOG_RESET_COLORCYAN, faultLogCAN.faultLogDataExchanged);
            //Rest would be 0xFF


            AckPmu.sequence_number = 8;
            AckPmu.parameter_number = 0xFF;
            AckPmu.limit_setting_value = 0xFF;

            can_message_data[0] = AckPmu.can_message_ack; 
            can_message_data[1] = AckPmu.sequence_number;
            can_message_data[2] = AckPmu.parameter_number;
            can_message_data[3] = AckPmu.limit_setting_value;
            memcpy(&can_message_data[4], AckPmu.unused, 4);
            send_can_message(ACK_PMU_ID, can_message_data, 8);
            break;
        
        default:
            break;
    }

}

