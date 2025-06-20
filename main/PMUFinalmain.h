/*Created on : 22/11/2024
 *Author : Dhruv Joshi & Niharika Singh  
 *File Description : Header that includes the declaration of the enums for project 
 */

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include "cJSON.h"
#include "mqtt_client.h"



#ifndef MAIN_PMUFINALMAIN_H_
#define MAIN_PMUFINALMAIN_H_

#define NTP_TAG "NTP"
#define OCPP_TAG "OCPP"
//----------------------------------- Enums Definition -----------------------------------------

typedef enum {
    CONNECTOR_CHARGING = 1,
    CONNECTOR_FINISHED = 2,
    CONNECTOR_AVAILABLE = 3,
    CONNECTOR_UNAVAILABLE = 4,
    CONNECTOR_PREPARING = 5,
    CONNECTOR_OCCUPIED = 6
} ConnectorStatusEnum;

typedef enum {
	NO_INTERNET = 0,
	WEBSOCKET_DISCONNECTED = 1,
	WEBSOCKET_CONNECTED = 2
}WebSocketStatusEnum;

typedef enum {
	FAULT_NOERROR = 0,
	FAULT_EMERGENCYBUTTON = 1,
	FAULT_CANA_FAILED = 2,
	FAULT_CANB_FAILED = 3,
	FAULT_EARTH_FAULT = 4,
	FAULT_SMOKE_FAULT = 5,
	FAULT_DOOR_FAULT = 6,
	FAULT_GRID_NOT_AVAILABLE = 7,
	FAULT_BATTERY_OVER_VOLT = 8,
	FAULT_DCLINK_OVER_VOLT = 9,
	FAULT_DCLINK_IMBALANCE = 10,
	FAULT_BATTERY_OVER_CURRENT = 11,
	FAULT_GRID_OVER_CURRENT = 12,
	FAULT_HARDWARE_TRIP = 13,
	FAULT_SOFTWARE_CHARGE_FAIL = 14,
	FAULT_HV_LINK_LOW = 15,
	FAULT_THERMAL_FAULT = 16,
	FAULT_POWERED_UP_IN_DEFAULT = 17,
	FAULT_EV_BATTERY_VOLTAGE_EXCEED = 18,
	FAULT_IMD_FAULT_OCCUR = 19,
	FAULT_PCM_EARTH_FAULT = 20,
	FAULT_SPURIOUS_FAULT = 21
}faultStatusEnum;

typedef enum {
	CHARGER = 0,
	CONNECTOR_1 = 1,
	CONNECTOR_2 = 2
} ConnectorEnum;

typedef enum {
	INITIATED = 0,
	STARTED = 1,
	FINISHED = 2,
	TERMINATED = 3
} EventEnum ;

typedef enum {
	NA = 0,
	REASON_STOP_BUTTON = 1,
	REASON_EARTH_FAULT = 2,
	REASON_SMOKE_DETECTION = 3,
	REASON_DOOR_OPEN = 4,
	REASON_IMD_FAULT = 5,
	REASON_GRID_FAILURE = 6,
	REASON_BATTERY_OVER_VOLTAGE = 10,
	REASON_DC_LINK_OVER_VOLTAGE = 11,
	REASON_DC_LINK_IMBALANCE = 12,
	REASON_BATT_OVER_CURRENT = 13,
	REASON_HARDWARE_TRIP = 15,
	REASON_PCM_EARTH_FAULT = 16,
	REASON_SOFT_CHARGE_FAIL = 17,
	REASON_HV_LINK_LOW = 18,
	REASON_THERMAL_FAULT = 19,
	REASON_SPURIOUS_FAULT = 20,
	REASON_CANA_FAILED = 21,
	REASON_CANB_FAILED = 22,
	REASON_EV = 23,
	REASON_MOBILE_APP = 26,
	REASON_RFID = 27,
	REASON_CME_ERROR_STATE = 29,
	REASON_CHARGING_COMPLETE = 30,
	REASON_EM_BUTTON = 31,
	REASON_ID_TAG_DEAUTHORISED = 32,
	REASON_CHARGING_AMOUNT_LIMIT = 33,
	REASON_CFE101 = 34,
	REASON_CFE102 = 35,
	REASON_CFE103 = 36,
	REASON_CFE104 = 37,
	REASON_CFE105 = 38,
	REASON_CFE106 = 39,
	REASON_CFE107 = 40,
	REASON_CFE108 = 41,
	REASON_CFE109 = 42,
	REASON_CFE110 = 43,
	REASON_CFE111 = 44,
	REASON_CFE112 = 45,
	REASON_CFE113 = 46,
	REASON_CFE114 = 47,
	REASON_CFE115 = 48,
	REASON_CFE116 = 49,
	REASON_CFE117 = 50
} FinishReasonEnum;

typedef enum{
	POWER_MODULE_DATA=0,
	POWER_MODULE_DATA_1 = 1,
	POWER_MODULE_DATA_2 = 2,
	POWER_MODULE_DATA_3 = 3,
	POWER_MODULE_DATA_4 = 4,
	POWER_MODULE_DATA_5 = 5,
	POWER_MODULE_DATA_6 = 6,
	POWER_MODULE_DATA_7 = 7,
	POWER_MODULE_DATA_8 = 8,
	POWER_MODULE_DATA_9 = 9,
	POWER_MODULE_DATA_10 = 10,
	EV_EVSE_COMM_CONTROLLER = 11,
	EV_EVSE_COMM_CONTROLLER_1 = 12,
	EV_EVSE_COMM_CONTROLLER_2 = 13,
	OCPP_CONTROLLER = 14,
	WEBSOCKET_CONNECTION = 15,
	GRID_SUPPLY = 16
} ComponentNameEnum;

typedef enum{
	GRID_UNAVAILABLE = 0,
	GRID_AVAILABLE = 1
}gridSupplyEnum;

typedef enum{
	FAILED= 0,  //Earlier FAILED_CAN
	FINE = 1     // FINE_CAN
}CommunicationEnum;

// typedef enum{
// 	ETH_LINK_DOWN_DETECTED = 104,
// 	POWERED_UP_ON_STATE_B = 101
// }ErrorCode0Enum;

typedef enum {
    POWERED_UP_ON_STATE_B = 101U,
    TCP_CONNECTION_CLOSED = 102U,
    RESULT_NOT_READY_RECEIVED_IN_CM_VALIDATE_CNF = 103U,
    ETH_LINK_DOWN_DETECTED = 104U,
    V2G_SECC_Sequence_Timeout_ERR = 105U,
    V2G_SECC_CPState_Detection_Timeout_ERR = 106U,
    TT_EVSE_SLAC_init_Timeout_ERR = 107U,
    TT_match_sequence_Timeout = 108U,
    TT_match_response_Timeout = 109U,
    TT_EVSE_match_session_Timeout_ERR = 110U,
    V2G_SEQUENCE_ERROR = 111U,
    UNEXPECTED_STATE_DETECTED_DURING_PRECHARGE = 112U,
    UNEXPECTED_STATE_DETECTED_DURING_CURRENTDEMAND = 113U,
    SESSIONSTOP_RECEIVED_BEFORE_CHARGE = 114U,
    UNKNOWN_V2G_MESSGAE_ERR = 115U,
    UNABLE_TO_ENCODE_V2G_MESSAGE = 116U,
    UNABLE_TO_DECODE_V2G_MESSAGE = 117U
} ErrorCode0Enum;

typedef enum{
	PYRAMID_ELECTRONICS = 1,
	ELECTROWAVES_ELECTRONICS = 2,
	Ez_PYRAMID = 3,
	VOLTIC = 4,
	CHARGESOL = 5,
	ZENERGIZE = 6	
}BrandEnum;

typedef enum{
	disable = 1,
	enable = 2
}valueOfSettingEnum;

typedef enum{
	VIRTUAL = 0,
	ACTUAL = 1	
}energyMeterSettingEnum;

typedef enum{
	ENABLE = 1,
	DISABLE = 0
}zeroMeterStartEnableEnum;

typedef enum{
	OFFLINE = 1,
	ONLINE = 0
}chargerModeEnum;

typedef enum {
    DCFC = 0, // Direct Current Fast Charger
    ACCharger = 1
} projectInfoEnum;

typedef enum{
    chargingSession = 1,
    chargerPeriodicData,
	PCMPeriodicData,
    componentStatus,
    getChargerId,
    chargerStatus
}publishMsgEnumType;

typedef enum componentName {
    powerModulesData = 0,
    powerModulesData1,
    powerModulesData2,
    powerModulesData3,
    powerModulesData4,
    powerModulesData5,
    powerModulesData6,
    powerModulesData7,
    powerModulesData8,
    powerModulesData9,
    powerModulesData10,
    ev_evseCommController,
    ev_evseCommController1,
    ev_evseCommController2,
    ocppController,
    websocketConnection,
    gridSupply
}componentName;

typedef enum {
    reasonStopButton = 1U,
    reasonEarthFault = 2U,
    reasonSmokeDetected = 3U,
    reasonDoorOpen = 4U,
    reasonIMDFault = 5U,
    reasonGridFailure = 6U,
    reasonBattOverVoltage = 10U,
    reasonDCLinkOV = 11U,
    reasonDCLnkImbalance = 12U,
    reasonBattOverCurrent = 13U,
    reasonHardwareTrip = 15U,
    reasonPCMEarthFault = 16U,
    reasonSoftChargeFail = 17U,
    reasonHVLinkLow = 18U,
    reasonThermalFault = 19U,
    reasonSpuriousFault = 20U,
    reasonCANAFailed = 21U,
    reasonCANBFailed = 22U,
    reasonEV = 23U,
    reasonCableCheckFailed = 25U,
    reasonMobileApp = 26U,
    reasonRFID = 27U,
    reasonResetReceivedFromServer = 28U,
    reasonCMEErrorState = 29U,
    reasonChargingComplete = 30U,
    reasonEMButton = 31U,
    reasonIDTagDeauthorized = 32U,
    reasonChargingAmountLimit = 33U,
    unknownError = 34U,

    // POWERED_UP_ON_STATE_B = 101U,
    // TCP_CONNECTION_CLOSED = 102U,
    // RESULT_NOT_READY_RECEIVED_IN_CM_VALIDATE_CNF = 103U,
    // ETH_LINK_DOWN_DETECTED = 104U,
    // V2G_SECC_Sequence_Timeout_ERR = 105U,
    // V2G_SECC_CPState_Detection_Timeout_ERR = 106U,
    // TT_EVSE_SLAC_init_Timeout_ERR = 107U,
    // TT_match_sequence_Timeout = 108U,
    // TT_match_response_Timeout = 109U,
    // TT_EVSE_match_session_Timeout_ERR = 110U,
    // V2G_SEQUENCE_ERROR = 111U,
    // UNEXPECTED_STATE_DETECTED_DURING_PRECHARGE = 112U,
    // UNEXPECTED_STATE_DETECTED_DURING_CURRENTDEMAND = 113U,
    // SESSIONSTOP_RECEIVED_BEFORE_CHARGE = 114U,
    // UNKNOWN_V2G_MESSGAE_ERR = 115U,
    // UNABLE_TO_ENCODE_V2G_MESSAGE = 116U,
    // UNABLE_TO_DECODE_V2G_MESSAGE = 117U
}sessionErrorReason;

typedef enum{
    initiatedButNotStarted = 1,
    initiatedStartedNormalTermination = 2,
    initiatedStartedFaultyTermination = 3,
    initiatedStartedPowerOutage = 4
}sessionTypeEnum;

//-------------------------------------------------xxxxxx-------------------------------------------
//MACROS
#define NO_OF_CONNECTORS 3
#define MAX_FAULTS 22
#define MAX_METERS 3
#define MAX_GRID_PHASES 3
#define MAX_DC_PHASES 2
#define MAX_AC_LINES 3
#define MAX_URL_LENGTH 100
#define TOTAL_NO_OF_PCM_MODULES 4
#define NOT_SET 255
#define MAX_PCM_MODULE    10
#define TOTAL_LINE 2048
#define MAX_QUEUE_SIZE 100
#define MAX_QUEUE_SIZE_Fault 50
#define FILEPATH "/spiffs/chargerData.txt"
#define TEMPFILEPATH "/spiffs/tempChargerData.txt"
#define MSG_STRING_LEN 512
#define CHARGERCONFIGKEYS "ccg"
#define SEND_CONFIG_FLAG_KEY "CfgFlag"
#define DEQUE_MSG_BUFFER 2048
#define DEQUE_MSG_BUFFER_FAULT 350
#define RETRY_INTERVAL 20000
#define FAULT_LOG_FILEPATH "/spiffs/fault_log.txt"
#define TEMP_FAULT_LOG_FILEPATH "/spiffs/temp_fault_log.txt"
//3/3/2025
#define FAULT_LOG_FILEPATH_OnlineCharger "/spiffs/fault_log_onlineCharger.txt"
#define FAULT_LOG_FILEPATH_OfflineCharger "/spiffs/fault_log_offlineCharger.txt"
#define FAULT_LOG_FILEPATH_CONNECTOR "/spiffs/fault_log_offlineConn%d.txt"

/* Tags for Logging */
#define SPIFFS_TAG "SPIFFS"
#define NVS_TAG "NVS"

#define LOG_COLOR_PINK "\033[1;35m"
#define LOG_RESET_COLOR "\033[0m"
// #define LOG_COLOR_CYAN  "\x1B[36m"
// #define LOG_RESET_COLORCYAN "\x1B[0m"

#define NVS_KEY_TIMESTAMP "timestamp"
#define NVS_NAMESPACE "UpTime"
#define NVS_INTERNET_TOTAL_UP_TIME "iUT"
#define NVS_INTERNET_TOTAL_DOWN_TIME "iDT"
#define NVS_WEBSOCKET_TOTAL_UP_TIME "wUT"
#define NVS_WEBSOCKET_TOTAL_DOWN_TIME "wDT"

//faultlog
// Define file selection macros for clarity
#define FILE_SELECT_REQUEST 1
#define FILE_SELECT_ONLINE_CHARGER 2
#define FILE_SELECT_OFFLINE_CHARGER 3
#define FILE_SELECT_CONNECTOR_BASE 4  // Connector indexes will start from this value


//---------------------- Variables for CAN : PMU --> MCU transmission  --------------------

//1) This Structure is for the Message PMU Information 
typedef struct {
    uint8_t sequence_number_1;   // Byte 1: Sequence Number
    uint8_t internet_status : 1;  // Bit 1: Internet Connection Status
    uint8_t broker_status : 1;    // Bit 2: Broker Connection Status
    uint8_t reserved : 6;         // Bits 3-8: Reserved, set to 1
    double pmu_major_version;      // Byte 3: PMU Major Version Number
    double pmu_minor_version;      // Byte 4: PMU Minor Version Number
    uint16_t quequeSizeIPC;
    uint16_t quequeSizeCAN;
    uint8_t unused[4];         // Bytes 5-8: Not Used
} can_message_t;

//2) This Structure is for the Message Acknowlegment by PMU 
typedef struct {
	uint8_t can_message_ack;      // Byte 1: CAN Message Name Acknowledgement
    uint8_t sequence_number;      // Byte 2: Sequence Number
    uint8_t parameter_number;     // Byte 3: Parameter Number
    uint8_t limit_setting_value;  // Byte 4: Limit Setting Parameter Value
    uint8_t unused[4];              // Bytes 5-8: Reserved (set to 0xFF)
}CAN_Message;

//3) This structure is for Receiving the Fault Information from MCU via CAN
typedef struct {
	uint8_t ConnectorNumber : 2; 	// 2 bits: Connector Number
	uint8_t FaultStatus : 2;	  	// 2 bits: Fault Status
	uint8_t ConnectorStatus : 3;	// 3 bits: Connector Status
	uint8_t reserved : 1;           // 1 bit: Unused/Reserved (for alignment)
	uint8_t FaultCode ;				// Byte 2: Fault Code
	uint8_t finishReason;
	uint8_t Date;                   // Byte 3: Date (DD)
	uint8_t Month;                	// Byte 4: Month (MM)
	uint8_t Year;                   // Byte 5: Year (YY)
	uint8_t Hour;                   // Byte 6: Hour (HH)
	uint8_t Minute;                 // Byte 7: Minute (MM)
	uint8_t Second;               	// Byte 8: Second (SS)
}FaultInfo_t;
extern FaultInfo_t faultInfoMQTT;

 //4) This structure is for Receiving the Session Information from MCU via CAN
 typedef struct {
    sessionTypeEnum sessionType;
    uint8_t connector : 4;
    sessionErrorReason stopReason;
    char startDateTime[25];
    char endDateTime[25];
    uint8_t chargePercentage;
    uint32_t energyConsumed;
    double estimatedCost;
    uint32_t chargeDuration;
    uint8_t startSoC;
    uint8_t endSoC;
    uint8_t chargingSessionOccured : 1;
    uint8_t prevChargingSession : 1;
 }SessionInfo_t;
extern SessionInfo_t sessionInfoCAN;
extern SessionInfo_t sessionInfoIPC;
extern SessionInfo_t sessionInfoMQTT;

 //5) This structure is used for Charger Periodic Data Reception from CAN 
typedef struct {
    // Group of Enum
        CommunicationEnum MCUComm;                 // MCU Communication
        CommunicationEnum ev_evseCommRecv[NO_OF_CONNECTORS];        // EV-EVSE communication statuses
        CommunicationEnum acMeterComm;         // Energy meter communication statuses
		CommunicationEnum dcMeterComm[NO_OF_CONNECTORS];
        chargerModeEnum chargerMode;             // Charger Mode
        // OcppControllerEnum ocppController;       // OCPP Controller
        uint8_t connectorStatus[NO_OF_CONNECTORS];     // Connector statuses
        faultStatusEnum faultStatus[MAX_FAULTS];             // Fault statuses
         
        uint32_t acEnergy;             // Energy meter settings
		uint32_t dcEnergy[NO_OF_CONNECTORS]; 
        uint32_t virtualEnergyMeter;
        
        uint32_t dcMeterPower[NO_OF_CONNECTORS];         // DC meter power data

    // Group of Float Arrays
        double gridVoltages[MAX_GRID_PHASES];     // Grid voltages
        double gridCurrents[MAX_GRID_PHASES];     // Grid currents
        double dcVoltage[NO_OF_CONNECTORS];          // DC voltages
        double dcCurrent[NO_OF_CONNECTORS];          // DC currents
        double acMeterLineVoltages[MAX_AC_LINES]; // AC meter line voltages

    // Group of Single Float Variables
        double RealPowerFactor;                   // Real Power Factor
        double AvgPowerFactor;                    // Average Power Factor

        uint8_t ErrorCode0Recv[2];
        
		CommunicationEnum LCUCommStatus;
		CommunicationEnum LCUInternetStatus;
		WebSocketStatusEnum LCUWebsocketStatus; // For calculation 
        CommunicationEnum websocketConnection; // WebSocket Connection

    // Group of Strings
        char ocppCmsConnectionURL[MAX_URL_LENGTH]; // OCPP CMS connection URL
        uint8_t CANSuccess;
        uint8_t flagUpTime:1;
        uint8_t receivedDataChargerPeriodic:1;

    // Changes on 5/3/2025
        uint8_t chargerStatus;
        uint8_t connectorsStatus[NO_OF_CONNECTORS];
        uint8_t chargerFaultCode;
        uint8_t connectorFaultCode[NO_OF_CONNECTORS];   

    // Changes By Dhruv 10/03/2025
        uint16_t trackNoofFrames;
} ChargerPeriodicStatus_t;  //Niharika

//6) Structure for PCM Periodic Data for MQTT and CAN implementation 
typedef struct {
    double acVoltages[3];
    double acCurrents[3];
    struct {
        double Vdcp;
        double Vdcn;
        double Vdc;
        double Vdc2_1;
        double Vdc2_2;
        double Vout1;
        double Vout2;
        double fanVoltageMaster;
        double fanVoltageSlave;
    } dcVoltages;
    struct {
        double iTrf;
        double Iout1;
        double Iout2;
    } dcCurrents;
    struct {
        double GSC;
        double HFIInv;
        double HFRectifier;
        double Buck;
    } Temperatures;
    faultStatusEnum fault;
    gridSupplyEnum gridAvailability;
    uint8_t versionMajorNumber;
    uint8_t versionMinorNumber;
    uint16_t CANFailureCount;
	uint8_t CANFailure : 1;
	uint8_t CANSuccess : 1;
}powerModule_t;   //Niharika

//7) Structure for Charger Configuration Data for MQTT and CAN 
typedef struct{
    uint8_t numConnectors : 4;      
    uint8_t numPCM : 4;           
    uint8_t powerInputType : 1;
    uint8_t pmuEnable : 1;
    projectInfoEnum projectInfo : 5;  
    uint8_t parameterName;
    uint8_t limitSettingParameterName;
    uint8_t configKeyValue;
    uint8_t stringLength;
    float tarrifAmount;
    
    uint8_t Brand : 7;
    //BrandEnum Brand;
    valueOfSettingEnum AutoCharge;
    valueOfSettingEnum DynamicPowerSharing;
    energyMeterSettingEnum EnergyMeterSetting;
    zeroMeterStartEnableEnum ZeroMeterStartEnable;
    
    char serialNumber[25];
    struct{
        double chargerPower;
        double gunAVoltage;
        double gunACurrent;
        double gunAPower;
        double gunBVoltage;
        double gunBCurrent;
        double gunBPower;
        double serverChargerPower;
        double serverConnectorAPower;
        double serverConnectorACurrent;
        double serverConnectorBPower;
        double serverConnectorBCurrent;
      }limitSetting;
      
    struct{
        char hmiVersion[45];
        char lcuVersion[45];
        char mcuVersion[45];
        char evseCommContoller[2][45];
        char PCM[MAX_PCM_MODULE][30];
        char PMU[45];
    }versionInfo;
    
    char ocppURL[100];
    char wifi_ssid[50];
    char wifi_password[100];
    char mqtt_broker[100];
    char serialNumber2[26];
    char dateTime[25];
    uint8_t receivedConfigDataMQTT : 1;
    uint8_t eepromCommStatus : 1;
    uint8_t rtcCommStatus: 1;
}chargerConfig_t;

//8) Structure used for FaultLog Messages 
typedef struct {
    uint8_t faultType : 4;
    uint8_t player : 4;
    uint8_t faultCode ;
    char occuranceDateTime[25];
    char clearanceDateTime[25];
    uint8_t faultLogDataExchanged : 1;
    uint8_t messageReceived ;
}faultLog_t;
extern faultLog_t faultLogCAN;
extern faultLog_t faultLogIPC;
extern faultLog_t faultLogMQTT;

//This is the structure of Flags that needs to be raised 
typedef struct {
    uint8_t send_component_status : 1;										// Event Based
    uint8_t charger_periodic_data : 1;											
    uint8_t pcm_periodic_data : 1;								   
    uint8_t charging_session : 1;												  // Event Based
    uint8_t get_charger_id : 1;
    uint8_t charger_status : 1;													 //Event Based
    uint8_t config_data : 1;												// Event Based
    uint8_t subscribeFlag : 1;	
    uint8_t configDataExchanged : 1;	
    uint8_t last_charging_session : 1;		
    uint8_t toggleChargingSession : 3;	
    uint8_t toggleFaultLog : 1;									// This Flag will be raised once when receiving the ChargerId to subscribe to other topics										
} publishMsgFlagType;

typedef struct {
    uint8_t energyMeterArray_objectDelete : 1;
    uint8_t dcCurrentArray_objectDelete : 1;
	uint8_t dcVoltageArray_objectDelete : 1;
    uint8_t acMeterLineVoltagesArray_objectDelete : 1;
    uint8_t dcMeterPowerArray_objectDelete : 1;
	uint8_t PCMCommArray_objectDelete : 1;
} ObjectDeleteFlag_t;

typedef struct {
    uint8_t powerModule_objectDelete : 1;  // 1-bit field
    uint8_t acVoltages_objectDelete : 1;        // 1-bit field
	uint8_t dcVoltages_objectDelete : 1;
    uint8_t acCurrents_objectDelete : 1;        // 1-bit field
    uint8_t dcCurrent_objectDelete : 1;         // 1-bit field
    uint8_t temp_objectDelete : 1;              // 1-bit field
} PowerModuleDeleteFlag_t;

typedef struct {
    uint32_t days;
    uint32_t hours;
    uint32_t minutes;
    uint32_t seconds;
    uint32_t counter;
}timeDuration_t;

typedef struct{
    uint8_t status:1;
    timeDuration_t upTime;
    timeDuration_t downTime;
    uint32_t totalUpTime;
    uint32_t totalDownTime;
    double upTimePercent;
    double downTimePercent;
}internetStatus_t;

typedef struct{
    uint8_t status:1;
    timeDuration_t upTime;
    timeDuration_t downTime;
    uint32_t totalUpTime;
    uint32_t totalDownTime;
    double upTimePercent;
    double downTimePercent;
}websocketStatus_t;

typedef struct {
    char logFilePath[50];
    uint8_t entries;
}connectorFileName_t;

typedef struct{
    bool chargingSessionResponse: 1;
    bool faultInfoResponse: 1;
}responseReceivedCriticalMsg_t;

extern internetStatus_t LCUinternetStatus;
extern internetStatus_t LCUinternetStatusComponent;
extern websocketStatus_t websocketStatus;
extern websocketStatus_t websocketStatusComponent;
extern connectorFileName_t *fileNameConnector;
extern responseReceivedCriticalMsg_t responseReceived;

typedef struct {
    uint8_t request_file_entries;
    uint8_t offlineCharger_file_entries;
    uint8_t onlineCharger_file_entries;
    uint16_t totalEntries;
} fileStruct_t;

extern fileStruct_t fileInfo;

//Flags to Detect trigger for each 
typedef struct {
    uint8_t numConnectorsFlag : 1;
    uint8_t numConnector1 : 1;
    uint8_t numConnector2 : 1;
    uint8_t numPCMFlag : 1;
    uint8_t powerInputTypeFlag : 1;
    uint8_t pmuEnabled : 1;
    uint8_t projectInfoFlag : 1;
    uint8_t parameterNameFlag : 1;
    uint8_t limitSettingParameterNameFlag : 1;
    uint8_t configKeyValueFlag : 1;
    uint8_t stringLengthFlag : 1;
    uint8_t tarrifAmountFlag : 1;
    uint8_t BrandFlag : 1;
    uint8_t AutoChargeFlag : 1;
    uint8_t DynamicPowerSharingFlag : 1;
    uint8_t EnergyMeterSettingFlag : 1;
    uint8_t ZeroMeterStartEnableFlag : 1;
    uint8_t serialNumberFlag : 1;
    uint8_t gunAVoltageFlag : 1;
    uint8_t gunACurrentFlag : 1;
    uint8_t gunAPowerFlag : 1;
    uint8_t gunBVoltageFlag : 1;
    uint8_t gunBCurrentFlag : 1;
    uint8_t gunBPowerFlag : 1;
    uint8_t chargerPowerFlag : 1;
    uint8_t versionInfoFlag : 1;
    uint8_t commController1 : 1;
    uint8_t commController2 : 1;
    uint8_t hmiVersionFlag : 1;
    uint8_t lcuVersionFlag : 1;
    uint8_t mcuVersionFlag : 1;
    uint8_t pmuVersionFlag : 1;
    uint8_t ocppURLFlag : 1;
    uint8_t wifi_ssidFlag : 1;
    uint8_t wifi_passwordFlag : 1;
    uint8_t mqtt_brokerFlag : 1;
    uint8_t serialNumber2Flag : 1;
    //Addition of server Parameters
    uint8_t serverChargerPowerFlag : 1;
    uint8_t serverConnectorAPowerFlag : 1;
    uint8_t serverConnectorACurrentFlag : 1;
    uint8_t serverConnectorBPowerFlag : 1;
    uint8_t serverConnectorBCurrentFlag : 1;
    uint8_t eepromCommStatusFlag : 1;
    uint8_t rtcCommStatusFlag : 1;
    }flags_t; 

extern publishMsgFlagType publishMsgFlag;
extern char get_Charger_Id_topic[50];
extern char get_Charger_Id_payload[256];
extern uint8_t configDataExchangedDetectedFromNVS ;
extern uint8_t valueofChangedParameter;


//"sendComponentStatus/chargerId"
extern int global_component_value;
// extern int componentId;
extern int global_id_value;

extern powerModule_t powerModulesMQTT[];

extern gridSupplyEnum gridStatus;
extern const char *MQTT_TAG;

//extern structure
extern chargerConfig_t chargerConfigMQTT;
extern chargerConfig_t chargerConfigIPC;
extern ObjectDeleteFlag_t ObjectDeleteFlags;
extern PowerModuleDeleteFlag_t  powerModuleDeleteFlags;

//PMU Information Message 
extern can_message_t PMUInformation;
extern CAN_Message AckPmu;
extern uint64_t countPerioidicCAN;
extern int counterAck;
extern uint8_t printChargerConfigData;
extern int prevSequenceNo;
extern bool chargerConfigReceived ;
extern bool eepromFaultedFlag;
extern bool chargerPeriodicReceived ;
extern bool pcmPeriodicReceived ;
extern uint8_t pcm_index;

//MQTT client
extern esp_mqtt_client_handle_t mqtt_client;
extern char pubTopic[];
extern bool wifiConnected;

//Critical Messages
extern uint8_t critical_retry_count;


//Flags
extern uint32_t counterPeriodicCharger;
extern uint32_t counterPeriodic;
extern uint8_t Not_send_config_data_flag;
extern flags_t flagConfig;


/* NVS related variables */
typedef enum {
    TYPE_U8    = 0x01,  /*!< Type uint8_t */
    TYPE_I8    = 0x11,  /*!< Type int8_t */
    TYPE_U16   = 0x02,  /*!< Type uint16_t */
    TYPE_I16   = 0x12,  /*!< Type int16_t */
    TYPE_U32   = 0x04,  /*!< Type uint32_t */
    TYPE_I32   = 0x14,  /*!< Type int32_t */
    TYPE_U64   = 0x08,  /*!< Type uint64_t */
    TYPE_I64   = 0x18,  /*!< Type int64_t */
    TYPE_STR   = 0x21,  /*!< Type string */
    TYPE_BLOB  = 0x42,  /*!< Type blob */
    TYPE_BOOL_  = 0x43,  /*!< Type bool */
    TYPE_ANY   = 0xff   /*!< Must be last */
} NVSDatatypes;

//Uptime Variables
extern uint8_t LCUInternetUp;
extern uint8_t LCUInternetDown;
extern uint8_t LCUWebsocketUp;
extern uint8_t LCUWebsocketDown;

extern void save_payload_to_spiffs(const char *filename, const char *payload);
extern void publish_payload_from_spiffs();
extern char* get_action_from_uuid(const char *uuid);
extern char *extract_uuid_from_json_response(const char *json_response);
extern int handle_response_message(const char *response_uuid);
extern void handle_fault_response_message(const char *response_uuid);
extern void save_data_to_NVS(const char *namespace_name, const char *key, void *value, NVSDatatypes datatype);
extern char* get_queued_request_messages(uint8_t *is_message_important);
extern char* get_queued_fault_messages(int num_connectors);
extern void delete_entry(const char *msg);
extern void print_file_entries(const char* file_path);
extern int search_and_delete_message_from_any_file(const char *target_message);
extern int delete_line_if_found(const char *filepath, const char *target);

//extern NVS files
extern void fetch_keys();
extern void open_ccg_namespace();
extern uint8_t process_chargingSession_payload(cJSON *payload);
extern void fetch_uptime_keys();


extern void write_nvs_value(const char *key, uint32_t new_value);
extern uint32_t read_nvs_value(const char *key);

extern char msg_string[MSG_STRING_LEN];
extern int id_requested;
extern char configDataBootUUID[];
extern uint8_t firstTimeBootUpFlag;
#define IS_MQTT_NETWORK_ONLINE PMUInformation.broker_status && wifiConnected && PMUInformation.internet_status
extern SemaphoreHandle_t mutex_spiffs;
extern SemaphoreHandle_t mutex;

//26/02/2025
extern uint32_t prev_LCUInternetUp ;
extern uint32_t prev_LCUInternetDown;
extern uint32_t prev_LCUWebsocketUp ;
extern uint32_t prev_LCUWebsocketDown ;
extern uint32_t internetCountDown;
/* Files */
#endif /* MAIN_PMUFINALMAIN_H_ */
