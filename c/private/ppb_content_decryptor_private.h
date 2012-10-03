/* Copyright (c) 2012 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* From private/ppb_content_decryptor_private.idl,
 *   modified Mon Oct 01 20:33:45 2012.
 */

#ifndef PPAPI_C_PRIVATE_PPB_CONTENT_DECRYPTOR_PRIVATE_H_
#define PPAPI_C_PRIVATE_PPB_CONTENT_DECRYPTOR_PRIVATE_H_

#include "ppapi/c/pp_bool.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_macros.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/c/pp_stdint.h"
#include "ppapi/c/pp_var.h"
#include "ppapi/c/private/pp_content_decryptor.h"

#define PPB_CONTENTDECRYPTOR_PRIVATE_INTERFACE_0_2 \
    "PPB_ContentDecryptor_Private;0.2"
#define PPB_CONTENTDECRYPTOR_PRIVATE_INTERFACE \
    PPB_CONTENTDECRYPTOR_PRIVATE_INTERFACE_0_2

/**
 * @file
 * This file defines the <code>PPB_ContentDecryptor_Private</code>
 * interface. Note: This is a special interface, only to be used for Content
 * Decryption Modules, not normal plugins.
 */


/**
 * @addtogroup Interfaces
 * @{
 */
/**
 * <code>PPB_ContentDecryptor_Private</code> structure contains the function
 * pointers the browser must implement to support plugins implementing the
 * <code>PPP_ContentDecryptor_Private</code> interface. This interface provides
 * browser side support for the Content Decryption Module (CDM) for v0.1 of the
 * proposed Encrypted Media Extensions: http://goo.gl/rbdnR
 */
struct PPB_ContentDecryptor_Private_0_2 {
  /**
   * The decryptor requires a key that has not been provided.
   *
   * Sent when the decryptor encounters encrypted content, but it does not have
   * the key required to decrypt the data. The plugin will call this method in
   * response to a call to the <code>Decrypt()</code> method on the
   * <code>PPP_ContentDecryptor_Private<code> interface.
   *
   * The browser must notify the application that a key is needed, and, in
   * response, the web application must direct the browser to call
   * <code>AddKey()</code> on the <code>PPP_ContentDecryptor_Private<code>
   * interface.
   *
   * @param[in] key_system A <code>PP_Var</code> of type
   * <code>PP_VARTYPE_STRING</code> containing the name of the key system.
   *
   * @param[in] session_id A <code>PP_Var</code> of type
   * <code>PP_VARTYPE_STRING</code> containing the session ID.
   *
   * @param[in] init_data A <code>PP_Var</code> of type
   * <code>PP_VARTYPE_ARRAYBUFFER</code> containing container-specific
   * initialization data.
   */
  void (*NeedKey)(PP_Instance instance,
                  struct PP_Var key_system,
                  struct PP_Var session_id,
                  struct PP_Var init_data);
  /**
   * A key has been added as the result of a call to the <code>AddKey()</code>
   * method on the <code>PPP_ContentDecryptor_Private</code> interface.
   *
   * Note: The above describes the most simple case. Depending on the key
   * system, a series of <code>KeyMessage()</code> calls from the CDM will be
   * sent to the browser, and then on to the web application. The web
   * application must then provide more data to the CDM by directing the browser
   * to pass the data to the CDM via calls to <code>AddKey()</code> on the
   * <code>PPP_ContentDecryptor_Private</code> interface.
   * The CDM must call <code>KeyAdded()</code> when the sequence is completed,
   * and, in response, the browser must notify the web application.
   *
   * @param[in] key_system A <code>PP_Var</code> of type
   * <code>PP_VARTYPE_STRING</code> containing the name of the key system.
   *
   * @param[in] session_id A <code>PP_Var</code> of type
   * <code>PP_VARTYPE_STRING</code> containing the session ID.
   */
  void (*KeyAdded)(PP_Instance instance,
                   struct PP_Var key_system,
                   struct PP_Var session_id);
  /**
   * A message or request has been generated for key_system in the CDM, and
   * must be sent to the web application.
   *
   * For example, when the browser invokes <code>GenerateKeyRequest()</code>
   * on the <code>PPP_ContentDecryptor_Private</code> interface, the plugin
   * must send a key message containing the key request.
   *
   * Note that <code>KeyMessage()</code> can be used for purposes other than
   * responses to <code>GenerateKeyRequest()</code> calls. See also the text
   * in the comment for <code>KeyAdded()</code>, which describes a sequence of
   * <code>AddKey()</code> and <code>KeyMessage()</code> calls required to
   * prepare for decryption.
   *
   * @param[in] key_system A <code>PP_Var</code> of type
   * <code>PP_VARTYPE_STRING</code> containing the name of the key system.
   *
   * @param[in] session_id A <code>PP_Var</code> of type
   * <code>PP_VARTYPE_STRING</code> containing the session ID.
   *
   * @param[in] resource A <code>PP_Resource</code> corresponding to a
   * <code>PPB_Buffer_Dev</code> resource that contains the message.
   *
   * @param[in] default_url A <code>PP_Var</code> of type
   * <code>PP_VARTYPE_STRING</code> containing the default URL for the message.
   */
  void (*KeyMessage)(PP_Instance instance,
                     struct PP_Var key_system,
                     struct PP_Var session_id,
                     PP_Resource message,
                     struct PP_Var default_url);
  /**
   * An error occurred in a <code>PPP_ContentDecryptor_Private</code> method,
   * or within the plugin implementing the interface.
   *
   * @param[in] key_system A <code>PP_Var</code> of type
   * <code>PP_VARTYPE_STRING</code> containing the name of the key system.
   *
   * @param[in] session_id A <code>PP_Var</code> of type
   * <code>PP_VARTYPE_STRING</code> containing the session ID.
   *
   * @param[in] media_error A MediaKeyError.
   *
   * @param[in] system_error A system error code.
   */
  void (*KeyError)(PP_Instance instance,
                   struct PP_Var key_system,
                   struct PP_Var session_id,
                   int32_t media_error,
                   int32_t system_code);
  /**
   * Called after the <code>Decrypt()</code> method on the
   * <code>PPP_ContentDecryptor_Private</code> interface completes to
   * deliver decrypted_block to the browser for decoding and rendering.
   *
   * @param[in] decrypted_block A <code>PP_Resource</code> corresponding to a
   * <code>PPB_Buffer_Dev</code> resource that contains a decrypted data
   * block.
   *
   * @param[in] decrypted_block_info A <code>PP_DecryptedBlockInfo</code> that
   * contains the result code and tracking info associated with the
   * <code>decrypted_block</code>.
   */
  void (*DeliverBlock)(
      PP_Instance instance,
      PP_Resource decrypted_block,
      const struct PP_DecryptedBlockInfo* decrypted_block_info);
  /**
   * Called after the <code>DecryptAndDecode()</code> method on the
   * <code>PPP_ContentDecryptor_Private</code> interface completes to deliver
   * a decrypted and decoded video frame to the browser for rendering.
   *
   * @param[in] decrypted_frame A <code>PP_Resource</code> corresponding to a
   * <code>PPB_Buffer_Dev</code> resource that contains a video frame.
   *
   * @param[in] decrypted_frame_info A <code>PP_DecryptedFrameInfo</code> that
   * contains the result code, tracking info, and buffer format associated with
   * <code>decrypted_frame</code>.
   */
  void (*DeliverFrame)(
      PP_Instance instance,
      PP_Resource decrypted_frame,
      const struct PP_DecryptedFrameInfo* decrypted_frame_info);
  /**
   * Called after the <code>DecryptAndDecode()</code> method on the
   * <code>PPP_ContentDecryptor_Private</code> interface completes to
   * deliver a buffer of decrypted and decoded audio samples to the browser for
   * rendering.
   *
   * @param[in] decrypted_samples A <code>PP_Resource</code> corresponding to a
   * <code>PPB_Buffer_Dev</code> resource that contains a decrypted buffer
   * of decoded audio samples.
   *
   * @param[in] decrypted_block_info A <code>PP_DecryptedBlockInfo</code> that
   * contains the tracking info and result code associated with the
   * <code>decrypted_block</code>.
   */
  void (*DeliverSamples)(
      PP_Instance instance,
      PP_Resource decrypted_samples,
      const struct PP_DecryptedBlockInfo* decrypted_block_info);
};

typedef struct PPB_ContentDecryptor_Private_0_2 PPB_ContentDecryptor_Private;
/**
 * @}
 */

#endif  /* PPAPI_C_PRIVATE_PPB_CONTENT_DECRYPTOR_PRIVATE_H_ */

