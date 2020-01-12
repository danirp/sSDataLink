/** @file protocol.h
 *
 * @brief RX - TX, code - decode, and definitions.
 *
 */

#ifndef _SUPER_SIMPLE_DATA_LINK_H_
#define _SUPER_SIMPLE_DATA_LINK_H_

/* *************************************************************************
 * *                                 Includes                              *
 * ************************************************************************* */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* *************************************************************************
 * *                             Public Constants                          *
 * ************************************************************************* */

/* *************************************************************************
 * *                               Public Types                            *
 * ************************************************************************* */

typedef enum
{
	SSDL_DECODER_SUCCESS,
	SSDL_DECODER_TIMEOUT,
	SSDL_DECODER_DECODING,
	SSDL_DECODER_FERROR

}SSDL_decoderStatus_t;

typedef struct
{
    uint16_t receivedBytes;
    uint8_t timeoutCnt;
    uint8_t * buff;
    uint16_t len;
    bool isReceiving;
    bool lastByteWasEscaped;

    SSDL_decoderStatus_t status;
}SSDL_decoderHandle_t;

typedef void (*ssdl_txFunction_t)( uint8_t* data, uint16_t size );


/* *************************************************************************
 * *                    Public Functions Declaration                       *
 * ************************************************************************* */

/**
 * @brief Inits.
 *
 * @param timeout.
 * @return WIFI_RES_OK: Semaphore was signaled.
 * @return WIFI_RES_TIMEOUT: Timeout elapsed.
 */
void SSDL_InitDecoder( SSDL_decoderHandle_t* handle, uint8_t* buff );

void SSDL_Decode( SSDL_decoderHandle_t* handle, uint8_t* inBuff, uint16_t inLen );

bool SSDL_EncodeAndSend( uint16_t cmd, uint8_t* data, uint16_t dataLen, ssdl_txFunction_t txFunction  );

/* *************************************************************************
 * *                             OS Objects                                *
 * ************************************************************************* */

#endif /* #ifndef _SUPER_SIMPLE_DATA_LINK_H_ */

/*** end of file ***/
