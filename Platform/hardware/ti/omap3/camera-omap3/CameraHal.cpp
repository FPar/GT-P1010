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
/**
 * @file CameraHal.cpp
 *
 * This file maps the Camera Hardware Interface to V4L2.
 *
 */
#undef LOG_TAG
#define LOG_TAG "CameraHal"

#include "CameraHal.h"
#include <cutils/properties.h>

//CAM_MEM
#define USE_MEMCOPY_FOR_VIDEO_FRAME 0
#define USE_NEW_OVERLAY 1
//CAM_MEM
//Eclair L25.14
#define HD_WIDTH       1280
#define HD_HEIGHT      720
#define WIDE_WIDTH     1280
#define WIDE_HEIGHT    720
#define QQVGA_WIDTH    160
#define QQVGA_HEIGHT   120
#define QCIF_WIDTH     176
#define QCIF_HEIGHT    144
#define WQQVGA_WIDTH   200
#define WQQVGA_HEIGHT  120
#define QVGA_WIDTH     320
#define QVGA_HEIGHT    240
#define CIF_WIDTH      352
#define CIF_HEIGHT     288
#define WQVGA_WIDTH    400
#define WQVGA_HEIGHT   240
#define VGA_WIDTH      640
#define VGA_HEIGHT     480
#define D1_WIDTH       720
#define D1_HEIGHT      480
#define WVGA_WIDTH     800
#define WVGA_HEIGHT    480
#define SVGA_WIDTH     800
#define SVGA_HEIGHT    600
#define WSVGA_WIDTH    1024
#define WSVGA_HEIGHT   600

namespace android {
	/*****************************************************************************/

	/*
	 * This is the overlay_t object, it is returned to the user and represents
	 * an overlay. here we use a subclass, where we can store our own state.
	 * This handles will be passed across processes and possibly given to other
	 * HAL modules (for instance video decode modules).
	 */
	struct overlay_true_handle_t : public native_handle {
		/* add the data fields we need here, for instance: */
		int ctl_fd;
		int shared_fd;
		int width;
		int height;
		int format;
		int num_buffers;
		int shared_size;
		int device_id;    /* Video pipe 1 and video pipe 2 */

	};

	//CAM_MEM
	/* Defined in liboverlay */
	typedef struct {
		int fd;
		size_t length;
		uint32_t offset;
		void *ptr;
		int nQueueToOverlay;   //OVL_PATCH //VIK_DBG  0 Not Q to Ovl, 1 Q to Ovl, -1 Query returned Error.
	} mapping_data_t;

	//[20091023 myungwoo ko
	static const char* wb_value[] = {
		"minus1",       //CAMERA_WB_MIN_MINUS_1,
		"auto",         //CAMERA_WB_AUTO = 1,  /* This list must match aeecamera.h */
		"daylight",     //CAMERA_WB_DAYLIGHT,
		"cloudy-daylight",       //CAMERA_WB_CLOUDY_DAYLIGHT,    
		"incandescent", //CAMERA_WB_INCANDESCENT,
		"fluorescent",   //CAMERA_WB_FLUORESCENT,
		"maxplus1"      //CAMERA_WB_MAX_PLUS_1

	};

#define MAX_WBLIGHTING_EFFECTS 6

	static const char* color_effects[] = {
		"minus1", // to match camera_effect_t 
		"none", //CAMERA_EFFECT_OFF = 1,  /* This list must match aeecamera.h */
		"sharpen", //CAMERA_EFFECT_SHARPEN.
		"purple",//CAMERA_EFFECT_PURPLE,
		"negative", //CAMERA_EFFECT_NEGATIVE,
		"sepia", //CAMERA_EFFECT_SEPIA,
		"aqua",//CAMERA_EFFECT_AQUA,
		"green-tint", //CAMERA_EFFECT_GREEN,
		"blue-tint", //CAMERA_EFFECT_BLUE,
		"pink", //CAMERA_EFFECT_PINK,
		"yellow", //CAMERA_EFFECT_YELLOW,
		"mono", //CAMERA_EFFECT_GRAY,
		"red-tint", //CAMERA_EFFECT_RED,
		"mono", //CAMERA_EFFECT_BW,    
		"antique", //CAMERA_EFFECT_ANTIQUE,    
		"maxplus" // CAMERA_EFFECT_MAX_PLUS_1
	};

#define MAX_COLOR_EFFECTS 15

	static const char* scene_mode[] = {
		"minus1", // to match camera_effect_t 
		"auto", //CAMERA_SCENE_OFF= 1,  /* This list must match aeecamera.h */
		"asd", //CE147_SCENE_ASD,
		"sunset", //CAMERA_SCENE_SUNSET,
		"dusk-dawn", //CAMERA_SCENE_DAWN,
		"candlelight", //CAMERA_SCENE_CANDLELIGHT.
		"beach", //CAMERA_SCENE_BEACH_SNOW,
		"back-light", //CAMERA_BACK-LIGHT(CAMERA_AGAINST_LIGHT) ,    
		"text", //CAMERA_SCENE_TEXT,
		"night", //CAMERA_SCENE_NIGHT,
		"landscape", //CAMERA_SCENE_LANDSCAPE,
		"fireworks", //CAMERA_SCENE_FIREWORK,
		"portrait", //CAMERA_PORTRAIT,
		"fall-color", //CAMERA_SCENE_FALL,    
		"party", //CAMERA_SCENE_INDOORS,
		"sports",//CAMERA_SCENE_SPORTS,
		"maxplus" // CAMERA_SCENE_MAX_PLUS_1
	};

#define MAX_SCENE_MODE 16

	static const char* flash_mode[] = {
		"minus1", // to match camera_effect_t 
		"off", //CAMERA_FLASH_OFF = 1,  /* This list must match aeecamera.h */
		"on", //CAMERA_FLASH_ON,
		"auto", //CAMERA_FLASH_AUTO,
		"torch", //CAMERA_FLASH_TORCH,
		"maxplus" // CAMERA_FLASH_MAX_PLUS_1
	};

#define MAX_FLASH_MODE 5
	//[ 20091106 myungwoo
	static const char* focus_mode[] = {
		"minus1", // to match focus_mode 
		"auto", //CAMERA_FOCUS_AUTO = 1,  /* This list must match aeecamera.h */
		"macro", //CAMERA_FOCUS_MACRO,
		"facedetect", //CAMERA_FOCUS_FACEDETECT,
		"maxplus" // CAMERA_FOCUS_MAX_PLUS_1
	};

#define MAX_FOCUS_MODE 4

	static const char* metering_mode[] = {
		"minus1", // to match metering 
		"matrix", //CAMERA_METERING_MATRIX = 1,  /* This list must match aeecamera.h */    
		"center", //CAMERA_METERING_CENTER,
		"spot", //CAMERA_METERING_SPOT,
		"maxplus" // CAMERA_METERING_MAX_PLUS_1
	};

#define MAX_METERING 4
	//]
#ifdef HALO_ISO
#define MAX_ISO 7

	static const char* iso_mode[] = {
		"minus1", // to match metering
		"auto",   // CAMERA_ISO_AUTO = 1,  /* This list must match aeecamera.h */
		"50",     // CAMERA_ISO_50
		"100",    // CAMERA_ISO_100
		"200",    // CAMERA_ISO_200
		"400",    // CAMERA_ISO_400
		"800"     // CAMERA_ISO_800
	};
#endif
	static const char* anti_banding_values[] = {
		"off",	//CAMERA_ANTIBANDING_OFF, as defined in qcamera/common/camera.h
		"60hz",	//CAMERA_ANTIBANDING_60HZ,
		"50hz",	//CAMERA_ANTIBANDING_50HZ,
		"auto",	//CAMERA_ANTIBANDING_AUTO,
		"max"		//CAMERA_MAX_ANTIBANDING,
	};
#define MAX_ANTI_BANDING_VALUES 5
#define MAX_ZOOM_STEPS 23
#define MAX_ZOOM 30

#define CLEAR(x) memset (&(x), 0, sizeof (x))


	/* M4MO Auto Focus Mode */
#define AF_START           1
#define AF_STOP            2
#define CAF_START            5
	//]

#define FLASH_AF_OFF          1
#define FLASH_AF_ON           2
#define FLASH_AF_AUTO         3
#define FLASH_AF_TORCH        4

#define FLASH_MOVIE_OFF       1
#define FLASH_MOVIE_ON        2
#define FLASH_MOVIE_AUTO			3

#define ISO_AUTO        1
#define ISO_50          2
#define ISO_100         3
#define ISO_200         4
#define ISO_400         5
#define ISO_800         6

#define JPEG_SUPERFINE        1
#define JPEG_FINE             2
#define JPEG_NORMAL           3
#define JPEG_ECONOMY          4
#define JPEG_SUPERFINE_limit  75   
#define JPEG_FINE_limit       50
#define JPEG_NORMAL_limit     25

#define YUV_SUPERFINE   100
#define YUV_FINE        75
#define YUV_ECONOMY     50
#define YUV_NORMAL      25

#define DISPLAYFPS 15

	int CameraHal::camera_device = NULL;
	wp<CameraHardwareInterface> CameraHal::singleton;

	CameraHal::CameraHal(int cameraId)
		:mParameters(),
		mOverlay(NULL),
		mPreviewRunning(0),
		mRecordingFrameCount(0),
		mRecordingFrameSize(0),
		// Eclair HAL
		mNotifyCb(0),
		mDataCb(0),
		mDataCbTimestamp(0),
		mCallbackCookie(0),
		mMsgEnabled(0),
		mRecordEnabled(0),
		mVideoBufferCount(0),
		mVideoHeap(0),
		mVideoConversionHeap(0),
		mHeapForRotation(0),
		nOverlayBuffersQueued(0),
		nCameraBuffersQueued(0),   
		mfirstTime(0),
		pictureNumber(0),
		file_index(0),
		mflash(2),
		mcapture_mode(1),
		mcaf(0),
		j(0),
		myuv(3),
		mMMSApp(0),
		mCurrentTime(0),
		mCaptureFlag(0),
		mCamera_Mode(CAMERA_MODE_JPEG),
		//mCameraIndex(MAIN_CAMERA),
		mCameraIndex(cameraId),
		mYcbcrQuality(75),
		mASDMode(false),
		mPreviewFrameSkipValue(0),
		mCamMode(1),
		mCameraMode(1), 
		mSamsungCamera (0),   
		mCounterSkipFrame(0),
		mSkipFrameNumber(0),
		mPreviewSkip(0),
		mPassedFirstFrame(0),
		// OVL_PATCH [[
		mDSSActive (false),                   //OVL_PATCH
		dequeue_from_dss_failed(0),
		// OVL_PATCH ]]
		mOldResetCount(0),
		mBufferCount_422(0),
		m_chk_dataline(0),
		m_chk_dataline_end(false),
#ifdef SAMSUNG_SECURITY
		m_cur_security(0),
#endif	
		mPreviousGPSLatitude(0),
		mPreviousGPSLongitude(0),
		mPreviousGPSAltitude(0),
		mPreviousGPSTimestamp(0),
		mQuikMode(0),
		mIsResized(0)
	{
#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS
		gettimeofday(&ppm_start, NULL);
#endif
		const char* error;

		isStart_JPEG = false;
		isStart_Scale = false;
		mFalsePreview = false;  //Android HAL
		mRecorded = false;  	//Android HAL
		mAutoFocusRunning = false;   
		iOutStandingBuffersWithEncoder = 0;
#ifdef OMAP_ENHANCEMENT	  
		jpegEncoder = NULL;
#endif
#ifdef FOCUS_RECT
		focus_rect_set = 0;
#endif
		Neon_Rotate = NULL;
		neon_args = NULL;
		pTIrtn = NULL;

		//Get the handle of rotation shared library.

		pTIrtn = dlopen("librotation.so", RTLD_LOCAL | RTLD_LAZY);
		if (!pTIrtn) {
			LOGE("Open NEON Rotation Library Failed \n");
		}

		Neon_Rotate = (NEON_fpo) dlsym(pTIrtn, "Neon_RotateCYCY");

		if ((error = dlerror()) != NULL) {
			LOGE("Couldnot find  Neon_RotateCYCY symbol\n");
			dlclose(pTIrtn);
			pTIrtn = NULL;
		} 
		if(!neon_args)
		{
			neon_args   = (NEON_FUNCTION_ARGS*)malloc(sizeof(NEON_FUNCTION_ARGS));
		}

		for(int i = 0; i < VIDEO_FRAME_COUNT_MAX; i++)
		{
			mVideoBuffer[i] = 0;
			buffers_queued_to_dss[i] = 0;
#ifdef VT_BACKGROUND_SOLUTION
			mVTHeaps[i] = 0;
			mVTBuffer[i] = 0;
#endif //VT_BACKGROUND_SOLUTION
		}

#if 0 /* For the test */
		if(CameraCreate(cameraId) < 0) {
			LOGE("ERROR CameraCreate()\n");
			mNotifyCb(CAMERA_MSG_ERROR,CAMERA_DEVICE_ERROR_FOR_EXIT,0,mCallbackCookie);
		}
#endif

		initDefaultParameters(cameraId);

#ifdef CAMERA_ALGO
		camAlgos = new CameraAlgo();
#endif
		ICaptureCreate();

		mPreviewThread = new PreviewThread(this, cameraId);
		mPreviewThread->run("CameraPreviewThread", PRIORITY_URGENT_DISPLAY);
	}

	void CameraHal::initDefaultParameters(int cameraId)
	{
		CameraParameters p;

		LOG_FUNCTION_NAME
			//[20091023 myungwoo ko
		
		HAL_PRINT("in initDefaultParameters() %d (0:Rear, 1:Front)", cameraId);

		mPreviousWB = 1;
		mPreviousEffect = 1;
		mPreviousAntibanding = 0;
		mPreviousSceneMode = 1;
		mPreviousFlashMode = 0;
		mPreviousBrightness = 5;
		mPreviousExposure = 5;
		mPreviousZoom = 1;
#ifndef HALO_ISO
		mPreviousIso = 1;
#endif
		mPreviousContrast = 4;
		mPreviousSaturation = 4;
		mPreviousSharpness = 4;
		mPreviousWdr = 1;
		mPreviousAntiShake = 1;
		mPreviousFocus = 1;
		mPreviousMetering = 2;
		mPreviousPretty = 1;
		mPreviousQuality = 100;
		mPreviousFlag = 1;
#ifdef HALO_ISO
		mPreviousISO = 1;
#endif
		mPreviousGPSLatitude = 0;
		mPreviousGPSLongitude = 0;
		mPreviousGPSAltitude = 0;
		mPreviousGPSTimestamp = 0;	
		mPreviewWidth = PREVIEW_WIDTH;
		mPreviewHeight = PREVIEW_HEIGHT;

		p.set(p.KEY_ZOOM_SUPPORTED,"false");
		if( cameraId == MAIN_CAMERA) {
#if 0 /* For CTS test */
			//p.set(p.KEY_SUPPORTED_PREVIEW_SIZES,"320x240,640x480,800x600,1024x600");
#else
			p.set(p.KEY_SUPPORTED_PREVIEW_SIZES,"1024x600,800x600,640x480");
#endif
			p.set(p.KEY_SUPPORTED_PICTURE_SIZES,"2048x1536,2048x1232,1600x1200,1600x960,1024x600,800x600");
			p.setPictureSize(PICTURE_WIDTH, PICTURE_HEIGHT);

			p.set(p.KEY_SUPPORTED_FOCUS_MODES,"auto,macro,facedetect");
			p.set(p.KEY_FOCUS_MODE,"auto");
			p.set(p.KEY_FOCUS_DISTANCES, BACK_CAMERA_AUTO_FOCUS_DISTANCES_STR);
		
			p.set(p.KEY_FOCAL_LENGTH, "2.78");


			p.set(p.KEY_SUPPORTED_PREVIEW_FPS_RANGE, "(7000,30000)");
			p.set(CameraParameters::KEY_PREVIEW_FPS_RANGE, "7000,30000");
			p.set(p.KEY_SUPPORTED_PREVIEW_FRAME_RATES, "30,20,15,10,7"); /* FIXME */
			p.setPreviewFrameRate(30);


			p.set(CameraParameters::KEY_SUPPORTED_JPEG_THUMBNAIL_SIZES,
					"400x240,320x240,0x0");
			p.set(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH, "320");
			p.set(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT, "240");

		}
		else { /* Front Camera */
#if 0 /* For CTS test */
			p.set(p.KEY_SUPPORTED_PREVIEW_SIZES,"320x240,640x480");
#else
			p.set(p.KEY_SUPPORTED_PREVIEW_SIZES,"640x480");
#endif
			p.set(p.KEY_SUPPORTED_PICTURE_SIZES,"1280x960");
			p.setPictureSize(1280, 960);
			
			p.set(p.KEY_SUPPORTED_FOCUS_MODES,"fixed");
			p.set(p.KEY_FOCUS_MODE, "fixed");
			p.set(p.KEY_FOCUS_DISTANCES, FRONT_CAMERA_FOCUS_DISTANCES_STR);
			p.set(p.KEY_FOCAL_LENGTH, "2.17");

#if 0			/* For the FPS range - CTS test  */
			p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FPS_RANGE, "(7000,15000)");
			p.set(CameraParameters::KEY_PREVIEW_FPS_RANGE, "7000,15000");
			p.setPreviewFrameRate(15);
			p.set(p.KEY_SUPPORTED_PREVIEW_FRAME_RATES, "15,10,7"); /* FIXME */
#else
			p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FPS_RANGE, "(7000,16000)");
			p.set(CameraParameters::KEY_PREVIEW_FPS_RANGE, "7000,16000");
			p.setPreviewFrameRate(16);
			p.set(p.KEY_SUPPORTED_PREVIEW_FRAME_RATES, "16,10,7"); /* FIXME */
#endif

//			p.remove(p.KEY_ZOOM);
//			p.remove(p.KEY_MAX_ZOOM);
//			p.remove(p.KEY_ZOOM_RATIOS);
//			p.remove(p.KEY_ZOOM_SUPPORTED);


			p.set(CameraParameters::KEY_SUPPORTED_JPEG_THUMBNAIL_SIZES,
					"160x120,0x0");
			p.set(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH, "160");
			p.set(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT, "120");
		}

		/* COMMON Setting */
		p.set(p.KEY_SUPPORTED_PICTURE_FORMATS,"jpeg");
		p.setPictureFormat("jpeg");//p.PIXEL_FORMAT_JPEG
		p.set(p.KEY_SUPPORTED_PREVIEW_FORMATS,"yuv420sp");
		p.set(p.KEY_SUPPORTED_WHITE_BALANCE,"auto,daylight,cloudy-daylight,incandescent,fluorescent");
		p.set(p.KEY_SUPPORTED_EFFECTS,"none,sharpen,mono,negative,sepia,aqua,green,blue,pink,yellow,gray,red,purple,antique");
		p.set(p.KEY_SUPPORTED_SCENE_MODES,"auto,asd,sunset,dusk-dawn,candlelight,beach,back-light,text,night,landscape,fireworks,portrait,fall-color,party,sports");
		p.set(p.KEY_SUPPORTED_FLASH_MODES,"off,on,auto,torch");
		//p.set(p.KEY_ZOOM_SUPPORTED,"true");
		//    p.set("max-zoom","12");
		//p.set(p.KEY_MAX_ZOOM,"30");
		/*	char str[20];
			sprintf(str, "%d", JPEG_THUMBNAIL_WIDTH);
			p.set("jpeg-thumbnail-width",   str);
			sprintf(str, "%d", JPEG_THUMBNAIL_HEIGHT);
			p.set("jpeg-thumbnail-height",  str);
		 */
		p.set(p.KEY_HORIZONTAL_VIEW_ANGLE, "51.2");
		p.set(p.KEY_VERTICAL_VIEW_ANGLE, "39.4");  	   
		//       p.set(p.KEY_ZOOM_RATIOS, "100,125,150,175,200,225,250,275,300,325,350,375,400");	
		//p.set(p.KEY_ZOOM_RATIOS, "100,110,120,130,140,150,160,170,180,190,200,210,220,230,240,250,260,270,280,290,300,310,320,330,340,350,360,370,380,390,400");	   
		p.set(p.KEY_EXPOSURE_COMPENSATION, "0");
		p.set(p.KEY_MAX_EXPOSURE_COMPENSATION, "2");
		p.set(p.KEY_MIN_EXPOSURE_COMPENSATION, "-2");
		p.set(p.KEY_EXPOSURE_COMPENSATION_STEP, "0.5");
		p.set(p.KEY_GPS_LATITUDE, "0");
		p.set(p.KEY_GPS_LONGITUDE, "0");
		p.set(p.KEY_GPS_ALTITUDE, "0");
		p.set(p.KEY_GPS_TIMESTAMP, "0");
		p.set(p.KEY_GPS_PROCESSING_METHOD, "GPS");

		p.set(p.KEY_JPEG_THUMBNAIL_QUALITY, "100");   	

		p.set(p.KEY_EFFECT, p.EFFECT_NONE);
		p.set(p.KEY_WHITE_BALANCE, p.WHITE_BALANCE_AUTO);
		p.set(p.KEY_ANTIBANDING, "off");
		p.set(p.KEY_SUPPORTED_ANTIBANDING, "auto,50hz,60hz,off");	
		p.set(p.KEY_SCENE_MODE, p.SCENE_MODE_AUTO);
		p.set(p.KEY_FLASH_MODE, p.FLASH_MODE_OFF);
		p.set("zoom", "0");
//		p.set(p.KEY_JPEG_QUALITY, "85");
		p.set(mParameters.KEY_JPEG_QUALITY,"100");
		p.set(mParameters.KEY_ROTATION,"0");	
		//Preview
		p.setPreviewSize(PREVIEW_WIDTH, PREVIEW_HEIGHT);
		p.setPreviewFormat("yuv420sp");
		//    p.setPreviewFormat("yuv422i-uyvy");

		//Picture
		//p.setPictureSize(PICTURE_WIDTH, PICTURE_HEIGHT);
		//p.setPictureFormat("jpeg");//p.PIXEL_FORMAT_JPEG

		if (setParameters(p) != NO_ERROR) 
		{
			LOGE("Failed to set default parameters?!\n");
		}

		LOG_FUNCTION_NAME_EXIT

	}


	CameraHal::~CameraHal()
	{
		int err = 0;
		LOG_FUNCTION_NAME

/* Move it for fixing the random CTS failure, referred from CL 1108714 */
#if 0 

			if(mPreviewThread != NULL) 
			{
				Message msg;
				msg.command = PREVIEW_KILL;
				previewThreadCommandQ.put(&msg);
				previewThreadAckQ.get(&msg);
			}

		sp<PreviewThread> previewThread;

		{ // scope for the lock
			Mutex::Autolock lock(mLock);
			previewThread = mPreviewThread;
		}

		// don't hold the lock while waiting for the thread to quit
		if (previewThread != 0) 
		{
			previewThread->requestExitAndWait();
		}
#if 0 /* Clear Capture memory */
		if(mVGAYUVPictureHeap != NULL)
			mVGAYUVPictureHeap.clear();
		if(mVGAYUVPictureBuffer != NULL)
			mVGAYUVPictureBuffer.clear();
		if (mPictureBuffer != NULL)
			mPictureBuffer.clear();
		if(mPictureHeap != NULL)
			mPictureHeap.clear();
#endif


		{ // scope for the lock
			Mutex::Autolock lock(mLock);
			mPreviewThread.clear();
		}


#ifdef CAMERA_ALGO
		camAlgos->unInitFaceTracking();

		delete camAlgos;
#endif

#ifdef EVE_CAM

		if(mVideoHeaps_422 != 0)
		{
			mVideoHeaps_422.clear();
			for(int i=0 ; i<VIDEO_FRAME_COUNT_MAX ; i++)
			{
				mVideoBuffer_422[i].clear();
			}
			mVideoHeaps_422 = 0;
			for(int i=0 ; i<VIDEO_FRAME_COUNT_MAX ; i++)
			{
				mVideoBuffer_422[i] = 0;
			}
		}


		if(neon_args)
		{
			free((NEON_FUNCTION_ARGS *)neon_args);
		}	
#endif //EVE_CAM

#ifdef VT_BACKGROUND_SOLUTION
		for(int i = 0; i < VIDEO_FRAME_COUNT_MAX; i++)
		{
			if(mVTHeaps[i] != 0)
			{
				mVTHeaps[i].clear();
				mVTBuffer[i].clear();
			}
		}
#endif //VT_BACKGROUND_SOLUTION

		mRecordEnabled = false; // Eclair HAL
		mFalsePreview =false;   // Eclair HAL
		mRecorded = false;      // Eclair HAL
		mCallbackCookie = NULL; // Eclair HAL

		mCurrentTime = 0;
		ICaptureDestroy();

		CameraDestroy();

		if ( mOverlay != NULL && mOverlay.get() != NULL )		// Latona TD/Heron : VT_BACKGROUND_SOLUTION 
		{
			HAL_PRINT("Destroying current overlay\n");
			mOverlay->destroy();
			mOverlay = NULL;
			nOverlayBuffersQueued = 0;
			// OVL_PATCH [[
			mDSSActive = false;                  //OVL_PATCH
			// OVL_PATCH ]]
		}
		if(pTIrtn != NULL)
		{
			dlclose(pTIrtn);  
			pTIrtn = NULL;
			LOGD("Unloaded NEON Rotation Library");
		}

#endif
		singleton.clear();

		HAL_PRINT("<<< Release >>>\n");
	}

	void CameraHal::facetrackingThread()
	{
	}

	static void debugShowFPS()
	{
		static int mFrameCount = 0;
		static int mLastFrameCount = 0;
		static nsecs_t mLastFpsTime = 0;
		static float mFps = 0;
		mFrameCount++;
		if (mFrameCount==150) 
		{
			nsecs_t now = systemTime();
			nsecs_t diff = now - mLastFpsTime;
			mFps =  (mFrameCount * float(s2ns(1))) / diff;
			mLastFpsTime = now;
			LOGD("####### [%d] Frames, %f FPS\n", mFrameCount, mFps);
			mFrameCount = 0;
		}

		// XXX: mFPS has the value we want
	}

	int CameraHal::beginPictureThread(void *cookie)
	{
		//LOGV("%s()", __FUNCTION__);
		CameraHal *c = (CameraHal*)cookie;
		return c->CapturePicture();
	}

	void CameraHal::previewThread(int cameraId)
	{
		Message msg;
		bool  shouldLive = true;
		bool has_message;
		int err; 
		int frameCount = 0 ;

		struct v4l2_control vc;
		CLEAR(vc);    

		LOG_FUNCTION_NAME

		while(shouldLive) 
		{
			has_message = false;

			if( mPreviewRunning && !m_chk_dataline_end )
			{
				if(mASDMode)
				{
					frameCount++;
					if(!(frameCount%20))
					{
						vc.id = V4L2_CID_SCENE;
						vc.value = 0;
						if (ioctl (camera_device, VIDIOC_G_CTRL, &vc) < 0)
						{
							LOGE("getscene fail\n");
						}
#ifdef OMAP_ENHANCEMENT	 
						mNotifyCb(CAMERA_MSG_ASD,vc.value,0,mCallbackCookie);
#endif
						frameCount = 0;
					}

				}
				//process 1 preview frame
				if(mCamMode == VT_MODE || mOverlay != NULL)					
					nextPreview();

				if( !previewThreadCommandQ.isEmpty() ) 
				{
					previewThreadCommandQ.get(&msg);
					has_message = true;
				}
			}
			else
			{
				//block for message
				previewThreadCommandQ.get(&msg);
				has_message = true;
			}

			if( !has_message )
			{
				continue;
			}

			switch(msg.command)
			{
				case PREVIEW_START:
					{
						HAL_PRINT("Receive Command: PREVIEW_START\n");              
						err = 0;

						mFalsePreview =false;   //Eclair HAL

						if( !mPreviewRunning ) 
						{
#ifdef TIMECHECK
							PPM("CONFIGURING CAMERA TO RESTART PREVIEW\n");
#endif 
							if(CameraCreate(cameraId) < 0)
							{
								LOGE("ERROR CameraCreate()\n");
								mFalsePreview = false;
								if(mCamMode == Trd_PART)
								{
									mNotifyCb(CAMERA_MSG_ERROR,CAMERA_DEVICE_ERROR_FOR_RESTART,0,mCallbackCookie);
									msg.command = PREVIEW_ACK;
								}
								else
								{
									mNotifyCb(CAMERA_MSG_ERROR,CAMERA_DEVICE_ERROR_FOR_RESTART,0,mCallbackCookie);
									msg.command = PREVIEW_ACK;
								}
								previewThreadAckQ.put(&msg);
								err = -1;
								break;
							}
#if 1
							if (mOverlay == NULL && !getVTMode()) {
								mFalsePreview = true;
								msg.command = PREVIEW_ACK;
							}else{
#endif
								mFalsePreview = false;
								if (CameraConfigure() < 0)
								{
									LOGE("ERROR CameraConfigure()\n");                    
									if(mCamMode == Trd_PART)
									{
										CameraDestroy();
										msg.command = PREVIEW_NACK;
									}
									else
									{
										mNotifyCb(CAMERA_MSG_ERROR,CAMERA_DEVICE_ERROR_FOR_EXIT,0,mCallbackCookie);
										msg.command = PREVIEW_ACK;
									} 
									previewThreadAckQ.put(&msg);
									err = -1;
									break;
								}

								if (CameraStart() < 0)
								{
									LOGE("ERROR CameraStart()\n");                    
									if(mCamMode == Trd_PART)
									{
										CameraDestroy();
										msg.command = PREVIEW_NACK;
									}
									else
									{
										mNotifyCb(CAMERA_MSG_ERROR,CAMERA_DEVICE_ERROR_FOR_EXIT,0,mCallbackCookie);
										msg.command = PREVIEW_ACK;
									} 
									previewThreadAckQ.put(&msg);
									err = -1;
									break;
								}     
							}							
#ifdef TIMECHECK
							PPM("PREVIEW STARTED AFTER CAPTURING\n");
#endif

#ifdef CAMERA_ALGO
							if( initAlgos() < 0 )
							{
								LOGE("Error while initializing Camera Algorithms\n");
							}
#endif
						}
						else
						{
							err = -1;
						}

						HAL_PRINT("PREVIEW_START %s\n", err ? "NACK" : "ACK");
						msg.command = err ? PREVIEW_NACK : PREVIEW_ACK;

						if( !err )
						{
							HAL_PRINT("Preview Started!\n");
							mPreviewRunning = true;
							m_chk_dataline_end = false;
						}

						previewThreadAckQ.put(&msg);
					}
					break;

				case PREVIEW_STOP:
					{
						HAL_PRINT("Receive Command: PREVIEW_STOP\n");
						if( mPreviewRunning ) 
						{
							mPreviewRunning = false;
							if( CameraStop() < 0)
							{
								LOGE("ERROR CameraStop()\n"); 
								if(mCamMode == Trd_PART)
								{
									CameraDestroy();
									msg.command = PREVIEW_NACK;
								}
								else
								{
									mNotifyCb(CAMERA_MSG_ERROR,CAMERA_DEVICE_ERROR_FOR_EXIT,0,mCallbackCookie);                        
									msg.command = PREVIEW_ACK;
								}   
							}
							else
							{
								msg.command = PREVIEW_ACK;
							}
							mFalsePreview = false;  //Eclair HAL  
						}
						else
						{
							msg.command = PREVIEW_NACK;
						}

						//mPreviewRunning = false;

						previewThreadAckQ.put(&msg);
					}
					break;
#if 0 // It was changed to thread.
				case PREVIEW_AF_START:
					{
						HAL_PRINT("Receive Command: PREVIEW_AF_START\n");
						err = 0;

						if( camera_device < 0 || !mPreviewRunning )
						{
							LOGD("WARNING PREVIEW NOT RUNNING!\n");
							msg.command = PREVIEW_NACK;
						}
						else
						{
							vc.id = V4L2_CID_AF;
							vc.value = AF_STOP;

							if (ioctl (camera_device, VIDIOC_S_CTRL, &vc) < 0)
								LOGE("release autofocus fail\n");

							if(mMsgEnabled & CAMERA_MSG_FOCUS)
							{
								//[ 20091111 myungwoo ko
								vc.id = V4L2_CID_AF;
								vc.value = AF_START;
								if (ioctl (camera_device, VIDIOC_S_CTRL, &vc) < 0)
								{   
									LOGE("setautofocus fail\n");
									mNotifyCb(CAMERA_MSG_FOCUS,CAMERA_AF_FAIL,0,mCallbackCookie);
									mAFState = false;                                                   
								}
								else
								{
									mAFState = true;
								}
								//]
								msg.command = err ? PREVIEW_NACK : PREVIEW_ACK;   
							}

						}
						HAL_PRINT("Receive Command: PREVIEW_AF_START %s\n", msg.command == PREVIEW_NACK ? "NACK" : "ACK"); 
						previewThreadAckQ.put(&msg);
					}
					break;

				case PREVIEW_AF_CANCEL:
					{
						HAL_PRINT("Receive Command: PREVIEW_AF_CANCEL\n");

						msg.command = PREVIEW_ACK;

						if( camera_device < 0 || !mPreviewRunning )
						{
							LOGD("WARNING PREVIEW NOT RUNNING!\n");
						}
						else
						{
							vc.id = V4L2_CID_AF;
							vc.value = AF_STOP;

							if (ioctl (camera_device, VIDIOC_S_CTRL, &vc) < 0)
							{   
								LOGE("release autofocus fail\n");
							}
							else
							{
							//	mNotifyCb(CAMERA_MSG_FOCUS,CAMERA_AF_CANCEL,0,mCallbackCookie);
							}
							mAFState = false; 
						}
						previewThreadAckQ.put(&msg);
					}
					break;
#endif

				case PREVIEW_CAF_START:
					{
						HAL_PRINT("Receive Command: PREVIEW_CAF_START\n");
						err=0;

						if( camera_device < 0 || !mPreviewRunning )
						{
							msg.command = PREVIEW_NACK;
						}
						else
						{
							msg.command = err ? PREVIEW_NACK : PREVIEW_ACK;
						}
						HAL_PRINT("Receive Command: PREVIEW_CAF_START %s\n", msg.command == PREVIEW_NACK ? "NACK" : "ACK"); 
						previewThreadAckQ.put(&msg);
					}
					break;

				case PREVIEW_CAF_STOP:
					{
						HAL_PRINT("Receive Command: PREVIEW_CAF_STOP\n");
						err = 0;

						if( camera_device < 0 || !mPreviewRunning )
						{
							msg.command = PREVIEW_NACK;
						}
						else
						{
							msg.command = err ? PREVIEW_NACK : PREVIEW_ACK;
						}
						HAL_PRINT("Receive Command: PREVIEW_CAF_STOP %s\n", msg.command == PREVIEW_NACK ? "NACK" : "ACK"); 
						previewThreadAckQ.put(&msg);
					}
					break;

				case PREVIEW_CAPTURE:
					{
						int flg_AF;
						int flg_CAF;
						err = 0;

						HAL_PRINT("ENTER OPTION PREVIEW_CAPTURE\n");
#ifdef TIMECHECK
						PPM("RECEIVED COMMAND TO TAKE A PICTURE\n");
#endif
						//In burst mode the preview is not reconfigured between each picture 
						//so it can not be based on it to decide wheter the state is incorrect or not


						if( camera_device < 0)
						{
							err = -1;
						}
						else 

						{
#ifdef OPP_OPTIMIZATION
							if ( RMProxy_RequestBoost(MAX_BOOST) != OMX_ErrorNone ) 
							{
								LOGE("OPP Boost failed\n");
							} 
							else 
							{
								LOGE("OPP Boost success\n");
							}

#endif
							if(mPreviewRunning)
							{
								if( CameraStop() < 0)
								{
									LOGE("ERROR CameraStop()\n");
									if(mCamMode == Trd_PART)
									{
										CameraDestroy();
										msg.command = CAPTURE_NACK;
									}
									else
									{
										mNotifyCb(CAMERA_MSG_ERROR,CAMERA_DEVICE_ERROR_FOR_RESTART,0,mCallbackCookie);
										msg.command = CAPTURE_ACK;
									}                        
									previewThreadAckQ.put(&msg); 
									break;
								}          
								mPreviewRunning =false;
							}

#ifdef TIMECHECK
							PPM("STOPPED PREVIEW\n");
#endif
							if(mCamMode == Trd_PART) {
								if(createThread(beginPictureThread, this) == false) {
									LOGE("ERROR CapturePicture()\n");    
									CameraDestroy();
									msg.command = CAPTURE_NACK;
									previewThreadAckQ.put(&msg); 
									break;
								}

							} else {

								if (createThread(beginPictureThread, this) == false) {
									msg.command = CAPTURE_NACK;
									previewThreadAckQ.put(&msg);  
									break;
								}
							}
							/*                    if( CapturePicture() < 0)
									      {
									      LOGE("ERROR CapturePicture()\n");    
									      if(mCamMode == Trd_PART)
									      {
									      CameraDestroy();
									      msg.command = CAPTURE_NACK;
									      }
									      else
									      {
									      mNotifyCb(CAMERA_MSG_ERROR,CAMERA_DEVICE_ERROR_FOR_RESTART,0,mCallbackCookie);
									      msg.command = CAPTURE_ACK;
									      }                        
									      previewThreadAckQ.put(&msg); 
									      break;
									      }   
									      break;*/
						}

						HAL_PRINT("EXIT OPTION PREVIEW_CAPTURE\n");
						msg.command = CAPTURE_ACK;
						previewThreadAckQ.put(&msg);  
					}
					break;

				case PREVIEW_CAPTURE_CANCEL:
					{
						HAL_PRINT("Receive Command: PREVIEW_CAPTURE_CANCEL\n");
						if(camera_device < 0)
						{
							LOGE("Camera device null!!\n");
						}
						else
						{
							msg.command = PREVIEW_NACK;
							previewThreadAckQ.put(&msg);                
						}
					}
					break;

				case PREVIEW_FPS:
					{
						enum v4l2_buf_type type;
						struct v4l2_requestbuffers creqbuf;
						HAL_PRINT("Receive Command: PREVIEW_FPS\n");

						if( camera_device < 0 || !mPreviewRunning )
						{
							LOGE("Camera device null!!\n");
						}
						else
						{
							creqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
							if (ioctl(camera_device, VIDIOC_STREAMOFF, &creqbuf.type) == -1) 
							{
								LOGE("VIDIOC_STREAMOFF Failed\n");
							}
							HAL_PRINT("After VIDIOC_STREAMOFF\n");
						
							CameraSetFrameRate();
							
							type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
							err = ioctl(camera_device, VIDIOC_STREAMON, &type);
							if ( err < 0) 
							{
								LOGE("VIDIOC_STREAMON Failed\n");
							}
							HAL_PRINT("After VIDIOC_STREAMON\n");
						}
					}
					break;

				case PREVIEW_KILL:
					{
						LOGD("Receive Command: PREVIEW_KILL\n");
						err = 0;

						if( mPreviewRunning ) 
						{
							if( CameraStop() < 0)
							{
								LOGE("ERROR FW3A_Stop()\n");
								err = -1;
							}
							mPreviewRunning = false;
						}

						msg.command = err ? PREVIEW_NACK : PREVIEW_ACK;
						LOGD("Receive Command: PREVIEW_KILL %s\n", msg.command == PREVIEW_NACK ? "NACK" : "ACK"); 

						previewThreadAckQ.put(&msg);
						shouldLive = false;
					}
					break;
			}
		}

		LOG_FUNCTION_NAME_EXIT
	}

#ifdef CAMERA_ALGO

	status_t CameraHal::initAlgos()
	{
		int w,h;

		mParameters.getPreviewSize(&w, &h);

		return camAlgos->initFaceTracking(FACE_COUNT, w, h, AMFPAF_OPF_EQUAL, FRAME_SKIP, STABILITY, RATIO);
	}

#endif

	int CameraHal::CameraCreate(int cameraId)
	{
		int err = 0;

		LOG_FUNCTION_NAME


		if(!camera_device)
		{
			//LOGD("getCameraSelect value,[%x] (0 : Rear, 1 : Front) \n", cameraId);    
			if(cameraId)
			{
				camera_device = open(VIDEO_DEVICE5, O_RDWR);
				mCamera_Mode = CAMERA_MODE_YUV;
				mCameraIndex = VGA_CAMERA;
			}
			else
			{
				camera_device = open(VIDEO_DEVICE, O_RDWR);
				mCamera_Mode = CAMERA_MODE_JPEG;
				mCameraIndex = MAIN_CAMERA;
			}

			if (camera_device < 0) 
			{
				LOGE ("Could not open the camera device: %s\n",  strerror(errno) );
				if (errno == 2)
				{            	
					LOGE ("No such directroy....");
					mNotifyCb(CAMERA_MSG_ERROR,CAMERA_DEVICE_ERROR_FOR_EXIT,0,mCallbackCookie);
				}
				err = -1;
				goto exit;
			}

#if OMAP_SCALE
			if((!isStart_Scale) && mCameraIndex == VGA_CAMERA)
			{
				if ( scale_init(PREVIEW_WIDTH, PREVIEW_HEIGHT, PREVIEW_WIDTH, PREVIEW_HEIGHT, PIX_YUV422I, PIX_YUV422I) < 0 ) 
				{
					isStart_Scale = false;
					LOGE("scale_init() failed");
				} 
				else 
				{
					isStart_Scale = true;
					LOGD("scale_init() Done");
				}
			}
#endif
		}

		LOG_FUNCTION_NAME_EXIT
		return 0;

exit:
		return err;
	}


	int CameraHal::CameraDestroy()
	{
		int err = 0;
		int buffer_count;
		LOG_FUNCTION_NAME

#if 1 /* For the torch off */	
		if(mPreviousFlashMode == 4) {
			struct v4l2_control vc;            
			CLEAR(vc);
			vc.id = V4L2_CID_FLASH_CAPTURE;
			vc.value = 5; /* Torch off */

			if (ioctl (camera_device, VIDIOC_S_CTRL, &vc) < 0) {
				LOGE("setFlashMode fail\n");
				return UNKNOWN_ERROR; 
			}
		}
#endif
			if(camera_device)
			{
				close(camera_device);
				camera_device = NULL;
			}

		if (mOverlay != NULL) {
			buffer_count = mOverlay->getBufferCount();

			for ( int i = 0 ; i < buffer_count ; i++ )
			{
				// need to free buffers and heaps mapped using overlay fd before it is destroyed
				// otherwise we could create a resource leak
				// a segfault could occur if we try to free pointers after overlay is destroyed
				mVideoBuffer[i].clear();
				mVideoHeaps[i].clear();
				buffers_queued_to_dss[i] = 0;
			}
			mOverlay->destroy();
			mOverlay = NULL;
			nOverlayBuffersQueued = 0;
		}
#if OMAP_SCALE
		if(isStart_Scale)
		{
			err = scale_deinit();
			isStart_Scale= false;
		}

		if( err ) 
			LOGE("scale_deinit() failed\n");
		else 
			LOGD("scale_deinit() OK\n");
#endif
		LOG_FUNCTION_NAME_EXIT
		return 0;
	}

	int CameraHal::CameraConfigure()
	{
		struct v4l2_format format;

		int w, h;
		int image_width, image_height; 
		
		int mVTMode           = getVTMode(); //Vtmode
		int samsungCameraMode = mSamsungCamera; // Samsung/3rd Party    

		LOG_FUNCTION_NAME    

		HAL_PRINT("[%s:%d] mCameraIndex : %d, mCamMode : %d, mVTMode : %d", 
				__func__, __LINE__, mCameraIndex, mCamMode, mVTMode);
#if 1
			switch(mCameraMode)
			{
				case 1:
					{
						if(mVTMode == 1 || mVTMode == 3)
						{
							mCamMode = Trd_PART;
							mQuikMode = true;
						}
						else if(mVTMode == 2)
						{
							mCamMode = VT_MODE;
						}				
						else
						{
							if(samsungCameraMode)
							{
								mCamMode = CAMERA_MODE;
							}
							else
							{
								mCamMode = Trd_PART;
							}
						}
					}
					break;

				case 2:
					{
						if(mVTMode == 1 || mVTMode == 3)
						{
							mCamMode = Trd_PART;
							mQuikMode = true;	
						}
						else if(mVTMode == 2)
						{
							mCamMode = VT_MODE;
						}	
						else
						{
							if(samsungCameraMode)
							{
								mCamMode = CAMCORDER_MODE;
							}
							else
							{
								mCamMode = Trd_PART;
							}
						}
					}
					break;

				default:
					{
						LOGE ("invalid value \n");
					}
					break;
			}    

#endif
		setDatalineCheckStart();

		setWB(getWBLighting());
		setEffect(getEffect());
		setAntiBanding(getAntiBanding());
		setSceneMode(getSceneMode());

		setFlashMode(getFlashMode());

		setExposure(getExposure());
		setZoom(getZoomValue());
		setFocusMode(getFocusMode());
		setJpegMainimageQuality(getJpegMainimageQuality());
#if 0
		if(mCameraIndex == VGA_CAMERA)
			setDatalineCheckStart();
#endif
		setGPSLatitude(getGPSLatitude());
		setGPSLongitude(getGPSLongitude());
		setGPSAltitude(getGPSAltitude());
		setGPSTimestamp(getGPSTimestamp());
		setGPSProcessingMethod(getGPSProcessingMethod());
		setJpegThumbnailSize(getJpegThumbnailSize());	

		if(mSamsungCamera)
		{		
			HAL_PRINT("!!!!!!!!!mSamsungCamera2\n");	
			//setWDRMode(getWDRMode());
			//setAntiShakeMode(getAntiShakeMode());		
			setBrightness(getBrightness());
			setISO(getISO());		
			setContrast(getContrast());		
			setSaturation(getSaturation());		
			setSharpness(getSharpness());
			setMetering(getMetering());	
			setPrettyMode(getPrettyValue());		
		}
		if( (mCameraIndex == MAIN_CAMERA && mCamMode == VT_MODE) 
				|| (mCameraIndex == VGA_CAMERA && mQuikMode == true))
		{
			mParameters.getPreviewSize(&h, &w);
		}
		else
		{
			mParameters.getPreviewSize(&w, &h);
		}

		/* Set preview format */
		format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		format.fmt.pix.width = w;
		format.fmt.pix.height = h;
		format.fmt.pix.pixelformat = PIXEL_FORMAT;

		if (initCameraSetFrameRate())
		{
			LOGE("Error in setting Camera frame rate\n");
			return -1;
		}

		HAL_PRINT("\nHAL_DBG: CameraConfigure ioctl %d x %d\n", w, h);
		if ( ioctl(camera_device, VIDIOC_S_FMT, &format) < 0 )
		{
			LOGE ("Failed to set VIDIOC_S_FMT.\n");
			return -1;
		}


		HAL_PRINT("CameraConfigure PreviewFormat: w=%d h=%d\n", format.fmt.pix.width, format.fmt.pix.height);	

		LOG_FUNCTION_NAME_EXIT
			return 0;
	}

	int CameraHal::CameraStart()
	{
		int w, h;
		int err;
		int nSizeBytes;
		int buffer_count;
		struct v4l2_format format;
		enum v4l2_buf_type type;
		struct v4l2_requestbuffers creqbuf;
		int mPreviewFrameSizeConvert = 0;    

		LOG_FUNCTION_NAME

		nCameraBuffersQueued = 0;  
		nOverlayBuffersQueued = 0;  //VIK_DBG_CHK 


		mCounterSkipFrame = 0 ;

		mPassedFirstFrame = false;

#if 0
		if(mCameraIndex != VGA_CAMERA)
			setDatalineCheckStart();
#endif

		mParameters.getPreviewSize(&w, &h);
		mPreviewWidth = w;
		mPreviewHeight = h;

		HAL_PRINT("**CaptureQBuffers: preview size=%dx%d\n", w, h);

		if(mOverlay != NULL)
		{
			LOGD("OK,mOverlay not NULL\n");
			int cropValue = getCropValue();
			if(cropValue != 0)
			{
				cropValue /= 2;
				mOverlay->setCrop(cropValue,0,w-cropValue,h);
			}
#ifdef VT_BACKGROUND_SOLUTION
			buffer_count = mOverlay->getBufferCount();
			if(buffer_count > VIDEO_FRAME_COUNT_MAX)
				buffer_count = VIDEO_FRAME_COUNT_MAX;
#endif //VT_BACKGROUND_SOLUTION
		}
		else
		{
			LOGD("WARNING, mOverlay is NULL!!\n");
#ifdef VT_BACKGROUND_SOLUTION
			buffer_count = VIDEO_FRAME_COUNT_MAX;
#endif //VT_BACKGROUND_SOLUTION
		}

#ifndef VT_BACKGROUND_SOLUTION
		buffer_count = mOverlay->getBufferCount();
#endif //VT_BACKGROUND_SOLUTION
		nBuffToStartDQ = buffer_count -1;    
		HAL_PRINT("number of buffers = %d\n", buffer_count);

		creqbuf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		creqbuf.memory = V4L2_MEMORY_USERPTR;
		creqbuf.count  =  buffer_count ; 
		if (ioctl(camera_device, VIDIOC_REQBUFS, &creqbuf) < 0) 
		{
			LOGE ("VIDIOC_REQBUFS Failed. %s\n", strerror(errno));
			goto fail_reqbufs;
		}

		mPreviewFrameSize = w * h * 2;
		if (mPreviewFrameSize & 0xfff)
		{
			mPreviewFrameSize = (mPreviewFrameSize & 0xfffff000) + 0x1000;
		}
		HAL_PRINT("mPreviewFrameSize = 0x%x = %d", mPreviewFrameSize, mPreviewFrameSize);
#if OMAP_SCALE
		if(mCameraIndex == VGA_CAMERA && mCamMode != VT_MODE)	//SelfShotMode		
		{	
			mVideoHeap_422.clear();
			mVideoHeap_422 = new MemoryHeapBase(VIDEO_FRAME_COUNT_MAX * mPreviewFrameSize);
		}
#endif
		for (int i = 0; i < (int)creqbuf.count; i++) 
		{
			v4l2_cam_buffer[i].type = creqbuf.type;
			v4l2_cam_buffer[i].memory = creqbuf.memory;
			v4l2_cam_buffer[i].index = i;

			if (ioctl(camera_device, VIDIOC_QUERYBUF, &v4l2_cam_buffer[i]) < 0) 
			{
				LOGE("VIDIOC_QUERYBUF Failed\n");
				goto fail_loop;
			}
#ifdef VT_BACKGROUND_SOLUTION
			if(mOverlay != NULL)
			{
#endif //VT_BACKGROUND_SOLUTION
				if(mCameraIndex == VGA_CAMERA && mCamMode != VT_MODE) {
					mOverlay->setParameter(MIRRORING,1); //selwin added
				}
				else {
					mOverlay->setParameter(MIRRORING,0); //selwin added
				}

				if(mCameraIndex == MAIN_CAMERA &&(mCamMode == CAMERA_MODE || mCamMode == CAMCORDER_MODE))
				{
					mOverlay->setParameter(CHECK_CAMERA,1);
				}
				//CAM_MEM
				mapping_data_t* data = (mapping_data_t*) mOverlay->getBufferAddress((void*)i);
				if ( data == NULL ) 
				{
					LOGE(" getBufferAddress returned NULL");
					goto fail_loop;
				}
#if OMAP_SCALE			
				if(mCameraIndex == VGA_CAMERA && mCamMode != VT_MODE)	//SelfShotMode		
				{					
					mVideoBuffer_422[i] = new MemoryBase(mVideoHeap_422, i * mPreviewFrameSize, mPreviewFrameSize);
					LOGD("mVideoBuffer_422[%d]: Pointer[%p]", i, mVideoBuffer_422[i]->pointer());
					//Assign Pointer
					v4l2_cam_buffer[i].m.userptr = (long unsigned int)mVideoBuffer_422[i]->pointer();			
				}else			
#endif
					v4l2_cam_buffer[i].m.userptr = (unsigned long) data->ptr;
			
				v4l2_cam_buffer[i].length = data->length;

				//CAM_MEM
				strcpy((char *)v4l2_cam_buffer[i].m.userptr, "hello");
				if (strcmp((char *)v4l2_cam_buffer[i].m.userptr, "hello")) 
				{
					LOGI("problem with buffer\n");
					goto fail_loop;
				}

				HAL_PRINT("User Buffer [%d].start = %p  length = %d\n", i,
						(void*)v4l2_cam_buffer[i].m.userptr, v4l2_cam_buffer[i].length);

				if(!data->nQueueToOverlay)
				{
					LOGD("Overlay buffer[%d] is not used. Stats:%d",i,data->nQueueToOverlay);
					buffers_queued_to_dss[i] = 0;
				}
				else
				{
					LOGD("Overlay buffer[%d] is already queued. Stats:%d", i,data->nQueueToOverlay);
					buffers_queued_to_dss[i] = 1;
					nOverlayBuffersQueued++;
				} 

				if (buffers_queued_to_dss[i] == 0)
				{
					if (ioctl(camera_device, VIDIOC_QBUF, &v4l2_cam_buffer[i]) < 0) {
						LOGE("CameraStart VIDIOC_QBUF Failed: %s", strerror(errno) );
						goto fail_loop;
					}else{
						buffers_queued_to_camera_driver[i] = 1;		// Added for CSR - OMAPS00242402				
						nCameraBuffersQueued++;
					}
				}
				else 
				{
					LOGI("CameraStart::Could not queue buffer %d to Camera because it is being held by Overlay\n", i);
				}
#ifdef VT_BACKGROUND_SOLUTION   
			}
			else
			{
				if(mVTHeaps[i] == 0)
				{
					mVTHeaps[i] = new MemoryHeapBase(mPreviewFrameSize);
					mVTBuffer[i] = new MemoryBase(mVTHeaps[i], 0, mPreviewFrameSize);
					LOGD("mVTHeaps[%d]: ID:%d,Base:[%p],size:%d", i, mVTHeaps[i]->getHeapID(), mVTHeaps[i]->getBase() ,mVTHeaps[i]->getSize());
					LOGD("mVTBuffer[%d]: Pointer[%p]", i, mVTBuffer[i]->pointer());
				} 	
				//Assign Pointer
				v4l2_cam_buffer[i].m.userptr = (long unsigned int)mVTBuffer[i]->pointer();

				// Check Memory
				strcpy((char *)v4l2_cam_buffer[i].m.userptr, "hello");
				if (strcmp((char *)v4l2_cam_buffer[i].m.userptr, "hello")) 
				{
					LOGI("problem with buffer\n");
					goto fail_loop;
				}

				if (ioctl(camera_device, VIDIOC_QBUF, &v4l2_cam_buffer[i]) < 0) 
				{
					LOGE("CameraStart VIDIOC_QBUF Failed: %s", strerror(errno) );
					goto fail_loop;
				}        
				else
				{
					buffers_queued_to_camera_driver[i] = 1;
					nCameraBuffersQueued++;	
				}
			}
#endif //VT_BACKGROUND_SOLUTION    
		}

		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		err = ioctl(camera_device, VIDIOC_STREAMON, &type);
		if ( err < 0) 
		{
			LOGE("VIDIOC_STREAMON Failed\n");
			goto fail_loop;
		}
#ifdef EVE_CAM
#if 0 /* for the test */
		mPreviewFrameSize = w * h * 2;
		mVideoHeap_422.clear();

		if(mVideoHeaps_422 == 0)
		{
			int i;
			mVideoHeaps_422  = new MemoryHeapBase(mPreviewFrameSize * VIDEO_FRAME_COUNT_MAX);
			for(i=0 ; i<VIDEO_FRAME_COUNT_MAX ; i++)
			{
				mVideoBuffer_422[i] = new MemoryBase(mVideoHeaps_422, i * mPreviewFrameSize, mPreviewFrameSize);
			}
		}
#endif
		mPreviewFrameSizeConvert = (w*h*3/2);
		HAL_PRINT("AKMM: Clear the  old  mVideoConversionBuffer memory \n");
		mVideoConversionHeap.clear();
		for( int i = 0; i < buffer_count ; i++)		// Latona TD/Heron : VT_BACKGROUND_SOLUTION
		{
			mVideoConversionBuffer[i].clear();
		}
		HAL_PRINT("Mmap the mVideoConversionBuffer Memory %d\n", mPreviewFrameSizeConvert );

		mVideoConversionHeap = new MemoryHeapBase(mPreviewFrameSizeConvert * buffer_count);		// Latona TD/Heron : VT_BACKGROUND_SOLUTION   
		HAL_PRINT("mVideoConversionHeap ID:%d , Base:[%x],size:%d", mVideoConversionHeap->getHeapID(),
				mVideoConversionHeap->getBase(),mVideoConversionHeap->getSize());
		for(int i = 0; i < buffer_count ; i++)		// Latona TD/Heron : VT_BACKGROUND_SOLUTION
		{

			mVideoConversionBuffer[i] = new MemoryBase(mVideoConversionHeap, mPreviewFrameSizeConvert *i, mPreviewFrameSizeConvert );
			HAL_PRINT("AKMM Conversion Buffer:[%x],size:%d,offset:%d\n", mVideoConversionBuffer[i]->pointer(),mVideoConversionBuffer[i]->size(),mVideoConversionBuffer[i]->offset());
		}
		// ensure we release any stale ref's to sp
		mHeapForRotation.clear();
		mBufferForRotation.clear();

		mHeapForRotation = new MemoryHeapBase(mPreviewFrameSizeConvert);
		mBufferForRotation = new MemoryBase(mHeapForRotation, 0, mPreviewFrameSizeConvert );
#endif //EVE_CAM

		LOG_FUNCTION_NAME_EXIT

			return 0;

fail_loop:
fail_reqbufs:

		return -1;

	}

	int CameraHal::CameraStop()
	{
		int err = 0;
		int af_retry_count = 0;

		LOG_FUNCTION_NAME

#if 0
		while(mAutoFocusRunning) {
			usleep(100*1000);
			af_retry_count++;
			if(af_retry_count > 3) {
				mAutoFocusRunning = false;
			}
		}
#endif
			if(!mFalsePreview)
			{
				struct v4l2_requestbuffers creqbuf;
				struct v4l2_buffer cfilledbuffer;

				int ret,i = 0;

				cfilledbuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
				cfilledbuffer.memory = V4L2_MEMORY_USERPTR;

				mPassedFirstFrame = 0;

				if(getPreviewFlashOn() == 1) {
					setMovieFlash(FLASH_MOVIE_OFF);
				}
				while(nCameraBuffersQueued){
					nCameraBuffersQueued--;
				}
				HAL_PRINT("Done dequeuing from Camera!\n");

				creqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
				if (ioctl(camera_device, VIDIOC_STREAMOFF, &creqbuf.type) < 0) 
				{
					LOGE("VIDIOC_STREAMOFF Failed\n");
					err = -1;
				}

				mVideoHeap.clear();
				mVideoHeap_422.clear();

				for ( int i = 0 ; i < mVideoBufferCount ; i++ )
				{
					// need to free buffers and heaps mapped using overlay fd before it is destroyed
					// otherwise we could create a resource leak
					// a segfault could occur if we try to free pointers after overlay is destroyed

					mVideoBuffer[i].clear();
					mVideoBuffer[i] = 0;	

					mVideoHeaps[i].clear();
					mVideoHeaps[i]  = 0;

					mPreviewBlocks[i] = 0;			
					buffers_queued_to_dss[i] = 0;
				}
#ifdef VT_BACKGROUND_SOLUTION        
				for(i = 0; i < VIDEO_FRAME_COUNT_MAX; i++)
				{
					if(mVTHeaps[i] != 0)
					{
						mVTHeaps[i].clear();
						mVTBuffer[i].clear();
						mVTHeaps[i] = 0;
						mVTBuffer[i] = 0;
					}
				} 
#endif //VT_BACKGROUND_SOLUTION        	
				// Clearing of heap
				mVideoConversionHeap.clear();
				for( int i = 0; i < VIDEO_FRAME_COUNT_MAX ; i++)		// Latona TD/Heron : VT_BACKGROUND_SOLUTION
				{
					mVideoConversionBuffer[i].clear();
				}
				HAL_PRINT("MM Team - camera close ... heap cleared \n");

				mHeapForRotation.clear();
				mBufferForRotation.clear();
#if 0
				if (mOverlay != NULL && true == mRecorded) 
				{
					HAL_PRINT("--------------------Destroy overlay now!------------------\n");
					mOverlay->destroy();
					mOverlay = NULL;
					nOverlayBuffersQueued = 0;
					mRecorded = false;
				}
#endif
				mCounterSkipFrame = 0;
				// Eclair Camera
			}

		LOG_FUNCTION_NAME_EXIT

			return err;
	}
#ifdef EVE_CAM
	void Rotate90YUV422(uint8_t* pIn, uint8_t* pOut, uint8_t* pTempYUV444_1, uint8_t* pTempYUV444_2)
	{
		typedef struct{
			uint8_t u;
			uint8_t y0;
			uint8_t v;
			uint8_t y1;
		} __attribute__ ((packed))UYVY;

		typedef struct{
			uint8_t u;
			uint8_t v;
			uint8_t y;
		} __attribute__ ((packed))UVY;

		UVY *pUVYBuff444   = (UVY *)pTempYUV444_1; 
		UVY *pUVYBuff444_Degree90 = (UVY *)pTempYUV444_2;
		int x, y, i, indexSrc, indexDest;

		for(i=0; i<176*144/2; i++) {
			pUVYBuff444[i*2].u   = pIn[(i*4)];
			pUVYBuff444[i*2+1].u  = pIn[(i*4)];
			pUVYBuff444[i*2].v   = pIn[(i*4)+2];
			pUVYBuff444[i*2+1].v  = pIn[(i*4)+2];
			pUVYBuff444[i*2].y   = pIn[(i*4)+1];
			pUVYBuff444[i*2+1].y  = pIn[(i*4)+3];
		}

		for(y=0 ; y<176 ; y++)
		{
			for(x=0 ; x<144 ; x++)
			{
				memcpy((void*)&pUVYBuff444_Degree90[(x*176+(175-y))], (void*)&pUVYBuff444[(y*144+x)], sizeof(UVY));
			}
		}

		for(i=0 ; i<176*144/2 ; i++) {
			pOut[i*4] = pUVYBuff444_Degree90[i*2].u;
			pOut[i*4+1] = pUVYBuff444_Degree90[i*2].y;
			pOut[i*4+2] = pUVYBuff444_Degree90[i*2].v;
			pOut[i*4+3] = pUVYBuff444_Degree90[i*2+1].y;
		}
	}
#endif //EVE_CAM 

#ifdef SAMSUNG_SECURITY
	int CameraHal::getSecurityCheck(void)
	{
		int androidSystemPropertiesSc = 10;
		char CAM_DPM_PROPERTY[2] = {0,};
		property_get("dpm.allowcamera", CAM_DPM_PROPERTY, "1");
		androidSystemPropertiesSc = atoi(CAM_DPM_PROPERTY);	//Allow:1 | Not allow:0
		//LOGE("func(%s):CAM_DPM_PROPERTY(%s):androidSystemPropertiesSc(%d)", __FUNCTION__,CAM_DPM_PROPERTY,androidSystemPropertiesSc);
		m_security = androidSystemPropertiesSc;
		return m_security;
	}
#endif

	/*
	//NCB-TI
	New nextPreview() code is to make CameraHal compatible with the Inc3.4 Overlay module. 
	The changes are done in the Queueing and Dequeueing of the Preview frame to the Overlay.
	Reset of the code remains unchange.
	//NCB-TI
	 */
	void CameraHal::nextPreview()
	{
		const int RETRY_COUNT = 5;
		mCfilledbuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		mCfilledbuffer.memory = V4L2_MEMORY_USERPTR;
		int ret, queue_to_dss_failed, nBuffers_queued_to_dss;
		overlay_buffer_t overlaybuffer;// contains the index of the buffer dque
		int index;
		int buffer_count;    
		mapping_data_t* data = NULL; 
		bool queueBufferCheck = true;
		int error = 0;
#if OMAP_SCALE
		sp<MemoryHeapBase> 	TempHeapBase;
		sp<MemoryBase>	 	TempBase;
		sp<IMemoryHeap> 	TempHeap;
#endif

#ifdef SAMSUNG_SECURITY
		if(!mSamsungCamera)
		{
			if ((m_cur_security%60) == 59)
			{
				if(!getSecurityCheck())
				{
					CameraDestroy();
					return;
				}
				m_cur_security = 0;
			}
			else
			{
				m_cur_security++;
				//LOGE("============= %d===============\n",m_cur_security);
			}	
		}
#endif	

		/* De-queue the next avaliable buffer */
		if (ioctl(camera_device, VIDIOC_DQBUF, &mCfilledbuffer) < 0)  
		{
			LOGE("Camera ESD shock!!\n");

			CameraStop();
			if(camera_device)
			{
				close(camera_device);
				camera_device = NULL;
				LOGD("Camera Driver unload done!\n");
			}
			//usleep(500*1000);
			if(CameraCreate(mCameraIndex) < 0){
				LOGD("CameraCreate Failed!\n");
				return ;
			}
			if(CameraConfigure() < 0){
				LOGD("CameraConfigure Failed!\n");
				return;
			}
			if(CameraStart()<0){
				LOGD("CameraStart Failed!\n");
				return;
			}
			return ;
#if 0
			LOGE("VIDIOC_DQBUF Failed!!! %d\n", mPassedFirstFrame + 1);
			if(!mPassedFirstFrame) {
				CameraStop();
				if(camera_device) {
					close(camera_device);
					camera_device = NULL;
				}
				CameraCreate(mCameraIndex);
				mPreviewRunning = false;
				startPreview();
				//mNotifyCb(CAMERA_MSG_ERROR,CAMERA_DEVICE_ERROR_FOR_RESTART,0,mCallbackCookie);
			}
			else
				mNotifyCb(CAMERA_MSG_ERROR,CAMERA_DEVICE_ERROR_FOR_EXIT,0,mCallbackCookie);
			return;
#endif
		}
		else {
			if(nCameraBuffersQueued > 0) {
				nCameraBuffersQueued--;
			}
			buffers_queued_to_camera_driver[mCfilledbuffer.index] = 0;
		}
#if OMAP_SCALE
		//HAL_PRINT("[%s:%d] mCameraIndex : %d , mCamMode : %d", __func__, __LINE__, mCameraIndex, mCamMode);
		//if(mCameraIndex == VGA_CAMERA && !mQuikMode && mVideoBuffer_422[mCfilledbuffer.index] != NULL)
		if(mCameraIndex == VGA_CAMERA && (mQuikMode == false) && mVideoBuffer_422[mCfilledbuffer.index] != NULL)
		{
			ssize_t newoffset;
			size_t newsize; 
			int mFrameSizeConvert = (mPreviewWidth*mPreviewHeight*2) ;

			vpp_buffer =  (uint8_t*)mVideoBuffer_422[mCfilledbuffer.index]->pointer();
			mapping_data_t* data = (mapping_data_t*) mOverlay->getBufferAddress((void*)mCfilledbuffer.index);				
			if ( data == NULL ) 				
			{					
				LOGE(" ERROR: getBufferAddress returned NULL");		
				yuv_buffer = NULL;
			}
			else
				yuv_buffer = (uint8_t*) data->ptr;
			if(scale_process(vpp_buffer, mPreviewWidth,mPreviewHeight, yuv_buffer, mPreviewHeight, mPreviewWidth, 0, PIX_YUV422I, 1))
			{
				LOGE("scale_process() failed\n");
			}
			//dumpFrame(vpp_buffer,mPreviewHeight * mPreviewWidth * 2, "/data/scale_front_before.yuv");
			//dumpFrame(yuv_buffer,mPreviewHeight * mPreviewWidth * 2, "/data/scale_front_after.yuv");

			TempHeapBase = new MemoryHeapBase(mFrameSizeConvert);
			TempBase = new MemoryBase(TempHeapBase,0,mFrameSizeConvert);
			TempHeap = TempBase->getMemory(&newoffset, &newsize);
			neon_args->pIn = (unsigned char*)yuv_buffer;
			neon_args->pOut = (unsigned char*)TempHeap->base();   	
			neon_args->width = mPreviewHeight;
			neon_args->height = mPreviewWidth;
			neon_args->rotate = NEON_ROT90;
			error = 0;   
			if (Neon_Rotate != NULL)
				error = (*Neon_Rotate)(neon_args);
			else
				LOGE("Rotate Fucntion pointer Null");

			if (error < 0) {
				LOGE("Error in Rotation 90");
			}
			memcpy((void*)yuv_buffer, (void*)TempHeap->base(), mPreviewWidth*mPreviewHeight*2);

			mIsResized = true;
			//dumpFrame(TempHeap->base(),mPreviewHeight * mPreviewWidth * 2, "/data/vga_rotate.yuv");
		}
		else if(mCameraIndex == VGA_CAMERA &&(mQuikMode == true)) {
			
			vpp_buffer =  (uint8_t*)mVideoBuffer_422[mCfilledbuffer.index]->pointer();
			mapping_data_t* data = (mapping_data_t*) mOverlay->getBufferAddress((void*)mCfilledbuffer.index);				
			if ( data == NULL ) 				
			{					
				LOGE(" ERROR: getBufferAddress returned NULL");		
				yuv_buffer = NULL;
			}
			else
				yuv_buffer = (uint8_t*) data->ptr;
			
			neon_args->pIn = (unsigned char*)vpp_buffer;
			neon_args->pOut = (unsigned char*)yuv_buffer;
			neon_args->width = mPreviewHeight;
			neon_args->height = mPreviewWidth;
			neon_args->rotate = NEON_ROT90;
			error = 0;   
			if (Neon_Rotate != NULL)
				error = (*Neon_Rotate)(neon_args);
			else
				LOGE("Rotate Fucntion pointer Null");

			if (error < 0) {
				LOGE("Error in Rotation 90");
			}

			mIsResized = false;
		}
		else
			mIsResized = false;
#endif
		if(mMsgEnabled & CAMERA_MSG_PREVIEW_FRAME)
		{

			vpp_buffer =  (uint8_t*)mCfilledbuffer.m.userptr;

			if(mCamMode == VT_MODE)
			{
				if(mCameraIndex == MAIN_CAMERA)
				{	
					Neon_Convert_yuv422_to_YUV420P((unsigned char *)vpp_buffer, (unsigned char*)mBufferForRotation->pointer(), mPreviewHeight, mPreviewWidth);
					rotate90_out((unsigned char*)mBufferForRotation->pointer(), (unsigned char *)mVideoConversionBuffer[mCfilledbuffer.index]->pointer(), mPreviewWidth, mPreviewHeight);
				}
				else
				{
					Neon_Convert_yuv422_to_YUV420P((unsigned char *)vpp_buffer,(unsigned char *)mVideoConversionBuffer[mCfilledbuffer.index]->pointer(), mPreviewWidth, mPreviewHeight); 				
				}
				mDataCb( CAMERA_MSG_PREVIEW_FRAME,mVideoConversionBuffer[mCfilledbuffer.index] ,mCallbackCookie);		
			}
			else
			{
				if(mPreviewWidth!=HD_WIDTH)
				{      
					if((mQuikMode == true) && mCameraIndex == VGA_CAMERA) {
#if 1
#if 1
						neon_args->pIn = (unsigned char *)yuv_buffer;
						neon_args->pOut = (unsigned char*)mVideoBuffer_422[mCfilledbuffer.index]->pointer();
						neon_args->width = mPreviewHeight;
						neon_args->height = mPreviewWidth;
						neon_args->rotate = NEON_ROT180;
						error = 0;
						if (Neon_Rotate != NULL)
							error = (*Neon_Rotate)(neon_args);
						else
							LOGE("Rotate Fucntion pointer Null");

						if (error < 0){
							LOGE("Error in Rotation 180");
						}
#endif
						Neon_Convert_yuv422_to_NV21((unsigned char*)mVideoBuffer_422[mCfilledbuffer.index]->pointer(),
								(unsigned char *)mVideoConversionBuffer[mCfilledbuffer.index]->pointer(), 
								mPreviewWidth, mPreviewHeight);	
						mDataCb(CAMERA_MSG_PREVIEW_FRAME,mVideoConversionBuffer[mCfilledbuffer.index] ,mCallbackCookie);				
#else
						//Neon_Convert_yuv422_to_NV21((unsigned char *)vpp_buffer,(unsigned char *)mVideoConversionBuffer[mCfilledbuffer.index]->pointer(), mPreviewWidth, mPreviewHeight);
						Neon_Convert_yuv422_to_NV21((unsigned char *)yuv_buffer,(unsigned char *)mVideoConversionBuffer[mCfilledbuffer.index]->pointer(), mPreviewWidth, mPreviewHeight);
						mDataCb( CAMERA_MSG_PREVIEW_FRAME,mVideoConversionBuffer[mCfilledbuffer.index] ,mCallbackCookie);

#endif
					}
					else {
						if(mIsResized == true)
							Neon_Convert_yuv422_to_NV21((unsigned char *)yuv_buffer,(unsigned char *)mVideoConversionBuffer[mCfilledbuffer.index]->pointer(), mPreviewWidth, mPreviewHeight);
						else
							Neon_Convert_yuv422_to_NV21((unsigned char *)vpp_buffer,(unsigned char *)mVideoConversionBuffer[mCfilledbuffer.index]->pointer(), mPreviewWidth, mPreviewHeight);
						if(!mPreviewSkip)
							mDataCb( CAMERA_MSG_PREVIEW_FRAME,mVideoConversionBuffer[mCfilledbuffer.index] ,mCallbackCookie);
					}
				}
			}
		}

#ifdef CAMERA_ALGO
		int delay;
		AMFPAF_FACERES *faces;

		gettimeofday(&algo_before, 0);
		faces = camAlgos->detectFaces( (uint8_t *) mCfilledbuffer.m.userptr);
		gettimeofday(&algo_after, 0);
		delay = (algo_after.tv_sec - algo_before.tv_sec)*1000;
		delay += (algo_after.tv_usec - algo_before.tv_usec)/1000;
		//LOGD("Facetracking Completed in %d [msec.]", delay);
		if( faces->nFace != 0)
		{
			for( int i = 0; i < faces->nFace; i++)
			{
				drawRect( (uint8_t *) mCfilledbuffer.m.userptr, FOCUS_RECT_GREEN,  faces->rcFace[i].left, faces->rcFace[i].top, faces->rcFace[i].right, faces->rcFace[i].bottom, w, h);
			}
		}
		lastOverlayIndex = mCfilledbuffer.index;
#endif  //#ifdef CAMERA_ALGO
#ifdef CHECK_FRAMERATE
		debugShowFPS();
#endif  //#ifdef CHECK_FRAMERATE
		if(true == mRecordEnabled && iOutStandingBuffersWithEncoder < 3)//dont send more than 4.
		{
			if(mCounterSkipFrame == 0)
			{
				mRecordingLock.lock();
				iOutStandingBuffersWithEncoder++;
				iConsecutiveVideoFrameDropCount = 0;
				mRecordingLock.unlock();

				mCurrentTime = systemTime();

				if(mQuikMode == true){
					if(mCameraIndex == VGA_CAMERA){
#if 1
						neon_args->pIn = (unsigned char*)mCfilledbuffer.m.userptr;
						neon_args->pOut = (unsigned char*)mVideoBuffer_422[mCfilledbuffer.index]->pointer();
						neon_args->width = mPreviewHeight;
						neon_args->height = mPreviewWidth;
						neon_args->rotate = NEON_ROT270;
						error = 0;
						if (Neon_Rotate != NULL)
							error = (*Neon_Rotate)(neon_args);
						else
							LOGE("Rotate Fucntion pointer Null");

						if (error < 0)
						{
							LOGE("Error in Rotation 90");

						}
						mDataCbTimestamp(mCurrentTime,CAMERA_MSG_VIDEO_FRAME,mVideoBuffer_422[mCfilledbuffer.index],mCallbackCookie,0,0);
#else
						mDataCbTimestamp(mCurrentTime,CAMERA_MSG_VIDEO_FRAME,mVideoBuffer[mCfilledbuffer.index],mCallbackCookie,0,0);
						//mDataCbTimestamp(mCurrentTime,CAMERA_MSG_VIDEO_FRAME,mVideoBuffer_422[mCfilledbuffer.index],mCallbackCookie,0,0);
#endif

					}
					else { /* MAIN CAMERA */
						/* We don't have to do anything */
						if( getVTMode() == 3){ /* Recording & Share */
							LOGE("Recording\n");
							mDataCbTimestamp(mCurrentTime,CAMERA_MSG_VIDEO_FRAME,mVideoBuffer[mCfilledbuffer.index],mCallbackCookie,0,0);
						}
						else{ /* VT Call */
							mDataCbTimestamp(mCurrentTime,CAMERA_MSG_VIDEO_FRAME,mVideoBuffer_422[mCfilledbuffer.index],mCallbackCookie,0,0);
						}
					}
				}
				else{
					mDataCbTimestamp(mCurrentTime,CAMERA_MSG_VIDEO_FRAME,mVideoBuffer[mCfilledbuffer.index],mCallbackCookie,0,0);
				}
#if 0
				if(mCamMode == VT_MODE)
				{
					mBufferCount_422++;
					if(mBufferCount_422 >= VIDEO_FRAME_COUNT_MAX)
					{
						mBufferCount_422 = 0;
					}
					if(mCameraIndex == MAIN_CAMERA)
					{
						neon_args->pIn = (unsigned char*)mCfilledbuffer.m.userptr;
						neon_args->pOut = (unsigned char*)mVideoBuffer_422[mBufferCount_422]->pointer();   	
						neon_args->width = mPreviewWidth;
						neon_args->height = mPreviewHeight;
						neon_args->rotate = NEON_ROT90;
						error = 0;   
						if (Neon_Rotate != NULL)
							error = (*Neon_Rotate)(neon_args);
						else
							LOGE("Rotate Fucntion pointer Null");

						if (error < 0) {
							LOGE("Error in Rotation 90");

						}
#ifdef OMAP_ENHANCEMENT	 		
						mDataCbTimestamp(mCurrentTime,CAMERA_MSG_VIDEO_FRAME,mVideoBuffer_422[mBufferCount_422],mCallbackCookie,0,0);	
#endif
					}
					else
					{
						memcpy((void*)mVideoBuffer_422[mBufferCount_422]->pointer(), (void*)mCfilledbuffer.m.userptr, 176*144*2);
#ifdef OMAP_ENHANCEMENT	 
						mDataCbTimestamp(mCurrentTime,CAMERA_MSG_VIDEO_FRAME,mVideoBuffer_422[mBufferCount_422],mCallbackCookie,0,0);
#endif
					}
				}
				else
				{
#ifdef OMAP_ENHANCEMENT	 
					mDataCbTimestamp(mCurrentTime,CAMERA_MSG_VIDEO_FRAME,mVideoBuffer[(int)mCfilledbuffer.index],mCallbackCookie,0,0);
#endif
				}
#endif
			}


			queueBufferCheck = false;

		}
		else
		{
			queueBufferCheck = true;

			if(true == mRecordEnabled && iOutStandingBuffersWithEncoder == 3) //if opencore has 3 buffers
			{
				iConsecutiveVideoFrameDropCount++;
				if(iConsecutiveVideoFrameDropCount % 30 == 0) //alert every 1sec if opencore is not encoding
				{
					LOGE("consecutive drop cnt=%d", iConsecutiveVideoFrameDropCount);
				}
			}
		}
		//Queue Buffer to Overlay    

		if (!mCounterSkipFrame && mOverlay != NULL)	// Latona TD/Heron : VT_BACKGROUND_SOLUTION
		{
			if(!mPreviewSkip) {
				// Normal Flow send the frame for display
				mCounterSkipFrame = mSkipFrameNumber;   //Reset to Zero

				// Notify overlay of a new frame.	
				if(buffers_queued_to_dss[mCfilledbuffer.index] != 1)
				{
					nBuffers_queued_to_dss = mOverlay->queueBuffer((void*)mCfilledbuffer.index);

					if (nBuffers_queued_to_dss < 0)
					{
						LOGE("nextPreview(): mOverlay->queueBuffer(%d) failed",mCfilledbuffer.index);
					}
					else
					{
						nOverlayBuffersQueued++;
						buffers_queued_to_dss[mCfilledbuffer.index] = 1; //queued
						if (nBuffers_queued_to_dss != nOverlayBuffersQueued)
						{
							LOGE("nBuffers_queued_to_dss = %d, nOverlayBuffersQueued = %d", nBuffers_queued_to_dss, nOverlayBuffersQueued);
							LOGE("buffers in DSS \n %d %d %d  %d %d %d", buffers_queued_to_dss[0], buffers_queued_to_dss[1],
									buffers_queued_to_dss[2], buffers_queued_to_dss[3], buffers_queued_to_dss[4], buffers_queued_to_dss[5]);
							index = mCfilledbuffer.index; //NCB-TI Fix for error case queue buff fail
							for(int k = 0; k < MAX_CAMERA_BUFFERS; k++)
							{
								if ((buffers_queued_to_dss[k] == 1) && (k != index))
								{
									buffers_queued_to_dss[k] = 0;
									nOverlayBuffersQueued--;

								}
							}
						}
					}
				}
				else
				{
					LOGE("NCB_DBG:Buffer already with Overlay %d",mCfilledbuffer.index);
				}	
				if (nOverlayBuffersQueued >= NUM_BUFFERS_TO_BE_QUEUED_FOR_OPTIMAL_PERFORMANCE)
				{
					dequeue_from_dss_failed = mOverlay->dequeueBuffer(&overlaybuffer);
					if(dequeue_from_dss_failed){
						//OVL_PATCH		NCB-TI E					
						//[This patch is taken from the Halo CameraHal to handle the cases: Dequeue fail for stream OFF]	
						// Overlay went from stream on to stream off
						// We need to reclaim all of these buffers since the video queue will be lost
						if(mDSSActive)
						{
							unsigned int nDSS_ct = 0;
							LOGD("Overlay went from STREAM ON to STREAM OFF");
							buffer_count =  mOverlay->getBufferCount();
							for(int i =0; i < buffer_count ; i++)
							{
								LOGD("NCB_DBG:Buf Ct %d, DSS[%d] = %d, Ovl Ct %d",buffer_count,i,
										buffers_queued_to_dss[i],nOverlayBuffersQueued);
								if(buffers_queued_to_dss[i] == 1)
								{
									// we need to find out if this buffer was queued after overlay called stream off
									// if so, then we should not reclaim it

									if((data = (mapping_data_t*) mOverlay->getBufferAddress((void*)i)) != NULL)
									{
										if(!data->nQueueToOverlay)
										{
											nOverlayBuffersQueued--;
											buffers_queued_to_dss[i] = 0;
											nDSS_ct++;
										}
										else
										{
											LOGE("Trying to Reclaim[%d] to CameraHAL but is already queued to overlay", i);
										}
									}
									else
									{
										LOGE("Error getBufferAddress couldn't return any pointer.");
									}
								}
							}

							LOGD("Done reclaiming buffers, there are [%d] buffers queued to overlay actually %d", 
									nOverlayBuffersQueued,nDSS_ct);
							mDSSActive = false;
						}
						else
						{
							LOGE("nextPreview(): mOverlay->dequeueBuffer() failed Ct %d\n",nOverlayBuffersQueued);									
						}
						//OVL_PATCH     NCB-TI X
					}
					else	{
						nOverlayBuffersQueued--;
						buffers_queued_to_dss[(int)overlaybuffer] = 0;
						lastOverlayBufferDQ = (int)overlaybuffer;
						//[[        //OVL_PATCH
						mDSSActive = true;
						//]]       //OVL_PATCH
					}
				}
			}
			else {
				mPreviewSkip--;
			}
		}        
		else
		{
			// skip the frame for display
			mCounterSkipFrame--;
			if(mCounterSkipFrame < 0)
				mCounterSkipFrame = 0;
		}

		if(queueBufferCheck)
		{
			if (ioctl(camera_device, VIDIOC_QBUF, &v4l2_cam_buffer[mCfilledbuffer.index]) < 0) 
			{
				LOGE("nextPreview. VIDIOC_QBUF Failed.");
			}
			else
			{
				buffers_queued_to_camera_driver[mCfilledbuffer.index] = 1;   // Added for CSR - OMAPS00242402
				nCameraBuffersQueued++;
			}
		}
	}


#if 0 //def EVE_CAM //NCB-TI
	void CameraHal::PreviewConversion(uint8_t *pInBuffer, uint8_t *pOutBuffer)
	{
		if(mCameraIndex == VGA_CAMERA && mCamMode != VT_MODE && mCaptureFlag)
		{
			if(isStart_VPP)
			{
				if(ColorConvert_Process((char *)pInBuffer,(char *)pOutBuffer))
				{
					LOGE("ColorConvert_Process() failed\n");
				}
			}
		}
		else
		{
			if(!mCaptureFlag)
			{
				if(mCamMode == VT_MODE)
				{
					if(mCameraIndex == MAIN_CAMERA)
					{
						Neon_Convert_yuv422_to_YUV420P((unsigned char *)pInBuffer, (unsigned char*)mBufferForRotation->pointer(), mPreviewHeight, mPreviewWidth);
						rotate90_out((unsigned char*)mBufferForRotation->pointer(), (unsigned char*)pOutBuffer, mPreviewWidth, mPreviewHeight);
					}
					else
					{
						Neon_Convert_yuv422_to_YUV420P((unsigned char *)pInBuffer, (unsigned char *)pOutBuffer, mPreviewWidth, mPreviewHeight);
					}
				}
				else
				{
					Neon_Convert_yuv422_to_NV21((unsigned char *)pInBuffer, (unsigned char *)pOutBuffer, mPreviewWidth, mPreviewHeight);
				}
			}
			else
			{
				HAL_PRINT("Neon_Convert() Called for Capture Case\n");
				Neon_Convert_yuv422_to_NV21((unsigned char *)pInBuffer, (unsigned char *)pOutBuffer, mPreviewWidth, mPreviewHeight);
			}
		}
	}
#endif //EVE_CAM //NCB-TI
	int CameraHal::ICaptureCreate(void)
	{
		int res = 0;
		overlay_buffer_t overlaybuffer;
		int image_width, image_height;

		LOG_FUNCTION_NAME

		isStart_JPEG = false;
		isStart_VPP = false;
		isStart_Scale = false;

		mParameters.getPictureSize(&image_width, &image_height);
		LOGD("ICaptureCreate: Picture Size %d x %d\n", image_width, image_height);

#ifdef OMAP_ENHANCEMENT	 
#ifdef HARDWARE_OMX

		mippMode=0;

#if JPEG
		jpegEncoder = new JpegEncoder;

		if( NULL != jpegEncoder )
		{
			isStart_JPEG = true;
		}
#ifdef jpeg_decoder
		jpegDecoder = new JpegDecoder;
#endif //of jpeg_decoder
#endif //of JPEG
#endif //of HARDWARE_OMX
#endif //of OMAP_ENHANCEMENT

		LOG_FUNCTION_NAME_EXIT
		return res;

fail_jpeg_buffer:
fail_yuv_buffer:
fail_init:

#ifdef OMAP_ENHANCEMENT
#ifdef HARDWARE_OMX
#if JPEG
		delete jpegEncoder;
#ifdef jpeg_decoder
		delete jpegDecoder;
#endif //jpeg_decoder
#endif //JPEG    
#endif //HARDWARE_OMX 
#endif //OMAP_ENHANCEMENT  

fail_icapture:
exit:
		return -1;
	}


	int CameraHal::ICaptureDestroy(void)
	{
		int err;
#ifdef OMAP_ENHANCEMENT	
#ifdef HARDWARE_OMX
#if JPEG
		if( isStart_JPEG )
		{
			delete jpegEncoder;
			isStart_JPEG = NULL;
			isStart_JPEG = false;
			jpegEncoder = NULL;
		}
#ifdef jpeg_decoder
		delete jpegDecoder;
#endif //jpeg_decoder
#endif //JPEG
#endif //HARDWARE_OMX
#endif //OMAP_ENHANCEMENT

		return 0;
	}

	status_t CameraHal::setOverlay(const sp<Overlay> &overlay)
	{
		LOG_FUNCTION_NAME
			Mutex::Autolock lock(mLock);
		int w,h;

		HAL_PRINT("CameraHal setOverlay/1/%08lx/%08lx\n", (long unsigned int)overlay.get(), (long unsigned int)mOverlay.get());
		// De-alloc any stale data
		if ( mOverlay != NULL && mOverlay.get() != NULL )			// Latona TD/Heron : VT_BACKGROUND_SOLUTION 
		{
			LOGD("Destroying current overlay\n");
			// Eclair Camera L25.12     
			int buffer_count = mOverlay->getBufferCount();
			for(int i =0; i < buffer_count ; i++)
			{
				buffers_queued_to_dss[i] = 0;
			}
			// Eclair Camera L25.12
			mOverlay->destroy();
			mOverlay = NULL;
			nOverlayBuffersQueued = 0;
		}

		mOverlay = overlay;
		if (mOverlay == NULL)
		{
			LOGE("Trying to set overlay, but overlay is null!\n");
		}
		else if (mFalsePreview)   // Eclair HAL
		{
			mParameters.getPreviewSize(&w, &h);
			if ((w == HD_WIDTH) && (h == HD_HEIGHT))
			{
				mOverlay->setParameter(CACHEABLE_BUFFERS, 1);
				mOverlay->setParameter(MAINTAIN_COHERENCY, 0);
				mOverlay->resizeInput(w, h);
			}
			// Restart the preview (Only for Overlay Case)
			//LOGD("In else overlay");
			mPreviewRunning = false;
#if 0
			Message msg;
			msg.command = PREVIEW_START;
			previewThreadCommandQ.put(&msg);
			previewThreadAckQ.get(&msg);
#else
			startPreview();
#endif
		} // Eclair HAL

		LOG_FUNCTION_NAME_EXIT

			return NO_ERROR;
	}

	status_t CameraHal::startPreview()
	{
		LOG_FUNCTION_NAME 

#if 0
		if(mCameraIndex == MAIN_CAMERA && mSamsungCamera) 
			mPreviewSkip = 5; /* For AWB stablization */
		else if(mSamsungCamera) 
			mPreviewSkip = 3;
#else
		if(mCameraIndex == MAIN_CAMERA) {
			mPreviewSkip = 3;
			//mCounterSkipFrame = 5;
		}
		else {
			//mCounterSkipFrame = 3;
			mPreviewSkip = 3;
		}
#endif
		
#ifdef SAMSUNG_SECURITY
			if(!mSamsungCamera && !getSecurityCheck())
			{
				return UNKNOWN_ERROR;
			}
#endif

			if(mPreviewRunning == true) {
				LOGD("%s : Preview was already running.\n", __func__);
				return INVALID_OPERATION;
			}

#if 0
#ifndef VT_BACKGROUND_SOLUTION
		if(mOverlay == NULL)
		{
			LOGD("Return from camera Start Preview\n");
			mPreviewRunning = true;
			mFalsePreview = true;
			return NO_ERROR;
		}
#endif //VT_BACKGROUND_SOLUTION
#endif

		Message msg;
		msg.command = PREVIEW_START;
		previewThreadCommandQ.put(&msg);
		previewThreadAckQ.get(&msg);

		LOG_FUNCTION_NAME_EXIT
		return msg.command == PREVIEW_ACK ? NO_ERROR : INVALID_OPERATION;
	}

	void CameraHal::stopPreview()
	{
		LOG_FUNCTION_NAME
		Mutex::Autolock lock(mCaptureLock);
		Message msg;
		msg.command = PREVIEW_STOP;
		previewThreadCommandQ.put(&msg);
		previewThreadAckQ.get(&msg);
		LOG_FUNCTION_NAME_EXIT
	}

	status_t CameraHal::autoFocus()
	{
		LOGV("%s()", __FUNCTION__);
		if(mPreviewRunning && !mAutoFocusRunning)
		{
			mAutoFocusRunning = true;
			Mutex::Autolock lock(mLock);
			if (createThread(beginAutoFocusThread, this) == false)
			{
				LOGE("ERR(%s):Fail - createThread", __FUNCTION__);
				return UNKNOWN_ERROR;
			}
		}
		return NO_ERROR;
	}

	int CameraHal::beginAutoFocusThread(void *cookie)
	{
		LOGV("%s()", __FUNCTION__);
		CameraHal *c = (CameraHal *)cookie;
		c->autoFocusThread();
		return 0;
	}

	void CameraHal::autoFocusThread()
	{
		LOG_FUNCTION_NAME
		Message msg;
		const unsigned int RETRY_CNT = 100;
		int err = 0;
		int count = 0; /* For get af polling */
		int night_scn_flag;
		struct v4l2_control vc;
		CLEAR(vc);

		HAL_PRINT("Receive Command: PREVIEW_AF_START\n");

		if( camera_device < 0 || !mPreviewRunning )
		{
			LOGD("WARNING PREVIEW NOT RUNNING!\n");
			msg.command = PREVIEW_NACK;
		}
		else
		{
			vc.id = V4L2_CID_AF;
			vc.value = AF_START;
			if (ioctl (camera_device, VIDIOC_S_CTRL, &vc) < 0)
			{
				LOGE("setautofocus fail\n");
				mAutoFocusRunning = false;
				mNotifyCb(CAMERA_MSG_FOCUS,CAMERA_AF_FAIL,0,mCallbackCookie);
			}
			CLEAR(vc);
			count = 0;
			night_scn_flag = 0;
			usleep(300*1000); /* 300msec, AF applied delay */
			if(strcmp(getSceneMode(),"night") == 0) {
				LOGE("NIGHT SCENE!!!!!!!!!!!!!!!!!!!!\n");
				night_scn_flag = 1;
				usleep(300*1000); /* 100msec, low fps delay in low light mode*/
			}
			while(count < 600)
			{
				count++;
				vc.id = V4L2_CID_AF;
				vc.value = 0;
				if (ioctl (camera_device, VIDIOC_G_CTRL, &vc) < 0)
				{
					LOGE("getautofocus fail\n");
					break;
				}

				if(vc.value == 1) { /* AF still ongoing */
					usleep(67*1000); /* 67msec, AF applied delay */
					continue;
				}
				else if(vc.value == 2) /* AF success */
				{
					if (mMsgEnabled & CAMERA_MSG_FOCUS)
						mNotifyCb(CAMERA_MSG_FOCUS,CAMERA_AF_SUCCESS,0,mCallbackCookie);
					break;
				}
				else if(vc.value == 3)
				{
					if (mMsgEnabled & CAMERA_MSG_FOCUS)
						mNotifyCb(CAMERA_MSG_FOCUS,CAMERA_AF_FAIL,0,mCallbackCookie);
					break;
				}
				else if(vc.value == 4)/* AF Cancel */
				{
					//      if(night_scn_flag) /* Night shot case */

					//              usleep(600*1000); /* 2sec capture delay */
					if (mMsgEnabled & CAMERA_MSG_FOCUS)
						mNotifyCb(CAMERA_MSG_FOCUS,CAMERA_AF_CANCEL,0,mCallbackCookie);
					break;
				}
			}
		}

		mAutoFocusRunning = false;
		LOG_FUNCTION_NAME_EXIT
	}
	
	status_t CameraHal::cancelAutoFocus()
	{
#if 1 
		LOG_FUNCTION_NAME

		struct v4l2_control vc;
		CLEAR(vc);
		HAL_PRINT("Receive Command: PREVIEW_AF_CANCEL\n");

		if( camera_device < 0 || !mPreviewRunning )
		{
			LOGD("WARNING PREVIEW NOT RUNNING!\n");
		}
		else
		{
			vc.id = V4L2_CID_AF;
			vc.value = AF_STOP;

			if (ioctl (camera_device, VIDIOC_S_CTRL, &vc) < 0)
			{
				LOGE("release autofocus fail\n");
				return NO_ERROR;
			}
			else
			{
			//	mNotifyCb(CAMERA_MSG_FOCUS,CAMERA_AF_CANCEL,0,mCallbackCookie);
			}
		}
		mAutoFocusRunning = false;

		LOG_FUNCTION_NAME_EXIT
		return NO_ERROR;
#endif
#if 0
		LOG_FUNCTION_NAME
		if(mPreviewRunning && mAutoFocusRunning)
		{
			mAutoFocusRunning = true;
			Mutex::Autolock lock(mLock);
			if (createThread(beginCancelAutoFocusThread, this) == false)
			{
				LOGE("ERR(%s):Fail - createThread", __FUNCTION__);
				return UNKNOWN_ERROR;
			}
		}
		LOG_FUNCTION_NAME_EXIT
		return NO_ERROR;
#endif
	}

	int CameraHal::beginCancelAutoFocusThread(void *cookie)
	{
		LOGV("%s()", __FUNCTION__);
		CameraHal *c = (CameraHal *)cookie;
		c->cancelAutoFocusThread();
		return 0;
	}
	
	void CameraHal::cancelAutoFocusThread()
	{
		LOG_FUNCTION_NAME

		struct v4l2_control vc;
		CLEAR(vc);
		HAL_PRINT("Receive Command: PREVIEW_AF_CANCEL\n");

		if( camera_device < 0 || !mPreviewRunning )
		{
			LOGD("WARNING PREVIEW NOT RUNNING!\n");
		}
		else
		{
			vc.id = V4L2_CID_AF;
			vc.value = AF_STOP;

			if (ioctl (camera_device, VIDIOC_S_CTRL, &vc) < 0)
			{
				LOGE("release autofocus fail\n");
				return ;
			}
			else
			{
				mNotifyCb(CAMERA_MSG_FOCUS,CAMERA_AF_CANCEL,0,mCallbackCookie);
			}
		}
		mAutoFocusRunning = false;

		LOG_FUNCTION_NAME_EXIT
		return ;
	}


	bool CameraHal::previewEnabled()
	{
		return mPreviewRunning;
	}

	status_t CameraHal::startRecording( )
	{
		LOG_FUNCTION_NAME
		int w,h;
		int i = 0;
		const char *error = 0;
#ifdef VT_BACKGROUND_SOLUTION    
		if(mOverlay == NULL)
		{
			LOGE("Overlay is NULL. Cannot start recording. \n");
			return UNKNOWN_ERROR;
		} 
#endif //VT_BACKGROUND_SOLUTION   

		for(i = 0; i < VIDEO_FRAME_COUNT_MAX; i++)
		{
			mVideoBufferUsing[i] = 0;
		}

		mParameters.getPreviewSize(&w, &h);

		// Just for the same size case
		mRecordingFrameSize = w * h * 2;

		overlay_handle_t overlayhandle = mOverlay->getHandleRef();

		overlay_true_handle_t true_handle;

		//Eclair L25.14
		if ( overlayhandle == NULL ) 
		{
			LOGD("overlayhandle is received as NULL. \n");
			return UNKNOWN_ERROR;
		}
		//Eclair L25.14
		memcpy(&true_handle,overlayhandle,sizeof(overlay_true_handle_t));

		int overlayfd = true_handle.ctl_fd;

		HAL_PRINT("#Overlay driver FD:%d \n",overlayfd);

		mVideoBufferCount =  mOverlay->getBufferCount();

		if(mMsgEnabled & CAMERA_MSG_VIDEO_FRAME)
		{
			HAL_PRINT("Clear the old memory \n");
			mVideoHeap.clear();
			for(i = 0; i < mVideoBufferCount; i++)
			{
				//Eclair L25.14
				mVideoHeaps[i].clear();
				mVideoBuffer[i].clear();
			}
			HAL_PRINT("Mmap the video Memory %d\n", mPreviewFrameSize);


			for(i = 0; i < mVideoBufferCount; i++)
			{
				mapping_data_t* data = (mapping_data_t*) mOverlay->getBufferAddress((void*)i);
				if(data != NULL)
				{
					mVideoHeaps[i]  = new MemoryHeapBase(data->fd,mPreviewFrameSize, 0, data->offset);
					mVideoBuffer[i] = new MemoryBase(mVideoHeaps[i], 0, mRecordingFrameSize);
					mPreviewBlocks[i] = data->ptr;
					HAL_PRINT("mVideoHeaps[%d]: ID:%d,Base:[%p],size:%d", i,mVideoHeaps[i]->getHeapID(),
							mVideoHeaps[i]->getBase(),mVideoHeaps[i]->getSize());
					HAL_PRINT("mVideoBuffer[%d]: Pointer[%p]", i,mVideoBuffer[i]->pointer());
				} else{
					for(int j = 0; j < i+1; j++)
					{
						mVideoHeaps[j].clear();
						mVideoBuffer[j].clear();
					}
					LOGD("Error: data from overlay returned null");
					return UNKNOWN_ERROR;
				}		
			}
			mVideoHeap_422.clear();	
			if(mCamMode == VT_MODE)
			{	
				if(mVideoHeaps_422 == 0)
				{
					mVideoHeaps_422  = new MemoryHeapBase(mRecordingFrameSize * VIDEO_FRAME_COUNT_MAX);
					for(i=0 ; i<VIDEO_FRAME_COUNT_MAX ; i++)
					{
						mVideoBuffer_422[i] = new MemoryBase(mVideoHeaps_422, i * mRecordingFrameSize, mRecordingFrameSize);
					}		
				}


			}

		}

		if(mPreviousFlashMode == 2) {
			setMovieFlash(FLASH_MOVIE_ON);
		}
		if(mPreviousFlashMode == 3) {
			setMovieFlash(FLASH_MOVIE_AUTO);
		}
		mRecordingLock.lock();
		mRecordEnabled =true;  
		mRecorded = true;
		iOutStandingBuffersWithEncoder = 0;
		iConsecutiveVideoFrameDropCount = 0;
		mRecordingLock.unlock();

		LOG_FUNCTION_NAME_EXIT
		return NO_ERROR;
	}

	void CameraHal::stopRecording()
	{
		LOG_FUNCTION_NAME

		if(mPreviousFlashMode == 2) {
			setMovieFlash(FLASH_MOVIE_OFF);
		}
		if(mPreviousFlashMode == 3) {
			setMovieFlash(FLASH_MOVIE_OFF);
		}

		mRecordingLock.lock();
		mRecordEnabled = false; //Eclair HAL
		mCurrentTime = 0;
#if 1			// fix for OMAPS00242402
		int buffer_count = mOverlay->getBufferCount();
		for (int i = 0; i < buffer_count ; i++) 
		{
			if (buffers_queued_to_camera_driver[i] == 0)
			{
				if (ioctl(camera_device, VIDIOC_QBUF, &v4l2_cam_buffer[i]) >= 0) 
				{
					buffers_queued_to_camera_driver[i] = 1;     // Added for CSR - OMAPS00242402
					nCameraBuffersQueued++;					
				}
			}

			if (nCameraBuffersQueued >= buffer_count )
			{
				HAL_PRINT("All Buffers with driver nCameraBuffersQueued %d, buffer_count %d,  i %d",nCameraBuffersQueued,buffer_count,i);
				break;
			}
		}
#endif
		mRecordingLock.unlock();

		LOG_FUNCTION_NAME_EXIT
	}

	bool CameraHal::recordingEnabled()
	{
		LOG_FUNCTION_NAME
			return (mRecordEnabled);  // to be FIXED later on. Fixed

	}

	void CameraHal::releaseRecordingFrame(const sp<IMemory>& mem)
	{
		//LOG_FUNCTION_NAME

		int index;


		mRecordingLock.lock();
		
		if(mQuikMode){
			if( getVTMode() == 3 && mCameraIndex == MAIN_CAMERA){ /* For recording & share in back camera mode */
				LOGE("Recording Release\n");
				for(index = 0; index <mVideoBufferCount; index ++){
					if(mem->pointer() == mVideoBuffer[index]->pointer()){
						break;
					}
				}

			}
			else{
				for(index = 0; index <mVideoBufferCount; index ++)
				{
					if(mem->pointer() == mVideoBuffer_422[index]->pointer())
					{
						break;
					}
				}
			}
		}
		else	
		{

			for(index = 0; index <mVideoBufferCount; index ++)
			{
				if(mem->pointer() == mVideoBuffer[index]->pointer())
				{
					break;
				}
			}

		}
#if 0
		if(mCamMode == VT_MODE)
		{
			for(index = 0; index <mVideoBufferCount; index ++)
			{
				if(mem->pointer() == mVideoBuffer_422[index]->pointer()) 
				{
					break;
				}
			}
		}
		else
		{

			for(index = 0; index <mVideoBufferCount; index ++)
			{
				if(mem->pointer() == mVideoBuffer[index]->pointer()) 
				{
					break;
				}
			}

		}
#endif

		mRecordingFrameCount++;
		iOutStandingBuffersWithEncoder--;

		mRecordingLock.unlock();

		if (ioctl(camera_device, VIDIOC_QBUF, &v4l2_cam_buffer[index]) < 0)
		{
			LOGE("VIDIOC_QBUF Failed, index [%d] line=%d",index,__LINE__);
		}
		else
		{
			buffers_queued_to_camera_driver[index] = 1;		// Added for CSR - OMAPS00242402
			nCameraBuffersQueued++;
		}

		return;
	}

	sp<IMemoryHeap>  CameraHal::getRawHeap() const
	{
		return mPictureHeap;
	}

	status_t CameraHal::takePicture( )
	{
		LOG_FUNCTION_NAME

		gettimeofday(&take_before, NULL);
		Message msg;
		msg.command = PREVIEW_CAPTURE;
		previewThreadCommandQ.put(&msg);
		previewThreadAckQ.get(&msg);

		LOG_FUNCTION_NAME_EXIT

		return msg.command == CAPTURE_ACK ? NO_ERROR : INVALID_OPERATION;
	}

	status_t CameraHal::cancelPicture( )
	{
		LOG_FUNCTION_NAME
		disableMsgType(CAMERA_MSG_RAW_IMAGE);
		disableMsgType(CAMERA_MSG_COMPRESSED_IMAGE);
		//CallbackCookie = NULL;   // done in destructor

		LOGE("Callbacks set to null\n");
		LOG_FUNCTION_NAME_EXIT
			return -1;
	}

	int CameraHal::validateSize(int w, int h)
	{
		if ((w < MIN_WIDTH) || (h < MIN_HEIGHT))
		{
			return false;
		}
		return true;
	}

	status_t CameraHal::setParameters(const CameraParameters &params)
	{
		HAL_PRINT("setParameters\n");

		int w, h;
		int framerate, fps_min, fps_max;
		int iso, af, mode, zoom, wb, exposure, scene;
		int effects, compensation, saturation, sharpness;
		int contrast, brightness, flash, caf;
		int error;
		Message msg;

		Mutex::Autolock lock(mLock);

		HAL_PRINT("PreviewFormat %s\n", params.getPreviewFormat());
		//[20091023 myungwoo ko
		if (strcmp(params.getPreviewFormat(), "yuv422i-uyvy") != 0
				&& strcmp(params.getPreviewFormat(), "yuv420sp") != 0
				&& strcmp(params.getPreviewFormat(), "yuv420p") != 0) 
		{
			LOGE("Only yuv422i-uyvy preview is supported");
			return -EINVAL;
		}
		//]    
		HAL_PRINT("PictureFormat %s\n", params.getPictureFormat());
		if (strcmp(params.getPictureFormat(), "jpeg") != 0) 
		{
			LOGE("Only jpeg still pictures are supported");
			return -EINVAL;
		}


		params.getPreviewSize(&w, &h);
		if (!validateSize(w, h)) 
		{
			LOGE("Preview size not supported");
			return -EINVAL;
		}
		HAL_PRINT("PreviewSize by App %d x %d\n", w, h);

		params.getPictureSize(&w, &h);
		if (!validateSize(w, h)) 
		{
			LOGE("Picture size not supported");
			return -EINVAL;    	
		}
		HAL_PRINT("Picture Size by App %d x %d\n", w, h);
#if 0
		framerate = params.getPreviewFrameRate();
		if(framerate != mParameters.getPreviewFrameRate() )
		{
			mParameters.setPreviewFrameRate(framerate);
		}
		HAL_PRINT("FRAMERATE %d\n", framerate);

		mMMSApp= params.getInt("MMS_APP");
		if (mMMSApp != 10)
		{
			mMMSApp = 0;
		}
		HAL_PRINT("mMMSApp =%d\n",mMMSApp);
#endif

		mParameters = params;

		mParameters.getPictureSize(&w, &h);
		HAL_PRINT("Picture Size by CamHal %d x %d\n", w, h);
		/* For Trd Camera App. */
		if(mCameraIndex == MAIN_CAMERA) {
			if((w == 2048 && h == 1536)
					||(w == 2048 && h == 1232)
					||(w == 1600 && h == 1200)
					||(w == 1600 && h ==  960)
					||(w == 1024 && h ==  600)
					||(w ==  800 && h ==  600)) {
				/* Correct picture size */
			}
			else {
				HAL_PRINT("[%s:%d] Change resolution avoid capture error\n", 
						__func__, __LINE__);
				mParameters.setPictureSize(800,600); /* Default picture size */
			}
		}
		mParameters.getPreviewSize(&w, &h);
		HAL_PRINT("Preview Size by CamHal %d x %d\n", w, h);

		if((w == QQVGA_WIDTH && h == QQVGA_HEIGHT)
				|| (w == QCIF_WIDTH && h == QCIF_HEIGHT)
				|| (w == QCIF_HEIGHT && h == QCIF_WIDTH)
				|| (w == WQQVGA_WIDTH && h == WQQVGA_HEIGHT)
				|| (w == QVGA_WIDTH && h == QVGA_HEIGHT)
				|| (w == CIF_WIDTH && h == CIF_HEIGHT)
				|| (w == WQVGA_WIDTH && h == WQVGA_HEIGHT)
				|| (w == VGA_WIDTH && h == VGA_HEIGHT)
				|| (w == D1_WIDTH && h == D1_HEIGHT)
				|| (w == WVGA_WIDTH && h == WVGA_HEIGHT)
				|| (w == SVGA_WIDTH && h == SVGA_HEIGHT)
				|| (w == WSVGA_WIDTH && h == WSVGA_HEIGHT)
				|| (w == HD_WIDTH && h == HD_HEIGHT))
		{
			HAL_PRINT("\ncorrect resolution for preview %d x %d\n", w, h);	
		}
		else
		{
			if (w > D1_WIDTH)
			{
				w = WVGA_WIDTH;
				h = WVGA_HEIGHT;
			}
			else if(w == D1_WIDTH)
			{
				w = D1_WIDTH;
				h = D1_WIDTH;
			}
			else
			{
				w = VGA_WIDTH;
				h = VGA_HEIGHT;
			}

			mParameters.setPreviewSize(w, h);
		}

		mParameters.getPreviewFpsRange(&fps_min, &fps_max);
		if ((fps_min > fps_max) || (fps_min < 0) || (fps_max < 0)) {
			LOGE("WARN(%s): request for preview frame is not initial. \n",__func__);
			return UNKNOWN_ERROR;
		} else 	{
			mParameters.setPreviewFrameRate(fps_max/1000);
		}
		HAL_PRINT("Framerate Range %d ~ %d\n", fps_min, fps_max);


		quality = params.getInt("jpeg-quality");
		if ( ( quality < 0 ) || (quality > 100) )
		{
			quality = 100;
		} 

		mPreviewFrameSkipValue = getPreviewFrameSkipValue();

		// [2010.05.13] bj.kwon
		if(strcmp(getSceneMode(),"auto") == 0)
		{
			if(setWB(getWBLighting()) < 0) return UNKNOWN_ERROR;
			if(setEffect(getEffect()) < 0) return UNKNOWN_ERROR;
			if(mCamMode != CAMCORDER_MODE)
			{
				if(mSamsungCamera)
				{
					if(setISO(getISO()) < 0) return UNKNOWN_ERROR;
				}
			}
			if(mSamsungCamera)
			{		
				if(setSaturation(getSaturation()) < 0) return UNKNOWN_ERROR;
				if(setSharpness(getSharpness()) < 0) return UNKNOWN_ERROR;
			}
		}

		if(setAntiBanding(getAntiBanding()) < 0) return UNKNOWN_ERROR;
		if(setSceneMode(getSceneMode()) < 0) return UNKNOWN_ERROR;
		setFlashMode(getFlashMode());
		if(setExposure(getExposure()) < 0) return UNKNOWN_ERROR;
		if(setZoom(getZoomValue()) < 0) return UNKNOWN_ERROR;
		//]
		//[ 20091106 myungwoo
		if(setFocusMode(getFocusMode()) < 0) return UNKNOWN_ERROR;
		if(setGPSLatitude(getGPSLatitude()) < 0) return UNKNOWN_ERROR;
		if(setGPSLongitude(getGPSLongitude()) < 0) return UNKNOWN_ERROR;
		if(setGPSAltitude(getGPSAltitude()) < 0) return UNKNOWN_ERROR;
		if(setGPSTimestamp(getGPSTimestamp()) < 0) return UNKNOWN_ERROR;
		if(setGPSProcessingMethod(getGPSProcessingMethod()) < 0) return UNKNOWN_ERROR;
		if(setJpegThumbnailSize(getJpegThumbnailSize()) < 0) return UNKNOWN_ERROR;

		if(mCamMode != CAMCORDER_MODE && mCamMode != VT_MODE)
		{
			setJpegMainimageQuality(getJpegMainimageQuality());
		}

		if(mSamsungCamera)
		{
			if(setBrightness(getBrightness()) < 0) return UNKNOWN_ERROR;
			if(setContrast(getContrast()) < 0) return UNKNOWN_ERROR;
			if(setMetering(getMetering()) < 0) return UNKNOWN_ERROR;
			if(setPrettyMode(getPrettyValue()) < 0) return UNKNOWN_ERROR;	
			//if(setWDRMode(getWDRMode()) < 0) return UNKNOWN_ERROR;
			//if(setAntiShakeMode(getAntiShakeMode()) < 0) return UNKNOWN_ERROR;		
			// chk_dataline
			if( mCamMode == Trd_PART) {
				if(getPreviewFlashOn() == 1) {
					setMovieFlash(FLASH_MOVIE_ON);
				}
				else {
					setMovieFlash(FLASH_MOVIE_OFF);
				}
			}
			m_chk_dataline = mParameters.getInt("chk_dataline");
			HAL_PRINT("\n == m_chk_dataline == %d\n",m_chk_dataline);
		}

		HAL_PRINT("exit setParameters\n");
		return NO_ERROR;
	}

	CameraParameters CameraHal::getParameters() const
	{
		CameraParameters params;

		LOG_FUNCTION_NAME

		{
			Mutex::Autolock lock(mLock);
			params = mParameters;
		}


		LOG_FUNCTION_NAME_EXIT

			return params;
	}

	status_t  CameraHal::dump(int fd, const Vector<String16>& args) const
	{
		return 0;
	}

	void CameraHal::dumpFrame(void *buffer, int size, char *path)
	{
		FILE* fIn = NULL;

		fIn = fopen(path, "w");
		if ( fIn == NULL ) 
		{
			LOGE("\n\n\n\nError: failed to open the file %s for writing\n\n\n\n", path);
			return;
		}

		fwrite((void *)buffer, 1, size, fIn);
		fclose(fIn);

	}

	void CameraHal::release()
	{
		LOG_FUNCTION_NAME

		if(mPreviewThread != NULL) 
		{
			Message msg;
			msg.command = PREVIEW_KILL;
			previewThreadCommandQ.put(&msg);
			previewThreadAckQ.get(&msg);
		}

		sp<PreviewThread> previewThread;

		{ // scope for the lock
			Mutex::Autolock lock(mLock);
			previewThread = mPreviewThread;
		}

		// don't hold the lock while waiting for the thread to quit
		if (previewThread != 0) 
		{
			previewThread->requestExitAndWait();
		}
#if 0 /* Clear Capture memory */
		if(mVGAYUVPictureHeap != NULL)
			mVGAYUVPictureHeap.clear();
		if(mVGAYUVPictureBuffer != NULL)
			mVGAYUVPictureBuffer.clear();
		if (mPictureBuffer != NULL)
			mPictureBuffer.clear();
		if(mPictureHeap != NULL)
			mPictureHeap.clear();
#endif


		{ // scope for the lock
			Mutex::Autolock lock(mLock);
			mPreviewThread.clear();
		}


#ifdef CAMERA_ALGO
		camAlgos->unInitFaceTracking();

		delete camAlgos;
#endif

#ifdef EVE_CAM

		if(mVideoHeaps_422 != 0)
		{
			mVideoHeaps_422.clear();
			for(int i=0 ; i<VIDEO_FRAME_COUNT_MAX ; i++)
			{
				mVideoBuffer_422[i].clear();
			}
			mVideoHeaps_422 = 0;
			for(int i=0 ; i<VIDEO_FRAME_COUNT_MAX ; i++)
			{
				mVideoBuffer_422[i] = 0;
			}
		}


		if(neon_args)
		{
			free((NEON_FUNCTION_ARGS *)neon_args);
		}	
#endif //EVE_CAM

#ifdef VT_BACKGROUND_SOLUTION
		for(int i = 0; i < VIDEO_FRAME_COUNT_MAX; i++)
		{
			if(mVTHeaps[i] != 0)
			{
				mVTHeaps[i].clear();
				mVTBuffer[i].clear();
			}
		}
#endif //VT_BACKGROUND_SOLUTION

		mRecordEnabled = false; // Eclair HAL
		mFalsePreview =false;   // Eclair HAL
		mRecorded = false;      // Eclair HAL
		mCallbackCookie = NULL; // Eclair HAL

		mCurrentTime = 0;
		ICaptureDestroy();

		CameraDestroy();

		if ( mOverlay != NULL && mOverlay.get() != NULL )		// Latona TD/Heron : VT_BACKGROUND_SOLUTION 
		{
			HAL_PRINT("Destroying current overlay\n");
			mOverlay->destroy();
			mOverlay = NULL;
			nOverlayBuffersQueued = 0;
			// OVL_PATCH [[
			mDSSActive = false;                  //OVL_PATCH
			// OVL_PATCH ]]
		}
		if(pTIrtn != NULL)
		{
			dlclose(pTIrtn);  
			pTIrtn = NULL;
			LOGD("Unloaded NEON Rotation Library");
		}

		LOG_FUNCTION_NAME_EXIT
	}

	int CameraHal::SaveFile(char *filename, char *ext, void *buffer, int size)
	{
		LOG_FUNCTION_NAME
			//Store image
			char fn [512];

		if (filename) 
		{
			strcpy(fn,filename);
		} 
		else 
		{
			if (ext==NULL)
			{ 
				ext = (char*)"tmp";
			}
			sprintf(fn, PHOTO_PATH, file_index, ext);
		}
		file_index++;
		LOGD("Writing to file: %s\n", fn);
		int fd = open(fn, O_RDWR | O_CREAT | O_SYNC);
		if (fd < 0) 
		{
			LOGE("Cannot open file %s : %s\n", fn, strerror(errno));
			return -1;
		} 
		else 
		{  
			int written = 0;
			int page_block, page = 0;
			int cnt = 0;
			int nw;
			char *wr_buff = (char*)buffer;
			LOGD("Jpeg size %d buffer 0x%x", size, ( unsigned int ) buffer);
			page_block = size / 20;
			while (written < size ) 
			{
				nw = size - written;
				nw = (nw>512*1024)?8*1024:nw;

				nw = ::write(fd, wr_buff, nw);
				if (nw<0) 
				{
					LOGD("write fail nw=%d, %s\n", nw, strerror(errno));
					break;
				}
				wr_buff += nw;
				written += nw;
				cnt++;

				page += nw;
				if (page>=page_block)
				{
					page = 0;
					LOGD("Percent %6.2f, wn=%5d, total=%8d, jpeg_size=%8d\n", 
							((float)written)/size, nw, written, size);
				}
			}

			close(fd);

			return 0;
		}
		LOG_FUNCTION_NAME_EXIT
	}


	sp<IMemoryHeap> CameraHal::getPreviewHeap() const
	{
		LOG_FUNCTION_NAME
		return mVideoConversionHeap;
		LOG_FUNCTION_NAME_EXIT

	}

	sp<CameraHardwareInterface> CameraHal::createInstance(int cameraId)
	{
		LOG_FUNCTION_NAME

		HAL_PRINT("createInstance! cameraID : %d \n", cameraId);
		if (singleton != 0) 
		{
			sp<CameraHardwareInterface> hardware = singleton.promote();
			if (hardware != 0) 
			{
				return hardware;
			}
		}

		sp<CameraHardwareInterface> hardware(new CameraHal(cameraId));

		singleton = hardware;
		return hardware;
	} 

	/*--------------------Eclair HAL---------------------------------------*/
	void CameraHal::setCallbacks(notify_callback notify_cb,
			data_callback data_cb,
			data_callback_timestamp data_cb_timestamp,
			void* user)
	{
		Mutex::Autolock lock(mLock);
		mNotifyCb = notify_cb;
		mDataCb = data_cb;
		mDataCbTimestamp = data_cb_timestamp;
		mCallbackCookie = user;
	}

	void CameraHal::enableMsgType(int32_t msgType)
	{
		Mutex::Autolock lock(mLock);
		mMsgEnabled |= msgType;
	}

	void CameraHal::disableMsgType(int32_t msgType)
	{
		Mutex::Autolock lock(mLock);
		mMsgEnabled &= ~msgType;
	}

	bool CameraHal::msgTypeEnabled(int32_t msgType)
	{
		Mutex::Autolock lock(mLock);
		return (mMsgEnabled & msgType);
	}	

	/*--------------------Eclair HAL---------------------------------------*/

	status_t CameraHal::sendCommand(int32_t cmd, int32_t arg1, int32_t arg2)
	{
		switch(cmd)
		{
			case HAL_AE_AWB_LOCK_UNLOCK:
				setAEAWBLockUnlock(arg1, arg2);
				break;
			case HAL_FACE_DETECT_LOCK_UNLOCK:
				setFaceDetectLockUnlock(arg1);
				break;
			case HAL_OBJECT_POSITION:
				setObjectPosition(arg1, arg2);
				break;
			//case HAL_OBJECT_TRACKING_STARTSTOP:
			//	setObjectTrackingStartStop(arg1);
			//	objectTracking(arg1);
				break;
			case HAL_TOUCH_AF_STARTSTOP:
				setTouchAFStartStop(arg1);
				break;
			case HAL_STOP_CHK_DATALINE:
				setDatalineCheckStop();
				break;				
			case HAL_SET_SAMSUNG_CAMERA:
				setSamsungCamera();
				break;	
			case HAL_SET_CAMERA_MODE:
				setCameraMode(arg1);
				break;							
			case HAL_SET_DEFAULT_IMEI:
				setDefultIMEI(arg1);
				break;	
			case HAL_START_CAF:		
				setCAFStart(arg1);
				break;	
			case HAL_STOP_CAF:				
				setCAFStop(arg1);
				break;			
			defualt:
				LOGV("%s()", __FUNCTION__);
				break;
		}
		return NO_ERROR;
	}

	int CameraHal::setAEAWBLockUnlock(int ae_lockunlock, int awb_lockunlock)
	{
		LOGV("%s(ae_lockunlock(%d) , (awb_lockunlock(%d))", __FUNCTION__, ae_lockunlock, awb_lockunlock);
		int ae_awb_status = 0;

		struct v4l2_control vc;

		if(ae_lockunlock == 0 && awb_lockunlock ==0)
			ae_awb_status = AE_UNLOCK_AWB_UNLOCK;
		else if (ae_lockunlock == 1 && awb_lockunlock ==0)
			ae_awb_status = AE_LOCK_AWB_UNLOCK;
		else if (ae_lockunlock == 0 && awb_lockunlock ==1)
			ae_awb_status = AE_UNLOCK_AWB_LOCK;
		else
			ae_awb_status = AE_LOCK_AWB_LOCK;	

		CLEAR(vc);
		vc.id = V4L2_CID_CAMERA_AE_AWB_LOCKUNLOCK;
		vc.value = ae_awb_status;

		if (ioctl(camera_device, VIDIOC_S_CTRL ,&vc) < 0) 
		{
			LOGE("ERR(%s):Fail on V4L2_CID_CAMERA_AE_AWB_LOCKUNLOCK", __FUNCTION__);
			return -1;
		}

		return 0;
	}

	int	CameraHal::setFaceDetectLockUnlock(int facedetect_lockunlock)
	{
		LOGV("%s(facedetect_lockunlock(%d))", __FUNCTION__, facedetect_lockunlock);

		struct v4l2_control vc;
		CLEAR(vc);
		vc.id = V4L2_CID_CAMERA_FACEDETECT_LOCKUNLOCK;
		vc.value = facedetect_lockunlock;

		if (ioctl(camera_device, VIDIOC_S_CTRL ,&vc) < 0) 
		{
			LOGE("ERR(%s):Fail on V4L2_CID_CAMERA_FACEDETECT_LOCKUNLOCK", __FUNCTION__);
			return -1;
		}

		return 0;
	}


	int CameraHal::setTouchAFStartStop(int start_stop)
	{
		LOG_FUNCTION_NAME
		HAL_PRINT("setTouchAF start_stop Value [%d]", start_stop);	

		struct v4l2_control vc;
		CLEAR(vc);
		vc.id = V4L2_CID_AF;
		vc.value = AF_STOP;

		if (ioctl (camera_device, VIDIOC_S_CTRL, &vc) < 0)
		{   
			LOGE("release autofocus fail\n");
		}

		if(m_touch_af_start_stop != start_stop)
		{
			m_touch_af_start_stop = start_stop;

			CLEAR(vc);
			vc.id = V4L2_CID_CAMERA_TOUCH_AF_START_STOP;
			vc.value = m_touch_af_start_stop;

			if (ioctl(camera_device, VIDIOC_S_CTRL, &vc) < 0) 
			{
				LOGE("ERR(%s):Fail on V4L2_CID_CAMERA_TOUCH_AF_START_STOP", __FUNCTION__);
				return -1;
			}
		}

		return 0;
	}

	int CameraHal::setCAFStart(int start)
	{
		LOG_FUNCTION_NAME
			HAL_PRINT("setCAFStart start Value [%d]", start);	

		struct v4l2_control vc;    

		CLEAR(vc);
		vc.id = V4L2_CID_FOCUS_MODE;
		vc.value = CAF_START;

		if (ioctl (camera_device, VIDIOC_S_CTRL, &vc) < 0)
		{
			LOGE("setFocus_mode fail\n");
			return UNKNOWN_ERROR; 
		}   

		CLEAR(vc);
		vc.id = V4L2_CID_AF;
		vc.value = AF_START;

		if (ioctl (camera_device, VIDIOC_S_CTRL, &vc) < 0)
		{
			LOGE("setFocus_mode fail\n");
			return UNKNOWN_ERROR; 
		}    
		return 0;
	}

	int CameraHal::setCAFStop(int stop)
	{
		LOG_FUNCTION_NAME
			HAL_PRINT("setCAFStart start Value [%d]", stop);	

		struct v4l2_control vc;            
		CLEAR(vc);
		vc.id = V4L2_CID_AF;
		vc.value = AF_STOP;

		if (ioctl (camera_device, VIDIOC_S_CTRL, &vc) < 0)
		{
			LOGE("setFocus_mode fail\n");
			return UNKNOWN_ERROR; 					
		}  
		return 0;
	}

	int CameraHal::setDefultIMEI(int imei)	
	{
		LOGV("%s(m_default_imei (%d))", __FUNCTION__, imei);

		if(m_default_imei != imei)
		{
			m_default_imei = imei;
		}
		return 0;
	} 

	//[ 20091022 myungwoo ko
	status_t CameraHal::setWB(const char* wb)
	{   
		if(camera_device && (mCameraIndex == MAIN_CAMERA  || mCamMode == VT_MODE))
		{
			int wbValue =1;
			int i = 1;

			if(strcmp(wb, wb_value[mPreviousWB]) !=0)
			{
				HAL_PRINT("myungwoo mPreviousWB =%d wb=%s\n", mPreviousWB,wb);
				for (i = 1; i < MAX_WBLIGHTING_EFFECTS; i++) 
				{
					if (! strcmp(wb_value[i], wb)) 
					{
						HAL_PRINT("In setWB : Match : %s : %d\n ", wb, i);
						wbValue = i;
						break;
					}
				}

				if(i == MAX_WBLIGHTING_EFFECTS)
				{
					LOGE("setWB : invalid value\n");
					return UNKNOWN_ERROR;           
				}
				else
				{
					struct v4l2_control vc;            
					CLEAR(vc);
					vc.id = V4L2_CID_WB;
					vc.value = wbValue;

					if (ioctl (camera_device, VIDIOC_S_CTRL, &vc) < 0)
					{
						LOGE("setWB fail\n");
						return UNKNOWN_ERROR;                 
					}

					mPreviousWB = i;
				}
			}
		}
		return NO_ERROR;
	}

	status_t CameraHal::setEffect(const char* effect)
	{    
		if(camera_device && (mCameraIndex == MAIN_CAMERA  || mCamMode == VT_MODE))
		{
			int effectValue =1;
			int i = 1;
			if(strcmp(effect, color_effects[mPreviousEffect]) !=0)
			{
				HAL_PRINT("myungwoo mPreviousEffect =%d effect=%s\n", mPreviousEffect,effect); 
				for(i = 1; i < MAX_COLOR_EFFECTS; i++) 
				{
					if (! strcmp(color_effects[i], effect)) 
					{
						HAL_PRINT("In setEffect : Match : %s : %d \n", effect, i);
						effectValue = i;
						break;
					}
				}
				if(i == MAX_COLOR_EFFECTS)
				{
					LOGE("setEffect : invalid value\n");
					return UNKNOWN_ERROR;           
				}
				else
				{    
					struct v4l2_control vc;            
					CLEAR(vc);
					vc.id = V4L2_CID_EFFECT;
					vc.value = effectValue;

					if (ioctl (camera_device, VIDIOC_S_CTRL, &vc) < 0)
					{
						LOGE("setEffect fail\n");
						return UNKNOWN_ERROR;  
					}

					mPreviousEffect = i;
				}
			}
		}

		return NO_ERROR;
	}

	status_t CameraHal::setAntiBanding(const char* antibanding)
	{
		if(camera_device && mCameraIndex == MAIN_CAMERA)
		{
			int antibandingValue =1;
			int i = 0;

			if(strcmp(antibanding, anti_banding_values[mPreviousAntibanding]) !=0)
			{
				HAL_PRINT("myungwoo mPreviousAntibanding =%d antibanding=%s\n", mPreviousAntibanding,antibanding);
				for(i = 1; i < MAX_ANTI_BANDING_VALUES; i++) 
				{
					if (! strcmp(anti_banding_values[i], antibanding)) 
					{
						HAL_PRINT("In setAntiBanding : Match : %s : %d \n", antibanding, i);
						antibandingValue = i;
						break;
					}
				}

				if(i == MAX_ANTI_BANDING_VALUES)
				{
					LOGE("setAntiBanding : invalid value\n");
					return UNKNOWN_ERROR;           
				}
				else
				{    
#if 0            
					struct v4l2_control vc;            
					CLEAR(vc);
					vc.id = V4L2_CID_ANTIBANDING;
					vc.value = antibandingValue;

					if (ioctl (camera_device, VIDIOC_S_CTRL, &vc) < 0)
					{
						LOGE("setAntiBanding fail\n");
						return UNKNOWN_ERROR;  
					}
#endif
					mPreviousAntibanding = i;
				}
			}
		}

		return NO_ERROR;
	}

	status_t CameraHal::setSceneMode(const char* scenemode)
	{
		if(camera_device && mCameraIndex == MAIN_CAMERA)
		{
			int sceneModeValue =1;
			int i = 1;

			if(strcmp(scenemode, scene_mode[mPreviousSceneMode]) !=0)
			{
				HAL_PRINT("myungwoo mPreviousSceneMode =%d scenemode=%s\n", mPreviousSceneMode,scenemode); 
				for(i = 1; i < MAX_SCENE_MODE; i++) 
				{
					if (! strcmp(scene_mode[i], scenemode)) 
					{
						HAL_PRINT("In setSceneMode : Match : %s : %d \n", scenemode, i);
						sceneModeValue = i;
						break;
					}
				}

				if(i == MAX_SCENE_MODE)
				{
					LOGE("setSceneMode : invalid value\n");
					return UNKNOWN_ERROR;           
				}
				else
				{      
					struct v4l2_control vc;

					CLEAR(vc);
					vc.id = V4L2_CID_SCENE;
					vc.value = sceneModeValue;

					if(sceneModeValue == 2)
					{
						mASDMode = true;
					}
					else
					{
						mASDMode = false;
					}

					if (ioctl (camera_device, VIDIOC_S_CTRL, &vc) < 0)
					{
						LOGE("setSceneMode fail\n");
						return UNKNOWN_ERROR; 
					}

					mPreviousSceneMode = i;
				}
			}
		}

		return NO_ERROR;
	}

	status_t CameraHal::setFlashMode(const char* flashmode)
	{

		HAL_PRINT("[%s:%d] flashmode = %s\n", __func__, __LINE__, flashmode);

		if(camera_device && mCameraIndex == MAIN_CAMERA)
		{
			int flashModeValue =1;
			int i = 1;

			HAL_PRINT("[%s:%d] mPreviousFlashMode : %d, flash_mode[%s]\n", 
					__func__, __LINE__, mPreviousFlashMode, flash_mode[mPreviousFlashMode]);

			if(strcmp(flashmode, flash_mode[mPreviousFlashMode]) !=0)
			{
				HAL_PRINT("myungwoo mPreviousFlashMode =%d flashmode=%s\n", mPreviousFlashMode,flashmode); 
				for(i = 1; i < MAX_FLASH_MODE; i++) 
				{
					if (! strcmp(flash_mode[i], flashmode)) 
					{
						HAL_PRINT("In setFlashMode : Match : %s : %d \n", flashmode, i);
#if 0 /* For the test */
						if(i == 2)
							i = 4;
#endif
						flashModeValue = i;
						break;
					}
				}

				if(i == MAX_FLASH_MODE)
				{
					LOGE("setFlashMode : invalid value\n");
					return UNKNOWN_ERROR;           
				}
				else
				{     
					struct v4l2_control vc;            
					CLEAR(vc);
					vc.id = V4L2_CID_FLASH_CAPTURE;
					if((mPreviousFlashMode == 4) && (i == 1))
						vc.value = 5; /* Torch off */
					else
						vc.value = flashModeValue;

					if (ioctl (camera_device, VIDIOC_S_CTRL, &vc) < 0)
					{
						LOGE("setFlashMode fail\n");
						return UNKNOWN_ERROR; 
					}

					mPreviousFlashMode = i;
				}
			}
		}

		return NO_ERROR;
	}

	status_t CameraHal::setMovieFlash(int flag)
	{
		if(camera_device && mCameraIndex == MAIN_CAMERA)
		{
			if(flag != mPreviousFlag)
			{
				struct v4l2_control vc;

				CLEAR(vc);
				vc.id = V4L2_CID_FLASH_MOVIE;
				vc.value = flag;

				if (ioctl (camera_device, VIDIOC_S_CTRL, &vc) < 0)
				{
					LOGE("setFlashMode fail\n");
					return UNKNOWN_ERROR; 
				}

				mPreviousFlag = flag;
			}
		}

		return NO_ERROR;
	}

	status_t CameraHal::setBrightness(int brightness)
	{
		if(camera_device)
		{
			struct v4l2_control vc;

			if(brightness != mPreviousBrightness)
			{
				HAL_PRINT("myungwoo mPreviousBrightness =%d brightness=%d\n", mPreviousBrightness,brightness);

				CLEAR(vc);
				vc.id = V4L2_CID_BRIGHTNESS;
				vc.value = brightness + 1;

				if (ioctl (camera_device, VIDIOC_S_CTRL, &vc) < 0)
				{
					LOGE("setBrightness fail\n");
					return UNKNOWN_ERROR; 
				}
			}

			mPreviousBrightness = brightness;
		}

		return NO_ERROR;    
	}

	status_t CameraHal::setExposure(int exposure)
	{
		if(camera_device)
		{
			struct v4l2_control vc;

			if(exposure != mPreviousExposure)
			{
				HAL_PRINT("myungwoo mPreviousExposure =%d exposure=%d\n", mPreviousExposure,exposure);

				CLEAR(vc);
				//vc.id = V4L2_CID_EXPOSURE;
				vc.id = V4L2_CID_BRIGHTNESS;
				vc.value = exposure + 5;

				if (ioctl (camera_device, VIDIOC_S_CTRL, &vc) < 0)
				{
					LOGE("setExposure fail\n");
					return UNKNOWN_ERROR;
				}
			}

			mPreviousExposure = exposure;
		}

		return NO_ERROR;    
	}

	status_t CameraHal::setZoom(int zoom)
	{
		if(camera_device && mCameraIndex == MAIN_CAMERA)
		{
			if(zoom < 0 || zoom > MAX_ZOOM) {
				if(zoom < 0) 
					mParameters.set(mParameters.KEY_ZOOM, 0);
				if(zoom > MAX_ZOOM) 
					mParameters.set(mParameters.KEY_ZOOM, MAX_ZOOM);
				return UNKNOWN_ERROR; 
			}

			struct v4l2_control vc;

			if(zoom != mPreviousZoom)
			{
				HAL_PRINT("myungwoo mPreviousZoom =%d zoom=%d\n", mPreviousZoom,zoom);

				CLEAR(vc);
				vc.id = V4L2_CID_ZOOM;
				vc.value = zoom;

				if (ioctl (camera_device, VIDIOC_S_CTRL, &vc) < 0)
				{
					LOGE("setZoom fail\n");
					return UNKNOWN_ERROR; 
				}
			}

			mPreviousZoom = zoom;
		}

		return NO_ERROR;
	}
	//]

	//[ 20091106 myungwoo
	//[ 2010 04 27
#ifndef HALO_ISO 
	status_t CameraHal::setISO(int iso)
	{
		if(camera_device && mCameraIndex == MAIN_CAMERA)
		{
			struct v4l2_control vc;

			if(iso != mPreviousIso)
			{
				HAL_PRINT("myungwoo mPreviousIso =%d iso=%d\n", mPreviousIso,iso);

				CLEAR(vc);
				vc.id = V4L2_CID_ISO;
#if 1
				switch(iso)
				{
					case 0:
						vc.value = ISO_AUTO;
						break;
					case 50:
						vc.value = ISO_50;
						break;
					case 100:
						vc.value = ISO_100;
						break;
					case 200:
						vc.value = ISO_200;
						break;
					case 400:
						vc.value = ISO_400;
						break;
					case 800:
						vc.value = ISO_800;
						break;
				}
#else
				vc.value = iso;
#endif
				if (ioctl (camera_device, VIDIOC_S_CTRL, &vc) < 0)
				{
					LOGE("setISO fail\n");
					return UNKNOWN_ERROR; 
				}
			}

			mPreviousIso = iso;
		}

		return NO_ERROR;    
	}
#endif
	//]
	status_t CameraHal::setContrast(int contrast)
	{
		if(camera_device && mCameraIndex == MAIN_CAMERA)
		{
			struct v4l2_control vc;

			if(contrast != mPreviousContrast)
			{
				HAL_PRINT("myungwoo mPreviousContrast =%d contrast=%d\n", mPreviousContrast,contrast);

				CLEAR(vc);
				vc.id = V4L2_CID_CONTRAST;
				vc.value = contrast + 2;

				if (ioctl (camera_device, VIDIOC_S_CTRL, &vc) < 0)
				{
					LOGE("setContrast fail\n");
					return UNKNOWN_ERROR; 
				}
			}

			mPreviousContrast = contrast;
		}

		return NO_ERROR;    
	}

	status_t CameraHal::setSaturation(int saturation)
	{
		if(camera_device && mCameraIndex == MAIN_CAMERA)
		{
			struct v4l2_control vc;

			if(saturation != mPreviousSaturation)
			{
				HAL_PRINT("myungwoo mPreviousSaturation =%d saturation=%d\n", mPreviousSaturation,saturation);

				CLEAR(vc);	
				vc.id = V4L2_CID_SATURATION;
				vc.value = saturation+2;

				if (ioctl (camera_device, VIDIOC_S_CTRL, &vc) < 0)
				{
					LOGE("setSaturation fail\n");
					return UNKNOWN_ERROR; 
				}
			}

			mPreviousSaturation = saturation;
		}

		return NO_ERROR;    
	}

	status_t CameraHal::setSharpness(int sharpness)
	{
		if(camera_device && mCameraIndex == MAIN_CAMERA)
		{
			struct v4l2_control vc;

			if(sharpness != mPreviousSharpness)
			{
				HAL_PRINT("myungwoo mPreviousSharpness =%d sharpness=%d\n", mPreviousSharpness,sharpness);

				CLEAR(vc);
				vc.id = V4L2_CID_SHARPNESS;
				vc.value = sharpness+2;

				if (ioctl (camera_device, VIDIOC_S_CTRL, &vc) < 0)
				{
					LOGE("setSharpness fail\n");
					return UNKNOWN_ERROR; 
				}
			}

			mPreviousSharpness = sharpness;
		}

		return NO_ERROR;    
	}

	status_t CameraHal::setWDRMode(int wdr)
	{
		if(camera_device && mCameraIndex == MAIN_CAMERA)
		{
			struct v4l2_control vc;

			if(wdr != mPreviousWdr)
			{
				HAL_PRINT("myungwoo mPreviousWdr =%d wdr=%d\n", mPreviousWdr,wdr);

				CLEAR(vc);
				vc.id = V4L2_CID_WDR;
				vc.value = wdr+1;

				if (ioctl (camera_device, VIDIOC_S_CTRL, &vc) < 0)
				{
					LOGE("setWDRMode fail\n");
					return UNKNOWN_ERROR; 
				}
			}

			mPreviousWdr = wdr;
		}

		return NO_ERROR;    
	}

	status_t CameraHal::setAntiShakeMode(int antiShake)
	{
		if(camera_device && mCameraIndex == MAIN_CAMERA)
		{
			struct v4l2_control vc;

			if(antiShake != mPreviousAntiShake)
			{
				HAL_PRINT("myungwoo mPreviousAntiShake =%d antiShake=%d\n", mPreviousAntiShake,antiShake);

				CLEAR(vc);
				vc.id = V4L2_CID_ANTISHAKE;
				vc.value = antiShake+1;

				if (ioctl (camera_device, VIDIOC_S_CTRL, &vc) < 0)
				{
					LOGE("setAntiShakeMode fail\n");
					return UNKNOWN_ERROR; 
				}
			}

			mPreviousAntiShake = antiShake;
		}

		return NO_ERROR;    
	}

	status_t CameraHal::setFocusMode(const char* focus)
	{
		if(camera_device && mCameraIndex == MAIN_CAMERA)
		{
			int focusValue =1;
			int i = 1;

			if(strcmp(focus, focus_mode[mPreviousFocus]) !=0)
			{
				HAL_PRINT("myungwoo mPreviousFocus =%d focus=%s\n", mPreviousFocus,focus); 
				for(i = 1; i < MAX_FOCUS_MODE; i++) 
				{
					if (! strcmp(focus_mode[i], focus)) 
					{
						LOGI("In setFocusMode : Match : %s : %d \n", focus, i);
						focusValue = i;
						break;
					}
				}

				if(i == MAX_FOCUS_MODE)
				{
					LOGE("setFocusMode : invalid value\n");
					return UNKNOWN_ERROR;           
				}
				else
				{    
					struct v4l2_control vc;            
					CLEAR(vc);
					vc.id = V4L2_CID_FOCUS_MODE;
					vc.value = focusValue;

					if (ioctl (camera_device, VIDIOC_S_CTRL, &vc) < 0)
					{
						LOGE("setEffect fail\n");
						return UNKNOWN_ERROR; 
					}

					mPreviousFocus = i;
				}
			}
		}

		return NO_ERROR;    
	}

	status_t CameraHal::setMetering(const char* metering)
	{
		if(camera_device && mCameraIndex == MAIN_CAMERA)
		{
			int meteringValue =1;
			int i = 1;

			if(strcmp(metering, metering_mode[mPreviousMetering]) !=0)
			{
				HAL_PRINT("myungwoo mPreviousMetering =%d metering=%s\n", mPreviousMetering,metering); 
				for(i = 1; i < MAX_METERING; i++) 
				{
					if (! strcmp(metering_mode[i], metering)) 
					{
						HAL_PRINT("In setMetering : Match : %s : %d \n", metering, i);
						meteringValue = i;
						break;
					}
				}

				if(i == MAX_METERING)
				{
					LOGE("setMetering : invalid value\n");
					return UNKNOWN_ERROR;           
				}
				else
				{    
					struct v4l2_control vc;     
					CLEAR(vc);
					vc.id = V4L2_CID_PHOTOMETRY;
					vc.value = meteringValue;

					if (ioctl (camera_device, VIDIOC_S_CTRL, &vc) < 0)
					{
						LOGE("setEffect fail\n");
						return UNKNOWN_ERROR; 
					}

					mPreviousMetering = i;
				}
			}
		}
		return NO_ERROR;    	
	}
	//]
	//[ 2010 04 27
#ifdef HALO_ISO
	status_t CameraHal::setISO(const char* iso)
	{
		if(camera_device && mCameraIndex == MAIN_CAMERA)
		{
			int isoValue =1;
			int i = 1;
			struct v4l2_control vc;
			if(strcmp(iso, iso_mode[mPreviousISO]) !=0)
			{
				HAL_PRINT("myungwoo mPreviousISO =%d iso=%s\n", mPreviousISO,iso); 
				for(i = 1; i < MAX_ISO; i++) 
				{
					if (! strcmp(iso_mode[i], iso)) 
					{
						HAL_PRINT("In setISO : Match : %s : %d \n", iso, i);
						isoValue = i;
						break;
					}
				}

				CLEAR(vc);            
				vc.id = V4L2_CID_ISO;            
				vc.value = isoValue;

				if (ioctl (camera_device, VIDIOC_S_CTRL, &vc) < 0)
				{
					LOGE("setISO fail\n");
					return UNKNOWN_ERROR;       
				}

				mPreviousISO = i;
			}
		}
		return NO_ERROR;    	
	}
#endif
	//]
	status_t CameraHal::setPrettyMode(int pretty)
	{
		if(camera_device && mCameraIndex == VGA_CAMERA)
		{
			struct v4l2_control vc;

			if(pretty != mPreviousPretty)
			{
				HAL_PRINT("myungwoo mPreviousPretty =%d pretty=%d\n", mPreviousPretty,pretty);

				CLEAR(vc);
				vc.id = V4L2_CID_PRETTY;
				vc.value = pretty;

				if (ioctl (camera_device, VIDIOC_S_CTRL, &vc) < 0)
				{
					LOGE("setPrettyMode fail\n");
					return UNKNOWN_ERROR; 
				}
			}

			mPreviousPretty = pretty;
		}

		return NO_ERROR;    
	}

	status_t CameraHal::setJpegMainimageQuality(int quality)
	{
		if(camera_device)
		{
			if(quality != mPreviousQuality)
			{                   
				HAL_PRINT("myungwoo mPreviousQuality = %d quality = %d\n", mPreviousQuality,quality);

				if(mCamera_Mode == CAMERA_MODE_JPEG)
				{        
					struct v4l2_control vc;           

					CLEAR(vc);
					vc.id = V4L2_CID_JPEG_QUALITY;

					if(quality < JPEG_NORMAL_limit)
					{
						vc.value = JPEG_ECONOMY;
					}
					else if((JPEG_NORMAL_limit <= quality) && (quality < JPEG_FINE_limit))
					{
						vc.value = JPEG_NORMAL;
					}
					else if((JPEG_FINE_limit <= quality) && (quality < JPEG_SUPERFINE_limit))
					{
						vc.value = JPEG_FINE;
					}
					else
					{
						vc.value = JPEG_SUPERFINE;
					}

					if (ioctl (camera_device, VIDIOC_S_CTRL, &vc) < 0)
					{
						LOGE("setQuality fail\n");
						return UNKNOWN_ERROR; 
					} 
				}   
				else
				{
					if(quality < JPEG_NORMAL_limit)
					{
						mYcbcrQuality = YUV_ECONOMY;
					}
					else if((JPEG_NORMAL_limit <= quality) && (quality < JPEG_FINE_limit))
					{
						mYcbcrQuality = YUV_NORMAL;
					}
					else if((JPEG_FINE_limit <= quality) && (quality < JPEG_SUPERFINE_limit))
					{
						mYcbcrQuality = YUV_FINE;
					}
					else
					{
						mYcbcrQuality = YUV_SUPERFINE;
					}
				}
			}

			mPreviousQuality = quality;
		}

		return NO_ERROR;    
	}

	//==========================================================CTS
	status_t CameraHal::setGPSLatitude(double gps_latitude)
	{
		if(camera_device)
		{
			if(gps_latitude != mPreviousGPSLatitude)
			{
				HAL_PRINT("mPreviousGPSLatitude =%2.2f gps_latitude = %2.2f\n", mPreviousGPSLatitude, gps_latitude);
			}    
			mPreviousGPSLatitude = gps_latitude;
		}   

		return NO_ERROR;    
	}

	status_t CameraHal::setGPSLongitude(double gps_longitude)
	{
		if(camera_device)
		{
			if(gps_longitude != mPreviousGPSLongitude)
			{
				HAL_PRINT("mPreviousGPSLongitude =%2.2f gps_latitude = %2.2f\n", mPreviousGPSLongitude, gps_longitude);
			}    
			mPreviousGPSLongitude = gps_longitude;
		} 

		return NO_ERROR;    
	}

	status_t CameraHal::setGPSAltitude(double gps_altitude)
	{
		if(camera_device)
		{
			if(gps_altitude != mPreviousGPSAltitude)
			{
				HAL_PRINT("mPreviousGPSAltitude =%2.2f gps_latitude = %2.2f\n", mPreviousGPSAltitude, gps_altitude);
			}    
			mPreviousGPSAltitude = gps_altitude;
		} 

		return NO_ERROR;    
	}

	status_t CameraHal::setGPSTimestamp(long gps_timestamp)
	{
		if(camera_device)
		{
			if(gps_timestamp != mPreviousGPSTimestamp)
			{
				HAL_PRINT("mPreviousGPSTimestamp = %ld gps_timestamp = %ld\n", mPreviousGPSTimestamp, gps_timestamp);
			}
			mPreviousGPSTimestamp = gps_timestamp;
			m_gps_time = mPreviousGPSTimestamp;
			m_timeinfo = localtime(&m_gps_time);
			m_gpsHour = m_timeinfo->tm_hour;
			m_gpsMin  = m_timeinfo->tm_min;
			m_gpsSec  = m_timeinfo->tm_sec;
			strftime((char *)m_gps_date, 11, "%Y-%m-%d", m_timeinfo);
		}

		return NO_ERROR;    
	}

	status_t CameraHal::setGPSProcessingMethod(const char * gps_processingmethod)
	{
		if(camera_device)
		{
			if(gps_processingmethod != NULL)
			{
				HAL_PRINT("gps_processingmethod = %s\n", gps_processingmethod);
				strcpy((char *)mPreviousGPSProcessingMethod, gps_processingmethod);

			}
			else
			{
				strcpy((char *)mPreviousGPSProcessingMethod, "GPS NETWORK HYBRID ARE ALL FINE.");
			}
		}

		return NO_ERROR;    
	}

	status_t CameraHal::setJpegThumbnailSize(imageInfo imgInfo)
	{
		if(camera_device)
		{
			if(mThumbnailWidth != imgInfo.mImageWidth || mThumbnailHeight != imgInfo.mImageHeight)
			{
				HAL_PRINT("mThumbnailWidth = %d mThumbnailHeight = %d\n", imgInfo.mImageWidth, imgInfo.mImageHeight);
			}

			mThumbnailWidth = imgInfo.mImageWidth;
			mThumbnailHeight = imgInfo.mImageHeight;
		}
		return NO_ERROR; 	
	}

	//==========================================================CTS

	int CameraHal::setObjectPosition(int32_t x, int32_t y)
	{
		LOGV("%s(setObjectPosition(x=%d, y=%d))", __FUNCTION__, x, y);

		struct v4l2_control vc;

		if(mPreviewWidth ==640)
			x = x - 80;

		CLEAR(vc);
		vc.id = V4L2_CID_CAMERA_OBJECT_POSITION_X;
		vc.value = x;
		if (ioctl(camera_device, VIDIOC_S_CTRL, &vc) < 0)
		{
			LOGE("ERR(%s):Fail on V4L2_CID_CAMERA_OBJECT_POSITION_X", __FUNCTION__);
			return -1;
		}

		CLEAR(vc);
		vc.id = V4L2_CID_CAMERA_OBJECT_POSITION_Y;
		vc.value = y;
		if (ioctl(camera_device, VIDIOC_S_CTRL, &vc) < 0)
		{
			LOGE("ERR(%s):Fail on V4L2_CID_CAMERA_OBJECT_POSITION_Y", __FUNCTION__);
			return -1;
		}

		return 0;
	}

	void CameraHal::setDatalineCheckStart()
	{
		struct v4l2_control vc;

		HAL_PRINT("setDatalineCheckStart= chk_dataline=%d\n", m_chk_dataline);

		CLEAR(vc);
		vc.id = V4L2_CID_CAMERA_CHECK_DATALINE;
		vc.value = m_chk_dataline;

		if (ioctl (camera_device, VIDIOC_S_CTRL, &vc) < 0)
		{
			LOGE("setDatalineCheck fail\n");
			mNotifyCb(CAMERA_MSG_ERROR,CAMERA_DEVICE_ERROR_FOR_UNKNOWN,0,mCallbackCookie);
		}
	}

	void CameraHal::setDatalineCheckStop()
	{
		struct v4l2_control vc;
		HAL_PRINT("setDatalineCheckStop= chk_dataline=%d\n", m_chk_dataline);

		CLEAR(vc);
		vc.id = V4L2_CID_CAMERA_CHECK_DATALINE_STOP;
		vc.value = m_chk_dataline;

		if (ioctl (camera_device, VIDIOC_S_CTRL, &vc) < 0)
		{
			LOGE("setDatalineCheck fail\n");
			mNotifyCb(CAMERA_MSG_ERROR,CAMERA_DEVICE_ERROR_FOR_UNKNOWN,0,mCallbackCookie);
		}
		m_chk_dataline_end = true;
	}

	void CameraHal::setCameraMode(int32_t mode)
	{
		HAL_PRINT("setCameraMode= %x\n", mode);
		mCameraMode = mode;
	}

	void CameraHal::setSamsungCamera()
	{
		HAL_PRINT("setSamsungCamera mode\n");
		mSamsungCamera = 1;

		mParameters.set("brightness", "5");
		mParameters.set("exposure", "5");
		mParameters.set("iso","1");
		mParameters.set("contrast","4");
		mParameters.set("saturation","4");
		mParameters.set("sharpness","4");
		mParameters.set("wdr","1");
		mParameters.set("anti-shake","1");
		mParameters.set("metering","center");
		mParameters.set("camera-id","1");
		mParameters.set("videoinput","0");
		mParameters.set("CamcorderPreviewMode","0");
		mParameters.set("vtmode","0");
		mParameters.set("vtmodeforbg","0");
		mParameters.set("pretty","1");
		mParameters.set("previewframeskip","0");
		mParameters.set("twosecondreviewmode","0");
		mParameters.set("previewflashon","0");
		mParameters.set("setcrop","0");
		mParameters.set("samsungcamera","0");
		mParameters.set("exifOrientation","0");
		mParameters.set("chk_dataline", 0);

	}

	//[ 20091023 myungwoo ko
	const char *CameraHal::getEffect() const
	{
		return mParameters.get(mParameters.KEY_EFFECT);
	}
	const char *CameraHal::getWBLighting() const
	{
		return mParameters.get(mParameters.KEY_WHITE_BALANCE);
	}

	const char *CameraHal::getAntiBanding() const
	{
		return mParameters.get(mParameters.KEY_ANTIBANDING);
	}

	const char *CameraHal::getSceneMode() const
	{
		return mParameters.get(mParameters.KEY_SCENE_MODE);
	}

	const char *CameraHal::getFlashMode() const
	{
		return mParameters.get(mParameters.KEY_FLASH_MODE);
	}

	int CameraHal::getJpegMainimageQuality() const
	{
		return atoi(mParameters.get(mParameters.KEY_JPEG_QUALITY));
	}
	int CameraHal::getBrightness() const
	{
		int brightnessValue = 0;
		if( mParameters.get("brightness") )
		{
			brightnessValue = atoi(mParameters.get("brightness"));
			if(brightnessValue != 0)
			{
				HAL_PRINT("in CameraParameters.cpp getbrightness not null int = %d \n", brightnessValue);
			}
			return brightnessValue;//atoi(get("brightness"));
		}
		else
		{
			HAL_PRINT("in CameraParameters.cpp getbrightness null \n");
			return 0;
		}
	}

	int CameraHal::getExposure() const
	{
		int exposureValue = 0;
		if( mParameters.get(mParameters.KEY_EXPOSURE_COMPENSATION) )
		{
			exposureValue = atoi(mParameters.get(mParameters.KEY_EXPOSURE_COMPENSATION));
			if(exposureValue != 0)
			{
				HAL_PRINT("in CameraParameters.cpp getExposure not null int = %d \n", exposureValue);
			}
			return exposureValue;//atoi(get("brightness"));
		}
		else
		{
			HAL_PRINT("in CameraParameters.cpp getExposure null \n");
			return 0;
		}
	}

	int CameraHal::getZoomValue() const
	{
		int zoomValue = 0;
		if( mParameters.get(mParameters.KEY_ZOOM) )
		{
			zoomValue = atoi(mParameters.get(mParameters.KEY_ZOOM));

			if( zoomValue != 0 )
			{
				HAL_PRINT("CameraParameters.cpp :: ZOOM Value not null int = %d \n", zoomValue);
			}
			return zoomValue;
		}
		else
		{
			HAL_PRINT("CameraParameters.cpp :: ZOOM Value null \n");
			return 0;
		}
	}

	double CameraHal::getGPSLatitude() const
	{
		double gpsLatitudeValue = 0;
		if( mParameters.get(mParameters.KEY_GPS_LATITUDE) )
		{
			gpsLatitudeValue = atof(mParameters.get(mParameters.KEY_GPS_LATITUDE));
			if(gpsLatitudeValue != 0)
			{
				HAL_PRINT("CameraParameters.cpp :: gpsLatitudeValue = %2.2f \n", gpsLatitudeValue);
			}
			return gpsLatitudeValue;
		}
		else
		{
			HAL_PRINT("CameraParameters.cpp :: gpsLatitudeValue null \n");
			return 0;
		}
	}

	double CameraHal::getGPSLongitude() const
	{
		double gpsLongitudeValue = 0;
		if( mParameters.get(mParameters.KEY_GPS_LONGITUDE) )
		{
			gpsLongitudeValue = atof(mParameters.get(mParameters.KEY_GPS_LONGITUDE));
			if(gpsLongitudeValue != 0)
			{
				HAL_PRINT("CameraParameters.cpp :: gpsLongitudeValue = %2.2f \n", gpsLongitudeValue);
			}
			return gpsLongitudeValue;
		}
		else
		{
			HAL_PRINT("CameraParameters.cpp :: gpsLongitudeValue null \n");
			return 0;
		}
	}

	double CameraHal::getGPSAltitude() const
	{
		double gpsAltitudeValue = 0;
		if( mParameters.get(mParameters.KEY_GPS_ALTITUDE) )
		{
			gpsAltitudeValue = atof(mParameters.get(mParameters.KEY_GPS_ALTITUDE));
			if(gpsAltitudeValue != 0)
			{
				HAL_PRINT("CameraParameters.cpp :: gpsAltitudeValue = %2.2f \n", gpsAltitudeValue);
			}
			return gpsAltitudeValue;
		}
		else
		{
			HAL_PRINT("CameraParameters.cpp :: gpsAltitudeValue null \n");
			return 0;
		}
	}

	long CameraHal::getGPSTimestamp() const
	{
		long gpsTimestampValue = 0;
		if( mParameters.get(mParameters.KEY_GPS_TIMESTAMP) )
		{
			gpsTimestampValue = atol(mParameters.get(mParameters.KEY_GPS_TIMESTAMP));
			if(gpsTimestampValue != 0)
			{
				HAL_PRINT("CameraParameters.cpp :: getGPSTimestamp = %ld \n", gpsTimestampValue);
			}
			return gpsTimestampValue;
		}
		else
		{
			HAL_PRINT("CameraParameters.cpp :: getGPSTimestamp null \n");
			return 0;
		}
	}

	const char *CameraHal::getGPSProcessingMethod() const
	{
		return mParameters.get(mParameters.KEY_GPS_PROCESSING_METHOD);
	}

#ifndef HALO_ISO
	int CameraHal::getISO() const
	{
		int isoValue = 0;
		if( mParameters.get("iso") )
		{
			isoValue = atoi(mParameters.get("iso"));
			if(isoValue!=0)
			{
				HAL_PRINT("in CameraParameters.cpp iso not null int = %d \n", isoValue);
			}
			return isoValue;//atoi(get("brightness"));
		}
		else
		{
			HAL_PRINT("in CameraParameters.cpp iso null \n");
			return 0;
		}
	}
#endif
	//]
	int CameraHal::getContrast() const
	{
		int contrastValue = 0;
		if( mParameters.get("contrast") )
		{
			contrastValue = atoi(mParameters.get("contrast"));
			if(contrastValue != 0)
			{
				HAL_PRINT("in CameraParameters.cpp contrastValue not null int = %d \n", contrastValue);
			}
			return contrastValue;//atoi(get("brightness"));
		}
		else
		{
			HAL_PRINT("in CameraParameters.cpp contrast null \n");
			return 0;
		}
	}

	int CameraHal::getSaturation() const
	{
		int saturationValue = 0;
		if( mParameters.get("saturation") )
		{
			saturationValue = atoi(mParameters.get("saturation"));
			if(saturationValue != 0)
			{
				HAL_PRINT("in CameraParameters.cpp saturationValue not null int = %d \n", saturationValue);
			}
			return saturationValue;//atoi(get("brightness"));
		}
		else
		{
			HAL_PRINT("in CameraParameters.cpp saturation null \n");
			return 0;
		}
	}

	int CameraHal::getSharpness() const
	{
		int sharpnessValue = 0;
		if( mParameters.get("sharpness") )
		{
			sharpnessValue = atoi(mParameters.get("sharpness"));
			if(sharpnessValue != 0)
			{
				HAL_PRINT("in CameraParameters.cpp sharpnessValue not null int = %d \n", sharpnessValue);
			}
			return sharpnessValue;//atoi(get("brightness"));
		}
		else
		{
			HAL_PRINT("in CameraParameters.cpp sharpness null \n");
			return 0;
		}
	}

	int CameraHal::getWDRMode() const
	{
		int wdrValue = 0;
		if( mParameters.get("wdr") )
		{
			wdrValue = atoi(mParameters.get("wdr"));
			if(wdrValue != 0)
			{
				HAL_PRINT("in CameraParameters.cpp wdrValue not null int = %d \n", wdrValue);
			}
			return wdrValue;//atoi(get("brightness"));
		}
		else
		{
			HAL_PRINT("in CameraParameters.cpp wdr null \n");
			return 0;
		}
	}

	int CameraHal::getAntiShakeMode() const
	{
		int antiShakeValue = 0;
		if( mParameters.get("anti-shake") )
		{
			antiShakeValue = atoi(mParameters.get("anti-shake"));
			if(antiShakeValue != 0)
			{
				HAL_PRINT("in CameraParameters.cpp antiShakeValue not null int = %d \n", antiShakeValue);
			}
			return antiShakeValue;//atoi(get("brightness"));
		}
		else
		{
			HAL_PRINT("in CameraParameters.cpp anti-shake null \n");
			return 0;
		}
	}

	const char *CameraHal::getFocusMode() const
	{
		return mParameters.get(mParameters.KEY_FOCUS_MODE);
	}

	const char *CameraHal::getMetering() const
	{
		return mParameters.get("metering");
	}

	//[ 2010 04 27
#ifdef HALO_ISO
	const char *CameraHal::getISO() const
	{
		return mParameters.get("iso");
	}
#endif


	int CameraHal::getCameraSelect() const
	{
		int CameraSelectValue = 0;
		if( mParameters.get("camera-id") || mParameters.get("videoinput"))
		{
			CameraSelectValue = atoi(mParameters.get("camera-id"));
			if(CameraSelectValue != 0)
			{
				HAL_PRINT("in CameraParameters.cpp getCameraSelect not null int = %d \n", CameraSelectValue);
			}
			return CameraSelectValue;//atoi(get("brightness"));
		}
		else
		{
			HAL_PRINT("in CameraParameters.cpp camera-id null \n");
			return 0;
		}
	}

	int CameraHal::getCamcorderPreviewValue() const
	{
		int camcorderPreviewValue = 0;
		if( mParameters.get("CamcorderPreviewMode") )
		{
			camcorderPreviewValue = atoi(mParameters.get("CamcorderPreviewMode"));
			if(camcorderPreviewValue != 0)
			{
				HAL_PRINT("in CameraParameters.cpp getCamcorderPreviewValue not null int = %d \n", camcorderPreviewValue);
			}
			return camcorderPreviewValue;//atoi(get("brightness"));
		}
		else
		{
			HAL_PRINT("in CameraParameters.cpp getCamcorderPreviewValue null \n");
			return 0;
		}
	}

	int CameraHal::getVTMode() const
	{
		int vtModeValue = 0;
		if( mParameters.get("vtmode") )
		{
			vtModeValue = atoi(mParameters.get("vtmode"));
			if(vtModeValue != 0)
			{
				HAL_PRINT("in CameraParameters.cpp getVTMode not null int = %d \n", vtModeValue);
			}
			return vtModeValue;//atoi(get("brightness"));
		}
		else
		{
			HAL_PRINT("in CameraParameters.cpp getVTMode null \n");
			return 0;
		}
	}

	int CameraHal::getPrettyValue() const
	{
		int prettyValue = 0;
		if( mParameters.get("pretty") )
		{
			prettyValue = atoi(mParameters.get("pretty"));
			if(prettyValue != 0)
			{
				HAL_PRINT("CameraParameters.cpp :: pretty Value not null int = %d \n", prettyValue);
			}
			return prettyValue;
		}
		else
		{
			HAL_PRINT("CameraParameters.cpp :: pretty Value null \n");
			return 0;
		}
	}

	int CameraHal::getPreviewFrameSkipValue() const
	{
		int previewFrameSkipValue = 0;
		if( mParameters.get("previewframeskip") )
		{
			previewFrameSkipValue = atoi(mParameters.get("previewframeskip"));
			if(previewFrameSkipValue != 0)
			{
				HAL_PRINT("CameraParameters.cpp :: previewFrameSkip Value not null int = %d \n", previewFrameSkipValue);
			}
			return previewFrameSkipValue;
		}
		else
		{
			HAL_PRINT("CameraParameters.cpp :: previewFrameSkip Value null \n");
			return 0;
		}
	}

	void CameraHal::setDriverState(int state)
	{
		if(camera_device)
		{
			struct v4l2_control vc;

			CLEAR(vc);
			vc.id = V4L2_CID_SELECT_STATE;
			vc.value = state;

			if (ioctl (camera_device, VIDIOC_S_CTRL, &vc) < 0)
			{
				LOGE("setDriverState fail\n");
				mNotifyCb(CAMERA_MSG_ERROR,CAMERA_DEVICE_ERROR_FOR_UNKNOWN,0,mCallbackCookie);
			}
		}
	}

	void CameraHal::mirrorForVTC(unsigned char * aSrcBufPtr, unsigned char * aDstBufPtr,int  aFramewidth,int  aFrameHeight)
	{
		for (int i= 0; i< aFrameHeight; i++)
		{
			for (int j= 0; j< aFramewidth * 2; j += 2)
			{
				*(aDstBufPtr + j + (aFramewidth*2*i)) = *(aSrcBufPtr + (aFramewidth * 2 * i+aFramewidth * 2)-(j+2));
				*(aDstBufPtr + j + 1 + (aFramewidth*2*i)) = *(aSrcBufPtr + (aFramewidth * 2 * i+aFramewidth * 2)-(j+2)+1);
			}
		}
	}

	int CameraHal::getTwoSecondReviewMode() const
	{
		int twoSecondReviewModeValue = 0;
		if( mParameters.get("twosecondreviewmode") )
		{
			twoSecondReviewModeValue = atoi(mParameters.get("twosecondreviewmode"));
			if(twoSecondReviewModeValue!=0)
			{
				HAL_PRINT("in CameraParameters.cpp getTwoSecondReviewMode not null int = %d \n", twoSecondReviewModeValue);
			}
			return twoSecondReviewModeValue;//atoi(get("brightness"));
		}
		else
		{
			HAL_PRINT("in CameraParameters.cpp getTwoSecondReviewMode null \n");
			return 0;
		}
	}

	int CameraHal::getPreviewFlashOn() const
	{
		int previewFlashOn = 0;
		if( mParameters.get("previewflashon") )
		{
			previewFlashOn = atoi(mParameters.get("previewflashon"));
			if(previewFlashOn!=0)
			{
				HAL_PRINT("in CameraParameters.cpp getPreviewFlashOn not null int = %d \n", previewFlashOn);
			}
			return previewFlashOn;//atoi(get("brightness"));
		}
		else
		{
			HAL_PRINT("in CameraParameters.cpp getPreviewFlashOn null \n");
			return 0;
		}
	}

	int CameraHal::getCropValue() const
	{
		int cropValue = 0;
		if( mParameters.get("setcrop") )
		{
			cropValue = atoi(mParameters.get("setcrop"));
			if(cropValue!=0)
			{
				HAL_PRINT("in CameraParameters.cpp crop not null int = %d \n", cropValue);
			}
			return cropValue;//atoi(get("brightness"));
		}
		else
		{
			HAL_PRINT("in CameraParameters.cpp crop null \n");
			return 0;
		}
	}

	int CameraHal::checkFirmware()
	{
		//[ 2010 05 02 01
		if(mCameraIndex == MAIN_CAMERA)
		{
			struct v4l2_control vc;
			int currentFirmware = 0, latestFirmware = 0;

			CLEAR(vc);
			vc.id = V4L2_CID_FW_VERSION;
			vc.value = 0;
			if (ioctl (camera_device, VIDIOC_G_CTRL, &vc) < 0)
			{
				LOGE("get fw version fail\n");
			}

			currentFirmware = vc.value;

			CLEAR(vc);
			vc.id = V4L2_CID_FW_LASTEST;
			vc.value = 0;
			if (ioctl (camera_device, VIDIOC_G_CTRL, &vc) < 0)
			{
				LOGE("get lastest fw version fail\n");
			}

			latestFirmware = vc.value;

			if(currentFirmware != latestFirmware)
			{
				return -1;
			}
		}
		//]
		return 0;

	}

	int CameraHal::getSamsungCameraValue() const
	{
		int samsungCameraValue = 0;
		if( mParameters.get("samsungcamera") )
		{
			samsungCameraValue = atoi(mParameters.get("samsungcamera"));
			if(samsungCameraValue!=0)
			{
				HAL_PRINT("in CameraParameters.cpp samsungcamera not null int = %d \n", samsungCameraValue);
			}
			return samsungCameraValue;//atoi(get("brightness"));
		}
		else
		{
			HAL_PRINT("in CameraParameters.cpp samsungcamera null \n");
			return 0;
		}
	}

	void CameraHal::rotate90_out(unsigned char *pInputBuf, unsigned char *pOutputBuf,int w, int h)
	{
		register int x, y, src_idx, dst_idx;
		int width, height, dst_pos;

		int ImgSize = w*h;
		unsigned char *pInBuf = pInputBuf;
		unsigned char *pInBuf_U = pInputBuf + ImgSize;
		unsigned char *pInBuf_V = pInBuf_U + (ImgSize>>2);
		unsigned char *pOutBuf = pOutputBuf;
		unsigned char *pOutBuf_U = pOutputBuf+ImgSize;
		unsigned char *pOutBuf_V = pOutBuf_U+(ImgSize>>2);


		width = w;
		height = h;
		src_idx = 0;

		for(y=width-1; y>=0; y--)
		{
			dst_idx = y;
			for(x=0; x<height; src_idx++,x++)
			{               
				pOutBuf[dst_idx] = pInBuf[src_idx];
				dst_idx += width;
			}
		}

		width = 88;
		src_idx = 0;
		for(y=width-1; y>=0; y--)
		{
			dst_idx = y;
			for(x=0; x<72; src_idx++,x++)
			{
				pOutBuf_U[dst_idx] = pInBuf_U[src_idx];
				pOutBuf_V[dst_idx] = pInBuf_V[src_idx];
				dst_idx += width;            
			}
		}
	}

	//[ 2010 0501
	int CameraHal::getOrientation() const
	{
		int rotationValue = 0;
		/*    if(mCamMode == Trd_PART)
		      {*/    		

		if( mParameters.get(mParameters.KEY_ROTATION) )
		{
			rotationValue = atoi(mParameters.get(mParameters.KEY_ROTATION));
			if(rotationValue!=0)
			{
				HAL_PRINT("in CameraParameters.cpp getRotation not null int = %d \n", rotationValue);
			}
			return rotationValue;
		}
		else
		{
			HAL_PRINT("in CameraParameters.cpp getRotation null \n");
			return 0;
		}

		/*    } 
		      else
		      {   		
		      if( mParameters.get("exifOrientation") )
		      {
		      rotationValue = atoi(mParameters.get("exifOrientation"));
		      if(rotationValue!=0)
		      {
		      HAL_PRINT("in CameraParameters.cpp getRotation not null int = %d \n", rotationValue);
		      }
		      return rotationValue;
		      }
		      else
		      {
		      HAL_PRINT("in CameraParameters.cpp getRotation null \n");
		      return 0;
		      }
		      }*/
	}
	//]

	imageInfo CameraHal::getJpegThumbnailSize() const
	{
		imageInfo ThumbImageInfo;

		if(mParameters.get(mParameters.KEY_JPEG_THUMBNAIL_WIDTH))
		{
			ThumbImageInfo.mImageWidth = atoi(mParameters.get(mParameters.KEY_JPEG_THUMBNAIL_WIDTH));		
		}

		if(mParameters.get(mParameters.KEY_JPEG_THUMBNAIL_HEIGHT))
		{
			ThumbImageInfo.mImageHeight = atoi(mParameters.get(mParameters.KEY_JPEG_THUMBNAIL_HEIGHT));

		}
		return ThumbImageInfo;
	}

#ifdef EVE_CAM
	void CameraHal::DrawOverlay(uint8_t *pBuffer, bool bCopy)
	{
		int nPreviewWidth, nPreviewHeight;
		int error;
		void* snapshot_buffer;
		overlay_buffer_t overlaybuffer;

		if ( mOverlay == NULL )
		{
			LOGE("DrawOverlay:mOverlay is NULL\n");
			return;
		}
		else if(lastOverlayBufferDQ < 0 || lastOverlayBufferDQ >= NUM_OVERLAY_BUFFERS_REQUESTED)
		{
			LOGD("DrawOverlay:Skip this buffer. lastOverlayBufferDQ[%d]\n", lastOverlayBufferDQ);
			return;
		}

		mParameters.getPreviewSize(&nPreviewWidth, &nPreviewHeight);

		mapping_data_t* data = (mapping_data_t*)mOverlay->getBufferAddress( (void*)(lastOverlayBufferDQ) );
		if ( data == NULL ) 
		{
			LOGE("DrawOverlay:getBufferAddress returned NULL\n");
			return;
		}
		snapshot_buffer = (void*)data->ptr;
		if(bCopy && snapshot_buffer != NULL)
		{
			if(pBuffer)
			{
				memcpy(snapshot_buffer,pBuffer,nPreviewWidth*nPreviewHeight*2);
			}
			else
			{
				uint8_t *pTempBuffer = (uint8_t*)snapshot_buffer;
				int nPreviewSize = nPreviewWidth*nPreviewHeight;
				for(int i=0 ; i<nPreviewSize ; i++)
				{
					pTempBuffer[i*2] = 0x80;
					pTempBuffer[i*2+1] = 0x10;
				}
			}
		}
		error = mOverlay->queueBuffer((void*)(lastOverlayBufferDQ));
		if (error)
		{
			LOGE("DrawOverlay:mOverlay->queueBuffer() failed!!!!\n");
		}
		else
		{
			buffers_queued_to_dss[lastOverlayBufferDQ]=1;
			nOverlayBuffersQueued++;
		}

		error = mOverlay->dequeueBuffer(&overlaybuffer);
		if(error)
		{
			LOGE("DrawOverlay:mOverlay->dequeueBuffer() failed!!!!\n");
		}
		else
		{
			nOverlayBuffersQueued--;
			buffers_queued_to_dss[(int)overlaybuffer] = 0;
			lastOverlayBufferDQ = (int)overlaybuffer;
		}
	}
#endif //EVE_CAM
	template <typename tn> inline tn clamp(tn x, tn a, tn b) { return min(max(x, a), b); };
#define FIXED2INT(x) (((x) + 128) >> 8)

	void CameraHal::DrawHorizontalLineMixedForOverlay(int row, int col, int size, uint8_t yValue, uint8_t cbValue, uint8_t crValue, int fraction, int previewWidth, int previewHeight)
	{
		uint8_t*    input;
		int start_offset = 0, end_offset = 0;
		int mod = 0;
		int i = 0;
		bool bNoColorUpdate = false;

		//LOGD("row %d, col %d, size %d, yValue %d, cbValue %d, crValue %d", row, col, size, yValue, cbValue, crValue);

		int weight1 = fraction; // (fraction + 128) & 255;
		int weight2 = 255 - weight1;

		//LOGD("weight1 : %d, weight2 : %d", weight1, weight2);
		//[ 2010 05 01 01
		//struct v4l2_buffer cfilledbuffer;
		//cfilledbuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		//cfilledbuffer.memory = V4L2_MEMORY_USERPTR;
		//] 
		input = (uint8_t*)mCfilledbuffer.m.userptr;
		//input = (uint8_t*)cfilledbuffer.m.userptr;

		start_offset = ((row * previewWidth) + col) * 2;
		end_offset =  start_offset + size*2;

		// check col
		if (col % 2)
		{
			// if col is odd, start of frame is cr. not cb. 
			// so skip cr and update y value only.
			start_offset += 1;// skip cr value

			// update y value
			*( input + start_offset) = clamp(FIXED2INT(yValue * weight1 + (*(input+start_offset)) * weight2), 0, 255);
			start_offset += 1;
		}

		for ( i = start_offset; i < end_offset; ++i)
		{
			if(i%2) // Y value
			{
				*( input + i ) = clamp(FIXED2INT(yValue * weight1 + (*(input+i)) * weight2), 0, 255);
			}    
			else // Cb & Cr
			{
				if(mod%2)// Cr value
				{
					*( input + i ) = clamp(FIXED2INT(crValue * weight1 + (*(input+i)) * weight2), 0, 255);
				}
				else // Cb value
				{
					*( input + i ) = clamp(FIXED2INT(cbValue * weight1 + (*(input+i)) * weight2), 0, 255);	
				}
				mod++;
			}
		}
	}

	void CameraHal::DrawHorizontalLineForOverlay(int yAxis, int xLeft, int xRight, uint8_t yValue, uint8_t cbValue, uint8_t crValue, int previewWidth, int previewHeight) 
	{
		uint8_t*    input;
		int start_offset = 0, end_offset = 0;
		int mod = 0;
		int i = 0;

		//LOGD("yAxis %d, xLeft %d, xRight %d", yAxis, xLeft, xRight);
		//LOGD("yAxis %d, xLeft %d, xRight %d yValue %d cbValue %d crValue %d previewWidth %d previewHeight %d", yAxis, xLeft, xRight, yValue, cbValue, crValue, previewWidth, previewHeight);

		// check boundary
		xLeft   = max(xLeft, 0);
		xRight  = min(xRight, (int)(previewWidth-1));

		input = (uint8_t*)mCfilledbuffer.m.userptr;
		//[ 2010 05 01 01
		//struct v4l2_buffer cfilledbuffer;
		//cfilledbuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		//cfilledbuffer.memory = V4L2_MEMORY_USERPTR;
		//input = (uint8_t*)cfilledbuffer.m.userptr;
		//] 
		start_offset = ((yAxis * previewWidth) + xLeft) * 2;
		end_offset =  start_offset + (xRight - xLeft)*2;

		for ( i = start_offset; i < end_offset; ++i)
		{
			if(i%2) // Y value
			{
				*( input + i ) = yValue;
			}
			else // Cb & Cr
			{
				if(mod%2)// Cr value
				{
					*( input + i ) = crValue;
				}
				else // Cb value
				{
					*( input + i ) = cbValue;			
				}
				mod++;
			}
		}	
	}

	void CameraHal::DrawVerticalLineForOverlay(int xAxis, int yTop, int yBottom, uint8_t yValue, uint8_t cbValue, uint8_t crValue, int previewWidth, int previewHeight)
	{
		uint8_t*    input;
		int start_offset = 0, end_offset = 0;
		int mod = 0;
		int i = 0, j = 0;
		bool bNoColorUpdate = false;

		// Return if color update is unnecessary.
		if (xAxis % 2)
		{
			bNoColorUpdate = true;
		}

		//LOGD("xAxis %d, yTop %d, yBottom %d", xAxis, yTop, yBottom);

		// check boundary
		yTop   = max(yTop, 0);
		yBottom     = min(yBottom, (int)(previewHeight-1));
		//[ 2010 05 01 01
		//struct v4l2_buffer cfilledbuffer;
		//cfilledbuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		//cfilledbuffer.memory = V4L2_MEMORY_USERPTR;
		//input = (uint8_t*)cfilledbuffer.m.userptr;
		//] 
		input = (uint8_t*)mCfilledbuffer.m.userptr;

		start_offset = ((yTop * previewWidth) + xAxis) * 2;
		end_offset =  ((yBottom * previewWidth) + xAxis) * 2;

		if(bNoColorUpdate)
		{
			for ( i = start_offset; i < end_offset; i += 2*previewWidth)
			{
				// update y values only
				*( input + i + 1) = yValue; 							
			}
		}
		else // with Color Update
		{
			for ( i = start_offset; i < end_offset; i += 2*previewWidth)
			{
				for( j = 0; j < 4; j++)
				{
					if(j % 2) // Y value
					{	
						if(j == 1)// update 1 line
						{
							*( input + i + j) = yValue; 		
						}
					}
					else // Cb & Cr
					{
						if(mod%2)// Cr value
						{
							*( input + i + j) = crValue;
						}
						else // Cb value
						{
							*( input + i + j) = cbValue;			
						}
						mod++;			
					}
				}
				mod = 0;
			}		
		}

	}

	static CameraInfo sCameraInfo[] = {
		{
			CAMERA_FACING_BACK,
			90,  /* orientation */
		},
		{
			CAMERA_FACING_FRONT,
			270,  /* orientation */
		}
	};

	extern "C" int HAL_getNumberOfCameras()
	{
		return sizeof(sCameraInfo) / sizeof(sCameraInfo[0]);
	}

	extern "C" void HAL_getCameraInfo(int cameraId, struct CameraInfo *cameraInfo)
	{
		memcpy(cameraInfo, &sCameraInfo[cameraId], sizeof(CameraInfo));
	}

	extern "C" sp<CameraHardwareInterface> HAL_openCameraHardware(int cameraId)
	{
		LOGD("opening TI camera hal\n");
		return CameraHal::createInstance(cameraId);
	}


}; // namespace android
