/*Created on : 22/11/2024
 *Author : Dhruv Joshi & Niharika Singh  
 *File Description : Header that includes the declaration of the enums for project 
 */

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <math.h>


#ifndef MAIN_PMUFINALMAIN_H_
#define MAIN_PMUFINALMAIN_H_

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
	FAILEDUART = 0,
	FINEUART = 1
}OcppControllerEnum;

typedef enum{
	GRID_UNAVAILABLE = 0,
	GRID_AVAILABLE = 1
}gridSupplyEnum;

typedef enum{
	FAILED= 0,  //Earlier FAILED_CAN
	FINE = 1     // FINE_CAN
}CommunicationEnum;

typedef enum{
	ETH_LINK_DOWN_DETECTED = 104,
	POWERED_UP_ON_STATE_B = 101
}ErrorCode0Enum;

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
	VIRTUAL = 1,
	ZERO_VIRTUAL = 2,
	ACTUAL = 3	
}energyMeterSettingEnum;

typedef enum{
	ENABLE = 1,
	DISABLE = 2
}zeroMeterStartEnableEnum;

typedef enum{
	OFFLINE = 0,
	ONLINE = 1
}chargerModeEnum;

typedef enum {
    DCFC = 0, // Direct Current Fast Charger
    ACCharger = 1
} projectInfoEnum;

typedef enum{
    chargingSession=1,
    chargerPeriodicData,
    sendComponentStatus,
    getChargerId,
    chargerStatus
}publishMsgEnumType;
//-------------------------------------------------xxxxxx-------------------------------------------

//MACROS
#define NO_OF_CONNECTORS 2
#define MAX_FAULTS 22
#define MAX_METERS 3
#define MAX_GRID_PHASES 3
#define MAX_DC_PHASES 2
#define MAX_AC_LINES 3
#define MAX_URL_LENGTH 100
#define TOTAL_NO_OF_PCM_MODULES 10
#define NOT_SET 255
#define MAX_PCM_MODULE    10

//---------------------- Variables for CAN : PMU --> MCU transmission  --------------------

//1) This Structure is for the Message PMU Information 
typedef struct {
    uint8_t sequence_number_1;   // Byte 1: Sequence Number
    uint8_t internet_status : 1;  // Bit 1: Internet Connection Status
    uint8_t broker_status : 1;    // Bit 2: Broker Connection Status
    uint8_t reserved : 6;         // Bits 3-8: Reserved, set to 1
    double pmu_major_version; // Byte 3: PMU Major Version Number
    double pmu_minor_version; // Byte 4: PMU Minor Version Number
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
	uint8_t Date;                   // Byte 3: Date (DD)
	uint8_t Month;                	// Byte 4: Month (MM)
	uint8_t Year;                   // Byte 5: Year (YY)
	uint8_t Hour;                   // Byte 6: Hour (HH)
	uint8_t Minute;                 // Byte 7: Minute (MM)
	uint8_t Second;               	// Byte 8: Second (SS)
}FaultInfo_t;

 //4) This structure is for Receiving the Session Information from MCU via CAN
 typedef struct {
	 uint8_t ConnectorNumber : 2;          //2 bits: Connector Number
	 uint8_t ChargingSessionStatus : 2;    //2 bits: Charging Session Status
	 uint8_t Reserved : 4;				   //4 bits : Reserved bits
	 uint8_t Reason;  					   //Byte 2 : Reason 
	 uint8_t unused[6];                    //Byte 3-8 : Unused bytes
 }SessionInfo_t;

 //5) This structure is used for Charger Periodic Data Reception from CAN 
typedef struct {
    // Group of Enum
        CommunicationEnum MCUComm;                 // MCU Communication
        CommunicationEnum ev_evseCommRecv[NO_OF_CONNECTORS];        // EV-EVSE communication statuses
        CommunicationEnum energyMeterComm[MAX_METERS];         // Energy meter communication statuses
        chargerModeEnum chargerMode;             // Charger Mode
        OcppControllerEnum ocppController;       // OCPP Controller
        WebSocketStatusEnum websocketConnection; // WebSocket Connection
        ConnectorStatusEnum connectorStatus[NO_OF_CONNECTORS+1];     // Connector statuses
        faultStatusEnum faultStatus[MAX_FAULTS];             // Fault statuses

        uint32_t energyMeter[MAX_METERS];             // Energy meter settings
        uint32_t virtualEnergyMeter[MAX_METERS];
        
        uint32_t dcMeterPower[NO_OF_CONNECTORS];         // DC meter power data

    // Group of Float Arrays
        double gridVoltages[MAX_GRID_PHASES];     // Grid voltages
        double gridCurrents[MAX_GRID_PHASES];     // Grid currents
        double dcVoltage[MAX_DC_PHASES];          // DC voltages
        double dcCurrent[MAX_DC_PHASES];          // DC currents
        double acMeterLineVoltages[MAX_AC_LINES]; // AC meter line voltages

    // Group of Single Float Variables
        double RealPowerFactor;                   // Real Power Factor
        double AvgPowerFactor;                    // Average Power Factor

        struct {
            uint8_t ev_evseCANCommRecv;
            uint8_t ErrorCode0Recv;
        }ev_evseCommControllers[2];

		CommunicationEnum LCUCommStatus;
		CommunicationEnum LCUInternetStatus;
		CommunicationEnum LCUWebsocketStatus;
    // Group of Strings
        char ocppCmsConnectionURL[MAX_URL_LENGTH]; // OCPP CMS connection URL
} ChargerPeriodicStatus_t;

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
    gridSupplyEnum gridAvailable;
    uint8_t versionMajorNumber;
    uint8_t versionMinorNumber;
}powerModule_t;

//7) Structure for Charger Configuration Data for MQTT and CAN 
typedef struct{
	
	uint8_t numConnectors : 4;      
    uint8_t numPCM : 4;           
    uint8_t powerInputType : 1;
    uint8_t projectInfo : 5;  
    uint8_t parameterName;
    uint8_t limitSettingParameterName;
    uint8_t configKeyValue;
    uint8_t stringLength;
    
	BrandEnum Brand;
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
	  }limitSetting;
	struct{
		char hmiVersion[10];
		char lcuVersion[10];
		char mcuVersion[10];
		char evseCommContoller[2][10];
		char PCM[MAX_PCM_MODULE][10];
		char PMU[10];
	}versionInfo;
	
	char ocppURL[100];
	char wifi_ssid[50];
	char wifi_password[100];
	char mqtt_broker[100];
}chargerConfig_t;

//This is the structure of Flags that needs to be raised 
typedef struct {
    uint8_t send_component_status : 1;										// Event Based
    uint8_t charger_periodic_data : 1;											
    uint8_t charger_periodic_data_pcm : 1;								   
    uint8_t charging_session : 1;												  // Event Based
    uint8_t get_charger_id : 1;
    uint8_t charger_status : 1;													 //Event Based
    uint8_t send_config_data : 1;												// Event Based
    uint8_t subscribeFlag : 1;														// This Flag will be raised once when receiving the ChargerId to subscribe to other topics										
} publishMsgFlagType;






extern publishMsgFlagType publishMsgFlag;
extern char get_Charger_Id_topic[50];
extern char get_Charger_Id_payload[256];

//"chargerStatus/chargerId"
extern ConnectorStatusEnum connectorStatus[3];
extern WebSocketStatusEnum webSocketStatus;
extern faultStatusEnum faultStatus[3];
//"chargingSession/chargerId"
extern ConnectorEnum connector;
extern EventEnum event ;
extern FinishReasonEnum finishReason;
//"sendConfigData/chargerId"
extern valueOfSettingEnum AutoCharge;
extern valueOfSettingEnum DynamicPowerSharing;
extern BrandEnum Brand;
extern energyMeterSettingEnum EnergyMeterSetting;


#endif /* MAIN_PMUFINALMAIN_H_ */
