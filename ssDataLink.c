/** @file ssDataLink.c
 *
 * @brief Super Simple Data Link.
 *
 */

#include "ssDataLink.h"
#include "hw.h"	/* Provides HW_CrcXmodem */

/* *************************************************************************
 * *                          Private Constants                            *
 * ************************************************************************* */
#define SSDL_SLIP_END        (0xC0)
#define SSDL_SLIP_ESC        (0xDB)
#define SSDL_SLIP_ESC_END    (0xDC)
#define SSDL_SLIP_ESC_ESC    (0xDD)

/* *************************************************************************
 * *                   Private Functions Declaration                       *
 * ************************************************************************* */

/**
 * @brief Fills outPosition with the slip representation of inByte.
 *
 * inByte represented in slip could be one or two bytes. The function
 * returns the number of bytes that have been written to outPosition.
 *
 * https://en.wikipedia.org/wiki/Serial_Line_Internet_Protocol
 *
 * @param [in] 	inByte Value to be coded in slip.
 * @param [out] outPosition Addr. where coded bytes are going to be written.
 * @return Number of bytes written at outPossition.
 */
static uint8_t ssdl_nextSlip( uint8_t inByte, uint8_t* outPosition);


/* *************************************************************************
 * *                    Private Functions Definition                       *
 * ************************************************************************* */

static uint8_t ssdl_nextSlip( uint8_t inByte, uint8_t* outPosition)
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

void SSDL_InitDecoder( SSDL_decoderHandle_t* handle, uint8_t* buff, uint16_t timeoutTicks )
{
	if( ( NULL != handle ) && ( NULL != buff ) )
	{
		handle->isReceiving = false;
		handle->lastByteWasEscaped = false;
		handle->receivedBytes = 0;
		handle->timeoutCnt = 0;
		handle->timeoutTop = timeoutTicks;
		handle->buff = buff;
		handle->len = 0;
	}
}

bool SSDL_TimeoutTick( SSDL_decoderHandle_t* handle )
{
	bool retVal = false;

	if( NULL != handle )
	{
		handle->timeoutCnt ++;
		if( handle->timeoutCnt >= handle->timeoutTop )
		{
			/* Restart decoder if timeout reached */
			SSDL_InitDecoder(handle, handle->buff, handle->timeoutTop);
			retVal = true;
		}
	}
	return retVal;
}

void SSDL_Decode( SSDL_decoderHandle_t* handle, uint8_t* inBuff, uint16_t inLen, ssdl_parseFunction_t parser )
{
    uint16_t tIndex = 0;
    uint8_t lastByte = 0;

    if( ( NULL != handle ) && ( NULL != inBuff ) )
    {
		handle->currentStatus = SSDL_DECODER_DECODING;

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
				uint16_t crc = HW_CrcXmodem( handle->buff, handle->len );

				if ( 0 == crc )
				{
					handle->currentStatus = SSDL_DECODER_SUCCESS;
					if( NULL != parser )
					{
						(void)parser( handle->buff, handle->len );
					}
				}
				else
				{
					handle->currentStatus = SSDL_DECODER_FERROR;
				}
				handle->lastStatus = handle->currentStatus;
			}
			else
			{
				if ( SSDL_SLIP_ESC == lastByte )
				{
					/* If escape byte, do not feed anything and save the state */
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

					/* Increase index as new byte arrived */
					handle->receivedBytes++;
				}
			}
		}
    }
}

bool SSDL_EncodeAndSend( uint8_t* data, uint16_t dataLen, ssdl_sendFunction_t sender  )
{
    uint32_t tIndex = 0;
    uint16_t crc = 0;

    uint8_t tmpBuf[2];
    uint8_t tmpSize = 0;

    if( ( NULL != data ) || ( NULL != sender ) )
    {
        /* Data */
        for(tIndex = 0; tIndex < dataLen; tIndex++)
        {
            tmpSize = ssdl_nextSlip( *(data + tIndex), tmpBuf );
            sender( tmpBuf, tmpSize);
        }

        /* CRC */
        //crc = ~crc16_be( crc, data, dataLen );
        crc = HW_CrcXmodem( data, dataLen );

        tmpSize = ssdl_nextSlip( (uint8_t)( crc >> 8 ) , tmpBuf );
        sender( tmpBuf, tmpSize);
        tmpSize = ssdl_nextSlip( (uint8_t)crc, tmpBuf );
        sender( tmpBuf, tmpSize);

        /* Terminator */
        tmpBuf[0] = SSDL_SLIP_END;
        sender( tmpBuf, 1U);
    }

    return true;
}

/*** end of file ***/
