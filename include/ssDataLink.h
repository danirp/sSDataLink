/** @file ssDataLink.h
 *
 * @brief Super Simple Datalink.
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

/**
 * Transaction status. Value to be checked after each execution of
 * SSDL_Decode to control what happens while it continues decoding a success
 * message decode or an error.
 */
typedef enum
{
    SSDL_DECODER_SUCCESS,    /* Successfully decoded a message */
    SSDL_DECODER_DECODING,  /* Nothing decoded yet, keep iterating  */
    SSDL_DECODER_FERROR        /* ERROR! Suffered a framing error (wrong crc) */

}SSDL_decoderStatus_t;

/**
 * Decoder handle, keeps record of the current transaction between calls to
 * SSDL_Decode and allows having multiple "instances" of decoders working
 * at the same time.
 */
typedef struct
{
    uint16_t receivedBytes;                /* Number of bytes decoded so far */
    uint8_t timeoutCnt;                    /* Current number of timeout ticks */
    uint8_t timeoutTop;                    /* Number timeout ticks to trigger a timeout */
    uint8_t * buff;                        /* Output decoded message buffer */
    uint16_t len;                        /* output decoded message length  */
    bool isReceiving;                    /* Flow control */
    bool lastByteWasEscaped;            /* Flow control */
    SSDL_decoderStatus_t currentStatus;        /* Status of the last execution, to be used by calling processes */
    SSDL_decoderStatus_t lastStatus;        /* Status of the last finished message, to be used by calling processes */

}SSDL_decoderHandle_t;

/**
 * Pointer to the function that SSDL_Decode will call with every time a
 * valid message is decoded. This function will process the data. When
 * this function returns, SSDL_Decode starts filling a new message if
 * there were remaining bytes in the last chunk of bytes.
 */
typedef void (*ssdl_parseFunction_t)( uint8_t* data, uint16_t size );

/**
 * Pointer to the function that SSDL_EncodeAndSend will call while decoding
 * every byte of the input buffer. This function could save the data for
 * sending it later or send the data directly over the medium.
 */
typedef void (*ssdl_sendFunction_t)( uint8_t* data, uint16_t size );


/* *************************************************************************
 * *                    Public Functions Declaration                       *
 * ************************************************************************* */

/**
 * @brief Resets a SSDL_decoderHandle_t "object" and assigns its buffer.
 *
 * @param handle, "instance" of a decoder.
 * @param [out] buff, output buffer assigned to this handle.
 * @return None.
 */
void SSDL_InitDecoder( SSDL_decoderHandle_t* handle, uint8_t* buff, uint16_t timeoutTicks );

/**
 * @brief Increases the timeout counter, checks if reaches the maximum and,
 * if this happens, resets the decoder.
 *
 * Only increases the timeout counter if the message is already started. If the
 * maximum timeout ticks are reached, the decoder state machine of handle is
 * reinitialized.
 *
 * @param handle, "instance" of a decoder.
 * @return True if a timeout condition happened and the decoder was reset.
 */
bool SSDL_TimeoutTick( SSDL_decoderHandle_t* handle );

/**
 * @brief Function to be called every time a chunk of bytes is received that,
 * eventually, will end up with a valid message decoded.
 *
 * During the process of the current chunk of bytes, if a message is correctly
 * decoded, parser is called pointing to the buffer of the handle. When parser
 * finishes, the remaining bytes in the chunk continue being processed.
 *
 * Analyzing handle->currentStatus the calling process can determine what
 * happened after processing the last byte of the chunk.
 *
 * SSDL_DECODER_DECODING:	Nothing decoded yet, keep iterating
 * SSDL_DECODER_SUCCESS:    Successfully decoded a message
 * SSDL_DECODER_FERROR:		ERROR! Suffered a framing error (wrong crc)
 *
 * Analyzing handle->lastStatus the calling process can determine what
 * happened after the last decoded message, either SSDL_DECODER_SUCCESS or
 * SSDL_DECODER_FERROR.
 *
 * @param handle, "instance" of a decoder.
 * @param [in] inBuff, chunk of bytes to be decoded.
 * @param [in] inLen, length of the chunk of bytes.
 * @param [in] parser, function to be called when a valid message is decoded.
 *
 * @return None.
 */
void SSDL_Decode( SSDL_decoderHandle_t* handle, uint8_t* inBuff, uint16_t inLen, ssdl_parseFunction_t parser );

/**
 * @brief Function to be called to encode and [send] a message.
 *
 * A slip coded string is generated byte by byte swiping dataLen bytes
 * from data. For every byte in data, sender is called with the bytes encoded
 * in slip (can be 1 or 2 bytes each time). At the end the CRC and the
 * SSDL_SLIP_END byte are also passed through sender.
 *
 * sender could be a function copying the data to another buffer or directly
 * a TX function.
 *
 * @param [in] data, chunk of bytes to be encoded and [sent].
 * @param [in] dataLen, length of the chunk of bytes.
 * @param [in] sender, function that store or send the output bytes.
 *
 * @return None.
 */
bool SSDL_EncodeAndSend( uint8_t* data, uint16_t dataLen, ssdl_sendFunction_t sender );

/* *************************************************************************
 * *                             OS Objects                                *
 * ************************************************************************* */

#endif /* #ifndef _SUPER_SIMPLE_DATA_LINK_H_ */

/*** end of file ***/
