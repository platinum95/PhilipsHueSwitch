/* ZBOSS Zigbee 3.0
 *
 * Copyright (c) 2012-2018 DSR Corporation, Denver CO, USA.
 * http://www.dsr-zboss.com
 * http://www.dsr-corporation.com
 * All rights reserved.
 *
 *
 * Use in source and binary forms, redistribution in binary form only, with
 * or without modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 2. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 3. This software, with or without modification, must only be used with a Nordic
 *    Semiconductor ASA integrated circuit.
 *
 * 4. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
PURPOSE: Dimmer Switch device definition
*/

#ifndef ZB_HA_HUE_DIMMER_SWITCH_H
#define ZB_HA_HUE_DIMMER_SWITCH_H 1

//#if defined ZB_HA_DEFINE_DEVICE_DIMMER_SWITCH || defined DOXYGEN

/** @cond DOXYGEN_HA_SECTION */

/**
 *  @defgroup ZB_HA_DEFINE_DEVICE_DIMMER_SWITCH Dimmer Switch
 *  @ingroup ZB_HA_DEVICES
 *  @{
    @details
        - @ref ZB_ZCL_BASIC \n
        - @ref ZB_ZCL_IDENTIFY \n
        - @ref ZB_ZCL_SCENES \n
        - @ref ZB_ZCL_GROUPS \n
        - @ref ZB_ZCL_ON_OFF \n
        - @ref ZB_ZCL_LEVEL_CONTROL

*/

#define ZB_HA_HUE_ZLL_DEVICE_VER_DIMMER_SWITCH 0  /*!< Dimmer Switch device version */

/** @cond internals_doc */

#define ZB_HA_HUE_ZLL_DIMMER_SWITCH_IN_CLUSTER_NUM 1  /*!< Dimmer Switch IN (server) clusters number */
#define ZB_HA_HUE_ZLL_DIMMER_SWITCH_OUT_CLUSTER_NUM 6 /*!< Dimmer Switch OUT (client) clusters number */

/** Dimmer switch total (IN+OUT) cluster number */
#define ZB_HA_HUE_ZLL_DIMMER_SWITCH_CLUSTER_NUM                                             \
  (ZB_HA_HUE_ZLL_DIMMER_SWITCH_IN_CLUSTER_NUM + ZB_HA_HUE_ZLL_DIMMER_SWITCH_OUT_CLUSTER_NUM)

/*! Number of attribute for reporting on Dimmer Switch device */
#define ZB_HA_HUE_ZLL_DIMMER_SWITCH_REPORT_ATTR_COUNT         0


/* ***** */

#define ZB_HA_HUE_ZHA_DIMMER_SWITCH_IN_CLUSTER_NUM 5  /*!< Dimmer Switch IN (server) clusters number */
#define ZB_HA_HUE_ZHA_DIMMER_SWITCH_OUT_CLUSTER_NUM 1 /*!< Dimmer Switch OUT (client) clusters number */

/** Dimmer switch total (IN+OUT) cluster number */
#define ZB_HA_HUE_ZHA_DIMMER_SWITCH_CLUSTER_NUM                                             \
  (ZB_HA_HUE_ZHA_DIMMER_SWITCH_IN_CLUSTER_NUM + ZB_HA_HUE_ZHA_DIMMER_SWITCH_OUT_CLUSTER_NUM)

/*! Number of attribute for reporting on Dimmer Switch device */
#define ZB_HA_HUE_ZHA_DIMMER_SWITCH_REPORT_ATTR_COUNT \
    ( ZB_ZCL_BINARY_INPUT_REPORT_ATTR_COUNT + ZB_ZCL_POWER_CONFIG_BAT_PACK_2_REPORT_ATTR_COUNT )


/** @endcond */

/** @brief Declare cluster list for Dimmer Switch device
    @param cluster_list_name - cluster list variable name
    @param basic_attr_list - attribute list for Basic cluster
    @param identify_attr_list - attribute list for Identify cluster
 */
// Hue dimmerswitch ZLL profile (ep 1)
/*
basic (server)
basic
identify
groups
on/off
level control
scenes
 */
#define ZB_HA_HUE_ZLL_DECLARE_DIMMER_SWITCH_CLUSTER_LIST( \
    cluster_list_name,                                    \
    basic_serv_attr_list,                                 \
    basic_client_attr_list,                               \
    identify_attr_list,                                   \
    groups_attr_list,                                     \
    on_off_attr_list,                                     \
    level_control_attr_list,                              \
    scenes_attr_list )                                    \
zb_zcl_cluster_desc_t cluster_list_name[] =               \
{                                                         \
  ZB_ZCL_CLUSTER_DESC(                                    \
    ZB_ZCL_CLUSTER_ID_BASIC,                              \
    ZB_ZCL_ARRAY_SIZE(basic_serv_attr_list, zb_zcl_attr_t),    \
    (basic_serv_attr_list),                               \
    ZB_ZCL_CLUSTER_SERVER_ROLE,                           \
    ZB_ZCL_MANUF_CODE_INVALID                             \
  ),                                                      \
    ZB_ZCL_CLUSTER_DESC(                                  \
    ZB_ZCL_CLUSTER_ID_BASIC,                              \
    ZB_ZCL_ARRAY_SIZE(basic_client_attr_list, zb_zcl_attr_t),    \
    (basic_client_attr_list),                             \
    ZB_ZCL_CLUSTER_CLIENT_ROLE,                           \
    ZB_ZCL_MANUF_CODE_INVALID                             \
  ),                                                      \
  ZB_ZCL_CLUSTER_DESC(                                    \
    ZB_ZCL_CLUSTER_ID_IDENTIFY,                           \
    ZB_ZCL_ARRAY_SIZE(identify_attr_list, zb_zcl_attr_t), \
    (identify_attr_list),                                 \
    ZB_ZCL_CLUSTER_CLIENT_ROLE,                           \
    ZB_ZCL_MANUF_CODE_INVALID                             \
  ),                                                      \
  ZB_ZCL_CLUSTER_DESC(                                    \
    ZB_ZCL_CLUSTER_ID_GROUPS,                             \
    ZB_ZCL_ARRAY_SIZE(groups_attr_list, zb_zcl_attr_t),   \
    (groups_attr_list),                                   \
    ZB_ZCL_CLUSTER_CLIENT_ROLE,                           \
    ZB_ZCL_MANUF_CODE_INVALID                             \
  ),                                                      \
   ZB_ZCL_CLUSTER_DESC(                                   \
    ZB_ZCL_CLUSTER_ID_ON_OFF,                             \
    ZB_ZCL_ARRAY_SIZE(on_off_attr_list, zb_zcl_attr_t),   \
    (on_off_attr_list),                                   \
    ZB_ZCL_CLUSTER_CLIENT_ROLE,                           \
    ZB_ZCL_MANUF_CODE_INVALID                             \
  ),                                                      \
  ZB_ZCL_CLUSTER_DESC(                                    \
    ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL,                      \
    ZB_ZCL_ARRAY_SIZE(level_control_attr_list, zb_zcl_attr_t),    \
    (level_control_attr_list),                            \
    ZB_ZCL_CLUSTER_CLIENT_ROLE,                           \
    ZB_ZCL_MANUF_CODE_INVALID                             \
  ),                                                      \
  ZB_ZCL_CLUSTER_DESC(                                    \
    ZB_ZCL_CLUSTER_ID_SCENES,                             \
    ZB_ZCL_ARRAY_SIZE(scenes_attr_list, zb_zcl_attr_t),   \
    (scenes_attr_list),                                   \
    ZB_ZCL_CLUSTER_CLIENT_ROLE,                           \
    ZB_ZCL_MANUF_CODE_INVALID                             \
  )                                                       \
}


#define ZB_ZCL_CLUSTER_DESC2(cluster_id, attr_count, attr_desc_list, cluster_role_mask, manuf_code)         \
{                                                                                                          \
  (cluster_id),                                                                                            \
  (attr_count),                                                                                            \
  (attr_desc_list),                                                                                        \
  (cluster_role_mask),                                                                                     \
  (manuf_code),                                                                                            \
  NULL                                                                                                     \
}

// Hue dimmerswitch ZHA profile (ep 2)
/*
basic (server)
Power config (server)
identify (server)
Binary input (server)
FC00 (server)
OTAU 
 */
#define ZB_HA_HUE_ZHA_DECLARE_DIMMER_SWITCH_CLUSTER_LIST( \
    cluster_list_name,                                    \
    basic_attr_list,                                      \
    power_config_attr_list,                               \
    identify_attr_list,                                   \
    binary_input_attr_list,                               \
    fc00_attr_list,                                       \
    otau_attr_list )                                      \
zb_zcl_cluster_desc_t cluster_list_name[] =               \
{                                                         \
  ZB_ZCL_CLUSTER_DESC(                                    \
    ZB_ZCL_CLUSTER_ID_BASIC,                              \
    ZB_ZCL_ARRAY_SIZE(basic_attr_list, zb_zcl_attr_t),    \
    (basic_attr_list),                                    \
    ZB_ZCL_CLUSTER_SERVER_ROLE,                           \
    ZB_ZCL_MANUF_CODE_INVALID                             \
  ),                                                      \
    ZB_ZCL_CLUSTER_DESC(                                  \
    ZB_ZCL_CLUSTER_ID_POWER_CONFIG,                       \
    ZB_ZCL_ARRAY_SIZE(power_config_attr_list, zb_zcl_attr_t),\
    power_config_attr_list,                               \
    ZB_ZCL_CLUSTER_SERVER_ROLE,                           \
    ZB_ZCL_MANUF_CODE_INVALID                             \
  ),                                                      \
  ZB_ZCL_CLUSTER_DESC(                                    \
    ZB_ZCL_CLUSTER_ID_IDENTIFY,                           \
    ZB_ZCL_ARRAY_SIZE(identify_attr_list, zb_zcl_attr_t), \
    identify_attr_list,                                   \
    ZB_ZCL_CLUSTER_SERVER_ROLE,                           \
    ZB_ZCL_MANUF_CODE_INVALID                             \
  ),                                                      \
  ZB_ZCL_CLUSTER_DESC(                                    \
    ZB_ZCL_CLUSTER_ID_BINARY_INPUT,                       \
    ZB_ZCL_ARRAY_SIZE(binary_input_attr_list, zb_zcl_attr_t),\
    binary_input_attr_list,                               \
    ZB_ZCL_CLUSTER_SERVER_ROLE,                           \
    ZB_ZCL_MANUF_CODE_INVALID                             \
  ),                                                      \
   ZB_ZCL_CLUSTER_DESC2(                                  \
    ZB_ZCL_CLUSTER_ID_TUNNEL,                             \
    ZB_ZCL_ARRAY_SIZE(fc00_attr_list, zb_zcl_attr_t),     \
    fc00_attr_list,                                       \
    ZB_ZCL_CLUSTER_SERVER_ROLE,                           \
    ZB_ZCL_MANUF_CODE_INVALID                             \
  ),                                                      \
  ZB_ZCL_CLUSTER_DESC(                                    \
    ZB_ZCL_CLUSTER_ID_OTA_UPGRADE,                        \
    ZB_ZCL_ARRAY_SIZE(otau_attr_list, zb_zcl_attr_t),     \
    otau_attr_list,                                       \
    ZB_ZCL_CLUSTER_CLIENT_ROLE,                           \
    ZB_ZCL_MANUF_CODE_INVALID                             \
  )                                                       \
}


/** @cond internals_doc */
/** @brief Declare simple descriptor for Dimmer switch device
    @param ep_name - endpoint variable name
    @param ep_id - endpoint ID
    @param in_clust_num - number of supported input clusters
    @param out_clust_num - number of supported output clusters
*/
#define ZB_ZLL_NON_COLOR_SCENE_CONTROLLER_DEVICE_ID 0x0830
#define ZB_ZCL_HUE_ZLL_DECLARE_DIMMER_SWITCH_SIMPLE_DESC(                     \
  ep_name, ep_id, in_clust_num, out_clust_num)                                \
  ZB_DECLARE_SIMPLE_DESC(in_clust_num, out_clust_num);                        \
  ZB_AF_SIMPLE_DESC_TYPE(in_clust_num, out_clust_num) simple_desc_##ep_name = \
  {                                                                           \
    ep_id,                                                                    \
    ZB_AF_ZLL_PROFILE_ID,                                                     \
    ZB_ZLL_NON_COLOR_SCENE_CONTROLLER_DEVICE_ID,                              \
    ZB_HA_HUE_ZLL_DEVICE_VER_DIMMER_SWITCH,                                   \
    0,                                                                        \
    in_clust_num,                                                             \
    out_clust_num,                                                            \
    {                                                                         \
      ZB_ZCL_CLUSTER_ID_BASIC,                                                \
      ZB_ZCL_CLUSTER_ID_BASIC,                                                \
      ZB_ZCL_CLUSTER_ID_IDENTIFY,                                             \
      ZB_ZCL_CLUSTER_ID_GROUPS,                                               \
      ZB_ZCL_CLUSTER_ID_ON_OFF,                                               \
      ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL,                                        \
      ZB_ZCL_CLUSTER_ID_SCENES,                                               \
    }                                                                         \
  }


#define ZB_ZCL_HUE_ZHA_DECLARE_DIMMER_SWITCH_SIMPLE_DESC(                     \
  ep_name, ep_id, in_clust_num, out_clust_num)                                \
  ZB_DECLARE_SIMPLE_DESC(in_clust_num, out_clust_num);                        \
  ZB_AF_SIMPLE_DESC_TYPE(in_clust_num, out_clust_num) simple_desc_##ep_name = \
  {                                                                           \
    ep_id,                                                                    \
    ZB_AF_HA_PROFILE_ID,                                                      \
    ZB_HA_SIMPLE_SENSOR_DEVICE_ID,     /* 0x000C */                           \
    ZB_HA_HUE_ZLL_DEVICE_VER_DIMMER_SWITCH,                                   \
    0,                                                                        \
    in_clust_num,                                                             \
    out_clust_num,                                                            \
    {                                                                         \
      ZB_ZCL_CLUSTER_ID_BASIC,                                                \
      ZB_ZCL_CLUSTER_ID_POWER_CONFIG,                                         \
      ZB_ZCL_CLUSTER_ID_IDENTIFY,                                             \
      ZB_ZCL_CLUSTER_ID_BINARY_INPUT,                                         \
      ZB_ZCL_CLUSTER_ID_TUNNEL,                                               \
      ZB_ZCL_CLUSTER_ID_OTA_UPGRADE,                                          \
    }                                                                         \
  }

/** @endcond */

/** @brief Declare endpoint for Dimmer Switch device
    @param ep_name - endpoint variable name
    @param ep_id - endpoint ID
    @param cluster_list - endpoint cluster list
 */
#define ZB_HA_HUE_ZLL_DECLARE_DIMMER_SWITCH_EP( ep_name, ep_id, cluster_list )                  \
  ZB_ZCL_HUE_ZLL_DECLARE_DIMMER_SWITCH_SIMPLE_DESC(ep_name, ep_id,                              \
      ZB_HA_HUE_ZLL_DIMMER_SWITCH_IN_CLUSTER_NUM, ZB_HA_HUE_ZLL_DIMMER_SWITCH_OUT_CLUSTER_NUM); \
                                                                                                \
  ZB_AF_DECLARE_ENDPOINT_DESC(ep_name, ep_id, ZB_AF_ZLL_PROFILE_ID, 0, NULL,                    \
                          ZB_ZCL_ARRAY_SIZE(cluster_list, zb_zcl_cluster_desc_t), cluster_list, \
                          (zb_af_simple_desc_1_1_t*)&simple_desc_##ep_name,                     \
                          0, NULL, /* No reporting ctx */                                       \
                          0, NULL) /* No CVC ctx */

#define ZB_HA_HUE_ZHA_DECLARE_DIMMER_SWITCH_EP( ep_name, ep_id, cluster_list )                  \
  ZB_ZCL_HUE_ZHA_DECLARE_DIMMER_SWITCH_SIMPLE_DESC(ep_name, ep_id,                              \
      ZB_HA_HUE_ZHA_DIMMER_SWITCH_IN_CLUSTER_NUM, ZB_HA_HUE_ZHA_DIMMER_SWITCH_OUT_CLUSTER_NUM); \
  ZBOSS_DEVICE_DECLARE_REPORTING_CTX( reporting_info## device_ctx_name,                         \
                                      ZB_HA_HUE_ZHA_DIMMER_SWITCH_REPORT_ATTR_COUNT );          \
                                                                                                \
  ZB_AF_DECLARE_ENDPOINT_DESC(ep_name, ep_id, ZB_AF_HA_PROFILE_ID, 0, NULL,                     \
                          ZB_ZCL_ARRAY_SIZE(cluster_list, zb_zcl_cluster_desc_t), cluster_list, \
                          (zb_af_simple_desc_1_1_t*)&simple_desc_##ep_name,                     \
                          ZB_HA_HUE_ZHA_DIMMER_SWITCH_REPORT_ATTR_COUNT,                        \
                          reporting_info## device_ctx_name,                                     \
                          0, NULL) /* No CVC ctx */

/*!
  @brief Declare application's device context for Dimmer Switch device
  @param device_ctx - device context variable
  @param ep_name - endpoint variable name
*/

#define ZB_HA_HUE_DECLARE_DIMMER_SWITCH_CTX(device_ctx, ep_name_zll, ep_name_zha) \
  ZBOSS_DECLARE_DEVICE_CTX_2_EP( device_ctx, ep_name_zll, ep_name_zha )

/*! @} */

/** @endcond */ /* DOXYGEN_HA_SECTION */

//#endif /* ZB_HA_DEFINE_DEVICE_DIMMER_SWITCH */


//#include <zb_zcl_power_config.h>

typedef struct
{
    zb_uint16_t voltage;
    enum zb_zcl_power_config_battery_size_e size;
    zb_uint8_t quantity;
    zb_uint8_t rated_voltage;
    enum zb_zcl_power_config_battery_alarm_mask_e alarm_mask;
    zb_uint8_t voltage_min_threshold;
    zb_uint8_t remaining;
    zb_uint8_t threshold1;
    zb_uint8_t threshold2;
    zb_uint8_t threshold3;
    zb_uint8_t min_threshold;
    zb_uint8_t percent_threshold1;
    zb_uint8_t percent_threshold2;
    zb_uint8_t percent_threshold3;
    enum zb_zcl_power_config_battery_alarm_state_e alarm_state;

} zb_zcl_power_config_attrs_ext_t;

typedef struct
{
    zb_uint8_t out_of_service;
    zb_uint8_t present_value;
    enum zb_zcl_binary_input_status_flag_value_e status_flag;
} zb_zcl_binary_input_attrs_t;


/** @brief Basic cluster attributes according to ZCL Spec 3.2.2.2 */
typedef struct
{
    zb_uint8_t zcl_version;
    zb_uint8_t app_version;
    zb_uint8_t stack_version;
    zb_uint8_t hw_version;
    zb_char_t  mf_name[32];
    zb_char_t  model_id[32];
    zb_char_t  date_code[16];
    zb_uint8_t power_source;
    zb_char_t  location_id[15];
    zb_uint8_t ph_env;
    zb_char_t  sw_ver[17];
    zb_uint8_t philips_device_flag;
    zb_uint16_t philips_en_flag;
} zb_zcl_basic_attrs_ext_hue_t;

typedef struct
{
    zb_ieee_addr_t upgrade_server;
    zb_uint32_t    file_offset;
    zb_uint32_t    file_version;
    zb_uint16_t    stack_version;
    zb_uint32_t    downloaded_file_ver;
    zb_uint32_t    downloaded_stack_ver;
    zb_uint8_t     image_status;
    zb_uint16_t    manufacturer;
    zb_uint16_t    image_type;
    zb_uint16_t    min_block_reque;
    zb_uint16_t    image_stamp;
    zb_uint16_t    server_addr;
    zb_uint8_t     server_ep;
} ota_client_ota_upgrade_attr_t;

/* Binary input attr list */


#endif /* ZB_HA_DIMMER_SWITCH_H */
