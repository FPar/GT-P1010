
/*
 * Copyright (C) Texas Instruments - http://www.ti.com/
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
/* ==============================================================================
*             Texas Instruments OMAP (TM) Platform Software
*  (c) Copyright Texas Instruments, Incorporated.  All Rights Reserved.
*
*  Use of this software is controlled by the terms and conditions found
*  in the license agreement under which this software has been supplied.
* ============================================================================ */
/**
* 
* This file implements JpegDecoder
*
*/

#undef LOG_TAG
#define LOG_TAG "JpegDecoder"

#include "JpegDecoder.h"
#include <utils/Log.h>

#define PRINTF LOGD

#define JPEG_DECODER_DUMP_INPUT_AND_OUTPUT 0
#define OPTIMIZE 0

#if JPEG_DECODER_DUMP_INPUT_AND_OUTPUT
	int eOutputCount = 0;
	int eInputCount = 0;
#endif

OMX_ERRORTYPE JPEGE_FillBufferDone (OMX_HANDLETYPE hComponent, OMX_PTR ptr, OMX_BUFFERHEADERTYPE* pBuffHead)
{
    ((JpegDecoder *)ptr)->FillBufferDone(pBuffHead->pBuffer,  pBuffHead->nFilledLen);
    return OMX_ErrorNone;
}


OMX_ERRORTYPE JPEGE_EmptyBufferDone(OMX_HANDLETYPE hComponent, OMX_PTR ptr, OMX_BUFFERHEADERTYPE* pBuffer)
{
    JpegDecoder * ImgDec = (JpegDecoder *)ptr;
    ImgDec->iLastState = ImgDec->iState;
    ImgDec->iState = JpegDecoder::STATE_EMPTY_BUFFER_DONE_CALLED;
    sem_post(ImgDec->semaphore) ;
    return OMX_ErrorNone;
}

OMX_ERRORTYPE JPEGE_EventHandler(OMX_HANDLETYPE hComponent,
                                    OMX_PTR pAppData,
                                    OMX_EVENTTYPE eEvent,
                                    OMX_U32 nData1,
                                    OMX_U32 nData2,
                                    OMX_PTR pEventData)
{
    ((JpegDecoder *)pAppData)->EventHandler(hComponent, eEvent, nData1, nData2, pEventData);
    return OMX_ErrorNone;
}

JpegDecoder::JpegDecoder()
{
    pInBuffHead = NULL;
    pOutBuffHead = NULL;
    semaphore = NULL;
    pOMXHandle = NULL;
    semaphore = (sem_t*)malloc(sizeof(sem_t)) ;
    sem_init(semaphore, 0x00, 0x00);
}

JpegDecoder::~JpegDecoder()
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
#if OPTIMIZE
    if( ( iLastState || iState ) && ( NULL != pOMXHandle ) ){
        eError = OMX_SendCommand(pOMXHandle,OMX_CommandStateSet, OMX_StateIdle, NULL);
        if ( eError != OMX_ErrorNone ) {
            PRINTF("\nError from SendCommand-Idle(nStop) State function\n");
            iState = STATE_ERROR;
            sem_post(semaphore) ;
        }

        Run();
    }
#endif    
	sem_destroy(semaphore);
	free(semaphore) ;	
    semaphore=NULL;
}

void JpegDecoder::FillBufferDone(OMX_U8* pBuffer, OMX_U32 size)
{

#if JPEG_ENCODER_DUMP_INPUT_AND_OUTPUT
    
    char path[50];
    snprintf(path, sizeof(path), "/temp/JEO_%d.yuv", eOutputCount);

    PRINTF("\nrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrr");
    
    SkFILEWStream tempFile(path);
    if (tempFile.write(pBuffer, size) == false)
        PRINTF("\nWriting to %s failed\n", path);
    else
        PRINTF("\nWriting to %s succeeded\n", path);

    eOutputCount++;
    PRINTF("\nrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrr");
#endif

    iLastState = iState;
    iState = STATE_FILL_BUFFER_DONE_CALLED;
    yuvsize = size;
   // sem_post(semaphore) ;
}


void JpegDecoder::EventHandler(OMX_HANDLETYPE hComponent,
                                            			OMX_EVENTTYPE eEvent,
                                            				OMX_U32 nData1,
                                            					OMX_U32 nData2,
                                            						OMX_PTR pEventData)
{

    switch ( eEvent ) {

        case OMX_EventCmdComplete:
            /* Do not move the common stmts in these conditionals outside. */
            /* We do not want to apply them in cases when these conditions are not met. */
            if ((nData1 == OMX_CommandStateSet) && (nData2 == OMX_StateIdle))
            {
                PRINTF ("\nComponent State Changed To OMX_StateIdle\n");
                iLastState = iState;
                iState = STATE_IDLE;
                sem_post(semaphore) ;
            }
            else if ((nData1 == OMX_CommandStateSet) && (nData2 == OMX_StateExecuting))
            {
                PRINTF ("\nComponent State Changed To OMX_StateExecuting\n");
                iLastState = iState;
                iState = STATE_EXECUTING;
                sem_post(semaphore) ;
            }
            else if ((nData1 == OMX_CommandStateSet) && (nData2 == OMX_StateLoaded))
            {
                PRINTF ("\nComponent State Changed To OMX_StateLoaded\n");
                iLastState = iState;
                iState = STATE_LOADED;
                sem_post(semaphore) ;
            }
            break;

        case OMX_EventError:
            PRINTF ("\n\n\nOMX Component  reported an Error!!!!\n\n\n");
            iLastState = iState;
            iState = STATE_ERROR;
            OMX_SendCommand(hComponent, OMX_CommandStateSet, OMX_StateInvalid, NULL);
            sem_post(semaphore) ;
            break;

        default:
            break;
    }

}


bool JpegDecoder::StartFromLoadedState()
{

    int nRetval;
    int nIndex1;
    int nIndex2;
    int bitsPerPixel;
    int nMultFactor = 0;
    int nHeightNew, nWidthNew;
    char strTIJpegDec[] = "OMX.TI.JPEG.decoder";  //"OMX.TI.JPEG.Decoder";
    
    OMX_S32 nCompId = 300;
    OMX_PORT_PARAM_TYPE PortType;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
        
    OMX_INDEXTYPE nCustomIndex = OMX_IndexMax;
    OMX_CALLBACKTYPE JPEGCallBack ={JPEGE_EventHandler, JPEGE_EmptyBufferDone, JPEGE_FillBufferDone};
	
    PRINTF("\nCalling TIOMX_Init()\n");
    eError = TIOMX_Init();
    if ( eError != OMX_ErrorNone ) {
        PRINTF("\n%d :: Error returned by OMX_Init()\n",__LINE__);
        goto EXIT;
    }

    PRINTF("\nCalling OMX_GetHandle\n");
    eError = TIOMX_GetHandle(&pOMXHandle, strTIJpegDec, (void *)this, &JPEGCallBack);
    if ( (eError != OMX_ErrorNone) ||  (pOMXHandle == NULL) ) {
        PRINTF ("Error in Get Handle function\n");
        goto EXIT;
    }

    PRINTF("\nCalling OMX_GetParameter() for OMX_IndexParamImageInit\n");
    eError = OMX_GetParameter(pOMXHandle, OMX_IndexParamImageInit, &PortType);
    if ( eError != OMX_ErrorNone ) {
        goto EXIT;
    }

    nIndex1 = PortType.nStartPortNumber;
    nIndex2 = nIndex1 + 1;

    /**********************************************************************/
    /* Set the component's OMX_PARAM_PORTDEFINITIONTYPE structure (INPUT) */
    /**********************************************************************/
    PRINTF("\nCalling OMX_GetParameter() for input port\n");

    InPortDef.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);	
    InPortDef.nPortIndex = PortType.nStartPortNumber;
	
    eError = OMX_GetParameter (pOMXHandle, OMX_IndexParamPortDefinition, &InPortDef);
    if ( eError != OMX_ErrorNone ) {
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    //InPortDef.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
    InPortDef.nVersion.s.nVersionMajor = 0x1;
    InPortDef.nVersion.s.nVersionMinor = 0x0;
    InPortDef.nVersion.s.nRevision = 0x0;
    InPortDef.nVersion.s.nStep = 0x0;
    InPortDef.eDir = OMX_DirInput;
    InPortDef.nBufferCountActual =1;
    InPortDef.nBufferCountMin = 1;
    InPortDef.bEnabled = OMX_TRUE;
    InPortDef.bPopulated = OMX_FALSE;
    InPortDef.eDomain = OMX_PortDomainImage;
    InPortDef.format.image.pNativeRender = NULL;
    InPortDef.format.image.nFrameWidth = mWidth;
    InPortDef.format.image.nFrameHeight = mHeight;
    InPortDef.format.image.nStride = -1;
    InPortDef.format.image.nSliceHeight = -1;
    InPortDef.format.image.bFlagErrorConcealment = OMX_FALSE;
    InPortDef.format.image.eCompressionFormat = OMX_IMAGE_CodingJPEG;
    InPortDef.nBufferSize = mInBuffSize;

	//[20091226 Ratnesh
    if(mIsPixelFmt420p){
		InPortDef.format.image.eColorFormat = OMX_COLOR_FormatYUV420PackedPlanar;	
     }
     else{
		InPortDef.format.image.eColorFormat = OMX_COLOR_FormatCbYCrY;
      }
   //]
   
    if (InPortDef.eDir == nIndex1 ) {
        InPortDef.nPortIndex = nIndex1;
    }
    else {
        InPortDef.nPortIndex = nIndex2;
    }
    
    PRINTF("\nCalling OMX_SetParameter() for input port\n");
    
    eError = OMX_SetParameter (pOMXHandle, OMX_IndexParamPortDefinition, &InPortDef);
    if ( eError != OMX_ErrorNone ) {
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    /***********************************************************************/
    /* Set the component's OMX_PARAM_PORTDEFINITIONTYPE structure (OUTPUT) */
    /***********************************************************************/
     PRINTF("\nCalling OMX_GetParameter() for output port\n");
     PortType.nStartPortNumber++;
	 
    OutPortDef.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
    OutPortDef.nPortIndex = PortType.nStartPortNumber;

    eError = OMX_GetParameter (pOMXHandle, OMX_IndexParamPortDefinition, &OutPortDef);
    if ( eError != OMX_ErrorNone ) {
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    //OutPortDef.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
    OutPortDef.nVersion.s.nVersionMajor = 0x1;
    OutPortDef.nVersion.s.nVersionMinor = 0x0;
    OutPortDef.nVersion.s.nRevision = 0x0;
    OutPortDef.nVersion.s.nStep = 0x0;
    OutPortDef.eDir = OMX_DirOutput;
    OutPortDef.nBufferCountActual = 1;
    OutPortDef.nBufferCountMin = 1;
    OutPortDef.bEnabled = OMX_TRUE;
    OutPortDef.bPopulated = OMX_FALSE;
    OutPortDef.eDomain = OMX_PortDomainImage;
    OutPortDef.format.image.pNativeRender = NULL;
    OutPortDef.format.image.nFrameWidth = mWidth;
    OutPortDef.format.image.nFrameHeight = mHeight;
    OutPortDef.format.image.nStride = -1;
    OutPortDef.format.image.nSliceHeight = -1;
    OutPortDef.format.image.bFlagErrorConcealment = OMX_FALSE;
    OutPortDef.format.image.eCompressionFormat = OMX_IMAGE_CodingUnused;
    OutPortDef.nBufferSize = mOutBuffSize;

   //[20091226 Ratnesh
    if(mIsPixelFmt420p){
		OutPortDef.format.image.eColorFormat = OMX_COLOR_FormatYUV420PackedPlanar;	
     }
     else{
		OutPortDef.format.image.eColorFormat = OMX_COLOR_FormatCbYCrY;
      }
   //]

    
    
    if (OutPortDef.eDir == nIndex1 ) {
        OutPortDef.nPortIndex = nIndex1;
    }
    else {
        OutPortDef.nPortIndex = nIndex2;
    }
	
     PRINTF("\nCalling OMX_SetParameter for output port\n");
    eError = OMX_SetParameter (pOMXHandle, OMX_IndexParamPortDefinition, &OutPortDef);
    if ( eError != OMX_ErrorNone ) {
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }
	
    PRINTF("\nCalling OMX_UseBuffer for input port\n");
    eError = OMX_UseBuffer(pOMXHandle, &pInBuffHead,  InPortDef.nPortIndex,  (void *)&nCompId, InPortDef.nBufferSize, (OMX_U8*)mInputBuffer);
    if ( eError != OMX_ErrorNone ) {
        PRINTF ("JPEGEnc test:: %d:error= %x\n", __LINE__, eError);
        goto EXIT;
    }

    PRINTF("\nCalling OMX_UseBuffer for output port\n");
    eError = OMX_UseBuffer(pOMXHandle, &pOutBuffHead,  OutPortDef.nPortIndex,  (void *)&nCompId, OutPortDef.nBufferSize, (OMX_U8*)mOutputBuffer);
    if ( eError != OMX_ErrorNone ) {
        PRINTF ("JPEGEnc test:: %d:error= %x\n", __LINE__, eError);
        goto EXIT;
    }

    pInBuffHead->nFilledLen = pInBuffHead->nAllocLen;

    PRINTF("\nCalling OMX_SendCommand for OMX_StateIdle state \n");
    eError = OMX_SendCommand(pOMXHandle, OMX_CommandStateSet, OMX_StateIdle ,NULL);
    if ( eError != OMX_ErrorNone ) {
        PRINTF ("Error from SendCommand-Idle(Init) State function\n");
        goto EXIT;
    }

    Run();

    return true;

EXIT:

    return false;

}

bool JpegDecoder::decodeImage(void* outputBuffer, int outBuffSize, void *inputBuffer, int inBuffSize, int width, int height, int quality,int isPixelFmt420p)
{
    LOGD(" \n JpegDecoder::decodeImage() called \n");
    bool ret = true;

    PRINTF("\niState = %d", iState);
    PRINTF("\nwidth = %d", width);
    PRINTF("\nheight = %d", height);
    PRINTF("\nquality = %d", quality);			
    PRINTF("\ninBuffSize = %d", inBuffSize);				
    PRINTF("\noutBuffSize = %d", outBuffSize);	
    PRINTF("\nisPixelFmt420p = %d", isPixelFmt420p);			

#if JPEG_DECODER_DUMP_INPUT_AND_OUTPUT

    char path[50];
    snprintf(path, sizeof(path), "/temp/JEI_%d_%dx%d.raw", eInputCount, width, height);

    PRINTF("\nrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrr");
    
    SkFILEWStream tempFile(path);
    if (tempFile.write(inputBuffer, inBuffSize) == false)
        PRINTF("\nWriting to %s failed\n", path);
    else
        PRINTF("\nWriting to %s succeeded\n", path);

    eInputCount++;
    PRINTF("\nrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrr");
#endif
#if OPTIMIZE
    if (
        (mWidth == width) &&
        (mHeight == height) &&
        (mQuality == quality) &&
        (iState == STATE_EMPTY_BUFFER_DONE_CALLED)
    )
    {
        PRINTF("\nI am already in EXECUTE state and all input params are same as before.");

        iState = STATE_EXECUTING;
        pInBuffHead->pBuffer = (OMX_U8*)inputBuffer;
        pInBuffHead->nFilledLen  = inBuffSize;
        //pInBuffHead->nFlags = OMX_BUFFERFLAG_EOS;
        pOutBuffHead->pBuffer = (OMX_U8*)outputBuffer;
        sem_post(semaphore);
        Run();

    }
    else
#endif    
    {
        mOutputBuffer = outputBuffer;
        mOutBuffSize = outBuffSize;
        mInputBuffer = inputBuffer;
        mInBuffSize = inBuffSize;
        mWidth = width;
        mHeight = height;
        mQuality = quality;
	 mIsPixelFmt420p = isPixelFmt420p;
        iLastState = STATE_LOADED;
        iState = STATE_LOADED;
    
        ret = StartFromLoadedState();
        if (ret == false)
        {
            PRINTF("\nThe image cannot be decoded for some reason");
            return  false;
        }
    }

    return true;

EXIT:

    return false;

}



void JpegDecoder::PrintState()
{
    
    switch(iState)
    {
        case STATE_LOADED:
            PRINTF("\n\nCurrent State is STATE_LOADED.\n");
            break;

        case STATE_IDLE:
            PRINTF("\n\nCurrent State is STATE_IDLE.\n");
            break;

        case STATE_EXECUTING:
            PRINTF("\n\nCurrent State is STATE_EXECUTING.\n");
            break;

        case STATE_EMPTY_BUFFER_DONE_CALLED:
            PRINTF("\n\nCurrent State is STATE_EMPTY_BUFFER_DONE_CALLED.\n");
            break;

        case STATE_FILL_BUFFER_DONE_CALLED:
            PRINTF("\n\nCurrent State is STATE_FILL_BUFFER_DONE_CALLED.\n");
            break;

        case STATE_ERROR:
            PRINTF("\n\nCurrent State is STATE_ERROR.\n");
            break;

        case STATE_EXIT:
            PRINTF("\n\nCurrent State is STATE_EXIT.\n");
            break;

        default:
            PRINTF("\n\nCurrent State is Invalid.\n");
            break;

    }
}

void JpegDecoder::Run()
{
    LOGD(" \n JpegDecoder::Run() called \n");
    int nRead;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

while(1){
    if (sem_wait(semaphore))
    {
        PRINTF("\nsem_wait returned the error");
        continue;
    }

    PrintState();

    switch(iState)
    {
        case STATE_IDLE:
            if (iLastState == STATE_LOADED)
            {
                eError = OMX_SendCommand(pOMXHandle,OMX_CommandStateSet, OMX_StateExecuting, NULL);
                if ( eError != OMX_ErrorNone ) {
                    PRINTF("\n Error from OMX_SendCommand for OMX_StateExecuting state\n");
                    iState = STATE_ERROR;
                    break;
                }
            }
            else if (iLastState == STATE_EMPTY_BUFFER_DONE_CALLED)
            {
                    eError = OMX_SendCommand(pOMXHandle,OMX_CommandStateSet, OMX_StateLoaded, NULL);
                    if ( eError != OMX_ErrorNone ) {
                        PRINTF("\nError from OMX_SendCommand for OMX_StateLoaded state\n");
                        iState = STATE_ERROR;
                        break;
                    }
                    
                    /* Free buffers */
                    eError = OMX_FreeBuffer(pOMXHandle, InPortDef.nPortIndex, pInBuffHead);
                    if ( eError != OMX_ErrorNone ) {
                        PRINTF("\nError from OMX_FreeBuffer for Input port.\n");
                        iState = STATE_ERROR;
                        break;
                    }

                    eError = OMX_FreeBuffer(pOMXHandle, OutPortDef.nPortIndex, pOutBuffHead);
                    if ( eError != OMX_ErrorNone ) {
                        PRINTF("\nError from OMX_FreeBuffer for Output port.\n");
                        iState = STATE_ERROR;
                        break;
                    }
            }
            else
            {
                iState = STATE_ERROR;
            }
            break;

        case STATE_EXECUTING:

            OMX_EmptyThisBuffer(pOMXHandle, pInBuffHead);
            OMX_FillThisBuffer(pOMXHandle, pOutBuffHead);
            break;

        case STATE_EMPTY_BUFFER_DONE_CALLED:
#if OPTIMIZE        
            // do nothing
#else
            eError = OMX_SendCommand(pOMXHandle,OMX_CommandStateSet, OMX_StateIdle, NULL);
            if ( eError != OMX_ErrorNone ) {
                PRINTF("Error from SendCommand-Idle(nStop) State function\n");
                iState = STATE_ERROR;
            }
#endif        
            break;

        case STATE_LOADED:
        case STATE_ERROR:
            /*### Assume that the Bitmap object/file need not be closed. */
            /*### More needs to be done here */
            /*### Do different things based on iLastState */

            if (pOMXHandle) {
                eError = TIOMX_FreeHandle(pOMXHandle);
                if ( (eError != OMX_ErrorNone) )    {
                    PRINTF("\nError in Free Handle function\n");
                }
            }

            eError = TIOMX_Deinit();
            if ( eError != OMX_ErrorNone ) {
                PRINTF("\nError returned by TIOMX_Deinit()\n");
            }

            iState = STATE_EXIT;

            break;

        default:
            break;
    }

    if (iState == STATE_ERROR) sem_post(semaphore) ;
    if (iState == STATE_EXIT) break;
#if OPTIMIZE
    if (iState == STATE_EMPTY_BUFFER_DONE_CALLED) break ;    
#endif    

    }

}

