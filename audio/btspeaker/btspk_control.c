/*
 * Copyright 2020, Cypress Semiconductor Corporation or a subsidiary of
 * Cypress Semiconductor Corporation. All Rights Reserved.
 *
 * This software, including source code, documentation and related
 * materials ("Software"), is owned by Cypress Semiconductor Corporation
 * or one of its subsidiaries ("Cypress") and is protected by and subject to
 * worldwide patent protection (United States and foreign),
 * United States copyright laws and international treaty provisions.
 * Therefore, you may use this Software only as provided in the license
 * agreement accompanying the software package from which you
 * obtained this Software ("EULA").
 * If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
 * non-transferable license to copy, modify, and compile the Software
 * source code solely for use in connection with Cypress's
 * integrated circuit products. Any reproduction, modification, translation,
 * compilation, or representation of this Software except as specified
 * above is prohibited without the express written permission of Cypress.
 *
 * Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
 * reserves the right to make changes to the Software without notice. Cypress
 * does not assume any liability arising out of the application or use of the
 * Software or any product or circuit described in the Software. Cypress does
 * not authorize its products for use in any products where a malfunction or
 * failure of the Cypress product may reasonably be expected to result in
 * significant property damage, injury or death ("High Risk Product"). By
 * including Cypress's product in a High Risk Product, the manufacturer
 * of such system or application assumes all risk of such use and in doing
 * so agrees to indemnify Cypress against all liability.
 */
/** @file
 *
 * BTSpeaker_pro Sample Application for the Audio Shield platform.
 *
 * The sample app demonstrates Bluetooth A2DP sink, HFP and AVRCP Controller (and Target for absolute volume control).
 *
 * Features demonstrated
 *  - A2DP Sink and AVRCP Controller (Target for absolute volume)
 *  - Handsfree Device
 *  - GATT
 *  - SDP and GATT descriptor/attribute configuration
 *  - This app is targeted for the Audio Shield platform
 *  - This App doesn't support HCI UART for logging, PUART is supported.
 *  - HCI Client Control is not supported.
 *
 * Setting up Connection
 * 1. press and hold the SW15 on BT board for at least 2 seconds.
 * 2. This will set device in discovery mode(A2DP,HFP and BLE) and LED will start blinking.
 * 3. Scan for 'btspeakerPro' device on the peer source device, and pair with the btspeakerpro.
 * 4. Once connected LED will stop blinking and turns on.
 * 5. If no connection is established within 30sec,LED will turn off and device will not be discoverable,repeat instructions from step 1 to start again.
 *
 * A2DP Play back
 * 1. Connect stereo speakers to J39 (4ohm impedance preferred )
 * 2. Start music play back from peer device, you should be able to hear music from speakers
 * 2. You can control play back and volume from peer device (Play, Pause, Stop) controls.
 *
 * AVRCP
 * 1. We can use buttons on CY9BTAUDIO_REV2 Audio Shield board for AVRCP control
 * 2. SW15 - play/Pause      - Click this button for play or pause the music play back.
 * 3. SW16 - Next/Forward    - Click this button to move to next track or long press the button to fast forward
 * 4. SW17 - Previous/Rewind - Click this button to move to previous track or long press the button to rewind
 * 5. SW18 - Volume Up       - Every click of this button will increase the play back volume
 * 6. SW19 - Volume down     - Every click of this button will decrease the play back volume
 *                            (There are 7 volume steps).
 * Hands-free
 * 1. Make a phone call to the peer device.
 * 2. In case of in-band ring mode is supported from peer device, you will hear the set ring tone
 * 3. In case of out-of-band ring tone, no tone will be heard on btspeaker.
 * 4. SW15  is used as multi-function button to accept,hang-up or reject call.
 * 5. Long press SW15 to reject the incoming call.
 * 6. click SW15 to accept the call or hang-up the active call.
 * 6. Every click of SW18(Volume Up) button will increase the volume
 * 7. Every click of SW19(Volume down) button will decrease the volume
 * 8. If the call is on hold click SW15 to hang-up the call.
 *
 * BLE
 *  - To connect Ble device: set btspeaker in discovery mode by long press of SW15 button
 *    search for 'btspeakerPro' device in peer side phone app (Ex:BLEScanner for Android and LightBlue for iOS) and connect.
 *  - From the peer side app you should be able to do GATT read/write of the elements listed.
 */
#include "btspk_control.h"
#include "btspk_control_le.h"
#include <bt_hs_spk_handsfree.h>
#include <hal/wiced_hal_puart.h>
#include "ofu_spp.h"
#include "wiced_bt_gatt.h"
#include "wiced_bt_ble.h"
#include <wiced_bt_ota_firmware_upgrade.h>
#include <wiced_bt_stack.h>
#include <wiced_bt_sdp.h>
#include "wiced_app_cfg.h"
#include "wiced_memory.h"
#include "wiced_bt_sdp.h"
#include "wiced_bt_dev.h"
#include "bt_hs_spk_control.h"
#include "wiced_platform.h"
#include "wiced_transport.h"
#include "wiced_app.h"
#include "wiced_bt_a2dp_sink.h"
#include "platform_led.h"
#include "wiced_led_manager.h"
#include "bt_hs_spk_button.h"
#include "wiced_audio_manager.h"
#include "wiced_hal_gpio.h"
#include "hci_control_api.h"
#include "wiced_bt_gfps.h"
#include "btspk_nvram.h"
#ifdef AUTO_ELNA_SWITCH
#include "cycfg_pins.h"
#include "wiced_hal_rfm.h"
#ifndef TX_PU
#define TX_PU   CTX
#endif
#ifndef RX_PU
#define RX_PU   CRX
#endif
#endif

/*****************************************************************************
**  Constants
*****************************************************************************/

/*****************************************************************************
**  Structures
*****************************************************************************/
/******************************************************
 *               Variables Definitions
 ******************************************************/
extern wiced_bt_a2dp_config_data_t  bt_audio_config;
extern uint8_t bt_avrc_ct_supported_events[];

const wiced_transport_cfg_t transport_cfg =
{
    .type = WICED_TRANSPORT_UART,
    .cfg.uart_cfg = {
        .mode = WICED_TRANSPORT_UART_HCI_MODE,
        .baud_rate = HCI_UART_MAX_BAUD,
    },
    .rx_buff_pool_cfg = {
        .buffer_size = 0,
        .buffer_count = 0,
    },
    .p_status_handler = NULL,
    .p_data_handler = NULL,
    .p_tx_complete_cback = NULL,
};

extern const uint8_t btspeaker_sdp_db[];
/******************************************************
 *               Function Declarations
 ******************************************************/
static wiced_result_t btspk_control_management_callback( wiced_bt_management_evt_t event, wiced_bt_management_evt_data_t *p_event_data );

#ifndef PLATFORM_LED_DISABLED
/*LED config for app status indication */
static wiced_led_config_t led_config =
{
		.led = PLATFORM_LED_1,
		.bright = 50,
};
#endif

/* Local Identify Resolving Key. */
typedef struct
{
    wiced_bt_local_identity_keys_t  local_irk;
    wiced_result_t                  result;
} btspk_control_local_irk_info_t;

static btspk_control_local_irk_info_t local_irk_info = {0};

/******************************************************
 *               Function Definitions
 ******************************************************/

/*
 * Restore local Identity Resolving Key
 */
static void btspk_control_local_irk_restore(void)
{
    uint16_t nb_bytes;

    nb_bytes = wiced_hal_read_nvram(BTSPK_NVRAM_ID_LOCAL_IRK,
                                    BTM_SECURITY_LOCAL_KEY_DATA_LEN,
                                    local_irk_info.local_irk.local_key_data,
                                    &local_irk_info.result);

    WICED_BT_TRACE("btspk_control_local_irk_restore (result: %d, nb_bytes: %d)\n",
                   local_irk_info.result,
                   nb_bytes);
}

/*
 * Update local Identity Resolving Key
 */
static void btspk_control_local_irk_update(uint8_t *p_key)
{
    uint16_t nb_bytes;
    wiced_result_t result;

    /* Check if the IRK shall be updated. */
    if (memcmp((void *) p_key,
               (void *) local_irk_info.local_irk.local_key_data,
               BTM_SECURITY_LOCAL_KEY_DATA_LEN) != 0)
    {
        nb_bytes = wiced_hal_write_nvram(BTSPK_NVRAM_ID_LOCAL_IRK,
                                         BTM_SECURITY_LOCAL_KEY_DATA_LEN,
                                         p_key,
                                         &result);

        WICED_BT_TRACE("Update local IRK (result: %d, nb_bytes: %d)\n",
                       result,
                       nb_bytes);

        if ((nb_bytes == BTM_SECURITY_LOCAL_KEY_DATA_LEN) &&
            (result == WICED_BT_SUCCESS))
        {
            memcpy((void *) local_irk_info.local_irk.local_key_data,
                   (void *) p_key,
                   BTM_SECURITY_LOCAL_KEY_DATA_LEN);

            local_irk_info.result = result;
        }
    }
}

/*
 *  btspk_control_init
 *  Does bt stack and audio buffer init
 */
void btspk_control_init( void )
{
    wiced_result_t ret = WICED_BT_ERROR;

    wiced_transport_init( &transport_cfg );

#ifdef WICED_BT_TRACE_ENABLE
    // Set the debug uart as WICED_ROUTE_DEBUG_NONE to get rid of prints
    // wiced_set_debug_uart(WICED_ROUTE_DEBUG_NONE);

    // Set to PUART to see traces on peripheral uart(puart)
    wiced_hal_puart_init();
    wiced_hal_puart_configuration(3000000, PARITY_NONE, STOP_BIT_2);
    wiced_set_debug_uart( WICED_ROUTE_DEBUG_TO_PUART );

    // Set to HCI to see traces on HCI uart - default if no call to wiced_set_debug_uart()
    // wiced_set_debug_uart( WICED_ROUTE_DEBUG_TO_HCI_UART );

    // Use WICED_ROUTE_DEBUG_TO_WICED_UART to send formatted debug strings over the WICED
    // HCI debug interface to be parsed by ClientControl/BtSpy.
    // Note: WICED HCI must be configured to use this - see wiced_trasnport_init(), must
    // be called with wiced_transport_cfg_t.wiced_tranport_data_handler_t callback present
    // wiced_set_debug_uart(WICED_ROUTE_DEBUG_TO_WICED_UART);
#endif

    WICED_BT_TRACE( "btspeaker APP START\n" );
    ret = wiced_bt_stack_init(btspk_control_management_callback, &wiced_bt_cfg_settings, wiced_app_cfg_buf_pools);
    if( ret != WICED_BT_SUCCESS )
    {
        WICED_BT_TRACE("wiced_bt_stack_init returns error: %d\n", ret);
        return;
    }

    /* Configure Audio buffer */
    ret = wiced_audio_buffer_initialize (wiced_bt_audio_buf_config);
    if( ret != WICED_BT_SUCCESS )
    {
        WICED_BT_TRACE("wiced_audio_buffer_initialize returns error: %d\n", ret);
        return;
    }

    /* Restore local Identify Resolving Key (IRK) for BLE Private Resolvable Address. */
    btspk_control_local_irk_restore();
}

#ifdef HCI_TRACE_OVER_TRANSPORT
/*
 *  Process all HCI packet received from stack
 */

void hci_control_hci_packet_cback( wiced_bt_hci_trace_type_t type, uint16_t length, uint8_t* p_data )
{
#if (WICED_HCI_TRANSPORT == WICED_HCI_TRANSPORT_UART)
    // send the trace
    wiced_transport_send_hci_trace( NULL, type, length, p_data  );
#endif
}
#endif

wiced_result_t btspk_post_bt_init(void)
{
    wiced_bool_t ret = WICED_FALSE;
    bt_hs_spk_control_config_t config = {0};
    bt_hs_spk_eir_config_t eir = {0};

    eir.p_dev_name              = (char *) wiced_bt_cfg_settings.device_name;
    eir.default_uuid_included   = WICED_TRUE;

    if(WICED_SUCCESS != bt_hs_spk_write_eir(&eir))
    {
        WICED_BT_TRACE("Write EIR Failed\n");
    }

    ret = wiced_bt_sdp_db_init( ( uint8_t * )btspeaker_sdp_db, wiced_app_cfg_sdp_record_get_size());
    if( ret != TRUE )
    {
        WICED_BT_TRACE("%s Failed to Initialize SDP databse\n", __FUNCTION__);
        return WICED_BT_ERROR;
    }

    config.conn_status_change_cb            = NULL;
#ifdef LOW_POWER_MEASURE_MODE
    config.discoverable_timeout             = 60;   /* 60 Sec */
#else
    config.discoverable_timeout             = 240;  /* 240 Sec */
#endif
    config.acl3mbpsPacketSupport            = WICED_TRUE;
    config.audio.a2dp.p_audio_config        = &bt_audio_config;
    config.audio.a2dp.p_pre_handler         = NULL;
    config.audio.a2dp.post_handler          = NULL;
    config.audio.avrc_ct.p_supported_events = bt_avrc_ct_supported_events;
    config.hfp.rfcomm.buffer_size           = 700;
    config.hfp.rfcomm.buffer_count          = 4;
#if (WICED_BT_HFP_HF_WBS_INCLUDED == TRUE)
    config.hfp.feature_mask                 = WICED_BT_HFP_HF_FEATURE_3WAY_CALLING | \
                                              WICED_BT_HFP_HF_FEATURE_CLIP_CAPABILITY | \
                                              WICED_BT_HFP_HF_FEATURE_REMOTE_VOLUME_CONTROL| \
                                              WICED_BT_HFP_HF_FEATURE_HF_INDICATORS | \
                                              WICED_BT_HFP_HF_FEATURE_CODEC_NEGOTIATION | \
                                              WICED_BT_HFP_HF_FEATURE_VOICE_RECOGNITION_ACTIVATION | \
                                              WICED_BT_HFP_HF_FEATURE_ESCO_S4_T2_SETTINGS_SUPPORT;
#else
    config.hfp.feature_mask                 = WICED_BT_HFP_HF_FEATURE_3WAY_CALLING | \
                                              WICED_BT_HFP_HF_FEATURE_CLIP_CAPABILITY | \
                                              WICED_BT_HFP_HF_FEATURE_REMOTE_VOLUME_CONTROL| \
                                              WICED_BT_HFP_HF_FEATURE_HF_INDICATORS | \
                                              WICED_BT_HFP_HF_FEATURE_VOICE_RECOGNITION_ACTIVATION | \
                                              WICED_BT_HFP_HF_FEATURE_ESCO_S4_T2_SETTINGS_SUPPORT;
#endif

    config.sleep_config.enable                  = WICED_TRUE;
    config.sleep_config.sleep_mode              = WICED_SLEEP_MODE_NO_TRANSPORT;
    config.sleep_config.host_wake_mode          = WICED_SLEEP_WAKE_ACTIVE_HIGH;
    config.sleep_config.device_wake_mode        = WICED_SLEEP_WAKE_ACTIVE_LOW;
    config.sleep_config.device_wake_source      = WICED_SLEEP_WAKE_SOURCE_GPIO;
    config.sleep_config.device_wake_gpio_num    = WICED_P02;

    config.nvram.link_key.id            = BTSPK_NVRAM_ID_LINK_KEYS;
    config.nvram.link_key.p_callback    = NULL;

    if(WICED_SUCCESS != bt_hs_spk_post_stack_init(&config))
    {
        WICED_BT_TRACE("bt_audio_post_stack_init failed\n");
        return WICED_BT_ERROR;
    }

    /*Set audio sink*/
    bt_hs_spk_set_audio_sink(AM_SPEAKERS);

#if (WICED_APP_LE_INCLUDED == TRUE)
    hci_control_le_enable();
#endif

#ifdef FASTPAIR_DISABLE
    wiced_bt_gfps_provider_disable();
#endif

    /*we will use the channel map provided by the phone*/
    ret = wiced_bt_dev_set_afh_channel_assessment(WICED_FALSE);
    WICED_BT_TRACE("wiced_bt_dev_set_afh_channel_assessment status:%d\n",ret);
    if(ret != WICED_BT_SUCCESS)
    {
        return WICED_BT_ERROR;
    }

    if (!wiced_ota_fw_upgrade_init(NULL, NULL, NULL))
    {
        WICED_BT_TRACE("wiced_ota_fw_upgrade_init failed\n");
    }

    if (WICED_SUCCESS != ofu_spp_init())
    {
        WICED_BT_TRACE("ofu_spp_init failed\n");
        return WICED_ERROR;
    }

#ifdef AUTO_ELNA_SWITCH
    wiced_hal_rfm_auto_elna_switch(1, TX_PU, RX_PU);
#endif

    return WICED_SUCCESS;
}

/*
 *  Management callback receives various notifications from the stack
 */
wiced_result_t btspk_control_management_callback( wiced_bt_management_evt_t event, wiced_bt_management_evt_data_t *p_event_data )
{
    wiced_result_t                     result = WICED_BT_SUCCESS;
    wiced_bt_dev_status_t              dev_status;
    wiced_bt_dev_ble_pairing_info_t   *p_pairing_info;
    wiced_bt_dev_encryption_status_t  *p_encryption_status;
    int                                bytes_written, bytes_read;
    int                                nvram_id;
    wiced_bt_dev_pairing_cplt_t        *p_pairing_cmpl;
    uint8_t                             pairing_result;
    const uint8_t *link_key;

#if (WICED_HCI_TRANSPORT == WICED_HCI_TRANSPORT_UART)
    WICED_BT_TRACE( "btspeaker bluetooth management callback event: %d\n", event );
#endif

    switch( event )
    {
        /* Bluetooth  stack enabled */
        case BTM_ENABLED_EVT:
            if( p_event_data->enabled.status != WICED_BT_SUCCESS )
            {
                WICED_BT_TRACE("arrived with failure\n");
            }
            else
            {
                btspk_post_bt_init();

#ifdef HCI_TRACE_OVER_TRANSPORT
            // Disable while streaming audio over the uart.
            wiced_bt_dev_register_hci_trace( hci_control_hci_packet_cback );
#endif

            if(WICED_SUCCESS != btspk_init_button_interface())
		        WICED_BT_TRACE("btspeaker button init failed\n");
#ifndef PLATFORM_LED_DISABLED
            if (WICED_SUCCESS != wiced_led_manager_init(&led_config))
		        WICED_BT_TRACE("btspeaker LED init failed\n");
#endif
            }
            break;

        case BTM_DISABLED_EVT:
            //hci_control_send_device_error_evt( p_event_data->disabled.reason, 0 );
            break;

        case BTM_PIN_REQUEST_EVT:
            WICED_BT_TRACE("remote address= %B\n", p_event_data->pin_request.bd_addr);
            //wiced_bt_dev_pin_code_reply(*p_event_data->pin_request.bd_addr, WICED_BT_SUCCESS, WICED_PIN_CODE_LEN, (uint8_t *)&pincode[0]);
            break;

        case BTM_USER_CONFIRMATION_REQUEST_EVT:
            // If this is just works pairing, accept. Otherwise send event to the MCU to confirm the same value.
            WICED_BT_TRACE("BTM_USER_CONFIRMATION_REQUEST_EVT BDA %B\n", p_event_data->user_confirmation_request.bd_addr);
            if (p_event_data->user_confirmation_request.just_works)
            {
                WICED_BT_TRACE("just_works \n");
                wiced_bt_dev_confirm_req_reply( WICED_BT_SUCCESS, p_event_data->user_confirmation_request.bd_addr );
            }
            else
            {
                WICED_BT_TRACE("Need to send user_confirmation_request, Key %d \n", p_event_data->user_confirmation_request.numeric_value);
                wiced_bt_gfps_provider_seeker_passkey_set(p_event_data->user_confirmation_request.numeric_value);
                wiced_bt_dev_confirm_req_reply( WICED_BT_SUCCESS, p_event_data->user_confirmation_request.bd_addr );
            }
            break;

        case BTM_PASSKEY_NOTIFICATION_EVT:
            WICED_BT_TRACE("PassKey Notification. BDA %B, Key %d \n", p_event_data->user_passkey_notification.bd_addr, p_event_data->user_passkey_notification.passkey );
            //hci_control_send_user_confirmation_request_evt(p_event_data->user_passkey_notification.bd_addr, p_event_data->user_passkey_notification.passkey );
            break;

        case BTM_PAIRING_IO_CAPABILITIES_BR_EDR_REQUEST_EVT:
            /* Use the default security for BR/EDR*/
            WICED_BT_TRACE("BTM_PAIRING_IO_CAPABILITIES_BR_EDR_REQUEST_EVT bda %B\n", p_event_data->pairing_io_capabilities_br_edr_request.bd_addr);

            if (wiced_bt_gfps_provider_pairing_state_get())
            {   // Google Fast Pair service Seeker triggers this pairing process.
                /* Set local capability to Display/YesNo to identify local device is not a
                 * man-in-middle device.
                 * Otherwise, the Google Fast Pair Service Seeker will terminate this pairing
                 * process. */
                p_event_data->pairing_io_capabilities_br_edr_request.local_io_cap = BTM_IO_CAPABILITIES_DISPLAY_AND_YES_NO_INPUT;
            }
            else
            {
                p_event_data->pairing_io_capabilities_br_edr_request.local_io_cap = BTM_IO_CAPABILITIES_NONE;
            }

            p_event_data->pairing_io_capabilities_br_edr_request.auth_req     = BTM_AUTH_SINGLE_PROFILE_GENERAL_BONDING_NO;
            p_event_data->pairing_io_capabilities_br_edr_request.oob_data     = WICED_FALSE;
//            p_event_data->pairing_io_capabilities_br_edr_request.auth_req     = BTM_AUTH_ALL_PROFILES_NO;
            break;

        case BTM_PAIRING_IO_CAPABILITIES_BR_EDR_RESPONSE_EVT:
            WICED_BT_TRACE("BTM_PAIRING_IO_CAPABILITIES_BR_EDR_RESPONSE_EVT (%B, io_cap: 0x%02X) \n",
                           p_event_data->pairing_io_capabilities_br_edr_response.bd_addr,
                           p_event_data->pairing_io_capabilities_br_edr_response.io_cap);

            if (wiced_bt_gfps_provider_pairing_state_get())
            {   // Google Fast Pair service Seeker triggers this pairing process.
                /* If the device capability is set to NoInput/NoOutput, end pairing, to avoid using
                 * Just Works pairing method. todo*/
                if (p_event_data->pairing_io_capabilities_br_edr_response.io_cap == BTM_IO_CAPABILITIES_NONE)
                {
                    WICED_BT_TRACE("Terminate the pairing process\n");
                }
            }
            break;

        case BTM_PAIRING_IO_CAPABILITIES_BLE_REQUEST_EVT:
            /* Use the default security for BLE */
            WICED_BT_TRACE("BTM_PAIRING_IO_CAPABILITIES_BLE_REQUEST_EVT bda %B\n",
                    p_event_data->pairing_io_capabilities_ble_request.bd_addr);

            p_event_data->pairing_io_capabilities_ble_request.local_io_cap  = BTM_IO_CAPABILITIES_NONE;
            p_event_data->pairing_io_capabilities_ble_request.oob_data      = BTM_OOB_NONE;
            p_event_data->pairing_io_capabilities_ble_request.auth_req      = BTM_LE_AUTH_REQ_SC_MITM_BOND;
            p_event_data->pairing_io_capabilities_ble_request.max_key_size  = 16;
            p_event_data->pairing_io_capabilities_ble_request.init_keys     = BTM_LE_KEY_PENC|BTM_LE_KEY_PID|BTM_LE_KEY_PCSRK|BTM_LE_KEY_LENC;
            p_event_data->pairing_io_capabilities_ble_request.resp_keys     = BTM_LE_KEY_PENC|BTM_LE_KEY_PID|BTM_LE_KEY_PCSRK|BTM_LE_KEY_LENC;
            break;

        case BTM_PAIRING_COMPLETE_EVT:
            p_pairing_cmpl = &p_event_data->pairing_complete;
            if(p_pairing_cmpl->transport == BT_TRANSPORT_BR_EDR)
            {
                pairing_result = p_pairing_cmpl->pairing_complete_info.br_edr.status;
                WICED_BT_TRACE( "BREDR Pairing Result: %02x\n", pairing_result );
                if (pairing_result == WICED_BT_SUCCESS)
                {
                    /* pairing success, check disable pairing mode or not */
                    bt_hs_spk_button_check_disable_pairing();
                }
            }
            else
            {
                pairing_result = p_pairing_cmpl->pairing_complete_info.ble.reason;
                WICED_BT_TRACE( "BLE Pairing Result: %02x\n", pairing_result );
            }
            WICED_BT_TRACE( "Pairing Result: %d\n", pairing_result );
            //btspeaker_control_pairing_completed_evt( pairing_result, p_event_data->pairing_complete.bd_addr );
            break;

        case BTM_ENCRYPTION_STATUS_EVT:
            p_encryption_status = &p_event_data->encryption_status;

            WICED_BT_TRACE( "Encryption Status:(%B) res:%d\n", p_encryption_status->bd_addr, p_encryption_status->result );

#if (WICED_APP_LE_SLAVE_CLIENT_INCLUDED == TRUE)
            if (p_encryption_status->transport == BT_TRANSPORT_LE)
                le_slave_encryption_status_changed(p_encryption_status);
#endif
            //hci_control_send_encryption_changed_evt( p_encryption_status->result, p_encryption_status->bd_addr );
            break;

        case BTM_SECURITY_REQUEST_EVT:
            WICED_BT_TRACE( "Security Request Event, Pairing allowed %d\n", hci_control_cb.pairing_allowed );
            if ( hci_control_cb.pairing_allowed )
            {
                wiced_bt_ble_security_grant( p_event_data->security_request.bd_addr, WICED_BT_SUCCESS );
            }
            else
            {
                // Pairing not allowed, return error
                result = WICED_BT_ERROR;
            }
            break;

        case BTM_PAIRED_DEVICE_LINK_KEYS_UPDATE_EVT:
            /* Update the link key to database and NVRAM. */
            bt_hs_spk_control_link_key_update(&p_event_data->paired_device_link_keys_update);
            break;

        case  BTM_PAIRED_DEVICE_LINK_KEYS_REQUEST_EVT:
            /* read existing key from the NVRAM  */
            if (bt_hs_spk_control_link_key_get(&p_event_data->paired_device_link_keys_request) == WICED_TRUE)
            {
                result = WICED_BT_SUCCESS;
            }
            else
            {
                result = WICED_BT_ERROR;
            }
            break;

        case BTM_LOCAL_IDENTITY_KEYS_UPDATE_EVT:
            WICED_BT_TRACE("BTM_LOCAL_IDENTITY_KEYS_UPDATE_EVT\n");

            btspk_control_local_irk_update(p_event_data->local_identity_keys_update.local_key_data);
            break;


        case BTM_LOCAL_IDENTITY_KEYS_REQUEST_EVT:
            /*
             * Request to restore local identity keys from NVRAM
             * (requested during Bluetooth start up)
             * */
            WICED_BT_TRACE("BTM_LOCAL_IDENTITY_KEYS_REQUEST_EVT (%d)\n", local_irk_info.result);

            if (local_irk_info.result == WICED_BT_SUCCESS)
            {
                memcpy((void *) p_event_data->local_identity_keys_request.local_key_data,
                       (void *) local_irk_info.local_irk.local_key_data,
                       BTM_SECURITY_LOCAL_KEY_DATA_LEN);
            }
            else
            {
                result = WICED_BT_NO_RESOURCES;
            }
            break;

        case BTM_BLE_SCAN_STATE_CHANGED_EVT:
            //hci_control_le_scan_state_changed( p_event_data->ble_scan_state_changed );
            break;

        case BTM_BLE_ADVERT_STATE_CHANGED_EVT:
            //hci_control_le_advert_state_changed( p_event_data->ble_advert_state_changed );
            break;

        case BTM_POWER_MANAGEMENT_STATUS_EVT:
            bt_hs_spk_control_btm_event_handler_power_management_status(&p_event_data->power_mgmt_notification);
            break;

        case BTM_SCO_CONNECTED_EVT:
        case BTM_SCO_DISCONNECTED_EVT:
        case BTM_SCO_CONNECTION_REQUEST_EVT:
        case BTM_SCO_CONNECTION_CHANGE_EVT:
            hf_sco_management_callback(event, p_event_data);
            break;

        case BTM_BLE_CONNECTION_PARAM_UPDATE:
            WICED_BT_TRACE("BTM_BLE_CONNECTION_PARAM_UPDATE (%B, status: %d, conn_interval: %d, conn_latency: %d, supervision_timeout: %d)\n",
                           p_event_data->ble_connection_param_update.bd_addr,
                           p_event_data->ble_connection_param_update.status,
                           p_event_data->ble_connection_param_update.conn_interval,
                           p_event_data->ble_connection_param_update.conn_latency,
                           p_event_data->ble_connection_param_update.supervision_timeout);
            break;
        case BTM_BLE_PHY_UPDATE_EVT:
            /* BLE PHY Update to 1M or 2M */
            WICED_BT_TRACE("PHY config is updated as TX_PHY : %dM, RX_PHY : %dM\n",
                    p_event_data->ble_phy_update_event.tx_phy,
                    p_event_data->ble_phy_update_event.rx_phy);
            break;
        default:
            result = WICED_BT_USE_DEFAULT_SECURITY;
            break;
   }
    return result;
}