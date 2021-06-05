#include"stdafx.h"
#include"myplaycap.h"
#include<stdio.h>

#pragma comment(lib,"strmiids.lib")

PLAYSTATE g_psCurrent = Stopped;
 IMediaControl * g_pMC = NULL;
 IMediaEventEx * g_pME = NULL;

HRESULT CaptureVideo(HWND hOwner)
{
   HRESULT hr;
   DWORD g_dwGraphRegister=0;

    IVideoWindow  * g_pVW = NULL;
   IBaseFilter *pMux =NULL;  
   IFileSinkFilter *pSink=NULL; 
    IGraphBuilder * g_pGraph = NULL;
    ICaptureGraphBuilder2 * g_pCapture = NULL;
	 
    //IBaseFilter *pSrcFilter=NULL;
//创建过滤器图表管理器
    hr = CoCreateInstance (CLSID_FilterGraph, NULL, CLSCTX_INPROC,
                           IID_IGraphBuilder, (void **) &g_pGraph);
    if (FAILED(hr))
        return -1;

    // 创建捕获设备管理器
    hr = CoCreateInstance (CLSID_CaptureGraphBuilder2 , NULL, CLSCTX_INPROC,
                           IID_ICaptureGraphBuilder2, (void **) &g_pCapture);
    if (FAILED(hr))
        return -1;
    
    // 获取媒体控制接口
    hr = g_pGraph->QueryInterface(IID_IMediaControl,(LPVOID *) &g_pMC);
    if (FAILED(hr))
        return hr;
	//获取视频窗口
    hr = g_pGraph->QueryInterface(IID_IVideoWindow, (LPVOID *) &g_pVW);
    if (FAILED(hr))
        return hr;
	//获取媒体事件处理器
    hr = g_pGraph->QueryInterface(IID_IMediaEventEx, (LPVOID *) &g_pME);
    if (FAILED(hr))
        return hr;

    // Set the window handle used to process graph events
    hr = g_pME->SetNotifyWindow((OAHWND)hOwner, WM_GRAPHNOTIFY, 0);
     if (FAILED(hr))
    {
        printf("Failed to get video interfaces!  hr=0x%x",hr);
        return hr;
    }
    hr = g_pCapture->SetFiltergraph(g_pGraph);
    if (FAILED(hr))
    {
        printf("Failed to set capture filter graph!  hr=0x%x", hr);
        return hr;
    }
    hr = S_OK;
    IBaseFilter * pSrc = NULL;
	IBaseFilter * pSrcAudio = NULL;
    IMoniker* pMoniker =NULL;
	//定义设备接口对象
    ICreateDevEnum *pDevEnum =NULL;
	//定义设备枚举对象
    IEnumMoniker *pClassEnum = NULL;
   hr = CoCreateInstance (CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC,
                           IID_ICreateDevEnum, (void **) &pDevEnum);
    if (FAILED(hr))
    {
        printf("Couldn't create system enumerator!  hr=0x%x", hr);
    }

    // Create an enumerator for the video capture devices

	if (SUCCEEDED(hr))
	{
	    hr = pDevEnum->CreateClassEnumerator (CLSID_VideoInputDeviceCategory, &pClassEnum, 0);
	    if (FAILED(hr))
	    {
			printf("Couldn't create class enumerator!  hr=0x%x", hr);
	    }
	}

	if (SUCCEEDED(hr))
	{
		// If there are no enumerators for the requested type, then 
		// CreateClassEnumerator will succeed, but pClassEnum will be NULL.
		if (pClassEnum == NULL)
		{
			printf("No video capture device was detected.\r\n\r\n");
			hr = E_FAIL;
		}
	}

    // Use the first video capture device on the device list.
    // Note that if the Next() call succeeds but there are no monikers,
    // it will return S_FALSE (which is not a failure).  Therefore, we
    // check that the return code is S_OK instead of using SUCCEEDED() macro.

	if (SUCCEEDED(hr))
	{
		hr = pClassEnum->Next (1, &pMoniker, NULL);
		if (hr == S_FALSE)
		{
	                 printf("Unable to access video capture device!");   
			hr = E_FAIL;
		}
	}

	if (SUCCEEDED(hr))
    {
        // Bind Moniker to a filter object
        hr = pMoniker->BindToObject(0,0,IID_IBaseFilter, (void**)&pSrc);
        if (FAILED(hr))
        {
            printf("Couldn't bind moniker to filter object!  hr=0x%x", hr);
        }
    }
   // Add Capture filter to our graph.
    hr = g_pGraph->AddFilter(pSrc, L"Video Capture");
    if (FAILED(hr))
    {
        printf("Couldn't add the capture filter to the graph!  hr=0x%x\r\n\r\n",hr);
        pSrc->Release();
        return hr;
    }
	//绑定Audio Filter
	hr = pDevEnum->CreateClassEnumerator (CLSID_AudioInputDeviceCategory, &pClassEnum, 0);
	if(SUCCEEDED(hr))
	{
		hr=pClassEnum->Next(1,&pMoniker,NULL);
		if(SUCCEEDED(hr))
		{
			hr=pMoniker->BindToObject(0,0,IID_IBaseFilter,(void**)&pSrcAudio);
		}
	
	}
    // Render the preview pin on the video capture filter
    // Use this instead of g_pGraph->RenderFile
	
    if (FAILED(hr))
    {
        printf("Couldn't render the video capture stream.  hr=0x%x\r\n",hr);
        pSrc->Release();
        return hr;
    }
	 hr = g_pGraph->AddFilter(pSrcAudio, L"Audio Capture");
	 

	 //写到文件
	//1.声明写滤波器
	IBaseFilter* pWriter;
	 hr = CoCreateInstance(CLSID_FileWriter,NULL,CLSCTX_INPROC,IID_IFileSinkFilter,(void**)&pSink);
	 //利用创建IFileSinkFilter2对象查询pWriter
	pSink->QueryInterface(IID_IBaseFilter,(void**)&pWriter);
	g_pGraph->QueryInterface(IID_IBaseFilter,(void**)&pWriter);
	 
	g_pGraph->AddFilter(pWriter,L"File Writer");
	//2.设置输出文件
	//g_pMC->Stop();
	hr=g_pCapture->SetOutputFileName(&MEDIASUBTYPE_Asf,L"d:\\vid2.asf",&pWriter,&pSink);
	//hr=g_pCapture->SetOutputFileName(&MEDIASUBTYPE_dvh1,L"d:\\vid1.wmv",&pWriter,&pSink);
	//注意如果用MEDIASUBTYPE_Avi无法打开视频，用MEDIASUBTYPE_Asf保存为.asf文件完全没有问题，保存为avi或者wmv或者mp4不太好
	
	hr = g_pCapture->RenderStream (&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video,
                                   pSrc, NULL, pWriter);

	 hr = g_pCapture->RenderStream (&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video,
                                   pSrc, NULL, NULL);
	 hr = g_pCapture->RenderStream (&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Audio,pSrcAudio, NULL, pWriter);
	if(FAILED(hr))  return hr;
	 

    
	 	/* hr = g_pCapture->RenderStream (&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Audio,
                                   pSrcAudio, NULL, NULL);*/
	  
    // Now that the filter has been added to the graph and we have
    // rendered its stream, we can release this reference to the filter.
   // pSrc->Release();

    // Set video window style and position
     // Set the video window to be a child of the main window
    hr = g_pVW->put_Owner((OAHWND)hOwner);
    if (FAILED(hr))
        return hr;
    
    // Set video window style
    hr = g_pVW->put_WindowStyle(WS_CHILD | WS_CLIPCHILDREN);
   

    // Use helper function to position video window in client rect 
    // of main application window
    if (g_pVW)
    {
        RECT rc;
        
        // Make the preview video fill our window
        GetClientRect(hOwner, &rc);
        g_pVW->SetWindowPosition(0, 0, rc.right, rc.bottom);
    }

    // Make the video window visible, now that it is properly positioned
    /*hr = */g_pVW->put_Visible(OATRUE);
    
   /* if (FAILED(hr))
    {
        printf("Couldn't initialize video window!  hr=0x%x", hr);
        
    }*/
	 
    
#ifdef REGISTER_FILTERGRAPH
    // Add our graph to the running object table, which will allow
    // the GraphEdit application to "spy" on our graph
    hr = AddGraphToRot(g_pGraph, &g_dwGraphRegister);
    if (FAILED(hr))
    {
        printf("Failed to register filter graph with ROT!  hr=0x%x", hr);
        g_dwGraphRegister = 0;
    }
#endif

    // Start previewing video data
    hr = g_pMC->Run();
    if (FAILED(hr))
    {
       printf("Couldn't run the graph!  hr=0x%x", hr);
        return hr;
    }

    // Remember current state
    g_psCurrent = Running;
        
    return hr;
}

 void StopCap()
 {
	 if(g_psCurrent == Running)
	 {
       g_psCurrent=Stopped;
	   g_pMC->Stop();
	 }
 }