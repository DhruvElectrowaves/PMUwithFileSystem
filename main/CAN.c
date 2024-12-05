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
#include <string.h>
#include <math.h>
#include "driver/gpio.h"
#include "driver/twai.h"
#include "esp_log.h"
#include "PMUFinalmain.h"
#include "CAN.h"

twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(TX_GPIO_NUM, RX_GPIO_NUM, TWAI_MODE_NORMAL);
twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS(); // 500 kbps
twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

char arr[100];
int flagVersionInfo = 0;
int flagOcppUrl = 0;
int flagPmuInitial = 0;
int flagPmuBroker = 0;
int flagString = 0;
int i=0;
int giveEntrytoPrint = 0; // This flag is used to give entry to if conditions in Cases 

char *EXAMPLE_TAG = "CAN";
char *CAN_MCU_TAG = "PMU_MCU-CAN";

void parseWiFiCredentials(const char *receivedString);

void twai_setup() {
    ESP_ERROR_CHECK(twai_driver_install(&g_config, &t_config, &f_config));
    ESP_ERROR_CHECK(twai_start());
    ESP_LOGI(EXAMPLE_TAG, "TWAI driver installed and started.");
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
    
    if (twai_transmit(&message, pdMS_TO_TICKS(100)) == ESP_OK) {
        ESP_LOGI(EXAMPLE_TAG, "Message Transmitted: ID=0x%" PRIx32 ", DLC=%d", message.identifier, message.data_length_code);
    } else {
        ESP_LOGW(EXAMPLE_TAG, "Failed to transmit message.");
    }
}

void handle_fault_info(uint8_t *data){
	
	// Populate the structure
	faultInfoCAN.ConnectorNumber = (data[0] & 0xC0) >> 6;	 // Bits 0.0 - 0.1 (2 bits)
	faultInfoCAN.FaultStatus = (data[0] & 0x30) >> 4;        // Bits 0.2 - 0.3 (2 bits)
	faultInfoCAN.ConnectorStatus =  (data[0] & 0x0E) >> 1;   // Bits 0.4 - 0.6 (3 bits)
	faultInfoCAN.FaultCode = data[1];                        // Byte 2
	
	if(faultInfoCAN.FaultStatus == 2) { 					// Fault Status: Power Outage
        faultInfoCAN.Date = 0;
        faultInfoCAN.Month = 0;
        faultInfoCAN.Year = 0;
        faultInfoCAN.Hour = 0;
        faultInfoCAN.Minute = 0;
        faultInfoCAN.Second = 0;
  } else {
	faultInfoCAN.Date = data[2];                             // Byte 3
	faultInfoCAN.Month = data[3];							 // Byte 4 
	faultInfoCAN.Year = data[4];                             // Byte 5
	faultInfoCAN.Hour = data[5]; 							 // Byte 6
	faultInfoCAN.Minute = data[6]; 							 // Byte 7
	faultInfoCAN.Second = data[7];                           // Byte 8 
  }
  
    ESP_LOGI(CAN_MCU_TAG, "Connector Number: %d", faultInfoCAN.ConnectorNumber);
    ESP_LOGI(CAN_MCU_TAG, "Fault Status: %d", faultInfoCAN.FaultStatus);
    ESP_LOGI(CAN_MCU_TAG, "Connector Status: %d", faultInfoCAN.ConnectorStatus);
    ESP_LOGI(CAN_MCU_TAG, "Fault Code: %d", faultInfoCAN.FaultCode);
    
    if (faultInfoCAN.FaultStatus == 2) {
        ESP_LOGI(CAN_MCU_TAG, "Power Outage detected: No date or time information available.");
    } else {
        ESP_LOGI(CAN_MCU_TAG, "Date: %02d-%02d-20%02d", faultInfoCAN.Date, faultInfoCAN.Month, faultInfoCAN.Year);
        ESP_LOGI(CAN_MCU_TAG, "Time: %02d:%02d:%02d", faultInfoCAN.Hour, faultInfoCAN.Minute, faultInfoCAN.Second);
    }
}

void handle_session_info(uint8_t *data){
	
	//Populate the Structure 
	sessionInfoCAN.ConnectorNumber = (data[0] & 0xC0) >> 6;         //Bits 0.0-0.1 (2 bits)
	sessionInfoCAN.ChargingSessionStatus = (data[0] & 0x30) >> 4;   //Bits 0.2-0.3 (2 bits)
	sessionInfoCAN.Reserved = (data[0] & 0x0F);						//Bits 0.4 - 0.7 (4 bits)
	sessionInfoCAN.Reason = data[1];                                //Byte 2 : Reason
	sessionInfoCAN.unused[0] = 0xFF;								//Byte 3 
	sessionInfoCAN.unused[1] = 0xFF;								//Byte 4 
	sessionInfoCAN.unused[2] = 0xFF;								//Byte 5 
	sessionInfoCAN.unused[3] = 0xFF;								//Byte 6
	sessionInfoCAN.unused[4] = 0xFF;								//Byte 7
	sessionInfoCAN.unused[5] = 0xFF;								//Byte 8  
	
	ESP_LOGI(CAN_MCU_TAG, "Connector Number: %d", sessionInfoCAN.ConnectorNumber);
	ESP_LOGI(CAN_MCU_TAG, "Charging Session Status: %d", sessionInfoCAN.ChargingSessionStatus);
	ESP_LOGI(CAN_MCU_TAG, "Reason: %d", sessionInfoCAN.Reason);
	 
}

void handle_periodic_data(uint8_t *data){
	uint8_t sequence_number = data[0];
	
	switch(sequence_number){
		
		case 1:
			
			chargerPeriodicStatusCAN.gridVoltages[0] = ((data[1] << 8) | data[2])/10.0;    //Phase A Voltage 
			chargerPeriodicStatusCAN.gridVoltages[1] = ((data[3] << 8) | data[4])/10.0;    //Phase B Voltage
			chargerPeriodicStatusCAN.gridVoltages[2] = ((data[5] << 8) | data[6])/10.0;	 //Phase C Voltage
			chargerPeriodicStatusCAN.energyMeterComm[0] = data[7];						 //AC Meter Modbus Communication 
			ESP_LOGI(CAN_MCU_TAG, "Grid Voltages: [%.1f, %.1f, %.1f]", chargerPeriodicStatusCAN.gridVoltages[0], chargerPeriodicStatusCAN.gridVoltages[1],  chargerPeriodicStatusCAN.gridVoltages[2]);
			ESP_LOGI(CAN_MCU_TAG, "AC Meter ModBus: %d",(int)chargerPeriodicStatusCAN.energyMeterComm[0]);
			break;
			
		case 2:
			
			chargerPeriodicStatusCAN.gridCurrents[0] = ((data[1] << 8) | data[2])/10.0;		//Phase A Current
			chargerPeriodicStatusCAN.gridCurrents[1] = ((data[3] << 8) | data[4])/10.0;		//Phase B Current
			chargerPeriodicStatusCAN.gridCurrents[2] = ((data[5] << 8) | data[6])/10.0;		//Phase C Current
			//Last Byte would contain 0xFF
			ESP_LOGI(CAN_MCU_TAG, "Grid Current A: %.1f",chargerPeriodicStatusCAN.gridCurrents[0]);
			ESP_LOGI(CAN_MCU_TAG, "Grid Current B: %.1f",chargerPeriodicStatusCAN.gridCurrents[1]);
			ESP_LOGI(CAN_MCU_TAG, "Grid Current C: %.1f",chargerPeriodicStatusCAN.gridCurrents[2]);
			break;
			
		case 3:
			
			chargerPeriodicStatusCAN.acMeterLineVoltages[0] = ((data[1] << 8) | data[2])/10.0;     //V12 line Voltage
			chargerPeriodicStatusCAN.acMeterLineVoltages[1] = ((data[3] << 8) | data[4])/10.0;	 //V23 line Voltage
			chargerPeriodicStatusCAN.acMeterLineVoltages[2] = ((data[5] << 8) | data[6])/10.0;     //V31 line Voltage
			//Last Byte would contain 0xFF
			ESP_LOGI(CAN_MCU_TAG, "acMeterLineVoltages: %.1f",chargerPeriodicStatusCAN.acMeterLineVoltages[0]);
			ESP_LOGI(CAN_MCU_TAG, "acMeterLineVoltages: %.1f",chargerPeriodicStatusCAN.acMeterLineVoltages[1]);
			ESP_LOGI(CAN_MCU_TAG, "acMeterLineVoltages: %.1f",chargerPeriodicStatusCAN.acMeterLineVoltages[2]);
			break;
		
		case 4:
			
			chargerPeriodicStatusCAN.energyMeter[0] =  ((data[1] << 16) | (data[2] << 8) | data[3]);       //Active Energy in Wh
			chargerPeriodicStatusCAN.RealPowerFactor = data[4]/100.0; 									   //Real PF
			chargerPeriodicStatusCAN.AvgPowerFactor  = data[5]/100.0;                                        //Avg PF
			//Byte 6 & 7 have 0xFF
			ESP_LOGI(CAN_MCU_TAG, "Energy Meter 0x%" PRIx32 ":",chargerPeriodicStatusCAN.energyMeter[0]);
			ESP_LOGI(CAN_MCU_TAG, "RealPowerFactor: %f",chargerPeriodicStatusCAN.RealPowerFactor);
			ESP_LOGI(CAN_MCU_TAG, "AvgPowerFactor: %f",chargerPeriodicStatusCAN.AvgPowerFactor);
		 	break;
		
		case 5:
			
			chargerPeriodicStatusCAN.energyMeter[1] = ((data[1] << 16) | (data[2] << 8) | data[3]); 		//Energy (in Wh)
			chargerPeriodicStatusCAN.energyMeterComm[1] = data[4];											//DC Meter Modbus Communication
			//Rest All Bytes are 0xFF
			ESP_LOGI(CAN_MCU_TAG, "Energy Meter 0x%" PRIx32 ":",chargerPeriodicStatusCAN.energyMeter[1]);
//			ESP_LOGI(CAN_MCU_TAG, "Energy Meter Comm 0x%" PRIx32 ":",chargerPeriodicStatusCAN.energyMeterComm[1]);
			break;
			
		case 6:
			
			chargerPeriodicStatusCAN.dcVoltage[0] = ((data[1] << 8) | data[2])/10.0;         				//Dc Meter 1 : Dc Voltage 
			chargerPeriodicStatusCAN.dcCurrent[0] = ((data[3] << 8) | data[4])/10.0;						//Dc Meter 2 : Dc Current
			chargerPeriodicStatusCAN.dcMeterPower[0] = (data[7] << 16) | (data[6] << 8) | data[5]; 		//Power in W
			ESP_LOGI(CAN_MCU_TAG, "DC Meter 1 Voltage: %f",chargerPeriodicStatusCAN.dcVoltage[0]);
			ESP_LOGI(CAN_MCU_TAG, "DC Meter 1 Current: %f",chargerPeriodicStatusCAN.dcCurrent[0]);

			break;
		
		case 7:
			
			chargerPeriodicStatusCAN.energyMeter[2] = ((data[1] << 16) | (data[2] << 8) | data[3]);			//Energy (in Wh)
			chargerPeriodicStatusCAN.energyMeterComm[2] = data[4];
			//Rest All Bytes are 0xFF
			//ESP_LOGI(CAN_MCU_TAG, "Energy Meter Comm: %d",chargerPeriodicStatusCAN.energyMeterComm[2]);
			break;
		
		case 8:
			
			chargerPeriodicStatusCAN.dcVoltage[1] = ((data[1] << 8) | data[2])/10.0;							//Dc Meter 2 : Dc Voltage 
			chargerPeriodicStatusCAN.dcCurrent[1] = ((data[3] << 8) | data[4])/10.0;			    			//Dc Meter 2 : Dc Current
			chargerPeriodicStatusCAN.dcMeterPower[1] = (data[7] << 16) | (data[6] << 8) | data[5]; 			//Power in W
			break;	
		
		case 9:
			
			chargerPeriodicStatusCAN.LCUCommStatus = data[1];												//LCU Communication Status
			chargerPeriodicStatusCAN.ev_evseCommRecv[0] = data[2];
			chargerPeriodicStatusCAN.ev_evseCommControllers[0].ev_evseCANCommRecv = data[2];
			chargerPeriodicStatusCAN.ev_evseCommControllers[0].ErrorCode0Recv = data[3];
			chargerPeriodicStatusCAN.ev_evseCommRecv[1] = data[4];
			chargerPeriodicStatusCAN.ev_evseCommControllers[1].ev_evseCANCommRecv = data[4]; 
			chargerPeriodicStatusCAN.ev_evseCommControllers[1].ErrorCode0Recv = data[5];
			chargerPeriodicStatusCAN.LCUInternetStatus = data[6];											//LCU Internet Status
			chargerPeriodicStatusCAN.LCUWebsocketStatus = data[7];											//LCU Websocket Status
			break;
			
		default:
			ESP_LOGI(CAN_MCU_TAG,"Invalid Sequence Number");
			break;				
	}
}

void handle_charger_config(uint8_t *data){
	uint8_t sequence_number = data[0];
	
	switch (sequence_number) {
			
			case 1:
			
				chargerConfigCAN.numConnectors = data[1] & 0x0F;
				chargerConfigCAN.numPCM = data[1] & 0xF0;		
				chargerConfigCAN.powerInputType = (data[2] >> 0) & 0x01;
				chargerConfigCAN.DynamicPowerSharing = (data[2] >> 1) & 0x01;
				chargerConfigCAN.EnergyMeterSetting = (data[2] >> 2) & 0x03;
				chargerConfigCAN.projectInfo = (data[2] >> 4) & 0x0F;
				chargerConfigCAN.Brand = data[3];
				chargerConfigCAN.serialNumber[0] = data[4];
				chargerConfigCAN.serialNumber[1] = data[5];
				chargerConfigCAN.serialNumber[2] = data[6];
				chargerConfigCAN.serialNumber[3] = data[7];
				break;
				
			case 2:
			
				chargerConfigCAN.serialNumber[4] = data[1];
				chargerConfigCAN.serialNumber[5] = data[2];
				chargerConfigCAN.serialNumber[6] = data[3];
				chargerConfigCAN.serialNumber[7] = data[4];
				chargerConfigCAN.serialNumber[8] = data[5];
				chargerConfigCAN.serialNumber[9] = data[6];
				chargerConfigCAN.serialNumber[10] = data[7];
				break;
				
			case 3: 
			
				chargerConfigCAN.serialNumber[11] = data[1];
				chargerConfigCAN.serialNumber[12] = data[2];
				chargerConfigCAN.serialNumber[13] = data[3];
				chargerConfigCAN.serialNumber[14] = data[4];
				chargerConfigCAN.serialNumber[15] = data[5];
				chargerConfigCAN.serialNumber[16] = data[6];
				chargerConfigCAN.serialNumber[17] = data[7];
				break;
			
			case 4: 
				
				chargerConfigCAN.serialNumber[18] = data[1];
				chargerConfigCAN.serialNumber[19] = data[2];
				chargerConfigCAN.serialNumber[20] = data[3];
				chargerConfigCAN.serialNumber[21] = data[4];
				chargerConfigCAN.serialNumber[22] = data[5];
				chargerConfigCAN.serialNumber[23] = data[6];
				chargerConfigCAN.serialNumber[24] = data[7];
				printf("Serial Number : %s",chargerConfigCAN.serialNumber);
				break;	
				
			case 5:
			
				chargerConfigCAN.parameterName = data[1];
				
				switch(chargerConfigCAN.parameterName){
						
						case 0:  
						   printf("Nothing to be Changed");
						   break;
						
						case 1:
							//AutoCharge Configuration Value
							chargerConfigCAN.AutoCharge = data[2];
							printf("Value is: %d",chargerConfigCAN.AutoCharge);  
						  	break;
						
						case 2:
							//ChargerMode Configuration Value
							chargerConfigCAN.configKeyValue = data[2];  
							printf("Value is: %d",chargerConfigCAN.configKeyValue);
							break;
						
						case 3: 
							//Limit Setting Variable
							chargerConfigCAN.limitSettingParameterName = data[2];
							
							switch(chargerConfigCAN.limitSettingParameterName){

								
									case 1:
										//ChargerPower
										chargerConfigCAN.limitSetting.chargerPower =  ((uint32_t)data[3] << 16) |  // Upper byte (Byte 4)
                                                                                      ((uint32_t)data[4] << 8)  |  // Middle byte (Byte 5)
                                                                                      (uint32_t)data[5];           // Lower byte (Byte 6)
										break;
										
									case 2:
										//gunAVoltage
										chargerConfigCAN.limitSetting.gunAVoltage =   ((uint32_t)data[3] << 16) |  // Upper byte (Byte 4)
                                                                                      ((uint32_t)data[4] << 8)  |  // Middle byte (Byte 5)
                                                                                      (uint32_t)data[5];           // Lower byte (Byte 6)
										break;
										
								    case 3:
								    	//gunACurrent
								    	chargerConfigCAN.limitSetting.gunACurrent = ((uint32_t)data[3] << 16) |  // Upper byte (Byte 4)
                                                                                    ((uint32_t)data[4] << 8)  |  // Middle byte (Byte 5)
                                                                                    (uint32_t)data[5];           // Lower byte (Byte 6)
										
										break;								    
								    
								    case 4:
								    	//gunAPower
								    	chargerConfigCAN.limitSetting.gunAPower =  ((uint32_t)data[3] << 16) |  // Upper byte (Byte 4)
                                                                                   ((uint32_t)data[4] << 8)  |  // Middle byte (Byte 5)
                                                                                   (uint32_t)data[5];           // Lower byte (Byte 6)
                                                                                   
								    	break;
								    
									case 5:
										//gunBVoltage 
										chargerConfigCAN.limitSetting.gunBVoltage = ((uint32_t)data[3] << 16) |  // Upper byte (Byte 4)
                                                                                    ((uint32_t)data[4] << 8)  |  // Middle byte (Byte 5)
                                                                                    (uint32_t)data[5];           // Lower byte (Byte 6)
                                                                                   
								    	break;	    
								    
								    case 6: 
								    	//gunBCurrent
								    	chargerConfigCAN.limitSetting.gunBCurrent = ((uint32_t)data[3] << 16) |  // Upper byte (Byte 4)
                                                                                    ((uint32_t)data[4] << 8)  |  // Middle byte (Byte 5)
                                                                                    (uint32_t)data[5];           // Lower byte (Byte 6)
                                                                                   
								    	break;	    
								    
								    case 7:
								    	//gunBPower
								    	chargerConfigCAN.limitSetting.gunBPower = ((uint32_t)data[3] << 16) |  // Upper byte (Byte 4)
                                                                                  ((uint32_t)data[4] << 8)  |  // Middle byte (Byte 5)
                                                                                  (uint32_t)data[5];           // Lower byte (Byte 6)
                                                                                  
								     	printf("The value of gunBVoltage is : %.2f",chargerConfigCAN.limitSetting.gunBPower );
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
									 printf("Character : %c at index %d \n", arr[i],i);
									 i++;
								 }
	  						break;
	  						
	  					case 5: 
	  						//ocppUrl String
	  						flagOcppUrl = 1;	
	  						chargerConfigCAN.stringLength = ((uint16_t)data[2] << 8) | (uint16_t)data[3]; 
	  						i=0;
	  	   					while(i<4 && i<chargerConfigCAN.stringLength){
									 arr[i] = data[i+4];
									 printf("Character : %c at index %d", arr[i],i);
									 i++;
								 }
	  						break;
	  						
	  					case 6:
	  						//PMU initials in the format "ssid,password"	
	  						flagPmuInitial = 1;	
	  						chargerConfigCAN.stringLength = ((uint16_t)data[2] << 8) | (uint16_t)data[3]; 
	  						i=0;
	  	   					while(i<4 && i<chargerConfigCAN.stringLength){
									 arr[i] = data[i+4];
									 printf("Character : %c at index %d \n", arr[i],i);
									 i++;
								 }
	  						break;
	  						
	  					case 7:
	  						//PMU Broker
	  						flagPmuBroker = 1;	
	  						chargerConfigCAN.stringLength = ((uint16_t)data[2] << 8) | (uint16_t)data[3]; 
	  						i=0;
	  	   					while(i<4 && i<chargerConfigCAN.stringLength){
									 arr[i] = data[i+4];
									 printf("Character : %c at index %d \n", arr[i],i);
									 i++;
								 }
	  						break;	
	  					
	  					case 8:
	  						//Serial Number 	
	  						flagString = 1;	
	  						chargerConfigCAN.stringLength = ((uint16_t)data[2] << 8) | (uint16_t)data[3]; 
	  						i=0;
	  	   					while(i<4 && i<chargerConfigCAN.stringLength){
									 arr[i] = data[i+4];
									 printf("Character : %c at index %d", arr[i],i);
									 i++;
								 }
	  						break;
	  					
	  					default:
	  						//Rare Occurence
	  						break;
	  					}
	  					
				break; // Case 5 ka Break 
					
			case 6:
			case 7:
			case 8:
			case 9:
			case 10:
			case 11:
			case 12:
			{
				int startIndex = i;
				while( i<startIndex+7 && i<chargerConfigCAN.stringLength){
					 arr[i] = data[i-startIndex+1];
					 printf("Character : %c at index %d \n", arr[i],i);
					 i++;
					 if(i==chargerConfigCAN.stringLength){
						 arr[i] = '\0';
						 giveEntrytoPrint =1;
						 printf("Array is %s \n" , arr);
					 }
				 }
				//printf("Array is %s" , arr);
				 
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
                			   
                			   case 5:
                			   		strncpy(chargerConfigCAN.versionInfo.PCM[0], token, sizeof(chargerConfigCAN.versionInfo.PCM[0]) - 1);
                			   		chargerConfigCAN.versionInfo.PCM[0][sizeof(chargerConfigCAN.versionInfo.PCM[0]) - 1] = '\0';
                			   		break;
                			   
                			   default:
                					printf("Unexpected data received\n");
                					break;
                			   
                			}
						 tokenIndex++;
        				 token = strtok(NULL, ",");						 
        			}//end of while 
        			 	
        			 	printf("HMI Version: %s\n", chargerConfigCAN.versionInfo.hmiVersion);
    					printf("LCU Version: %s\n", chargerConfigCAN.versionInfo.lcuVersion);
   	 					printf("MCU Version: %s\n", chargerConfigCAN.versionInfo.mcuVersion);
    					printf("EVSE Comm Controller 1: %s\n", chargerConfigCAN.versionInfo.evseCommContoller[0]);
    					printf("EVSE Comm Controller 2: %s\n", chargerConfigCAN.versionInfo.evseCommContoller[1]);
    					printf("PCM Module 1: %s\n", chargerConfigCAN.versionInfo.PCM[0]);
				 }//end of flagVersion
				 
				 
				
				if(flagOcppUrl == 1 && giveEntrytoPrint ==1 ){
					strncpy(chargerConfigCAN.ocppURL, arr, sizeof(chargerConfigCAN.ocppURL) - 1);
					chargerConfigCAN.ocppURL[sizeof(chargerConfigCAN.ocppURL) - 1] = '\0';  // Ensure null-termination		
					printf("Ocpp URL: %s\n", chargerConfigCAN.ocppURL);			
				} 
				
				if(flagPmuInitial == 1 && giveEntrytoPrint ==1){
					parseWiFiCredentials(arr);
					printf("SSID: %s\n", chargerConfigCAN.wifi_ssid);
    				printf("Password: %s\n", chargerConfigCAN.wifi_password);
				}
				
				if(flagPmuBroker == 1 && giveEntrytoPrint ==1){
					strncpy(chargerConfigCAN.mqtt_broker, arr, sizeof(chargerConfigCAN.mqtt_broker) - 1);
					chargerConfigCAN.mqtt_broker[sizeof(chargerConfigCAN.mqtt_broker) - 1] = '\0';  // Ensure null-termination	
					printf("Broker : %s\n", chargerConfigCAN.mqtt_broker);
				}
				
				if(flagString == 1 && giveEntrytoPrint ==1){
					strncpy(chargerConfigCAN.serialNumber, arr, sizeof(chargerConfigCAN.serialNumber) - 1);
					chargerConfigCAN.serialNumber[sizeof(chargerConfigCAN.serialNumber)- 1] = '\0';
					printf("Serial: %s\n", chargerConfigCAN.serialNumber);
				}
				
				break;
			}
		}
   }

void handle_ack_mcu(uint8_t *data){
	
}

void handle_pcm_message(uint32_t identifier, uint8_t *data){
	uint8_t pcm_index = identifier - 0x21 ; // This would tell me which PCM module data has been received .(e.g PCM1 --> index 0 , PCM2 --> index 1)
	printf("\n The PCM Number is : %d", (pcm_index+1));
	uint8_t sequence_number = data[0];
	printf("\n The Sequence Number is : %d", sequence_number);
	uint8_t tempVar = 0;
	
	if (pcm_index < MAX_PCM_MODULE){
		
		switch (sequence_number) {
			
			case 1: 
			
				powerModulesCAN[pcm_index].gridAvailable = (data[1] & 0x80) >> 7;       
				powerModulesCAN[pcm_index].fault = (data[1] & 0x7F);
			    powerModulesCAN[pcm_index].acVoltages[0] = ((data[2] << 8) | data[3])/10.0f;
				powerModulesCAN[pcm_index].acVoltages[1] = ((data[4] << 8) | data[5])/10.0f;
				powerModulesCAN[pcm_index].acVoltages[2] = ((data[6] << 8) | data[7])/10.0f;
				printf("\n The Grid Available state is : %d" ,powerModulesCAN[pcm_index].gridAvailable);
				printf("\n The fault code is : %d" ,powerModulesCAN[pcm_index].fault);
				printf("\n The AC Voltage Phase A is : %.1f" ,powerModulesCAN[pcm_index].acVoltages[0]);
				printf("\n The AC Voltage Phase B is : %.1f" ,powerModulesCAN[pcm_index].acVoltages[1]);
				printf("\n The AC Voltage Phase C is : %.1f" ,powerModulesCAN[pcm_index].acVoltages[2]);
				break;	
				
			case 2:
				
				powerModulesCAN[pcm_index].acCurrents[0] = ((data[1] << 8) | data[2])/10.0f;
				powerModulesCAN[pcm_index].acCurrents[1] = ((data[3] << 8) | data[4])/10.0f;
			    powerModulesCAN[pcm_index].acCurrents[2] = ((data[5] << 8) | data[6])/10.0f;
			    printf("\n The AC Current Phase A is : %.1f" ,powerModulesCAN[pcm_index].acCurrents[0]);
			    printf("\n The AC Current Phase B is : %.1f" ,powerModulesCAN[pcm_index].acCurrents[1]);
			    printf("\n The AC Current Phase C is : %.1f" ,powerModulesCAN[pcm_index].acCurrents[2]);
				//In the last byte we will have 0xFF .
				break;	
				
			case 3:
			
				powerModulesCAN[pcm_index].dcVoltages.Vdcp = ((data[1] << 8) | data[2])/10.0f;
				powerModulesCAN[pcm_index].dcVoltages.Vdcn = ((data[3] << 8) | data[4])/10.0f;
				powerModulesCAN[pcm_index].dcVoltages.Vdc =  ((data[5] << 8) | data[6])/10.0f;
				tempVar = data[7];
				printf("\n The Vdcp is : %.1f" ,powerModulesCAN[pcm_index].dcVoltages.Vdcp);
				printf("\n The Vdcn is : %.1f" ,powerModulesCAN[pcm_index].dcVoltages.Vdcn);
				printf("\n The Vdc is : %.1f" ,powerModulesCAN[pcm_index].dcVoltages.Vdc);
			//	powerModulesCAN[pcm_index].dcVoltages.Vdc2_1 = data[7];
				break;	
				
			case 4: 
						
				powerModulesCAN[pcm_index].dcVoltages.Vdc2_1  = ((data[1] << 8) | tempVar) /10.0f;
				powerModulesCAN[pcm_index].dcVoltages.Vdc2_2  = ((data[2] << 8) | data[3])/10.0f;
				powerModulesCAN[pcm_index].dcVoltages.Vout1   = ((data[4] << 8) | data[5])/10.0f;
				powerModulesCAN[pcm_index].dcVoltages.Vout2   = ((data[6] << 8) | data[7])/10.0f;
				printf("\n The Vdc2_1 is : %.1f" ,powerModulesCAN[pcm_index].dcVoltages.Vdc2_1);
				printf("\n The Vout1 is : %.1f" ,powerModulesCAN[pcm_index].dcVoltages.Vout1);
				printf("\n The Vout2 is : %.1f" ,powerModulesCAN[pcm_index].dcVoltages.Vout2);
				printf("\n The Vdc2_2 is : %.1f",powerModulesCAN[pcm_index].dcVoltages.Vdc2_2);
				break;	
				
			case 5:
			
				powerModulesCAN[pcm_index].dcCurrents.iTrf = ((data[1] << 8) | data[2])/10.0f;
				powerModulesCAN[pcm_index].dcCurrents.Iout1 = ((data[3] << 8) | data[4])/10.0f;
				powerModulesCAN[pcm_index].dcCurrents.Iout2 = ((data[5] << 8) | data[6])/10.0f;
				printf("\n The DC Current is : %.1f" ,powerModulesCAN[pcm_index].dcCurrents.iTrf);
				printf("\n The DC Current is : %.1f" ,powerModulesCAN[pcm_index].dcCurrents.Iout1);
				printf("\n The DC Current is : %.1f" ,powerModulesCAN[pcm_index].dcCurrents.Iout2);
				//In the last byte we will have 0xFF
				break;	
				
			case 6:
			
				powerModulesCAN[pcm_index].Temperatures.GSC = 	 ((data[1] << 8) | data[2])/10.0f;
				powerModulesCAN[pcm_index].Temperatures.HFIInv = ((data[3] << 8) | data[4])/10.0f;
				powerModulesCAN[pcm_index].Temperatures.HFRectifier = ((data[5] << 8) | data[6])/10.0f;
				printf("\n The Temp GSC is : %.1f" ,powerModulesCAN[pcm_index].Temperatures.GSC);
				printf("\n The Temp HFI Inv is : %.1f" ,powerModulesCAN[pcm_index].Temperatures.HFIInv);
				printf("\n The Temp HFI Rectifier is : %.1f" ,powerModulesCAN[pcm_index].Temperatures.HFRectifier);
				//In the last byte we will have 0xFF
				break;	
				
			case 7:
			
				powerModulesCAN[pcm_index].Temperatures.Buck = ((data[1] << 8) | data[2])/10.0f;
				powerModulesCAN[pcm_index].versionMajorNumber = ((data[3] << 8) | data[4]);
				powerModulesCAN[pcm_index].versionMinorNumber = ((data[5] << 8) | data[6]);
				printf("\n The Temp Buck is : %.1f" ,powerModulesCAN[pcm_index].Temperatures.Buck);
				printf("\n The VersionMajorNumber is : %d" ,powerModulesCAN[pcm_index].versionMajorNumber);
				printf("\n The VersionMinorNumber is : %d" ,powerModulesCAN[pcm_index].versionMinorNumber);
				//In the last byte we will have 0xFF
				break;		
				
			case 8:
				
				powerModulesCAN[pcm_index].dcVoltages.fanVoltageMaster = ((data[1] << 8) | data[2])/10.0f;
				powerModulesCAN[pcm_index].dcVoltages.fanVoltageSlave    = ((data[3] << 8) | data[4])/10.0f;
				printf("The Value of FanVoltageMaster : %.1f",powerModulesCAN[pcm_index].dcVoltages.fanVoltageMaster);
				printf("The Value of FanVoltageSlave : %.1f",powerModulesCAN[pcm_index].dcVoltages.fanVoltageSlave);
				//Rest of the Bytes are 0xFF	
				
			default:
				 ESP_LOGW(CAN_MCU_TAG, "Invalid sequence number");		
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


