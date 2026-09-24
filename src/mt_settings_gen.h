// AUTO-GENERATED from meshtastic/protobufs (config, module_config, channel, mesh)
// by /tmp/gen_mt_settings.py -- edit the protos, not this file.
#pragma once
#include <stdint.h>

// scope: 0=Config 1=ModuleConfig 2=Channel 3=Owner
// kind:  0=bool 1=uint 2=int 3=float 4=string/bytes 5=enum
struct MtFieldDef { uint8_t scope; uint8_t type; uint8_t field; uint8_t kind; uint8_t rep; int8_t en; const char* key; };
struct MtEnumDef  { uint8_t count; const int32_t* vals; const char* const* labels; };

static const int32_t MT_E0_V[] = {0, 1};
static const char* const MT_E0_L[] = {"Dhcp", "Static"};
static const MtEnumDef MT_E0 = {2, MT_E0_V, MT_E0_L};
static const int32_t MT_E1_V[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
static const char* const MT_E1_L[] = {"Codec2 Default", "Codec2 3200", "Codec2 2400", "Codec2 1600", "Codec2 1400", "Codec2 1300", "Codec2 1200", "Codec2 700", "Codec2 700b", "Codec2 700c", "Codec2 450"};
static const MtEnumDef MT_E1 = {11, MT_E1_V, MT_E1_L};
static const int32_t MT_E2_V[] = {0, 1, 2, 3, 4};
static const char* const MT_E2_L[] = {"All Enabled", "Disabled", "Notifications Only", "System Only", "Direct Msg Only"};
static const MtEnumDef MT_E2 = {5, MT_E2_V, MT_E2_L};
static const int32_t MT_E3_V[] = {0, 1, 2, 3, 4, 5, 6, 7};
static const char* const MT_E3_L[] = {"Degrees 0", "Degrees 90", "Degrees 180", "Degrees 270", "Degrees 0 Inverted", "Degrees 90 Inverted", "Degrees 180 Inverted", "Degrees 270 Inverted"};
static const MtEnumDef MT_E3 = {8, MT_E3_V, MT_E3_L};
static const int32_t MT_E4_V[] = {0};
static const char* const MT_E4_L[] = {"Unused"};
static const MtEnumDef MT_E4 = {1, MT_E4_V, MT_E4_L};
static const int32_t MT_E5_V[] = {0, 1, 2, 3};
static const char* const MT_E5_L[] = {"Default", "Twocolor", "Inverted", "Color"};
static const MtEnumDef MT_E5 = {4, MT_E5_V, MT_E5_L};
static const int32_t MT_E6_V[] = {0, 1};
static const char* const MT_E6_L[] = {"Metric", "Imperial"};
static const MtEnumDef MT_E6 = {2, MT_E6_V, MT_E6_L};
static const int32_t MT_E7_V[] = {0, 1, 2};
static const char* const MT_E7_L[] = {"Disabled", "Enabled", "Not Present"};
static const MtEnumDef MT_E7 = {3, MT_E7_V, MT_E7_L};
static const int32_t MT_E8_V[] = {0, 1, 2};
static const char* const MT_E8_L[] = {"Disabled", "Enabled", "Not Present"};
static const MtEnumDef MT_E8 = {3, MT_E8_V, MT_E8_L};
static const int32_t MT_E9_V[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 255};
static const char* const MT_E9_L[] = {"Unset", "Tlora V2", "Tlora V1", "Tlora V2 1 1p6", "Tbeam", "Heltec V2 0", "Tbeam V0p7", "T Echo", "Tlora V1 1p3", "Rak4631", "Heltec V2 1", "Heltec V1", "Lilygo Tbeam S3 Core", "Rak11200", "Nano G1", "Tlora V2 1 1p8", "Tlora T3 S3", "Nano G1 Explorer", "Nano G2 Ultra", "Lora Type", "Wiphone", "Wio Wm1110", "Rak2560", "Heltec Hru 3601", "Heltec Wireless Bridge", "Station G1", "Rak11310", "Makerfabs Tracker", "Makerfabs Reserved", "Canaryone", "Rp2040 Lora", "Station G2", "Lora Relay V1", "T Echo Plus", "Ppr", "Genieblocks", "Nrf52 Unknown", "Portduino", "Android Sim", "Diy V1", "Nrf52840 Pca10059", "Dr Dev", "M5stack", "Heltec V3", "Heltec Wsl V3", "Betafpv 2400 Tx", "Betafpv 900 Nano Tx", "Rpi Pico", "Heltec Wireless Tracker", "Heltec Wireless Paper", "T Deck", "T Watch S3", "Picomputer S3", "Heltec Ht62", "Ebyte Esp32 S3", "Esp32 S3 Pico", "Chatter 2", "Heltec Wireless Paper V1 0", "Heltec Wireless Tracker V1 0", "Unphone", "Td Lorac", "Cdebyte Eora S3", "Twc Mesh V4", "Nrf52 Promicro Diy", "Radiomaster 900 Bandit Nano", "Heltec Capsule Sensor V3", "Heltec Vision Master T190", "Heltec Vision Master E213", "Heltec Vision Master E290", "Heltec Mesh Node T114", "Sensecap Indicator", "Tracker T1000 E", "Rak3172", "Wio E5", "Radiomaster 900 Bandit", "Me25ls01 4y10td", "Rp2040 Feather Rfm95", "M5stack Corebasic", "M5stack Core2", "Rpi Pico2", "M5stack Cores3", "Seeed Xiao S3", "Ms24sf1", "Tlora C6", "Wismesh Tap", "Routastic", "Mesh Tab", "Meshlink", "Xiao Nrf52 Kit", "Thinknode M1", "Thinknode M2", "T Eth Elite", "Heltec Sensor Hub", "Muzi Base", "Heltec Mesh Pocket", "Seeed Solar Node", "Nomadstar Meteor Pro", "Crowpanel", "Link 32", "Seeed Wio Tracker L1", "Seeed Wio Tracker L1 Eink", "Muzi R1 Neo", "T Deck Pro", "T Lora Pager", "M5stack Reserved", "Wismesh Tag", "Rak3312", "Thinknode M5", "Heltec Mesh Solar", "T Echo Lite", "Heltec V4", "M5stack C6l", "M5stack Cardputer Adv", "Heltec Wireless Tracker V2", "T Watch Ultra", "Thinknode M3", "Wismesh Tap V2", "Rak3401", "Rak6421", "Thinknode M4", "Thinknode M6", "Meshstick 1262", "Tbeam 1 Watt", "T5 S3 Epaper Pro", "Tbeam Bpf", "Mini Epaper S3", "Tdisplay S3 Pro", "Heltec Mesh Node T096", "Mesh Tracker X1", "Thinknode M7", "Thinknode M8", "Thinknode M9", "Heltec V4 R8", "Heltec Mesh Node T1", "Station G3", "T Impulse Plus", "T Echo Card", "Seeed Wio Tracker L2", "Crowpanel P4", "Heltec Mesh Tower V2", "Meshnology W10", "Heltec Rc32", "Heltec Rc52", "Heltec Rcc6", "Seeed Wio Tracker L1 Pro 1w", "Meshnology W12", "Meshpager X2", "T Connect Pro", "Axiometa Genesis Mini", "Private Hw"};
static const MtEnumDef MT_E9 = {150, MT_E9_V, MT_E9_L};
static const int32_t MT_E10_V[] = {0, 10, 17, 18, 19, 20, 24, 27};
static const char* const MT_E10_L[] = {"None", "Select", "Up", "Down", "Left", "Right", "Cancel", "Back"};
static const MtEnumDef MT_E10 = {8, MT_E10_V, MT_E10_L};
static const int32_t MT_E11_V[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
static const char* const MT_E11_L[] = {"Long Fast", "Long Slow", "Very Long Slow", "Medium Slow", "Medium Fast", "Short Slow", "Short Fast", "Long Moderate", "Short Turbo", "Long Turbo", "Lite Fast", "Lite Slow", "Narrow Fast", "Narrow Slow", "Tiny Fast", "Tiny Slow", "Medium Turbo"};
static const MtEnumDef MT_E11 = {17, MT_E11_V, MT_E11_L};
static const int32_t MT_E12_V[] = {0, 1, 2, 3, 4, 5};
static const char* const MT_E12_L[] = {"Oled Auto", "Oled Ssd1306", "Oled Sh1106", "Oled Sh1107", "Oled Sh1107 128 128", "Oled Sh1107 Rotated"};
static const MtEnumDef MT_E12 = {6, MT_E12_V, MT_E12_L};
static const int32_t MT_E13_V[] = {0, 1, 2};
static const char* const MT_E13_L[] = {"Packet Signature Policy Compatible", "Packet Signature Policy Balanced", "Packet Signature Policy Strict"};
static const MtEnumDef MT_E13 = {3, MT_E13_V, MT_E13_L};
static const int32_t MT_E14_V[] = {0, 1, 2};
static const char* const MT_E14_L[] = {"Random Pin", "Fixed Pin", "No Pin"};
static const MtEnumDef MT_E14 = {3, MT_E14_V, MT_E14_L};
static const int32_t MT_E15_V[] = {0, 1, 2, 3, 4, 5};
static const char* const MT_E15_L[] = {"All", "All Skip Decoding", "Local Only", "Known Only", "None", "Core Portnums Only"};
static const MtEnumDef MT_E15 = {6, MT_E15_V, MT_E15_L};
static const int32_t MT_E16_V[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37};
static const char* const MT_E16_L[] = {"Unset", "Us", "Eu 433", "Eu 868", "Cn", "Jp", "Anz", "Kr", "Tw", "Ru", "In", "Nz 865", "Th", "Lora 24", "Ua 433", "Ua 868", "My 433", "My 919", "Sg 923", "Ph 433", "Ph 868", "Ph 915", "Anz 433", "Kz 433", "Kz 863", "Np 865", "Br 902", "Itu1 2m", "Itu2 2m", "Eu 866", "Eu 874", "Eu 917", "Eu N 868", "Itu3 2m", "Itu1 70cm", "Itu2 70cm", "Itu3 70cm", "Itu2 125cm"};
static const MtEnumDef MT_E16 = {38, MT_E16_V, MT_E16_L};
static const int32_t MT_E17_V[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
static const char* const MT_E17_L[] = {"Client", "Client Mute", "Router", "Router Client", "Repeater", "Tracker", "Sensor", "Tak", "Client Hidden", "Lost And Found", "Tak Tracker", "Router Late", "Client Base"};
static const MtEnumDef MT_E17 = {13, MT_E17_V, MT_E17_L};
static const int32_t MT_E18_V[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
static const char* const MT_E18_L[] = {"Baud Default", "Baud 110", "Baud 300", "Baud 600", "Baud 1200", "Baud 2400", "Baud 4800", "Baud 9600", "Baud 19200", "Baud 38400", "Baud 57600", "Baud 115200", "Baud 230400", "Baud 460800", "Baud 576000", "Baud 921600"};
static const MtEnumDef MT_E18 = {16, MT_E18_V, MT_E18_L};
static const int32_t MT_E19_V[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
static const char* const MT_E19_L[] = {"Default", "Simple", "Proto", "Textmsg", "Nmea", "Caltopo", "Ws85", "Ve Direct", "Ms Config", "Log", "Logtext"};
static const MtEnumDef MT_E19 = {11, MT_E19_V, MT_E19_L};
static const int32_t MT_E20_V[] = {0, 1, 2, 3, 4, 5};
static const char* const MT_E20_L[] = {"Logic Low", "Logic High", "Falling Edge", "Rising Edge", "Either Edge Active Low", "Either Edge Active High"};
static const MtEnumDef MT_E20 = {6, MT_E20_V, MT_E20_L};

static const MtEnumDef MT_ENUMS[] = {MT_E0, MT_E1, MT_E2, MT_E3, MT_E4, MT_E5, MT_E6, MT_E7, MT_E8, MT_E9, MT_E10, MT_E11, MT_E12, MT_E13, MT_E14, MT_E15, MT_E16, MT_E17, MT_E18, MT_E19, MT_E20};

static const MtFieldDef MT_FIELDS[] = {
  {0,0,1,5,0,17,"role"},
  {0,0,2,0,0,-1,"serial_enabled"},
  {0,0,4,1,0,-1,"button_gpio"},
  {0,0,5,1,0,-1,"buzzer_gpio"},
  {0,0,6,5,0,15,"rebroadcast_mode"},
  {0,0,7,1,0,-1,"node_info_broadcast_secs"},
  {0,0,8,0,0,-1,"double_tap_as_button_press"},
  {0,0,9,0,0,-1,"is_managed"},
  {0,0,10,0,0,-1,"disable_triple_click"},
  {0,0,11,4,0,-1,"tzdef"},
  {0,0,12,0,0,-1,"led_heartbeat_disabled"},
  {0,0,13,5,0,2,"buzzer_mode"},
  {0,1,1,1,0,-1,"position_broadcast_secs"},
  {0,1,2,0,0,-1,"position_broadcast_smart_enabled"},
  {0,1,3,0,0,-1,"fixed_position"},
  {0,1,4,0,0,-1,"gps_enabled"},
  {0,1,5,1,0,-1,"gps_update_interval"},
  {0,1,6,1,0,-1,"gps_attempt_time"},
  {0,1,7,1,0,-1,"position_flags"},
  {0,1,8,1,0,-1,"rx_gpio"},
  {0,1,9,1,0,-1,"tx_gpio"},
  {0,1,10,1,0,-1,"broadcast_smart_minimum_distance"},
  {0,1,11,1,0,-1,"broadcast_smart_minimum_interval_secs"},
  {0,1,12,1,0,-1,"gps_en_gpio"},
  {0,1,13,5,0,8,"gps_mode"},
  {0,2,1,0,0,-1,"is_power_saving"},
  {0,2,2,1,0,-1,"on_battery_shutdown_after_secs"},
  {0,2,3,3,0,-1,"adc_multiplier_override"},
  {0,2,4,1,0,-1,"wait_bluetooth_secs"},
  {0,2,6,1,0,-1,"sds_secs"},
  {0,2,7,1,0,-1,"ls_secs"},
  {0,2,8,1,0,-1,"min_wake_secs"},
  {0,2,9,1,0,-1,"device_battery_ina_address"},
  {0,2,32,1,0,-1,"powermon_enables"},
  {0,3,1,0,0,-1,"wifi_enabled"},
  {0,3,3,4,0,-1,"wifi_ssid"},
  {0,3,4,4,0,-1,"wifi_psk"},
  {0,3,5,4,0,-1,"ntp_server"},
  {0,3,6,0,0,-1,"eth_enabled"},
  {0,3,7,5,0,0,"address_mode"},
  {0,3,9,4,0,-1,"rsyslog_server"},
  {0,3,10,1,0,-1,"enabled_protocols"},
  {0,3,11,0,0,-1,"ipv6_enabled"},
  {0,4,1,1,0,-1,"screen_on_secs"},
  {0,4,2,5,0,4,"gps_format"},
  {0,4,3,1,0,-1,"auto_screen_carousel_secs"},
  {0,4,4,0,0,-1,"compass_north_top"},
  {0,4,5,0,0,-1,"flip_screen"},
  {0,4,6,5,0,6,"units"},
  {0,4,7,5,0,12,"oled"},
  {0,4,8,5,0,5,"displaymode"},
  {0,4,9,0,0,-1,"heading_bold"},
  {0,4,10,0,0,-1,"wake_on_tap_or_motion"},
  {0,4,11,5,0,3,"compass_orientation"},
  {0,4,12,0,0,-1,"use_12h_clock"},
  {0,4,13,0,0,-1,"use_long_node_name"},
  {0,4,14,0,0,-1,"enable_message_bubbles"},
  {0,5,1,0,0,-1,"use_preset"},
  {0,5,2,5,0,11,"modem_preset"},
  {0,5,3,1,0,-1,"bandwidth"},
  {0,5,4,1,0,-1,"spread_factor"},
  {0,5,5,1,0,-1,"coding_rate"},
  {0,5,6,3,0,-1,"frequency_offset"},
  {0,5,7,5,0,16,"region"},
  {0,5,8,1,0,-1,"hop_limit"},
  {0,5,9,0,0,-1,"tx_enabled"},
  {0,5,10,2,0,-1,"tx_power"},
  {0,5,11,1,0,-1,"channel_num"},
  {0,5,12,0,0,-1,"override_duty_cycle"},
  {0,5,13,0,0,-1,"sx126x_rx_boosted_gain"},
  {0,5,14,3,0,-1,"override_frequency"},
  {0,5,15,0,0,-1,"pa_fan_disabled"},
  {0,5,103,1,1,-1,"ignore_incoming"},
  {0,5,104,0,0,-1,"ignore_mqtt"},
  {0,5,105,0,0,-1,"config_ok_to_mqtt"},
  {0,5,106,5,0,7,"fem_lna_mode"},
  {0,5,107,0,0,-1,"serial_hal_only"},
  {0,6,1,0,0,-1,"enabled"},
  {0,6,2,5,0,14,"mode"},
  {0,6,3,1,0,-1,"fixed_pin"},
  {0,7,1,4,0,-1,"public_key"},
  {0,7,2,4,0,-1,"private_key"},
  {0,7,3,4,1,-1,"admin_key"},
  {0,7,4,0,0,-1,"is_managed"},
  {0,7,5,0,0,-1,"serial_enabled"},
  {0,7,6,0,0,-1,"debug_log_api_enabled"},
  {0,7,8,0,0,-1,"admin_channel_enabled"},
  {0,7,9,5,0,13,"packet_signature_policy"},
  {1,0,1,0,0,-1,"enabled"},
  {1,0,2,4,0,-1,"address"},
  {1,0,3,4,0,-1,"username"},
  {1,0,4,4,0,-1,"password"},
  {1,0,5,0,0,-1,"encryption_enabled"},
  {1,0,6,0,0,-1,"json_enabled"},
  {1,0,7,0,0,-1,"tls_enabled"},
  {1,0,8,4,0,-1,"root"},
  {1,0,9,0,0,-1,"proxy_to_client_enabled"},
  {1,0,10,0,0,-1,"map_reporting_enabled"},
  {1,1,1,0,0,-1,"enabled"},
  {1,1,2,0,0,-1,"echo"},
  {1,1,3,1,0,-1,"rxd"},
  {1,1,4,1,0,-1,"txd"},
  {1,1,5,5,0,18,"baud"},
  {1,1,6,1,0,-1,"timeout"},
  {1,1,7,5,0,19,"mode"},
  {1,1,8,0,0,-1,"override_console_serial_port"},
  {1,2,1,0,0,-1,"enabled"},
  {1,2,2,1,0,-1,"output_ms"},
  {1,2,3,1,0,-1,"output"},
  {1,2,8,1,0,-1,"output_vibra"},
  {1,2,9,1,0,-1,"output_buzzer"},
  {1,2,4,0,0,-1,"active"},
  {1,2,5,0,0,-1,"alert_message"},
  {1,2,10,0,0,-1,"alert_message_vibra"},
  {1,2,11,0,0,-1,"alert_message_buzzer"},
  {1,2,6,0,0,-1,"alert_bell"},
  {1,2,12,0,0,-1,"alert_bell_vibra"},
  {1,2,13,0,0,-1,"alert_bell_buzzer"},
  {1,2,7,0,0,-1,"use_pwm"},
  {1,2,14,1,0,-1,"nag_timeout"},
  {1,2,15,0,0,-1,"use_i2s_as_buzzer"},
  {1,3,1,0,0,-1,"enabled"},
  {1,3,2,0,0,-1,"heartbeat"},
  {1,3,3,1,0,-1,"records"},
  {1,3,4,1,0,-1,"history_return_max"},
  {1,3,5,1,0,-1,"history_return_window"},
  {1,3,6,0,0,-1,"is_server"},
  {1,4,1,0,0,-1,"enabled"},
  {1,4,2,1,0,-1,"sender"},
  {1,4,3,0,0,-1,"save"},
  {1,4,4,0,0,-1,"clear_on_reboot"},
  {1,5,1,1,0,-1,"device_update_interval"},
  {1,5,2,1,0,-1,"environment_update_interval"},
  {1,5,3,0,0,-1,"environment_measurement_enabled"},
  {1,5,4,0,0,-1,"environment_screen_enabled"},
  {1,5,5,0,0,-1,"environment_display_fahrenheit"},
  {1,5,6,0,0,-1,"air_quality_enabled"},
  {1,5,7,1,0,-1,"air_quality_interval"},
  {1,5,8,0,0,-1,"power_measurement_enabled"},
  {1,5,9,1,0,-1,"power_update_interval"},
  {1,5,10,0,0,-1,"power_screen_enabled"},
  {1,5,11,0,0,-1,"health_measurement_enabled"},
  {1,5,12,1,0,-1,"health_update_interval"},
  {1,5,13,0,0,-1,"health_screen_enabled"},
  {1,5,14,0,0,-1,"device_telemetry_enabled"},
  {1,5,15,0,0,-1,"air_quality_screen_enabled"},
  {1,6,1,0,0,-1,"rotary1_enabled"},
  {1,6,2,1,0,-1,"inputbroker_pin_a"},
  {1,6,3,1,0,-1,"inputbroker_pin_b"},
  {1,6,4,1,0,-1,"inputbroker_pin_press"},
  {1,6,5,5,0,10,"inputbroker_event_cw"},
  {1,6,6,5,0,10,"inputbroker_event_ccw"},
  {1,6,7,5,0,10,"inputbroker_event_press"},
  {1,6,8,0,0,-1,"updown1_enabled"},
  {1,6,9,0,0,-1,"enabled"},
  {1,6,10,4,0,-1,"allow_input_source"},
  {1,6,11,0,0,-1,"send_bell"},
  {1,7,1,0,0,-1,"codec2_enabled"},
  {1,7,2,1,0,-1,"ptt_pin"},
  {1,7,3,5,0,1,"bitrate"},
  {1,7,4,1,0,-1,"i2s_ws"},
  {1,7,5,1,0,-1,"i2s_sd"},
  {1,7,6,1,0,-1,"i2s_din"},
  {1,7,7,1,0,-1,"i2s_sck"},
  {1,8,1,0,0,-1,"enabled"},
  {1,8,2,0,0,-1,"allow_undefined_pin_access"},
  {1,9,1,0,0,-1,"enabled"},
  {1,9,2,1,0,-1,"update_interval"},
  {1,9,3,0,0,-1,"transmit_over_lora"},
  {1,10,1,0,0,-1,"led_state"},
  {1,10,2,1,0,-1,"current"},
  {1,10,3,1,0,-1,"red"},
  {1,10,4,1,0,-1,"green"},
  {1,10,5,1,0,-1,"blue"},
  {1,11,1,0,0,-1,"enabled"},
  {1,11,2,1,0,-1,"minimum_broadcast_secs"},
  {1,11,3,1,0,-1,"state_broadcast_secs"},
  {1,11,4,0,0,-1,"send_bell"},
  {1,11,5,4,0,-1,"name"},
  {1,11,6,1,0,-1,"monitor_pin"},
  {1,11,7,5,0,20,"detection_trigger_type"},
  {1,11,8,0,0,-1,"use_pullup"},
  {1,12,1,0,0,-1,"enabled"},
  {1,12,2,1,0,-1,"paxcounter_update_interval"},
  {1,12,3,2,0,-1,"wifi_threshold"},
  {1,12,4,2,0,-1,"ble_threshold"},
  {2,0,1,1,0,-1,"channel_num"},
  {2,0,2,4,0,-1,"psk"},
  {2,0,3,4,0,-1,"name"},
  {2,0,5,0,0,-1,"uplink_enabled"},
  {2,0,6,0,0,-1,"downlink_enabled"},
  {2,0,8,0,0,-1,"use_aead"},
  {3,0,1,4,0,-1,"id"},
  {3,0,2,4,0,-1,"long_name"},
  {3,0,3,4,0,-1,"short_name"},
  {3,0,4,4,0,-1,"macaddr"},
  {3,0,5,5,0,9,"hw_model"},
  {3,0,6,0,0,-1,"is_licensed"},
  {3,0,7,5,0,17,"role"},
  {3,0,8,4,0,-1,"public_key"},
  {3,0,9,0,1,-1,"is_unmessagable"},
};
#define MT_FIELD_COUNT 201
#define MT_ENUM_COUNT 21

