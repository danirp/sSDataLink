/** @file ssDataLink.c
 *
 * @brief RX - TX, code - decode, and definitions.
 *
 */

#include "ssDataLink.h"

#include <string.h>
#include <stdio.h>

#include "hw.h"	/* HW_CrcXmodem */


/* *************************************************************************
 * *                          Private Constants                            *
 * ************************************************************************* */
#define SSDL_SLIP_END        (0xC0)
#define SSDL_SLIP_ESC        (0xDB)
#define SSDL_SLIP_ESC_END    (0xDC)
#define SSDL_SLIP_ESC_ESC    (0xDD)

/* *************************************************************************
 * *                            Private Types                              *
 * ************************************************************************* */

/* *************************************************************************
 * *                          Private Variables                            *
 * ************************************************************************* */

/* *************************************************************************
 * *                   Private Functions Declaration                       *
 * ************************************************************************* */

/* *************************************************************************
 * *                   Private Functions Declaration                       *
 * ************************************************************************* */

/* *************************************************************************
 * *                    Private Functions Definition                       *
 * ************************************************************************* */
uint8_t ssdl_nextSlip( uint8_t inByte, uint8_t* outPosition)
{
    uint8_t tNbytes = 0;

    if( SSDL_SLIP_END == inByte )
    {
        *outPosition = SSDL_SLIP_ESC;
        tNbytes++;
        *(outPosition + 1) = SSDL_SLIP_ESC_END;
        tNbytes++;
    }
    else if( SSDL_SLIP_ESC == inByte )
    {
        *outPosition = SSDL_SLIP_ESC;
        tNbytes++;
        *(outPosition + 1) = SSDL_SLIP_ESC_ESC;
        tNbytes++;
    }
    else
    {
        *outPosition = inByte;
        tNbytes++;
    }

    return tNbytes;

}



/* *************************************************************************
 * *                      Public Functions Definition                      *
 * ************************************************************************* */

void SSDL_InitDecoder( SSDL_decoderHandle_t* handle, uint8_t* buff )
{
    handle->isReceiving = false;
    handle->lastByteWasEscaped = false;
    handle->receivedBytes = 0;
    handle->timeoutCnt = 0;
    handle->buff = buff;
    handle->len = 0;
}

void SSDL_Decode( SSDL_decoderHandle_t* handle, uint8_t* inBuff, uint16_t inLen )
{
    uint16_t tIndex = 0;
    uint8_t lastByte = 0;

    handle->status = SSDL_DECODER_DECODING;

    for(tIndex = 0; tIndex < inLen; tIndex++)
    {
        lastByte = *( inBuff + tIndex);

        if( SSDL_SLIP_END == lastByte )
        {
        	handle->isReceiving = false;
        	handle->len = handle->receivedBytes;
        	handle->receivedBytes = 0;
        	handle->lastByteWasEscaped = false;

            /* CRC */
            //uint16_t crc = ~crc16_be( 0xFFFF, handle->buff, handle->len);
        	uint16_t crc = HW_CrcXmodem( handle->buff, handle->len );

            if ( 0 == crc )
            {
            	handle->status = SSDL_DECODER_SUCCESS;
            }
            else
            {
            	handle->status = SSDL_DECODER_FERROR;
            }
        }
        else
        {
            if ( SSDL_SLIP_ESC == lastByte )
            {
                /* If scape byte, do not feed anything and save the state */
            	handle->lastByteWasEscaped = true;
            }
            else
            {
                /* First byte */
                if( false == handle->isReceiving )
                {
                	handle->timeoutCnt = 0;
                	handle->isReceiving = true;
                }
                /* Feed bytes */
                if( true == handle->lastByteWasEscaped)
                {
                	handle->lastByteWasEscaped = false;

                    if(SSDL_SLIP_ESC_END == lastByte)
                    {
                        *(handle->buff + handle->receivedBytes) = SSDL_SLIP_END;
                    }
                    else
                    {
                        *(handle->buff + handle->receivedBytes) = SSDL_SLIP_ESC;
                    }
                }
                else
                {
                    *(handle->buff + handle->receivedBytes) = lastByte;
                }

                /* Increase index as, new byte arrived */
                handle->receivedBytes++;
            }
        }
    }
}


bool SSDL_EncodeAndSend( uint16_t cmd, uint8_t* data, uint16_t dataLen, ssdl_txFunction_t txFunction  )
{
    uint32_t tIndex = 0;
    uint16_t crc = 0;

    uint8_t tmpBuf[2];
    uint8_t tmpSize = 0;

    uint8_t tmpCmd[2];

    memcpy( tmpCmd, &cmd, sizeof(uint16_t) );


    if( ( ( NULL != data ) ||
        ( ( NULL == data ) && ( 0 == dataLen) ) ) &&
        ( NULL != txFunction ) )
    {
        /* Command */
        tmpSize = ssdl_nextSlip( tmpCmd[0] , tmpBuf );
        txFunction( tmpBuf, tmpSize);
        tmpSize = ssdl_nextSlip( tmpCmd[1] , tmpBuf );
        txFunction( tmpBuf, tmpSize);

        /* Data */
        for(tIndex = 0; tIndex < dataLen; tIndex++)
        {
            tmpSize = ssdl_nextSlip( *(data + tIndex), tmpBuf );
            txFunction( tmpBuf, tmpSize);
        }

        /* CRC */
        //crc = ~crc16_be( crc, data, dataLen );
        crc = HW_CrcXmodem( data, dataLen );

        tmpSize = ssdl_nextSlip( (uint8_t)( crc >> 8 ) , tmpBuf );
        txFunction( tmpBuf, tmpSize);
        tmpSize = ssdl_nextSlip( (uint8_t)crc, tmpBuf );
        txFunction( tmpBuf, tmpSize);

        /* Terminator */
        tmpBuf[0] = SSDL_SLIP_END;
        txFunction( tmpBuf, 1U);
    }

    return true;
}

/*** end of file ***/
