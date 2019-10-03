/**
 * Copyright (c) 2018 - 2019, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/** @file
 *
 * @defgroup zigbee_examples_light_switch_groups main.c
 * @{
 * @ingroup zigbee_examples
 * @brief Dimmer switch for HA profile implementation with ZCL Groups functionality.
 */

#include "zboss_api.h"
#include "zb_mem_config_min.h"
#include "zb_error_handler.h"
#include "zigbee_helpers.h"

#include "app_timer.h"
#include "bsp.h"
#include "boards.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "zigbee_logger_eprxzcl.h"
#include "zboss_api_addons.h"

#include "zb_ha_hue_dimmer_switch.h"
#include "nrf_drv_saadc.h"

#define IEEE_CHANNEL_MASK                   (1l << ZIGBEE_CHANNEL)              /**< Scan only one, predefined channel to find the coordinator. */
#define LIGHT_SWITCH_ZLL_ENDPOINT               0x1                                   /**< ZLL Source endpoint used to control light bulb. */
#define LIGHT_SWITCH_ZHA_ENDPOINT               0x2                                   /**< ZHA Source endpoint used to control light bulb. */
#define MATCH_DESC_REQ_START_DELAY          (2 * ZB_TIME_ONE_SECOND)            /**< Delay between the light switch startup and light bulb finding procedure. */
#define MATCH_DESC_REQ_ROLE            ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE      // ZB_NWK_BROADCAST_ALL_DEVICES   /**< Find only non-sleepy device. */
#define DEFAULT_GROUP_ID                    0xB331                              /**< Group ID, which will be used to control all light sources with a single command. */
#define ERASE_PERSISTENT_CONFIG             ZB_FALSE                            /**< Do not erase NVRAM to save the network parameters after device reboot or power-off. NOTE: If this option is set to ZB_TRUE then do full device erase for all network devices before running other samples. */
#define ZIGBEE_NETWORK_STATE_LED            BSP_BOARD_LED_2                     /**< LED indicating that light switch successfully joind ZigBee network. */
#define BULB_FOUND_LED                      BSP_BOARD_LED_3                     /**< LED indicating that light witch found a light bulb to control. */
#define LIGHT_SWITCH_BUTTON_ON              BSP_BOARD_BUTTON_0                  /**< Button ID used to switch on the light bulb. */
#define LIGHT_SWITCH_BUTTON_OFF             BSP_BOARD_BUTTON_1                  /**< Button ID used to switch off the light bulb. */
#define LIGHT_LEVEL_BUTTON_UP               BSP_BOARD_BUTTON_2
#define LIGHT_LEVEL_BUTTON_DOWN             BSP_BOARD_BUTTON_3

#define LIGHT_SWITCH_DIMM_STEP              15                                  /**< Dim step size - increases/decreses current level (range 0x000 - 0xfe). */
#define LIGHT_SWITCH_DIMM_TRANSACTION_TIME  2                                   /**< Transition time for a single step operation in 0.1 sec units. 0xFFFF - immediate change. */

#define LIGHT_SWITCH_BUTTON_THRESHOLD       ZB_TIME_ONE_SECOND                      /**< Number of beacon intervals the button should be pressed to dimm the light bulb. */
#define LIGHT_SWITCH_BUTTON_SHORT_POLL_TMO  ZB_MILLISECONDS_TO_BEACON_INTERVAL(50)  /**< Delay between button state checks used in order to detect button long press. */
#define LIGHT_SWITCH_BUTTON_LONG_POLL_TMO   ZB_MILLISECONDS_TO_BEACON_INTERVAL(300) /**< Time after which the button state is checked again to detect button hold - the dimm command is sent again. */


/* Basic cluster attributes initial values. */
#define BULB_INIT_BASIC_APP_VERSION       01                                    /**< Version of the application software (1 byte). */
#define BULB_INIT_BASIC_STACK_VERSION     10                                    /**< Version of the implementation of the ZigBee stack (1 byte). */
#define BULB_INIT_BASIC_HW_VERSION        11                                    /**< Version of the hardware of the device (1 byte). */
#define BULB_INIT_BASIC_SW_VERSION        "5.45.1.17846"                                    /**< Version of the hardware of the device (1 byte). */
#define BULB_INIT_BASIC_MANUF_NAME        "Philips"                              /**< Manufacturer name (32 bytes). */
#define BULB_INIT_BASIC_MODEL_ID          "RWL021"                  /**< Model number assigned by manufacturer (32-bytes long string). */
#define BULB_INIT_BASIC_DATE_CODE         "20180416"                            /**< First 8 bytes specify the date of manufacturer of the device in ISO 8601 format (YYYYMMDD). The rest (8 bytes) are manufacturer specific. */
#define BULB_INIT_BASIC_POWER_SOURCE      ZB_ZCL_BASIC_POWER_SOURCE_BATTERY   /**< Type of power sources available for the device. For possible values see section 3.2.2.2.8 of ZCL specification. */
#define BULB_INIT_BASIC_LOCATION_DESC     "Office desk"                         /**< Describes the physical location of the device (16 bytes). May be modified during commisioning process. */
#define BULB_INIT_BASIC_PH_ENV            ZB_ZCL_BASIC_ENV_UNSPECIFIED          /**< Describes the type of physical environment. For possible values see section 3.2.2.2.10 of ZCL specification. */

#define ZB_PHILIPS_MANUF_CODE             0x100b    // Not required
#define RX_ON_IDLE                        ZB_FALSE  // Not required
#define HARDWARE_VERSION                  0x0000
#define BACKGROUND_DFU_DEFAULT_BLOCK_SIZE 64
#define OTA_UPGRADE_TEST_DATA_SIZE        BACKGROUND_DFU_DEFAULT_BLOCK_SIZE

#define PHILIPS_BRIDGE_ZHA_ENDPOINT       0x41
#define PHILIPS_BUTTON_EVENT_CMD_CODE 0x00

#define ADC_REF_VOLTAGE_IN_MILLIVOLTS   600                                     /**< Reference voltage (in milli volts) used by ADC while doing conversion. */
#define ADC_PRE_SCALING_COMPENSATION    6                                       /**< The ADC is configured to use VDD with 1/3 prescaling as input. And hence the result of conversion is to be multiplied by 3 to get the actual value of the battery voltage.*/
#define DIODE_FWD_VOLT_DROP_MILLIVOLTS  270                                     /**< Typical forward voltage drop of the diode . */
#define ADC_RES_10BIT                   1024                                    /**< Maximum digital value for 10-bit ADC conversion. */
#define BATTERY_LEVEL_MEAS_INTERVAL     APP_TIMER_TICKS(300000)                 /**< Battery level measurement interval (ticks). This value corresponds to 300 seconds (5 minutes). */

#define ADC_RESULT_IN_MILLI_VOLTS(ADC_VALUE)\
        ((((ADC_VALUE) * ADC_REF_VOLTAGE_IN_MILLIVOLTS) / ADC_RES_10BIT) * ADC_PRE_SCALING_COMPENSATION)



#if !defined ZB_ED_ROLE
#error Define ZB_ED_ROLE to compile light switch (End Device) source code.
#endif

typedef struct light_switch_button_s
{
  zb_bool_t in_progress;
  zb_bool_t mayClear;
  zb_bool_t longHold;
  zb_uint8_t progressButtonId;
  zb_time_t timestamp;
  zb_bool_t bufferInUse;
} light_switch_button_t;



typedef struct
{
    /* zll */
    zb_zcl_basic_attrs_ext_hue_t    zll_basic_serv_attr;
    zb_zcl_basic_attrs_ext_hue_t    zll_basic_client_attr;
    zb_zcl_identify_attrs_t         zll_identify_attr;
    zb_zcl_groups_attrs_t           zll_groups_attr;
    zb_zcl_on_off_attrs_ext_t       zll_on_off_attr;
    zb_zcl_level_control_attrs_t    zll_level_control_attr;
    zb_zcl_scenes_attrs_t           zll_scenes_attr;

    /* zha */
    zb_zcl_basic_attrs_ext_hue_t    zha_basic_serv_attr;
    zb_zcl_power_config_attrs_ext_t zha_pwrconf_serv_attr;
    zb_zcl_identify_attrs_t         zha_identify_serv_attr;
    zb_zcl_binary_input_attrs_t     zha_binary_input_serv_attr;
    zb_zcl_tunneling_attrs_t        zha_tunnelling_serv_attr;
    ota_client_ota_upgrade_attr_t   zha_otau_attr;

    /* other */
    light_switch_button_t           button;
    zb_addr_u                       bridge_short_addr;
    zb_bool_t                       nwk_joined;

    

} switch_ctx_t;

static switch_ctx_t m_device_ctx;

static nrf_saadc_value_t adc_buf[2];

APP_TIMER_DEF(m_battery_timer_id);                      /**< Battery measurement timer. */

static void battery_level_meas_timeout_handler(void * p_context);

//static zb_void_t find_light_bulb(zb_uint8_t param);

/* ZLL Cluster list declarations */
ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST_EXT( zll_basic_serv_attr_list,
                                      &m_device_ctx.zll_basic_serv_attr.zcl_version,
                                      &m_device_ctx.zll_basic_serv_attr.app_version,
                                      &m_device_ctx.zll_basic_serv_attr.stack_version,
                                      &m_device_ctx.zll_basic_serv_attr.hw_version,
                                      m_device_ctx.zll_basic_serv_attr.mf_name,
                                      m_device_ctx.zll_basic_serv_attr.model_id,
                                      m_device_ctx.zll_basic_serv_attr.date_code,
                                      &m_device_ctx.zll_basic_serv_attr.power_source,
                                      m_device_ctx.zll_basic_serv_attr.location_id,
                                      &m_device_ctx.zll_basic_serv_attr.ph_env,
                                      m_device_ctx.zll_basic_serv_attr.sw_ver,
                                      &m_device_ctx.zll_basic_serv_attr.philips_device_flag,
                                      &m_device_ctx.zll_basic_serv_attr.philips_en_flag );

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST_EXT( zll_basic_client_attr_list,
                                      &m_device_ctx.zll_basic_client_attr.zcl_version,
                                      &m_device_ctx.zll_basic_client_attr.app_version,
                                      &m_device_ctx.zll_basic_client_attr.stack_version,
                                      &m_device_ctx.zll_basic_client_attr.hw_version,
                                      m_device_ctx.zll_basic_client_attr.mf_name,
                                      m_device_ctx.zll_basic_client_attr.model_id,
                                      m_device_ctx.zll_basic_client_attr.date_code,
                                      &m_device_ctx.zll_basic_client_attr.power_source,
                                      m_device_ctx.zll_basic_client_attr.location_id,
                                      &m_device_ctx.zll_basic_client_attr.ph_env,
                                      m_device_ctx.zll_basic_client_attr.sw_ver,
                                      &m_device_ctx.zll_basic_client_attr.philips_device_flag,
                                      &m_device_ctx.zll_basic_client_attr.philips_en_flag );

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST( zll_identify_attr_list, &m_device_ctx.zll_identify_attr.identify_time );

ZB_ZCL_DECLARE_GROUPS_ATTRIB_LIST( zll_groups_attr_list, &m_device_ctx.zll_groups_attr.name_support );

ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST_EXT( zll_on_off_attr_list,
                                       &m_device_ctx.zll_on_off_attr.on_off,
                                       &m_device_ctx.zll_on_off_attr.global_scene_ctrl,
                                       &m_device_ctx.zll_on_off_attr.on_time,
                                       &m_device_ctx.zll_on_off_attr.off_wait_time );


ZB_ZCL_DECLARE_LEVEL_CONTROL_ATTRIB_LIST( zll_level_control_attr_list,
                                          &m_device_ctx.zll_level_control_attr.current_level,
                                          &m_device_ctx.zll_level_control_attr.remaining_time);

ZB_ZCL_DECLARE_SCENES_ATTRIB_LIST( zll_scenes_attr_list,
                                   &m_device_ctx.zll_scenes_attr.scene_count,
                                   &m_device_ctx.zll_scenes_attr.current_scene,
                                   &m_device_ctx.zll_scenes_attr.current_group,
                                   &m_device_ctx.zll_scenes_attr.scene_valid,
                                   &m_device_ctx.zll_scenes_attr.name_support);


/* ZHA Cluster list declarations */

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST_EXT( zha_basic_serv_attr_list,
                                      &m_device_ctx.zha_basic_serv_attr.zcl_version,
                                      &m_device_ctx.zha_basic_serv_attr.app_version,
                                      &m_device_ctx.zha_basic_serv_attr.stack_version,
                                      &m_device_ctx.zha_basic_serv_attr.hw_version,
                                      m_device_ctx.zha_basic_serv_attr.mf_name,
                                      m_device_ctx.zha_basic_serv_attr.model_id,
                                      m_device_ctx.zha_basic_serv_attr.date_code,
                                      &m_device_ctx.zha_basic_serv_attr.power_source,
                                      m_device_ctx.zha_basic_serv_attr.location_id,
                                      &m_device_ctx.zha_basic_serv_attr.ph_env,
                                      m_device_ctx.zha_basic_serv_attr.sw_ver,
                                      &m_device_ctx.zha_basic_serv_attr.philips_device_flag,
                                      &m_device_ctx.zha_basic_serv_attr.philips_en_flag );


ZB_ZCL_DECLARE_POWER_CONFIG_BATTERY_ATTRIB_LIST_EXT( zha_pwrconf_serv_attr_list,
                                                     &m_device_ctx.zha_pwrconf_serv_attr.voltage,
                                                     &m_device_ctx.zha_pwrconf_serv_attr.size,
                                                     &m_device_ctx.zha_pwrconf_serv_attr.quantity,
                                                     &m_device_ctx.zha_pwrconf_serv_attr.rated_voltage,
                                                     &m_device_ctx.zha_pwrconf_serv_attr.alarm_mask,
                                                     &m_device_ctx.zha_pwrconf_serv_attr.voltage_min_threshold,
                                                     &m_device_ctx.zha_pwrconf_serv_attr.remaining,
                                                     &m_device_ctx.zha_pwrconf_serv_attr.threshold1,
                                                     &m_device_ctx.zha_pwrconf_serv_attr.threshold2,
                                                     &m_device_ctx.zha_pwrconf_serv_attr.threshold3,
                                                     &m_device_ctx.zha_pwrconf_serv_attr.min_threshold,
                                                     &m_device_ctx.zha_pwrconf_serv_attr.percent_threshold1,
                                                     &m_device_ctx.zha_pwrconf_serv_attr.percent_threshold2,
                                                     &m_device_ctx.zha_pwrconf_serv_attr.percent_threshold3,
                                                     &m_device_ctx.zha_pwrconf_serv_attr.alarm_state );

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST( zha_identify_serv_attr_list, &m_device_ctx.zha_identify_serv_attr.identify_time );

ZB_ZCL_DECLARE_BINARY_INPUT_ATTRIB_LIST( zha_binary_input_serv_attr_list,
                                         &m_device_ctx.zha_binary_input_serv_attr.out_of_service,
                                         &m_device_ctx.zha_binary_input_serv_attr.present_value,
                                         &m_device_ctx.zha_binary_input_serv_attr.status_flag );

ZB_ZCL_DECLARE_TUNNELING_ATTR_LIST( zha_tunnel_serv_attr_list, m_device_ctx.zha_tunnelling_serv_attr );

/* OTA cluster attributes data */
ZB_ZCL_DECLARE_OTA_UPGRADE_ATTRIB_LIST( zha_otau_attr_list,
                                        m_device_ctx.zha_otau_attr.upgrade_server,
                                        &m_device_ctx.zha_otau_attr.file_offset,
                                        &m_device_ctx.zha_otau_attr.file_version,
                                        &m_device_ctx.zha_otau_attr.stack_version,
                                        &m_device_ctx.zha_otau_attr.downloaded_file_ver,
                                        &m_device_ctx.zha_otau_attr.downloaded_stack_ver,
                                        &m_device_ctx.zha_otau_attr.image_status,
                                        &m_device_ctx.zha_otau_attr.manufacturer,
                                        &m_device_ctx.zha_otau_attr.image_type,
                                        &m_device_ctx.zha_otau_attr.min_block_reque,
                                        &m_device_ctx.zha_otau_attr.image_stamp,
                                        &m_device_ctx.zha_otau_attr.server_addr,
                                        &m_device_ctx.zha_otau_attr.server_ep,
                                        HARDWARE_VERSION,
                                        OTA_UPGRADE_TEST_DATA_SIZE,
                                        ZB_ZCL_OTA_UPGRADE_QUERY_TIMER_COUNT_DEF );

/* Declare cluster list for Dimmer Switch device (Identify, Basic, Scenes, Groups, On Off, Level Control). */
/* Only clusters Identify and Basic have attributes. */
ZB_HA_HUE_ZLL_DECLARE_DIMMER_SWITCH_CLUSTER_LIST( dimmer_switch_zll_clusters,
                                                  zll_basic_serv_attr_list,
                                                  zll_basic_client_attr_list,
                                                  zll_identify_attr_list,
                                                  zll_groups_attr_list,       
                                                  zll_on_off_attr_list,       
                                                  zll_level_control_attr_list,
                                                  zll_scenes_attr_list );

ZB_HA_HUE_ZHA_DECLARE_DIMMER_SWITCH_CLUSTER_LIST( dimmer_switch_zha_clusters,
                                                  zha_basic_serv_attr_list,
                                                  zha_pwrconf_serv_attr_list,
                                                  zha_identify_serv_attr_list,
                                                  zha_binary_input_serv_attr_list,
                                                  zha_tunnel_serv_attr_list,
                                                  zha_otau_attr_list );

/* Declare endpoint for Dimmer Switch device. */
ZB_HA_HUE_ZLL_DECLARE_DIMMER_SWITCH_EP( dimmer_switch_zll_ep,
                                        LIGHT_SWITCH_ZLL_ENDPOINT,
                                        dimmer_switch_zll_clusters );

ZB_HA_HUE_ZHA_DECLARE_DIMMER_SWITCH_EP( dimmer_switch_zha_ep,
                                        LIGHT_SWITCH_ZHA_ENDPOINT,
                                        dimmer_switch_zha_clusters );

/* Declare application's device context (list of registered endpoints) for Dimmer Switch device. */
ZB_HA_HUE_DECLARE_DIMMER_SWITCH_CTX( dimmer_switch_ctx,
                                     dimmer_switch_zll_ep,
                                     dimmer_switch_zha_ep );

/**@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module. This creates and starts application timers.
 */
static void timers_init(void)
{
    ret_code_t err_code;

    // Initialize timer module.
    err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);

    // Create battery timer.
    NRF_LOG_INFO( "Create battery timer" );
    err_code = app_timer_create(&m_battery_timer_id,
                                APP_TIMER_MODE_REPEATED,
                                battery_level_meas_timeout_handler);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing the nrf log module.
 */
static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}



/**@brief Callback function for handling ZCL commands.
 *
 * @param[in]   param   Reference to ZigBee stack buffer used to pass received data.
 */
static zb_void_t zcl_device_cb(zb_uint8_t param)
{
    zb_uint8_t                       cluster_id;
    zb_uint8_t                       attr_id;
    uint8_t                          endpoint;           
    zb_buf_t                       * p_buffer = ZB_BUF_FROM_REF(param);
    zb_buf_t                       * p_buf_report;
    zb_uint8_t                     * p_cmd_ptr;
    zb_zcl_device_callback_param_t * p_device_cb_param =
                     ZB_GET_BUF_PARAM(p_buffer, zb_zcl_device_callback_param_t);

    UNUSED_VARIABLE( cluster_id );
    UNUSED_VARIABLE( attr_id );
    UNUSED_VARIABLE( endpoint );
    UNUSED_VARIABLE( p_buf_report );
    UNUSED_VARIABLE( p_cmd_ptr );

    NRF_LOG_INFO("zcl_device_cb id %hd", p_device_cb_param->device_cb_id);

    /* Set default response value. */
    p_device_cb_param->status = RET_OK;

    switch (p_device_cb_param->device_cb_id)
    {
        case ZB_ZCL_SET_ATTR_VALUE_CB_ID:
            cluster_id = p_device_cb_param->cb_param.set_attr_value_param.cluster_id;
            attr_id    = p_device_cb_param->cb_param.set_attr_value_param.attr_id;
            endpoint   = p_device_cb_param->endpoint;

            NRF_LOG_INFO( "Request to write ep/cluster/attr %d/0x%04x/0x%04x", endpoint, cluster_id, attr_id );
            

            // lets set up reporting here
//            p_buf_report = ZB_GET_OUT_BUF();

            break;

        default:
            p_device_cb_param->status = RET_ERROR;
            NRF_LOG_INFO( "Unhandled ZCL CB %d", p_device_cb_param->device_cb_id );
            break;
    }

    NRF_LOG_INFO("zcl_device_cb status: %hd", p_device_cb_param->status);
}




/**@brief Function for sending ON/OFF requests to the light bulb.
 *
 * @param[in]   param    Non-zero reference to ZigBee stack buffer that will be used to construct on/off request.
 * @param[in]   on_off   Requested state of the light bulb.
//  */
// static zb_void_t light_switch_send_on_off(zb_uint8_t param, zb_uint16_t on_off)
// {
// //    zb_uint8_t           cmd_id;
// //    zb_uint16_t          group_id = DEFAULT_GROUP_ID;
// //    zb_buf_t           * p_buf = ZB_BUF_FROM_REF(param);



//     NRF_LOG_INFO("Send ON/OFF command: %d", on_off);


//   //  m_device_ctx.zha_tunnelling_serv_attr.philips_type = 0x0001;
//     /*
//     ZB_ZCL_SET_ATTRIBUTE( LIGHT_SWITCH_ZHA_ENDPOINT, 
//                           ZB_ZCL_CLUSTER_ID_BASIC,    
//                           ZB_ZCL_CLUSTER_SERVER_ROLE,  
//                           ZB_ZCL_ATTR_BASIC_SHH_SECRET_ID,
//                           ( zb_uint8_t * )&m_device_ctx.zha_basic_serv_attr.shhh_secret,                        
//                           ZB_FALSE);
//     ZB_ZCL_SET_ATTRIBUTE( LIGHT_SWITCH_ZHA_ENDPOINT, 
//                           ZB_ZCL_CLUSTER_ID_BASIC,    
//                           ZB_ZCL_CLUSTER_SERVER_ROLE,  
//                           ZB_ZCL_ATTR_BASIC_USERTEST_ID,
//                           ( zb_uint8_t * )&m_device_ctx.zha_basic_serv_attr.usertest,                        
//                           ZB_FALSE);
//     ZB_ZCL_SET_ATTRIBUTE( LIGHT_SWITCH_ZHA_ENDPOINT, 
//                           ZB_ZCL_CLUSTER_ID_BASIC,    
//                           ZB_ZCL_CLUSTER_SERVER_ROLE,  
//                           ZB_ZCL_ATTR_BASIC_LED_INDICATION_ID,
//                           ( zb_uint8_t * )&m_device_ctx.zha_basic_serv_attr.led_indication,                        
//                           ZB_FALSE);

    
//     ZB_ZCL_SET_ATTRIBUTE( LIGHT_SWITCH_ZHA_ENDPOINT, 
//                           ZB_ZCL_CLUSTER_ID_TUNNEL,    
//                           ZB_ZCL_CLUSTER_SERVER_ROLE,  
//                           ZB_ZCL_ATTR_TUNNELING_PHILIPS_TYPE_ID,
//                           ( zb_uint8_t * )&m_device_ctx.zha_tunnelling_serv_attr.philips_type,                       
//                           ZB_FALSE);
// */

// /*
//     if (on_off)
//     {
//         cmd_id = ZB_ZCL_CMD_ON_OFF_ON_ID;
//     }
//     else
//     {
//         cmd_id = ZB_ZCL_CMD_ON_OFF_OFF_ID;
//     }

//     ZB_ZCL_ON_OFF_SEND_REQ(p_buf,
//                            group_id,
//                            ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT,
//                            0,
//                            LIGHT_SWITCH_ZLL_ENDPOINT,
//                            ZB_AF_HA_PROFILE_ID,
//                            ZB_ZCL_DISABLE_DEFAULT_RESPONSE,
//                            cmd_id,
//                            NULL);

//                            */
// }

/**@brief Function for sending step requests to the light bulb.
 *
 * @param[in]   param        Non-zero reference to ZigBee stack buffer that will be used to construct step request.
 * @param[in]   is_step_up   Boolean parameter selecting direction of step change.
 */
// static zb_void_t light_switch_send_step(zb_uint8_t param, zb_uint16_t is_step_up)
// {
// /*
//     zb_uint8_t           step_dir;
//     zb_uint16_t          group_id = DEFAULT_GROUP_ID;
//     zb_buf_t           * p_buf = ZB_BUF_FROM_REF(param);

//     if (is_step_up)
//     {
//         step_dir = ZB_ZCL_LEVEL_CONTROL_STEP_MODE_UP;
//     }
//     else
//     {
//         step_dir = ZB_ZCL_LEVEL_CONTROL_STEP_MODE_DOWN;
//     }

//     NRF_LOG_INFO("Send step level command: %d", is_step_up);

//     ZB_ZCL_LEVEL_CONTROL_SEND_STEP_REQ(p_buf,
//                                        group_id,
//                                        ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT,
//                                        0,
//                                        LIGHT_SWITCH_ZLL_ENDPOINT,
//                                        ZB_AF_HA_PROFILE_ID,
//                                        ZB_ZCL_DISABLE_DEFAULT_RESPONSE,
//                                        NULL,
//                                        step_dir,
//                                        LIGHT_SWITCH_DIMM_STEP,
//                                        LIGHT_SWITCH_DIMM_TRANSACTION_TIME);
// */
// }

/**@brief Perform local operation - leave network.
 *
 * @param[in]   param   Reference to ZigBee stack buffer that will be used to construct leave request.
 */
static void light_switch_leave_nwk(zb_uint8_t param)
{
    zb_ret_t zb_err_code;

    /* We are going to leave */
    if (param)
    {
        zb_buf_t                  * p_buf = ZB_BUF_FROM_REF(param);
        zb_zdo_mgmt_leave_param_t * p_req_param;

        p_req_param = ZB_GET_BUF_PARAM(p_buf, zb_zdo_mgmt_leave_param_t);
        UNUSED_RETURN_VALUE(ZB_BZERO(p_req_param, sizeof(zb_zdo_mgmt_leave_param_t)));

        /* Set dst_addr == local address for local leave */
        p_req_param->dst_addr = ZB_PIBCACHE_NETWORK_ADDRESS();
        p_req_param->rejoin   = ZB_FALSE;
        UNUSED_RETURN_VALUE(zdo_mgmt_leave_req(param, NULL));
    }
    else
    {
        zb_err_code = ZB_GET_OUT_BUF_DELAYED(light_switch_leave_nwk);
        ZB_ERROR_CHECK(zb_err_code);
    }
}

/**@brief Function for starting join/rejoin procedure.
 *
 * param[in]   leave_type   Type of leave request (with or without rejoin).
 */
static zb_void_t light_switch_retry_join(zb_uint8_t leave_type)
{
    zb_bool_t comm_status;

    if (leave_type == ZB_NWK_LEAVE_TYPE_RESET)
    {
        comm_status = bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
        ZB_COMM_STATUS_CHECK(comm_status);
    }
}

/**@brief Function for leaving current network and starting join procedure afterwards.
 *
 * @param[in]   param   Optional reference to ZigBee stack buffer to be reused by leave and join procedure.
 */
static zb_void_t light_switch_leave_and_join(zb_uint8_t param)
{
    if (ZB_JOINED())
    {
        /* Leave network. Joining procedure will be initiated inisde ZigBee stack signal handler. */
        light_switch_leave_nwk(param);
    }
    else
    {
        /* Already left network. Start joining procedure. */
        light_switch_retry_join(ZB_NWK_LEAVE_TYPE_RESET);

        if (param)
        {
            ZB_FREE_BUF_BY_REF(param);
        }
    }
}


/**@brief Function for sending add group request. As a result all light bulb's
 *        light controlling endpoints will participate in the same group.
 *
 * @param[in]   param   Non-zero reference to ZigBee stack buffer that will be used to construct add group request.
 */
// static zb_void_t add_group(zb_uint8_t param)
// {
//     zb_buf_t                   * p_buf = ZB_BUF_FROM_REF(param);
//     zb_zdo_match_desc_resp_t   * p_resp = (zb_zdo_match_desc_resp_t *) ZB_BUF_BEGIN(p_buf);    // Get the begining of the response
//     zb_apsde_data_indication_t * p_ind  = ZB_GET_BUF_PARAM(p_buf, zb_apsde_data_indication_t); // Get the pointer to the parameters buffer, which stores APS layer response
//     zb_uint16_t                  dst_addr = p_ind->src_addr;
//     zb_uint8_t                   dst_ep = *(zb_uint8_t *)(p_resp + 1);

//     NRF_LOG_INFO("Include device 0x%x, ep %d to the group 0x%x", dst_addr, dst_ep, DEFAULT_GROUP_ID);
// /*
//     ZB_ZCL_GROUPS_SEND_ADD_GROUP_REQ(p_buf,
//                                      dst_addr,
//                                      ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
//                                      dst_ep,
//                                      LIGHT_SWITCH_ZLL_ENDPOINT,
//                                      ZB_AF_HA_PROFILE_ID,
//                                      ZB_ZCL_DISABLE_DEFAULT_RESPONSE,
//                                      NULL,
//                                      DEFAULT_GROUP_ID);
// */
// }



/**@brief Callback for detecting button press duration.
 *
 * @param[in]   button   BSP Button that was pressed.
//  */
// static zb_void_t light_switch_button_handler(zb_uint8_t button)
// {
//     zb_time_t current_time;
//     zb_bool_t short_expired;
//     zb_bool_t on_off;
//     zb_ret_t zb_err_code;

//     current_time = ZB_TIMER_GET();

//     if (button == LIGHT_SWITCH_BUTTON_ON)
//     {
//         on_off = ZB_TRUE;
//     }
//     else
//     {
//         on_off = ZB_FALSE;
//     }

//     if (ZB_TIME_SUBTRACT(current_time, m_device_ctx.button.timestamp) > LIGHT_SWITCH_BUTTON_THRESHOLD)
//     {
//         short_expired = ZB_TRUE;
//     }
//     else
//     {
//         short_expired = ZB_FALSE;
//     }

//     /* Check if button was released during LIGHT_SWITCH_BUTTON_SHORT_POLL_TMO. */
//     if (!bsp_button_is_pressed(button))
//     {
//         if (!short_expired)
//         {
//             /* Allocate output buffer and send on/off command. */
//             //zb_err_code = ZB_GET_OUT_BUF_DELAYED2(light_switch_send_on_off, on_off);
//             light_switch_send_on_off( 0, on_off );
//             //ZB_ERROR_CHECK(zb_err_code);
//         }

//         /* Button released - wait for accept next event. */
//         m_device_ctx.button.in_progress = ZB_FALSE;
//     }
//     else
//     {
//         if (short_expired)
//         {
//             /* The button is still pressed - allocate output buffer and send step command. */
//             //zb_err_code = ZB_GET_OUT_BUF_DELAYED2(light_switch_send_step, on_off);
//             //ZB_ERROR_CHECK(zb_err_code);
//             zb_err_code = ZB_SCHEDULE_ALARM(light_switch_button_handler, button, LIGHT_SWITCH_BUTTON_LONG_POLL_TMO);
//             ZB_ERROR_CHECK(zb_err_code);
//         }
//         else
//         {
//             /* Wait another LIGHT_SWITCH_BUTTON_SHORT_POLL_TMO, until LIGHT_SWITCH_BUTTON_THRESHOLD will be reached. */
//             zb_err_code = ZB_SCHEDULE_ALARM(light_switch_button_handler, button, LIGHT_SWITCH_BUTTON_SHORT_POLL_TMO);
//             ZB_ERROR_CHECK(zb_err_code);
//         }
//     }
// }

void switchButtonEventCb( zb_uint8_t param ){
    NRF_LOG_INFO( "Button event command callback called" );
    ZB_FREE_BUF_BY_REF( param );
    m_device_ctx.button.bufferInUse = ZB_FALSE;
    if( m_device_ctx.button.mayClear ) {
        m_device_ctx.button.in_progress = ZB_FALSE;
        NRF_LOG_INFO( "Button event complete" );
    }
}


// static void sendHueButtonUpdateCommand( zb_uint64_t buttonEventData ){
//     NRF_LOG_INFO( "Send button data" );
//     zb_buf_t * buttonEventBuffer;
//     zb_uint8_t frameCtrl;
//     zb_uint8_t * cmd_ptr;

//     NRF_LOG_INFO( "Get buffer" );
//     // Get a free buffer
//     buttonEventBuffer = ZB_GET_OUT_BUF();

//     NRF_LOG_INFO( "Start packet" );
//    // cmd_ptr = ZB_ZCL_START_PACKET( buttonEventBuffer );

//     frameCtrl = ZB_ZCL_CONSTRUCT_FRAME_CONTROL( 
//         ZB_ZCL_FRAME_TYPE_CLUSTER_SPECIFIC,
//         ZB_ZCL_MANUFACTURER_SPECIFIC,
//         ZB_ZCL_FRAME_DIRECTION_TO_CLI,
//         1 );


//     //  TODO  use ZB_GET_OUT_BUF_DELAYED() since its non-blocking
//     NRF_LOG_INFO( "Start command header" );
//     cmd_ptr = (zb_uint8_t*)zb_zcl_start_command_header( buttonEventBuffer,
//                                  frameCtrl,
//                                  ZB_PHILIPS_MANUF_CODE,
//                                  PHILIPS_BUTTON_EVENT_CMD_CODE,
//                                  NULL );

//     NRF_LOG_INFO( "Add data" );
//     // Does a memcpy so should be fine
//     ZB_ZCL_PACKET_PUT_DATA64( cmd_ptr, &buttonEventData );

//     NRF_LOG_INFO( "Send packet" );

//     zb_uint16_t addr = 0x0001;
//     ZB_ZCL_FINISH_PACKET( buttonEventBuffer, cmd_ptr )
//     ZB_ZCL_SEND_COMMAND_SHORT(
//       buttonEventBuffer, addr, 
//       (ZB_APS_ADDR_MODE_16_ENDP_PRESENT), (PHILIPS_BRIDGE_ZHA_ENDPOINT), 
//       (LIGHT_SWITCH_ZHA_ENDPOINT), (ZB_AF_HA_PROFILE_ID), 
//       ZB_ZCL_CLUSTER_ID_TUNNEL, ( zb_callback_t ) switchButtonEventCb );

//     NRF_LOG_INFO( "Free the buffer" );
//    // ZB_FREE_BUF( buttonEventBuffer );
//     NRF_LOG_INFO( "Finished sending command" );

// }


/* Need to encode button information to fit into 16-bit CB parameter
    4 buttons -> 2 bits required.
    4 transition types -> 2 bits required
    Counter -> 8 bits required
    Encoding masks are therefore:
        0xF000 -> Button ID
        0x0F00 -> Transition type
        0x00FF -> Counter value
 */
#define ENCODE_BUTTON_INFO( buttonId, transitionType, counter ) \
    (zb_uint16_t) (                                             \
        ((zb_uint16_t)buttonId & 0x0003)       << 12 |          \
        ((zb_uint16_t)transitionType & 0x0003) << 8  |          \
        ((zb_uint16_t)counter & 0x00FF)                         \
    )
#define DECODE_BUTTON_INFO_ID( buttonData ) (zb_uint8_t) ( ( buttonData & 0xF000 ) >> 12 )
#define DECODE_BUTTON_INFO_TRANSITION_TYPE( buttonData ) (zb_uint8_t) ( ( buttonData & 0x0F00 ) >> 8 )
#define DECODE_BUTTON_INFO_COUNTER( buttonData ) (zb_uint8_t) ( buttonData & 0x00FF )

static zb_uint64_t generateButtonEventData( zb_uint8_t buttonId, zb_uint8_t buttonState, zb_uint8_t eventTime ){
    zb_uint64_t retVal;
    // retVal = ( ( (zb_uint64_t) 0x30 ) << 32 ) | ( ( (zb_uint64_t) 0x21 ) << 16 );
    // retVal |= ( ( (zb_uint64_t) buttonId ) << 56 ) | ( ( (zb_uint64_t) buttonState ) << 24 ) | ( ( (zb_uint64_t) eventTime ) << 8 );

    retVal = ( ( (zb_uint64_t) 0x30 ) << 24 ) | ( ( (zb_uint64_t) 0x21 ) << 40 );
    retVal |= ( ( (zb_uint64_t) buttonId ) << 0 ) | ( ( (zb_uint64_t) buttonState ) << 32 ) | ( ( (zb_uint64_t) eventTime ) << 48 );
    
    return retVal;
}


/**@brief Function for sending ON/OFF requests to the light bulb.
 *
 * @param[in]   param    Non-zero reference to ZigBee stack buffer that will be used to construct on/off request.
 * @param[in]   on_off   Requested state of the light bulb.
 */
static zb_void_t sendHueButtonUpdateCommand( zb_uint8_t param, zb_uint16_t buttonInfo ){
    NRF_LOG_INFO( "Send button data" );
    zb_buf_t * buttonEventBuffer;
    zb_uint8_t frameCtrl;
    zb_uint8_t * cmd_ptr;

    // Decode the button info
    zb_uint8_t buttonId = DECODE_BUTTON_INFO_ID( buttonInfo );
    zb_uint8_t buttonTransitionState = DECODE_BUTTON_INFO_TRANSITION_TYPE( buttonInfo );
    zb_uint8_t buttonTime = DECODE_BUTTON_INFO_COUNTER( buttonInfo );

    // Get command data from button info - TODO maybe reduce the number of functions n stuff here to reduce param passing
    zb_uint64_t commandData = generateButtonEventData( buttonId + 1, buttonTransitionState, buttonTime );

    NRF_LOG_INFO( "Get buffer" );
    // Get a free buffer
    buttonEventBuffer = ZB_BUF_FROM_REF( param );

    NRF_LOG_INFO( "Start packet" );

    frameCtrl = ZB_ZCL_CONSTRUCT_FRAME_CONTROL( 
        ZB_ZCL_FRAME_TYPE_CLUSTER_SPECIFIC,
        ZB_ZCL_MANUFACTURER_SPECIFIC,
        ZB_ZCL_FRAME_DIRECTION_TO_CLI,
        1 );

    NRF_LOG_INFO( "Start command header" );
    cmd_ptr = (zb_uint8_t*)zb_zcl_start_command_header( buttonEventBuffer,
                                 frameCtrl,
                                 ZB_PHILIPS_MANUF_CODE,
                                 PHILIPS_BUTTON_EVENT_CMD_CODE,
                                 NULL );

    NRF_LOG_INFO( "Add data" );
    // Does a memcpy so should be fine
    ZB_ZCL_PACKET_PUT_DATA64( cmd_ptr, &commandData );

    NRF_LOG_INFO( "Send packet" );

    zb_uint16_t addr = 0x0001;
    ZB_ZCL_FINISH_PACKET( buttonEventBuffer, cmd_ptr )
    ZB_ZCL_SEND_COMMAND_SHORT(
      buttonEventBuffer, addr, 
      (ZB_APS_ADDR_MODE_16_ENDP_PRESENT), (PHILIPS_BRIDGE_ZHA_ENDPOINT), 
      (LIGHT_SWITCH_ZHA_ENDPOINT), (ZB_AF_HA_PROFILE_ID), 
      ZB_ZCL_CLUSTER_ID_TUNNEL, ( zb_callback_t ) switchButtonEventCb );


    NRF_LOG_INFO( "Finished sending command" );

}


zb_void_t buttonHoldCallback( zb_uint8_t buttonId ){
    NRF_LOG_INFO( "Button-hold interval callback" );
    // Send command and schedule another alarm if we're not meant to be finishing up
    if( !m_device_ctx.button.mayClear && m_device_ctx.button.in_progress ){
        zb_ret_t zb_err_code;

        zb_time_t eventTimeBeaconInterval = ZB_TIME_SUBTRACT( ZB_TIMER_GET(), m_device_ctx.button.timestamp );
        zb_uint16_t eventTimeMs = ZB_TIME_BEACON_INTERVAL_TO_MSEC( eventTimeBeaconInterval );
        zb_uint8_t buttonTime = (zb_uint8_t) ( eventTimeMs / 100 );
        zb_uint8_t buttonTransitionState = 0x01;
        // TODO - ensure we don't try grab too many buffers here

        // Encode the button info for the 16-bit callback parameter
        zb_uint16_t buttonInfoEnc = ENCODE_BUTTON_INFO( buttonId, buttonTransitionState, buttonTime );

        if( m_device_ctx.button.bufferInUse ){
            NRF_LOG_INFO( "Could not send button-hold update as buffer is in use" );
        }else{
            zb_err_code = ZB_GET_OUT_BUF_DELAYED2( sendHueButtonUpdateCommand, buttonInfoEnc );
            ZB_ERROR_CHECK(zb_err_code);
        }

        zb_err_code = ZB_SCHEDULE_ALARM( buttonHoldCallback, buttonId, ZB_MILLISECONDS_TO_BEACON_INTERVAL( 800 ) );
        ZB_ERROR_CHECK( zb_err_code );
    }
}


/**@brief Callback for button events.
 *
 * @param[in]   evt      Incoming event from the BSP subsystem.
 */
static void buttons_handler(bsp_event_t evt)
{
    //zb_ret_t zb_err_code;
    zb_uint32_t button;
    zb_uint8_t buttonId;
    zb_uint8_t buttonPress;
    zb_uint64_t commandData;
    zb_ret_t zb_err_code;

    UNUSED_VARIABLE( button );
    UNUSED_VARIABLE( buttonId );
    UNUSED_VARIABLE( commandData );

    if( !m_device_ctx.nwk_joined ){
        NRF_LOG_INFO( "Device not connected so not sending command" );
        return;
    }

    switch(evt)
    {
        case BSP_EVENT_KEY_0:
            button = LIGHT_SWITCH_BUTTON_ON;
            buttonId = 0;
            buttonPress = 1;
            NRF_LOG_INFO( "ON button pressed" );
            break;
        case BSP_EVENT_KEY_1:
            button = LIGHT_SWITCH_BUTTON_ON;
            buttonId = 0;
            buttonPress = 0;
            NRF_LOG_INFO( "ON button released" );
            break;

        case BSP_EVENT_KEY_2:
            button = LIGHT_SWITCH_BUTTON_OFF;
            buttonId = 1;
            buttonPress = 1;
            NRF_LOG_INFO( "OFF button pressed" );
            break;
        case BSP_EVENT_KEY_3:
            button = LIGHT_SWITCH_BUTTON_OFF;
            buttonId = 1;
            buttonPress = 0;
            NRF_LOG_INFO( "OFF button released" );
            break;

        case BSP_EVENT_KEY_4:
            button = LIGHT_LEVEL_BUTTON_UP;
            buttonId = 2;
            buttonPress = 1;
            NRF_LOG_INFO( "LVL UP button pressed" );
            break;
        case BSP_EVENT_KEY_5:
            button = LIGHT_LEVEL_BUTTON_UP;
            buttonId = 2;
            buttonPress = 0;
            NRF_LOG_INFO( "LVL UP button released" );
            break;

        case BSP_EVENT_KEY_6:
            button = LIGHT_LEVEL_BUTTON_DOWN;
            buttonId = 3;
            buttonPress = 1;
            NRF_LOG_INFO( "LVL DOWN button pressed" );
            break;
        case BSP_EVENT_KEY_7:
            button = LIGHT_LEVEL_BUTTON_DOWN;
            buttonId = 3;
            buttonPress = 0;
            NRF_LOG_INFO( "LVL DOWN button released" );
            break;

        default:
            NRF_LOG_INFO("Unhandled BSP Event received: %d", evt);
            return;
    }
    zb_uint8_t buttonTransitionState;
    zb_uint8_t buttonTime;
    if( !m_device_ctx.button.in_progress && buttonPress == 1 ){
        m_device_ctx.button.in_progress = ZB_TRUE;
        m_device_ctx.button.progressButtonId = buttonId;
        m_device_ctx.button.timestamp = ZB_TIMER_GET();
        m_device_ctx.button.longHold = ZB_FALSE;
        buttonTransitionState = 0x00;
        buttonTime = 0x00;
        // Start blip-blip timer (800ms interval)
        zb_err_code = ZB_SCHEDULE_ALARM( buttonHoldCallback, button, ZB_MILLISECONDS_TO_BEACON_INTERVAL( 800 ) );
        ZB_ERROR_CHECK( zb_err_code );
        
    }
    else if( m_device_ctx.button.in_progress && buttonPress == 0 && m_device_ctx.button.progressButtonId == buttonId ){
        m_device_ctx.button.mayClear = ZB_TRUE;
        // Stop blip-blip timer
        zb_err_code = ZB_SCHEDULE_ALARM_CANCEL( buttonHoldCallback, button );
        ZB_ERROR_CHECK(zb_err_code);

        buttonTransitionState = m_device_ctx.button.longHold ? 0x03 : 0x02;

        zb_time_t eventTimeBeaconInterval = ZB_TIME_SUBTRACT( ZB_TIMER_GET(), m_device_ctx.button.timestamp );
        zb_uint16_t eventTimeMs = ZB_TIME_BEACON_INTERVAL_TO_MSEC( eventTimeBeaconInterval );
        buttonTime = m_device_ctx.button.longHold ? (zb_uint8_t) ( eventTimeMs / 100 ) : 0x01; 
        
    }
    else if( !m_device_ctx.button.in_progress && buttonPress == 0 ){
        // Button released when no command-event chain was in progress
        return;
    } else{
        return;
    }

    // Encode the button info for the 16-bit callback parameter
    zb_uint16_t buttonInfoEnc = ENCODE_BUTTON_INFO( buttonId, buttonTransitionState, buttonTime );

    zb_err_code = ZB_GET_OUT_BUF_DELAYED2( sendHueButtonUpdateCommand, buttonInfoEnc );
    ZB_ERROR_CHECK(zb_err_code);

}

/**@brief Function for initializing LEDs and buttons.
 */
static zb_void_t leds_buttons_init(void)
{
    ret_code_t error_code;

    /* Initialize LEDs and buttons - use BSP to control them. */
    error_code = bsp_init(BSP_INIT_LEDS | BSP_INIT_BUTTONS, buttons_handler);
    APP_ERROR_CHECK(error_code);
    /* By default the bsp_init attaches BSP_KEY_EVENTS_{0-4} to the PUSH events of the corresponding buttons. */
    bsp_event_to_button_action_assign( LIGHT_SWITCH_BUTTON_ON, BSP_BUTTON_ACTION_PUSH, BSP_EVENT_KEY_0 );
    bsp_event_to_button_action_assign( LIGHT_SWITCH_BUTTON_ON, BSP_BUTTON_ACTION_RELEASE, BSP_EVENT_KEY_1 );

    bsp_event_to_button_action_assign( LIGHT_SWITCH_BUTTON_OFF, BSP_BUTTON_ACTION_PUSH, BSP_EVENT_KEY_2 );
    bsp_event_to_button_action_assign( LIGHT_SWITCH_BUTTON_OFF, BSP_BUTTON_ACTION_RELEASE, BSP_EVENT_KEY_3 );

    bsp_event_to_button_action_assign( LIGHT_LEVEL_BUTTON_UP, BSP_BUTTON_ACTION_PUSH, BSP_EVENT_KEY_4 );
    bsp_event_to_button_action_assign( LIGHT_LEVEL_BUTTON_UP, BSP_BUTTON_ACTION_RELEASE, BSP_EVENT_KEY_5 );

    bsp_event_to_button_action_assign( LIGHT_LEVEL_BUTTON_DOWN, BSP_BUTTON_ACTION_PUSH, BSP_EVENT_KEY_6 );
    bsp_event_to_button_action_assign( LIGHT_LEVEL_BUTTON_DOWN, BSP_BUTTON_ACTION_RELEASE, BSP_EVENT_KEY_7 );

    bsp_board_leds_off();
}



/**@brief Function for handling the ADC interrupt.
 *
 * @details  This function will fetch the conversion result from the ADC, convert the value into
 *           percentage and send it to peer.
 */
void saadc_event_handler(nrf_drv_saadc_evt_t const * p_event)
{
    if (p_event->type == NRF_DRV_SAADC_EVT_DONE)
    {
        nrf_saadc_value_t adc_result;
        uint16_t          batt_lvl_in_milli_volts;
        uint32_t          err_code;
        zb_uint16_t       batt_lvl_pcnt;

        NRF_LOG_INFO( "ADC Event handler" );
        adc_result = p_event->data.done.p_buffer[0];

        err_code = nrf_drv_saadc_buffer_convert(p_event->data.done.p_buffer, 1);
        APP_ERROR_CHECK(err_code);

        batt_lvl_in_milli_volts = ADC_RESULT_IN_MILLI_VOLTS( adc_result ) + 
                                  DIODE_FWD_VOLT_DROP_MILLIVOLTS;
        NRF_LOG_INFO( "ADC: Battery at %dmV", batt_lvl_in_milli_volts );
        batt_lvl_pcnt = battery_level_in_percent( batt_lvl_in_milli_volts );
        m_device_ctx.zha_pwrconf_serv_attr.remaining = batt_lvl_pcnt * 2;

        NRF_LOG_INFO( "ADC: Battery at %dmV.", batt_lvl_in_milli_volts );
        //NRF_LOG_INFO( "ADC: Setting battery level to %d", m_device_ctx.zha_pwrconf_serv_attr.remaining );
        ZB_ZCL_SET_ATTRIBUTE( LIGHT_SWITCH_ZHA_ENDPOINT, 
            ZB_ZCL_CLUSTER_ID_POWER_CONFIG,    
            ZB_ZCL_CLUSTER_SERVER_ROLE,  
            ZB_ZCL_ATTR_POWER_CONFIG_BATTERY_REMAINING_ID,
            ( zb_uint8_t * )&m_device_ctx.zha_pwrconf_serv_attr.remaining,                       
            ZB_FALSE);

        
    }
}

/**@brief Function for configuring ADC to do battery level conversion.
 */
static void adc_configure(void)
{
    ret_code_t err_code = nrf_drv_saadc_init( NULL, saadc_event_handler );
    APP_ERROR_CHECK(err_code);

    nrf_saadc_channel_config_t config =
        NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_VDD);
    err_code = nrf_drv_saadc_channel_init(0, &config);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_saadc_buffer_convert(&adc_buf[0], 1);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_saadc_buffer_convert(&adc_buf[1], 1);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_saadc_sample();
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling the Battery measurement timer timeout.
 *
 * @details This function will be called each time the battery level measurement timer expires.
 *          This function will start the ADC.
 *
 * @param[in] p_context   Pointer used for passing some arbitrary information (context) from the
 *                        app_start_timer() call to the timeout handler.
 */
static void battery_level_meas_timeout_handler(void * p_context)
{
    UNUSED_PARAMETER(p_context);
    NRF_LOG_INFO( "ADC timer CB" );
    ret_code_t err_code;
    err_code = nrf_drv_saadc_sample();
    APP_ERROR_CHECK(err_code);
}

/**@brief ZigBee stack event handler.
 *
 * @param[in]   param   Reference to ZigBee stack buffer used to pass arguments (signal).
 */
void zboss_signal_handler(zb_uint8_t param)
{
    zb_zdo_app_signal_hdr_t      * p_sg_p         = NULL;
    zb_zdo_signal_leave_params_t * p_leave_params = NULL;
    zb_zdo_app_signal_type_t       sig            = zb_get_app_signal(param, &p_sg_p);
    zb_ret_t                       status         = ZB_GET_APP_SIGNAL_STATUS(param);
    zb_ret_t                       zb_err_code;

    switch(sig)
    {
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        case ZB_BDB_SIGNAL_DEVICE_REBOOT:
            if (status == RET_OK)
            {
                NRF_LOG_INFO("Joined network successfully");
                bsp_board_led_on(ZIGBEE_NETWORK_STATE_LED);
                m_device_ctx.nwk_joined = ZB_TRUE;
                app_timer_start(m_battery_timer_id, BATTERY_LEVEL_MEAS_INTERVAL, NULL);
               // zb_err_code = ZB_SCHEDULE_ALARM(find_light_bulb, param, MATCH_DESC_REQ_START_DELAY);
               // ZB_ERROR_CHECK(zb_err_code);
                param = 0; // Do not free buffer - it will be reused by find_light_bulb callback
            }
            else
            {
                NRF_LOG_ERROR("Failed to join network. Status: %d", status);
                bsp_board_led_off(ZIGBEE_NETWORK_STATE_LED);
                zb_err_code = ZB_SCHEDULE_ALARM(light_switch_leave_and_join, 0, ZB_TIME_ONE_SECOND);
                ZB_ERROR_CHECK(zb_err_code);
            }
            break;

        case ZB_ZDO_SIGNAL_LEAVE:
            if (status == RET_OK)
            {
                bsp_board_led_off(ZIGBEE_NETWORK_STATE_LED);
                p_leave_params = ZB_ZDO_SIGNAL_GET_PARAMS(p_sg_p, zb_zdo_signal_leave_params_t);
                NRF_LOG_INFO("Network left. Leave type: %d", p_leave_params->leave_type);
                light_switch_retry_join(p_leave_params->leave_type);
                m_device_ctx.nwk_joined = ZB_FALSE;
            }
            else
            {
                NRF_LOG_ERROR("Unable to leave network. Status: %d", status);
            }
            break;

        case ZB_COMMON_SIGNAL_CAN_SLEEP:
            {
                zb_zdo_signal_can_sleep_params_t *can_sleep_params = ZB_ZDO_SIGNAL_GET_PARAMS(p_sg_p, zb_zdo_signal_can_sleep_params_t);
                NRF_LOG_INFO("Can sleep for %ld ms", can_sleep_params->sleep_tmo);
                zb_sleep_now();
            }
            break;

        case ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY:
            if (status != RET_OK)
            {
                NRF_LOG_WARNING("Production config is not present or invalid");
            }
            break;

        default:
            /* Unhandled signal. For more information see: zb_zdo_app_signal_type_e and zb_ret_e */
            NRF_LOG_INFO("Unhandled signal %d. Status: %d", sig, status);
    }

    if (param)
    {
        ZB_FREE_BUF_BY_REF(param);
    }
}


void setBasicAttrs( zb_zcl_basic_attrs_ext_hue_t * basicAttrList ){
    /* Basic cluster attributes data */
    basicAttrList->zcl_version   = ZB_ZCL_VERSION;
    basicAttrList->app_version   = BULB_INIT_BASIC_APP_VERSION;
    basicAttrList->stack_version = BULB_INIT_BASIC_STACK_VERSION;
    basicAttrList->hw_version    = BULB_INIT_BASIC_HW_VERSION;

    /* Use ZB_ZCL_SET_STRING_VAL to set strings, because the first byte should
    * contain string length without trailing zero.
    *
    * For example "test" string wil be encoded as:
    *   [(0x4), 't', 'e', 's', 't']
    */
    ZB_ZCL_SET_STRING_VAL(basicAttrList->mf_name,
                        BULB_INIT_BASIC_MANUF_NAME,
                        ZB_ZCL_STRING_CONST_SIZE(BULB_INIT_BASIC_MANUF_NAME));

    ZB_ZCL_SET_STRING_VAL(basicAttrList->model_id,
                        BULB_INIT_BASIC_MODEL_ID,
                        ZB_ZCL_STRING_CONST_SIZE(BULB_INIT_BASIC_MODEL_ID));

    ZB_ZCL_SET_STRING_VAL(basicAttrList->date_code,
                        BULB_INIT_BASIC_DATE_CODE,
                        ZB_ZCL_STRING_CONST_SIZE(BULB_INIT_BASIC_DATE_CODE));

    basicAttrList->power_source = BULB_INIT_BASIC_POWER_SOURCE;

    ZB_ZCL_SET_STRING_VAL(basicAttrList->location_id,
                        BULB_INIT_BASIC_LOCATION_DESC,
                        ZB_ZCL_STRING_CONST_SIZE(BULB_INIT_BASIC_LOCATION_DESC));

    ZB_ZCL_SET_STRING_VAL( basicAttrList->sw_ver,
                         BULB_INIT_BASIC_SW_VERSION,
                         ZB_ZCL_STRING_CONST_SIZE( BULB_INIT_BASIC_SW_VERSION ) );

    basicAttrList->ph_env = BULB_INIT_BASIC_PH_ENV; 
} 

/**@brief Function for initializing all clusters attributes.
 */
static void bulb_clusters_attr_init(void)
{
    /* Set basic Attr list data */
    setBasicAttrs( &m_device_ctx.zll_basic_client_attr );
    setBasicAttrs( &m_device_ctx.zll_basic_serv_attr );
    setBasicAttrs( &m_device_ctx.zha_basic_serv_attr );

    /* Identify cluster attributes data */
    m_device_ctx.zll_identify_attr.identify_time = ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;
    m_device_ctx.zha_identify_serv_attr.identify_time = ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;

    ZB_ZCL_SET_ATTRIBUTE( LIGHT_SWITCH_ZHA_ENDPOINT, 
                          ZB_ZCL_CLUSTER_ID_BINARY_INPUT,    
                          ZB_ZCL_CLUSTER_SERVER_ROLE,  
                          ZB_ZCL_ATTR_BINARY_INPUT_PRESENT_VALUE_ID,
                          (zb_uint8_t *)&m_device_ctx.zha_binary_input_serv_attr.present_value,                        
                          ZB_FALSE);                   

    ZB_ZCL_SET_ATTRIBUTE( LIGHT_SWITCH_ZHA_ENDPOINT,                                       
                          ZB_ZCL_CLUSTER_ID_BINARY_INPUT,            
                          ZB_ZCL_CLUSTER_SERVER_ROLE,                 
                          ZB_ZCL_ATTR_BINARY_INPUT_STATUS_FLAG_ID, 
                          (zb_uint8_t *)&m_device_ctx.zha_binary_input_serv_attr.status_flag,                                       
                          ZB_FALSE);

    m_device_ctx.zha_basic_serv_attr.philips_device_flag = 1;

    m_device_ctx.zha_tunnelling_serv_attr.philips_type = 0x0001;
    ZB_ZCL_SET_ATTRIBUTE( LIGHT_SWITCH_ZHA_ENDPOINT, 
                          ZB_ZCL_CLUSTER_ID_BASIC,    
                          ZB_ZCL_CLUSTER_SERVER_ROLE,  
                          ZB_ZCL_ATTR_BASIC_PHILIPS_DEVICE_FLAG_ID,
                          ( zb_uint8_t * )&m_device_ctx.zha_basic_serv_attr.philips_device_flag,                        
                          ZB_FALSE);

    ZB_ZCL_SET_ATTRIBUTE( LIGHT_SWITCH_ZHA_ENDPOINT, 
                          ZB_ZCL_CLUSTER_ID_TUNNEL,    
                          ZB_ZCL_CLUSTER_SERVER_ROLE,  
                          ZB_ZCL_ATTR_TUNNELING_PHILIPS_TYPE_ID,
                          ( zb_uint8_t * )&m_device_ctx.zha_tunnelling_serv_attr.philips_type,                       
                          ZB_FALSE);

    /* OTA cluster attributes data */
    zb_ieee_addr_t addr = ZB_ZCL_OTA_UPGRADE_SERVER_DEF_VALUE;
    ZB_MEMCPY( m_device_ctx.zha_otau_attr.upgrade_server, addr, sizeof( zb_ieee_addr_t ) );
    m_device_ctx.zha_otau_attr.file_offset = ZB_ZCL_OTA_UPGRADE_FILE_OFFSET_DEF_VALUE;
    m_device_ctx.zha_otau_attr.file_version = 0x420045b6;
    m_device_ctx.zha_otau_attr.stack_version = ZB_ZCL_OTA_UPGRADE_FILE_HEADER_STACK_PRO;
    m_device_ctx.zha_otau_attr.downloaded_file_ver  = ZB_ZCL_OTA_UPGRADE_DOWNLOADED_FILE_VERSION_DEF_VALUE;
    m_device_ctx.zha_otau_attr.downloaded_stack_ver = ZB_ZCL_OTA_UPGRADE_DOWNLOADED_STACK_DEF_VALUE;
    m_device_ctx.zha_otau_attr.image_status = ZB_ZCL_OTA_UPGRADE_IMAGE_STATUS_DEF_VALUE;
    m_device_ctx.zha_otau_attr.manufacturer = ZB_PHILIPS_MANUF_CODE;
    m_device_ctx.zha_otau_attr.image_type = 0x0000;
    m_device_ctx.zha_otau_attr.min_block_reque = 0;
    m_device_ctx.zha_otau_attr.image_stamp = ZB_ZCL_OTA_UPGRADE_IMAGE_STAMP_MIN_VALUE;

/*

    ZB_ZCL_SET_ATTRIBUTE( LIGHT_SWITCH_ZHA_ENDPOINT, 
                          ZB_ZCL_CLUSTER_ID_BASIC,    
                          ZB_ZCL_CLUSTER_SERVER_ROLE,  
                          ZB_ZCL_ATTR_BASIC_ZCL_VERSION_ID,
                          ( zb_uint8_t * )&m_attr_zcl_version,                        
                          ZB_FALSE);
    ZB_ZCL_SET_ATTRIBUTE( LIGHT_SWITCH_ZHA_ENDPOINT, 
                          ZB_ZCL_CLUSTER_ID_BASIC,    
                          ZB_ZCL_CLUSTER_SERVER_ROLE,  
                          ZB_ZCL_ATTR_BASIC_APPLICATION_VERSION_ID,
                          ( zb_uint8_t * )&m_attr_app_version,                        
                          ZB_FALSE);
    ZB_ZCL_SET_ATTRIBUTE( LIGHT_SWITCH_ZHA_ENDPOINT, 
                          ZB_ZCL_CLUSTER_ID_BASIC,    
                          ZB_ZCL_CLUSTER_SERVER_ROLE,  
                          ZB_ZCL_ATTR_BASIC_STACK_VERSION_ID,
                          ( zb_uint8_t * )&m_attr_stack_version,                        
                          ZB_FALSE);
    ZB_ZCL_SET_ATTRIBUTE( LIGHT_SWITCH_ZHA_ENDPOINT, 
                          ZB_ZCL_CLUSTER_ID_BASIC,    
                          ZB_ZCL_CLUSTER_SERVER_ROLE,  
                          ZB_ZCL_ATTR_BASIC_HW_VERSION_ID,
                          ( zb_uint8_t * )&m_attr_hw_version,                       
                          ZB_FALSE);
    ZB_ZCL_SET_ATTRIBUTE( LIGHT_SWITCH_ZHA_ENDPOINT, 
                          ZB_ZCL_CLUSTER_ID_BASIC,    
                          ZB_ZCL_CLUSTER_SERVER_ROLE,  
                          ZB_ZCL_ATTR_BASIC_MANUFACTURER_NAME_ID,
                          ( zb_uint8_t * ) m_attr_mf_name,                        
                          ZB_FALSE);
    ZB_ZCL_SET_ATTRIBUTE( LIGHT_SWITCH_ZHA_ENDPOINT, 
                          ZB_ZCL_CLUSTER_ID_BASIC,    
                          ZB_ZCL_CLUSTER_SERVER_ROLE,  
                          ZB_ZCL_ATTR_BASIC_MODEL_IDENTIFIER_ID,
                          ( zb_uint8_t * )m_attr_model_id,                        
                          ZB_FALSE);
    ZB_ZCL_SET_ATTRIBUTE( LIGHT_SWITCH_ZHA_ENDPOINT, 
                          ZB_ZCL_CLUSTER_ID_BASIC,    
                          ZB_ZCL_CLUSTER_SERVER_ROLE,  
                          ZB_ZCL_ATTR_BASIC_DATE_CODE_ID,
                          ( zb_uint8_t * )m_attr_date_code,                        
                          ZB_FALSE);
    ZB_ZCL_SET_ATTRIBUTE( LIGHT_SWITCH_ZHA_ENDPOINT, 
                          ZB_ZCL_CLUSTER_ID_BASIC,    
                          ZB_ZCL_CLUSTER_SERVER_ROLE,  
                          ZB_ZCL_ATTR_BASIC_POWER_SOURCE_ID,
                          ( zb_uint8_t * )&m_attr_power_source,                        
                          ZB_FALSE);
    ZB_ZCL_SET_ATTRIBUTE( LIGHT_SWITCH_ZHA_ENDPOINT, 
                          ZB_ZCL_CLUSTER_ID_BASIC,    
                          ZB_ZCL_CLUSTER_SERVER_ROLE,  
                          ZB_ZCL_ATTR_BASIC_LOCATION_DESCRIPTION_ID,
                          ( zb_uint8_t * )m_attr_location_id,                        
                          ZB_FALSE);
    ZB_ZCL_SET_ATTRIBUTE( LIGHT_SWITCH_ZHA_ENDPOINT, 
                          ZB_ZCL_CLUSTER_ID_BASIC,    
                          ZB_ZCL_CLUSTER_SERVER_ROLE,  
                          ZB_ZCL_ATTR_BASIC_PHYSICAL_ENVIRONMENT_ID,
                          ( zb_uint8_t * )&m_attr_ph_env,                        
                          ZB_FALSE);
    ZB_ZCL_SET_ATTRIBUTE( LIGHT_SWITCH_ZHA_ENDPOINT, 
                          ZB_ZCL_CLUSTER_ID_BASIC,    
                          ZB_ZCL_CLUSTER_SERVER_ROLE,  
                          ZB_ZCL_ATTR_BASIC_SW_BUILD_ID,
                          ( zb_uint8_t * )m_attr_sw_ver,                        
                          ZB_FALSE);
*/

              
}

// void zb_zcl_attr_req_resp_cb(zb_uint8_t param){
//     NRF_LOG_INFO( "Response callback called" );
// }

zb_uint8_t zb_zcl_handler_cb( zb_uint8_t param ){
    return ZB_FALSE;
    /*
    zb_buf_t *zcl_cmd_buf = (zb_buf_t *)ZB_BUF_FROM_REF(param);
    zb_zcl_parsed_hdr_t *cmd_info = ZB_GET_BUF_PARAM(zcl_cmd_buf, zb_zcl_parsed_hdr_t);
   // bool toServ = cmd_info->cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_SRV;
   // bool isGeneral = cmd_info->is_common_command == 1;
    zb_uint8_t isManufSpecific = cmd_info->is_manuf_specific;
    zb_uint16_t manufSpec = cmd_info->manuf_specific;
    zb_uint16_t clusterId = cmd_info->cluster_id;
    zb_uint8_t cmdId = cmd_info->cmd_id;
    size_t                payload_length     = ZB_BUF_LEN( zcl_cmd_buf );
    const zb_uint8_t    * p_payload          = ZB_BUF_BEGIN( zcl_cmd_buf );
    zb_uint16_t bridgeAddr = ZB_ZCL_PARSED_HDR_SHORT_DATA( cmd_info ).source.u.short_addr;
    zb_uint8_t bridgeEp = ZB_ZCL_PARSED_HDR_SHORT_DATA( cmd_info ).src_endpoint;

    if( cmdId != ZB_ZCL_CMD_READ_ATTRIB || !( clusterId == ZB_ZCL_CLUSTER_ID_BASIC || clusterId == ZB_ZCL_CLUSTER_ID_TUNNEL ) ){
        return ZB_FALSE;
    }

    if( payload_length % 2 != 0 ){
        NRF_LOG_INFO( "Invalid number of destination attributes! Got %d, should be a multiple of 2", payload_length );
        return ZB_FALSE;
    }

    uint8_t numAttrs = payload_length / 2;
    if( numAttrs > 5 ){
        NRF_LOG_INFO( "Too many attributes requested!" );
        return ZB_FALSE;
    }
    

    zb_uint8_t* cmd_ptr;
    zb_af_endpoint_desc_t* endpoint_desc = zb_af_get_endpoint_desc( LIGHT_SWITCH_ZHA_ENDPOINT );
    if( !endpoint_desc ){
        NRF_LOG_INFO( "Endpoint could not be retrieved" );
        return ZB_FALSE;
    }
    zb_zcl_cluster_desc_t* cluster_desc = get_cluster_desc( endpoint_desc, clusterId, ZB_ZCL_CLUSTER_SERVER_ROLE );
    if( !cluster_desc ){
        NRF_LOG_INFO( "Cluster could not be retrieved" );
        return ZB_FALSE;
    }
    zb_zcl_attr_t * attr_descs[ 5 ] = {};

    NRF_LOG_INFO( "ZCL Handler, so far so good. Trying to respond with %d attrs", numAttrs );

    if( clusterId == ZB_ZCL_CLUSTER_ID_BASIC ){
        for( int i = 0; i < numAttrs; i++ ){
            const zb_uint8_t * attrStart = &p_payload[ i * 2 ];
            zb_uint16_t highBits = attrStart[ 1 ];
            zb_uint16_t lowBits = attrStart[ 0 ];
            zb_uint16_t attr = highBits << 8 | lowBits;
            NRF_LOG_INFO( "Attr 0x%4x", attr );
            switch( attr ){
                case ZB_ZCL_ATTR_BASIC_SHH_SECRET_ID:
                case ZB_ZCL_ATTR_BASIC_USERTEST_ID:
                case ZB_ZCL_ATTR_BASIC_LED_INDICATION_ID:
                {
                    attr_descs[ i ] = zb_zcl_get_attr_desc( cluster_desc, attr );
                    if( !attr_descs[ i ] ){
                        NRF_LOG_INFO( "Attrib could not be retrieved" );
                        return ZB_FALSE;
                    }
                    NRF_LOG_INFO( "Attr retrieved" );
                    break;
                }
                default:
                    return ZB_FALSE;
            }
        }
    }else if( clusterId == ZB_ZCL_CLUSTER_ID_TUNNEL ){
        for( int i = 0; i < numAttrs; i++ ){
            const zb_uint8_t * attrStart = &p_payload[ i * 2 ];
            zb_uint16_t attr = attrStart[ 1 ] << 8 | attrStart[ 0 ];
            NRF_LOG_INFO( "Attr 0x%4x", attr );
            switch( attr ){
                case ZB_ZCL_ATTR_TUNNELING_SHH_SECRET:
                {
                    attr_descs[ i ] = zb_zcl_get_attr_desc( cluster_desc, attr );
                    if( !attr_descs[ i ] ){
                        NRF_LOG_INFO( "Attrib could not be retrieved" );
                        return ZB_FALSE;
                    }
                    break;
                }
                default:
                    return ZB_FALSE;
            }
        }
    }


    NRF_LOG_INFO( "Attrs retrieved, putting response together" );
    ZB_ZCL_GENERAL_INIT_READ_ATTR_RESP_EXT( zcl_cmd_buf, cmd_ptr, 
                                            ZB_ZCL_FRAME_DIRECTION_TO_CLI, 
                                            ZB_ZCL_GET_SEQ_NUM(), 
                                            isManufSpecific ? ZB_ZCL_MANUFACTURER_SPECIFIC :
                                                                ZB_ZCL_NOT_MANUFACTURER_SPECIFIC, 
                                            manufSpec );

    NRF_LOG_INFO( "Adding attrs to response" );
    for( int i = 0; i < numAttrs; i++ ){
        ZB_ZCL_GENERAL_ADD_READ_ATTR_RESP_pete( zcl_cmd_buf, cmd_ptr, attr_descs[ i ] );
    }

    NRF_LOG_INFO( "Sending response" );
    ZB_ZCL_GENERAL_SEND_READ_ATTR_RESP( zcl_cmd_buf, cmd_ptr,
                                        bridgeAddr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                                        bridgeEp, LIGHT_SWITCH_ZHA_ENDPOINT,
                                        ZB_AF_HA_PROFILE_ID, clusterId,
                                        (zb_callback_t) zb_zcl_attr_req_resp_cb );

    
    
    NRF_LOG_INFO( "Response sent, returning" );
    //zigbee_logger_eprxzcl_ep_handler_pete( param );

    return ZB_TRUE;
    */
}


/**@brief Function for application main entry.
 */
int main(void)
{
    zb_ret_t       zb_err_code;
    zb_ieee_addr_t ieee_addr;

    /* Initialize timers, loging system and GPIOs. */
    timers_init();
    log_init();
    leds_buttons_init();
 //   adc_configure();

    m_device_ctx.nwk_joined = ZB_FALSE;

    /* Set ZigBee stack logging level and traffic dump subsystem. */
    ZB_SET_TRACE_LEVEL( ZIGBEE_TRACE_LEVEL );
    ZB_SET_TRACE_MASK( ZIGBEE_TRACE_MASK );
    ZB_SET_TRAF_DUMP_OFF();

    /* Initialize ZigBee stack. */
    ZB_INIT("Hue Dimmer Switch (ZHA)");

    /* Set device address to the value read from FICR registers. */
    zb_osif_get_ieee_eui64(ieee_addr);
    zb_set_long_address(ieee_addr);

    zb_set_network_ed_role( IEEE_CHANNEL_MASK );
    zigbee_erase_persistent_storage(ERASE_PERSISTENT_CONFIG);

    zb_set_ed_timeout(ED_AGING_TIMEOUT_64MIN);
    zb_set_keepalive_timeout(ZB_MILLISECONDS_TO_BEACON_INTERVAL(3000));
    //sleepy_device_setup();
    zb_set_rx_on_when_idle( RX_ON_IDLE );
    zb_set_node_descriptor_manufacturer_code( ZB_PHILIPS_MANUF_CODE );

    /* Initialize application context structure. */
    UNUSED_RETURN_VALUE( ZB_MEMSET( &m_device_ctx, 0, sizeof( switch_ctx_t ) ) );

    /* Register callback for handling ZCL commands. */
    ZB_ZCL_REGISTER_DEVICE_CB( zcl_device_cb );
    
    /* Register dimmer switch device context (endpoints). */
    ZB_AF_REGISTER_DEVICE_CTX(&dimmer_switch_ctx);

    // Register zcl endpoint handlers to debug commands coming in
    ZB_AF_SET_ENDPOINT_HANDLER( LIGHT_SWITCH_ZHA_ENDPOINT, zb_zcl_handler_cb );
 //   ZB_AF_SET_ENDPOINT_HANDLER( LIGHT_SWITCH_ZLL_ENDPOINT, zb_zcl_handler_cb );
    
    bulb_clusters_attr_init();

    zb_uint8_t tc_key[] = { 0x81, 0x42, 0x86, 0x86, 0x5D, 0xC1, 0xC8, 0xB2, 0xC8, 0xCB, 0xC5, 0x2E, 0x5D, 0x65, 0xD1, 0xB8 };
    zb_zdo_set_tc_standard_distributed_key( tc_key );
    NRF_LOG_DEBUG( "TC Key Set" );



    adc_configure();
    /** Start Zigbee Stack. */
    zb_err_code = zboss_start();
    ZB_ERROR_CHECK(zb_err_code);

    uint8_t zllReg = ZB_AF_IS_EP_REGISTERED( LIGHT_SWITCH_ZLL_ENDPOINT );
    uint8_t zhaReg = ZB_AF_IS_EP_REGISTERED( LIGHT_SWITCH_ZHA_ENDPOINT );
    NRF_LOG_INFO( "ZLL Reg %d, ZHA Reg %d", zllReg, zhaReg );

    while(1)
    {
        zboss_main_loop_iteration();
        UNUSED_RETURN_VALUE(NRF_LOG_PROCESS());
    }
}


/**
 * @}
 */
