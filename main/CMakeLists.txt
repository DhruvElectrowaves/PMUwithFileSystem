# See the build system documentation in IDF programming guide
# for more information about component CMakeLists.txt files.

idf_component_register(
    SRCS "PMUFinalmain.c" "CAN.c"  "MqttFunctions.c" "../components/Operation/ValidateMessage.c" "../components/Messages/MQTT.c" "../components/Operation/FormMessage.c" "../components/SPIFFS/SPIFFSUtils.c" "../components/SPIFFS/Dequeueing.c" "../components/NVS_lib/Fetch_NVSdata.c" "../components/NVS_lib/SaveNVSData"  # list the source files of this component
    INCLUDE_DIRS        # optional, add here public include directories
    PRIV_INCLUDE_DIRS   # optional, add here private include directories
    REQUIRES            # optional, list the public requirements (component names)
    PRIV_REQUIRES       # optional, list the private requirements
)
