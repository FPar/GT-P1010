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

/*
 * Author: Srini Gosangi <srini.gosangi@windriver.com>
 * Author: Michael Barabanov <michael.barabanov@windriver.com>
 */

#ifndef ANDROID_SURFACE_OUTPUT_OMAP34XXH_INCLUDED
#define ANDROID_SURFACE_OUTPUT_OMAP34XXH_INCLUDED

#include "android_surface_output.h"
#include "buffer_alloc_omap34xx.h"
#include "overlay_common.h"

// support for shared contiguous physical memory
#include <ui/Overlay.h>

//START:NCB-TI Changes for CSR: OMAPS00230025
//SoftCodec Thread Implementation
#include <utils/threads.h>
#include "MessageQueue.h"
#include <dlfcn.h>
#include "TI_ClrCnvrt_Interface.h"
#define NEON_OPT
//END:NCB-TI Changes for CSR: OMAPS00230025

// there will be video_out.yuv in data, if DUMP_VIDEO_YUV is defined.
//#define DUMP_VIDEO_YUV

#ifdef DUMP_VIDEO_YUV
#include <stdio.h>
#endif

using namespace android;


class AndroidSurfaceOutputOmap34xx : public AndroidSurfaceOutput
{
public:
    AndroidSurfaceOutputOmap34xx();

    // frame buffer interface
    virtual bool initCheck();
    virtual void setParametersSync(PvmiMIOSession aSession, PvmiKvp* aParameters, int num_elements, PvmiKvp * & aRet_kvp);
    virtual PVMFStatus getParametersSync(PvmiMIOSession aSession, PvmiKeyType aIdentifier,
            PvmiKvp*& aParameters, int& num_parameter_elements, PvmiCapabilityContext aContext);
    virtual PVMFStatus writeFrameBuf(uint8* aData, uint32 aDataLen, const PvmiMediaXferHeader& data_header_info, int aIndex);
    virtual PVMFCommandId writeAsync(uint8 aFormatType, int32 aFormatIndex, uint8* aData, uint32 aDataLen,
            const PvmiMediaXferHeader& data_header_info, OsclAny* aContext);
    virtual void closeFrameBuf();
    virtual void postLastFrame();
    OSCL_IMPORT_REF ~AndroidSurfaceOutputOmap34xx();

    BufferAllocOmap34xx   mbufferAlloc;
    
private:

//START:NCB-TI Changes for CSR: OMAPS00230025
    /* Thread Implementation for Software Codec */
    class SoftCodecThread : public Thread {
        AndroidSurfaceOutputOmap34xx* mHardware;
        public:
            SoftCodecThread(AndroidSurfaceOutputOmap34xx* hw)
                : Thread(false), mHardware(hw) { }

            virtual bool threadLoop() {
                mHardware->softcodecThread();

                return false;
            }
    }; 

    void  softcodecThread();
    PVMFStatus displayFrame(int);
//END:NCB-TI Changes for CSR: OMAPS00230025

    bool            mUseOverlay;
    sp<Overlay>     mOverlay;
    int             bufEnc;
    int32           iNumberOfBuffers;
    int32           iBufferSize;
    bool            mConvert;
    int             mOptimalQBufCnt;
    PVMFFormatType  iVideoCompressionFormat;
    int             iBytesperPixel;
    int             icropY;
    int             icropX;
    int             mBuffersQueuedToDSS;

//START:NCB-TI Changes for CSR: OMAPS00230025
    int             mQbufIndex;
    int             mQbufCount;
#ifdef NEON_OPT  //NCB-TI  
     NEON_clrcnvt_fpo Neon_ClrCnvrt;
     NEON_CLRCNVRT_ARGS* neon_args;
     void* pTIcnvr;
#endif //NEON_OPT

    bool mFalseVideo;
    bool mVideoRunning;
    mutable Mutex mLock;
    sp<SoftCodecThread>	mSoftCocecThread; 
    enum SoftCodecThreadComamnds 
    {
        //Commands
        THREAD_START = 0,
        THREAD_KILL,
        // ACKs
        THREAD_ACK,
        THREAD_NACK
    };

    MessageQueue softcodecThreadCommandQ;
    MessageQueue softcodecThreadAckQ;
    /* End of Thread Implementation */
//END:NCB-TI Changes for CSR: OMAPS00230025

#ifdef DUMP_VIDEO_YUV
    FILE* fp_video_output;
#endif   
};

#endif // ANDROID_SURFACE_OUTPUT_OMAP34XX_H_INCLUDED
