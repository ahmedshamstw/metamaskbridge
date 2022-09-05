#include <emscripten.h>
#include <stdio.h>
//#include <assert.h>

#ifdef __cplusplus
#error
extern "C" {
#include <string.h>	
#include <emscripten/bind.h>
#endif	
#include "twi_usb_wallet_if.h"
#include "twi_debug.h"
#ifdef __cplusplus
}
#endif

#define CHAIN_CODE_IDX        (13)
#define CHAIN_CODE_SZ         (32)
#define COMPRESSED_PUB_KEY_SZ (33)

typedef enum 
{
  CRYPTO_GUARD_IF_CONNECTED_EVT,
  CRYPTO_GUARD_IF_DISCONNECTED_EVT,
  CRYPTO_GUARD_IF_SEND_STATUS_EVT,
  CRYPTO_GUARD_IF_RECIEVED_DATA_EVT,
  CRYPTO_GUARD_IF_INVALID_EVT,
}
tenum_crypto_guard_if_event;

static tstr_usb_if_context* gp_curr_ctx = NULL;
/////////////////////////////////////////////////////////////////////////
///////////////////////////JS Helpers///////////////////////////////////
extern char* consoleLog(char* data);
extern twi_u8* allocateOnMemory(twi_u32 data_len);
extern void usbSend(twi_u32 data_len);
extern void onConnectionDone(void);
extern void onGetXpubResult(twi_u32 comp_pub_key_off, twi_u32 comp_pub_key_len, twi_u32 chain_code_off, twi_u32 chain_code_len, twi_s32 error_code); //in this function the bridge should notify the kyring with the operation result
/////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////
///////////////////////////Static functions//////////////////////////////
static void usb_scan_and_connect_cb(void* const pv_device, twi_u8* pu8_dvc_id, twi_u8 u8_dvc_id_len, twi_u16 u16_vid, twi_u16 u16_pid, twi_u32 u32_scan_time_out_msec, twi_u32 u32_mtu_sz)
{
  FUN_IN;
}

static void usb_disconnect_cb(void* const pv_device)
{
  FUN_IN;
}

static void usb_receive_cb(void* const pv_device, void *p_rx_buf, twi_u32* pu32_length)
{
  FUN_IN;
}

static void usb_stop_cb(void* const pv_device)
{
  FUN_IN;
}

static void usb_disable_cb(void* const pv_device)
{
  FUN_IN;
}

static void usb_dispatch_cb(void* const pv_device)
{
  FUN_IN;
}

static void usb_send_cb(void* const pv_device, twi_u8* const pu8_data, twi_u32 u32_data_sz)
{
  //allocate or copy to the JS bufefr
  FUN_IN;
  TWI_LOGGER("Send Buffer::");
  for(int i =0; i<u32_data_sz; i++)
  {
    TWI_LOGGER("%d",pu8_data[i]);
  }
  TWI_LOGGER("\r\n");

  memcpy(pv_device, pu8_data, u32_data_sz);
  usbSend(u32_data_sz);
}

static void usb_Start_Timer_cb(void* const pv_device, twi_u32 u32_idx, twi_u32 u32_dur_msec)
{
  FUN_IN;
}

static void usb_Stop_Timer_cb(void* const pv_device, twi_u32 u32_idx)
{
  FUN_IN;
}

//TODO: add the URL and port
static void usb_send_to_cloud_cb(void* const pv_device, twi_u8* const pu8_data, twi_u32 u32_data_sz)
{
  FUN_IN;
} 

static void usb_onUserConfirmationRequested_cb(void* const pv_device, twi_u32 u32_conf_type)
{
  FUN_IN;
}

static void usb_onUserConfirmationObtained_cb(void* const pv_device, twi_u32 u32_conf_type)
{
  FUN_IN;
}

static void usb_onGetExtendedPubKeyResult_cb(void* const pv_device, twi_u8* const pu8_pub_key, twi_u32 u32_pub_key_sz, twi_s32 s32_err)
{
  //map the buffers
  FUN_IN;
  TWI_LOGGER("public_key = %d, error = %d \r\n", u32_pub_key_sz, s32_err);
  for(int i =0; i<u32_pub_key_sz; i++)
  {
    TWI_LOGGER("%d", pu8_pub_key[i]);
  }
  memcpy(pv_device,  &pu8_pub_key[CHAIN_CODE_IDX], CHAIN_CODE_SZ);
  memcpy(&((twi_u8*)pv_device)[CHAIN_CODE_SZ],  &pu8_pub_key[CHAIN_CODE_IDX + CHAIN_CODE_SZ], COMPRESSED_PUB_KEY_SZ);

  onGetXpubResult(CHAIN_CODE_SZ, COMPRESSED_PUB_KEY_SZ,0, CHAIN_CODE_SZ, s32_err);
}

//TODO: add sign structure
static void usb_onSignTransactionResult_cb(void* const pv_device, void* pstr_signed_tx, twi_s32 s32_err)
{
  FUN_IN;
}

static void usb_onSignMessageResult_cb(void* const pv_device, void* pstr_signed_msg, twi_s32 s32_err)
{
  FUN_IN;
}

static void usb_onGetWalletIDResult_cb(void* const pv_device, twi_u8* pu8_wallet_id, twi_u8 u8_wallet_id_len, twi_s32 s32_err)
{
  FUN_IN;
}

static void usb_save_cb(void* const pv_device, twi_u16 id,  twi_u8* pu8_data, twi_u32 u32_data_sz)
{
  FUN_IN;
}

static void usb_load_cb(void* const pv_device, twi_u16 id, twi_u8* pu8_data, twi_u32* pu32_data_sz)
{
  FUN_IN;
}

static void usb_onConnectionDone_cb(void* const pv_device)
{
  FUN_IN;
  //notify the upper layer
  onConnectionDone();
}

static tstr_usb_if_context* cyrpto_guard_if_init(void)
{
  tstr_usb_if_context* presult = NULL;
  presult = twi_usb_if_new();
  //assert(NULL != presult);

  twi_usb_if_set_callbacks( presult, 
                            usb_scan_and_connect_cb            ,
                            usb_disconnect_cb                  ,
                            usb_receive_cb                     ,
                            usb_stop_cb                        ,
                            usb_disable_cb                     ,
                            usb_dispatch_cb                    ,
                            usb_send_cb                        ,
                            usb_Start_Timer_cb                 ,
                            usb_Stop_Timer_cb                  ,
                            usb_send_to_cloud_cb               ,
                            usb_onUserConfirmationRequested_cb ,
                            usb_onUserConfirmationObtained_cb  ,
                            usb_onGetExtendedPubKeyResult_cb   ,
                            usb_onSignTransactionResult_cb     ,
                            usb_onSignMessageResult_cb         ,
                            usb_onGetWalletIDResult_cb         ,
                            usb_save_cb                        ,
                            usb_load_cb                        ,
                            usb_onConnectionDone_cb            );
  return  presult;                         
}
/////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////
//////////////////////////////APIS///////////////////////////////////////
EMSCRIPTEN_KEEPALIVE
void crypto_guard_if_init_shared_mem(twi_u8* pu8_shared_mem)
{
    twi_usb_if_set_device_info(gp_curr_ctx, pu8_shared_mem);
}

EMSCRIPTEN_KEEPALIVE
void crypto_guard_if_get_xpub(twi_u8 *xpub_path, int num_of_step)
{
  FUN_IN;
  //communicate with the keyfon_cb(handshake and openning the nano-app) then getting the xpub
  tstr_usb_crypto_path str_usb_crypto_path = {0};
  str_usb_crypto_path.u8_steps_num = num_of_step;
  memcpy(str_usb_crypto_path.au32_path_steps, xpub_path, num_of_step*4);
  twi_usb_if_get_ext_pub_key(gp_curr_ctx, USB_WALLET_COIN_ETHEREUM, &str_usb_crypto_path,NULL, 0,TWI_FALSE);
}

EMSCRIPTEN_KEEPALIVE
void crypto_guard_if_notify(tenum_crypto_guard_if_event enum_event, twi_u8* data, int len, int error)
{
  FUN_IN;
  TWI_LOGGER("enum_event = %d, error = %d\r\n", enum_event, error);
  switch(enum_event)
  {
    case CRYPTO_GUARD_IF_CONNECTED_EVT:
    {
      gp_curr_ctx = cyrpto_guard_if_init();
      twi_usb_if_notify_connected(gp_curr_ctx, error);
      break;
    }

    case CRYPTO_GUARD_IF_DISCONNECTED_EVT:
    {
      twi_usb_if_notify_disconnected(gp_curr_ctx, 0, error);
      twi_usb_if_free(gp_curr_ctx);
      gp_curr_ctx = NULL;
      break;
    }

    case CRYPTO_GUARD_IF_SEND_STATUS_EVT:
    {
      twi_usb_if_notify_send_status(gp_curr_ctx, error);
      break;
    }

    case CRYPTO_GUARD_IF_RECIEVED_DATA_EVT:
    {
      twi_usb_if_notify_data_received(gp_curr_ctx, data, len, error);

      break;
    }

    default:
    {
      //Invalid event
      //assert(false);
    }
  }
}

EMSCRIPTEN_KEEPALIVE
void crypto_guard_if_dispatch(void)
{
  if(NULL != gp_curr_ctx)
  {
    twi_usb_if_dispatch(gp_curr_ctx);
  }
}
/////////////////////////////////////////////////////////////////////////
