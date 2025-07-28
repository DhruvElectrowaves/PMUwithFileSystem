/*Created on : 22/11/2024
 *Author : Dhruv Joshi  & Niharika Singh
 *File Description : Header that includes the declaration of the CAN for project 
 */

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <math.h>
#include <sys/_intsup.h>
#include "driver/gpio.h"
#include "driver/twai.h"
#include "esp_log.h"
#include "PMUFinalmain.h"

#ifndef MAIN_CAN_H_
#define MAIN_CAN_H_

#define TX_GPIO_NUM 32  // TWAI TX Pin
#define RX_GPIO_NUM 33  // TWAI RX Pin

#define PMU_MAJOR_VERSION 1 
#define PMU_MINOR_VERSION 2  
// #define MAX_PCM_MODULE    10

// Define identifiers for different message types received from MCU

#define FAULT_INFO_ID         0x31
#define SESSION_INFO_ID       0x32
#define PERIODIC_DATA_ID      0x33
#define CHARGER_CONFIG_ID     0x34
#define ACK_MCU_ID            0x35
#define FAULT_LOG             0x36

// Define identifiers for Sending message types transmitted from PMU
#define ACK_PMU_ID            0x41
#define PMU_INFO_ID           0x42
#define PMU_REQUEST           0x43


//----------------------------------- Variables of CAN ----------------------------
extern char *CAN_MCU_TAG;
extern int k;
extern bool chargerPeriodicReceived;


extern FaultInfo_t faultInfoCAN; 
extern SessionInfo_t sessionInfoCAN;
extern powerModule_t powerModulesCAN[MAX_PCM_MODULE];
extern chargerConfig_t chargerConfigCAN;
extern ChargerPeriodicStatus_t chargerPeriodicStatusCAN;

//---------------------------------- Functions of CAN Setup ------------------------------

extern void twai_setup();
extern void send_can_message(uint32_t id, uint8_t *data, size_t len);

// These are minor functions which are externed 
extern void handle_fault_info(uint8_t *data);
extern void handle_session_info(uint8_t *data);
extern void handle_periodic_data(uint8_t *data);
extern void handle_charger_config(uint8_t *data);
extern void handle_ack_mcu(uint8_t *data);
void handle_fault_log(uint8_t *data);

//This the minor function whcih are externed for PCM
extern void handle_pcm_message(uint32_t identifier, uint8_t *data);

#endif /* MAIN_CAN_H_ */