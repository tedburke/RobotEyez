//
// RobotEyez.cpp - A simple machine vision program
// Written by Ted Burke - last modified 30-11-2011
//
// Copyright Ted Burke, 2011, All rights reserved.
//
// Website: http://batchloaf.wordpress.com
//

// DirectShow header file
#include <dshow.h>

#include "FrameTransformFilter.h"

// For some reason, this is not included in the
// DirectShow headers. However, it's exported
// by strmiids.lib, so I'm just declaring it
// here as extern.
EXTERN_C const CLSID CLSID_NullRenderer;

// Used for various strings - filenames, command line args,
// etc (also defined in FrameTransformFilter.cpp)
#define STRING_LENGTH 200

// DirectShow objects
HRESULT hr;
ICreateDevEnum *pDevEnum = NULL;
IEnumMoniker *pEnum = NULL;
IMoniker *pMoniker = NULL;
IPropertyBag *pPropBag = NULL;
IGraphBuilder *pGraph = NULL;
ICaptureGraphBuilder2 *pBuilder = NULL;
IBaseFilter *pCap = NULL;
IBaseFilter *pTransform = NULL;
IBaseFilter *pNullRenderer = NULL;
IMediaControl *pMediaControl = NULL;

void exit_message(const char* error_message, int error)
{
	// Print an error message
	fprintf(stderr, error_message);
	fprintf(stderr, "\n");
	
	// Clean up DirectShow / COM stuff
	if (pMediaControl != NULL) pMediaControl->Release();
	if (pNullRenderer != NULL) pNullRenderer->Release();
	if (pTransform != NULL) pTransform->Release();
	if (pCap != NULL) pCap->Release();
	if (pBuilder != NULL) pBuilder->Release();
	if (pGraph != NULL) pGraph->Release();
	if (pPropBag != NULL) pPropBag->Release();
	if (pMoniker != NULL) pMoniker->Release();
	if (pEnum != NULL) pEnum->Release();
	if (pDevEnum != NULL) pDevEnum->Release();
	CoUninitialize();
	
	// Exit the program
	exit(error);
}

int main(int argc, char **argv)
{
	// Capture settings
	int delay = 2000;
	int period = 1000;
	int frames = 1;
	int number_files = 0;
	int list_devices = 0;
	int device_number = 1;
	char device_name[STRING_LENGTH];
	char filename[STRING_LENGTH];
	char filetype_string[4] = "pgm";
	int run_command = 0;
	char command[STRING_LENGTH];
	int show_renderer = 0;
	
	// Other variables
	char char_buffer[STRING_LENGTH];

	// Default device name and output filename
	strcpy(device_name, "");
	
	// Information message
	fprintf(stderr, "\nRobotEyez.exe - http://batchloaf.wordpress.com\n");
	fprintf(stderr, "Written by Ted Burke - this version 30-11-2011\n");
	fprintf(stderr, "Copyright Ted Burke, 2011, All rights reserved.\n\n");
	
	// Parse command line arguments. Available options:
	//
	//		/delay DELAY_IN_MILLISECONDS
	//		/period PERIOD_IN_MILLISECONDS
	//		/frames NUMBER_OF_FRAMES
	//		/number_files
	//		/devnum DEVICE_NUMBER
	//		/devname DEVICE_NAME
	//		/command COMMAND_TO_RUN
	//		/devlist
	//		/preview
	//		/bmp
	//
	int n = 1;
	while (n < argc)
	{
		// Process next command line argument
		if (strcmp(argv[n], "/devlist") == 0)
		{
			// Set flag to list devices rather than capture image
			list_devices = 1;
		}
		else if (strcmp(argv[n], "/preview") == 0)
		{
			// Set flag to list devices rather than capture image
			show_renderer = 1;
		}
		else if (strcmp(argv[n], "/delay") == 0)
		{
			// Set snapshot delay to specified value
			if (++n < argc) delay = atoi(argv[n]);
			else exit_message("Error: invalid delay specified", 1);
			
			if (delay <= 0)
				exit_message("Error: invalid delay specified", 1);
		}
		else if (strcmp(argv[n], "/period") == 0)
		{
			// Set time period between captures
			if (++n < argc) period = atoi(argv[n]);
			else exit_message("Error: invalid period specified", 1);
			
			if (period <= 0)
				exit_message("Error: invalid period specified", 1);
		}
		else if (strcmp(argv[n], "/frames") == 0)
		{
			// Set number of frames to capture
			if (++n < argc) frames = atoi(argv[n]);
			else exit_message("Error: invalid number of frames specified", 1);
		}
		else if (strcmp(argv[n], "/number_files") == 0)
		{
			// Set flag to include numbers in image filenames
			number_files = 1;
		}
		else if (strcmp(argv[n], "/devnum") == 0)
		{
			// Set device number to specified value
			if (++n < argc) device_number = atoi(argv[n]);
			else exit_message("Error: invalid device number", 1);
			
			if (device_number <= 0)
				exit_message("Error: invalid device number", 1);
		}
		else if (strcmp(argv[n], "/devname") == 0)
		{
			// Set device number to specified value
			if (++n < argc)
			{
				// Copy device name into char buffer
				strcpy(char_buffer, argv[n]);
				
				// Trim inverted commas if present and copy
				// provided string into device_name
				if (char_buffer[0] == '"')
				{
					strncat(device_name, char_buffer, strlen(char_buffer)-2);
				}
				else
				{
					strcpy(device_name, char_buffer);
				}
				
				// Remember to choose by name rather than number
				device_number = 0;
			}
			else exit_message("Error: invalid device name", 1);
		}
		else if (strcmp(argv[n], "/command") == 0)
		{
			// Set command to specified string
			if (++n < argc)
			{
				// Copy device name into char buffer
				strncpy(command, argv[n], STRING_LENGTH);
				
				// Remember to choose by name rather than number
				run_command = 1;
			}
			else exit_message("Error: invalid command specified", 1);
		}
		else if (strcmp(argv[n], "/bmp") == 0)
		{
			// Set flag to list devices rather than capture image
			strcpy(filetype_string, "bmp");
		}
		else
		{
			// Unknown command line argument
			fprintf(stderr, "Unrecognised option: %s\n", argv[n]);
			exit_message("", 1);			
		}
		
		// Increment command line argument counter
		n++;
	}
	
	// Intialise COM
	hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (hr != S_OK)
		exit_message("Could not initialise COM", 1);

	// Create filter graph
	hr = CoCreateInstance(CLSID_FilterGraph, NULL,
			CLSCTX_INPROC_SERVER, IID_IGraphBuilder,
			(void**)&pGraph);
	if (hr != S_OK)
		exit_message("Could not create filter graph", 1);
	
	// Create capture graph builder.
	hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL,
			CLSCTX_INPROC_SERVER, IID_ICaptureGraphBuilder2,
			(void **)&pBuilder);
	if (hr != S_OK)
		exit_message("Could not create capture graph builder", 1);

	// Attach capture graph builder to graph
	hr = pBuilder->SetFiltergraph(pGraph);
	if (hr != S_OK)
		exit_message("Could not attach capture graph builder to graph", 1);

	// Create system device enumerator
	hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL,
			CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDevEnum));
	if (hr != S_OK)
		exit_message("Could not crerate system device enumerator", 1);

	// Video input device enumerator
	hr = pDevEnum->CreateClassEnumerator(
					CLSID_VideoInputDeviceCategory, &pEnum, 0);
	if (hr != S_OK)
		exit_message("No video devices found", 1);
	
	// If the user has included the "/list" command line
	// argument, just list available devices, then exit.
	if (list_devices != 0)
	{
		fprintf(stderr, "Available capture devices:\n");
		n = 0;
		while(1)
		{
			// Find next device
			hr = pEnum->Next(1, &pMoniker, NULL);
			if (hr == S_OK)
			{
				// Increment device counter
				n++;
				
				// Get device name
				hr = pMoniker->BindToStorage(0, 0, IID_PPV_ARGS(&pPropBag));
				VARIANT var;
				VariantInit(&var);
				hr = pPropBag->Read(L"FriendlyName", &var, 0);
				fprintf(stderr, "  %d. %ls\n", n, var.bstrVal);
				VariantClear(&var);
			}
			else
			{
				// Finished listing device, so exit program
				if (n == 0) exit_message("No devices found", 0);
				else exit_message("", 0);
			}
		}
	}
	
	// Get moniker for specified video input device,
	// or for the first device if no device number
	// was specified.
	VARIANT var;
	n = 0;
	while(1)
	{
		// Access next device
		hr = pEnum->Next(1, &pMoniker, NULL);
		if (hr == S_OK)
		{
			n++; // increment device count
		}
		else
		{
			if (device_number == 0)
			{
				fprintf(stderr,
					"Video capture device %s not found\n",
					device_name);
			}
			else
			{
				fprintf(stderr,
					"Video capture device %d not found\n",
					device_number);
			}
			exit_message("", 1);
		}
		
		// If device was specified by name rather than number...
		if (device_number == 0)
		{
			// Get video input device name
			hr = pMoniker->BindToStorage(0, 0, IID_PPV_ARGS(&pPropBag));
			if (hr == S_OK)
			{
				// Get current device name
				VariantInit(&var);
				hr = pPropBag->Read(L"FriendlyName", &var, 0);
				
				// Convert to a normal C string, i.e. char*
				sprintf(char_buffer, "%ls", var.bstrVal);
				VariantClear(&var);
				pPropBag->Release();
				pPropBag = NULL;
				
				// Exit loop if current device name matched devname
				if (strcmp(device_name, char_buffer) == 0) break;
			}
			else
			{
				exit_message("Error getting device names", 1);
			}
		}
		else if (n >= device_number) break;
	}
	
	// Get video input device name
	hr = pMoniker->BindToStorage(0, 0, IID_PPV_ARGS(&pPropBag));
	VariantInit(&var);
	hr = pPropBag->Read(L"FriendlyName", &var, 0);
	fprintf(stderr, "Capture device: %ls\n", var.bstrVal);
	VariantClear(&var);
	
	// Create capture filter and add to graph
	hr = pMoniker->BindToObject(0, 0,
					IID_IBaseFilter, (void**)&pCap);
	if (hr != S_OK) exit_message("Could not create capture filter", 1);
		
	// Add capture filter to graph
	hr = pGraph->AddFilter(pCap, L"Capture Filter");
	if (hr != S_OK) exit_message("Could not add capture filter to graph", 1);

	// Create frame transform filter
	FrameTransformFilter *pFrameTransformFilter = NULL;
	pFrameTransformFilter = new FrameTransformFilter();
	if (!pFrameTransformFilter)
		exit_message("Could not create frame transform filter", 1);
	// NB Object will be automatically deleted when pTransform is released
	hr = pFrameTransformFilter->QueryInterface(
			IID_IBaseFilter, reinterpret_cast<void**>(&pTransform));
	if (hr != S_OK)
		exit_message("Could not get IBaseFilter interface of transform filter", 1);
	
	// Add transform filter to graph
	hr = pGraph->AddFilter(pTransform, L"FrameTransform");
	if (hr != S_OK)
		exit_message("Could not add frame transform filter to filter graph", 1);
	
	// Create Null Renderer filter
	hr = CoCreateInstance(CLSID_NullRenderer, NULL,
		CLSCTX_INPROC_SERVER, IID_IBaseFilter,
		(void**)&pNullRenderer);
	if (hr != S_OK)
		exit_message("Could not create Null Renderer filter", 1);
	
	// Add Null Renderer filter to filter graph
	hr = pGraph->AddFilter(pNullRenderer, L"NullRender");
	if (hr != S_OK)
		exit_message("Could not add Null Renderer to filter graph", 1);

		// Connect up the filter graph's preview stream
	if (show_renderer)
	{
		hr = pBuilder->RenderStream(
				&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video,
				pCap, pTransform, NULL);
	}
	else
	{
		hr = pBuilder->RenderStream(
				&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video,
				pCap, pTransform, pNullRenderer);
	}
	if (hr != S_OK && hr != VFW_S_NOPREVIEWPIN)
		exit_message("Could not render preview video stream", 1);
			
	// Get media control interfaces to graph builder object
	hr = pGraph->QueryInterface(IID_IMediaControl,
					(void**)&pMediaControl);
	if (hr != S_OK)
		exit_message("Could not get media control interface", 1);
	
	// Run graph
	while(1)
	{
		hr = pMediaControl->Run();
		
		// Hopefully, the return value was S_OK or S_FALSE
		if (hr == S_OK) break; // graph is now running
		if (hr == S_FALSE) continue; // graph still preparing to run
		
		// If the Run function returned something else,
		// there must be a problem
		fprintf(stderr, "Error: %u\n", hr);
		exit_message("Could not run filter graph", 1);
	}
	
	// A working message loop in the application thread
	// seems to be required to keep things moving.
	// Otherwise, the video preview window just goes white
	// after a couple of seconds. This may be because
	// some of the filters are instantiated directly here
	// in the main function (e.g. the transform filter)
	//
	// See the following link for more info:
	// http://msdn.microsoft.com/en-us/library/windows/desktop/dd407349%28v=vs.85%29.aspx
	//
	MSG msg;
	DWORD start_time = GetTickCount();
	DWORD last_frame_time = start_time;
	DWORD current_time = start_time;
	if (run_command) pFrameTransformFilter->setCommand(command);
	
	// Message loop
	while(1)
	{
		// Using PeekMessage instead of GetMessage so that
		// the thread doesn't block when no messages are
		// received which might otherwise prevent capture
		// from stopping promptly when the specified time
		// has elapsed.
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) == 0)
		{
			// Exit if requested number of frames have been captured.
			// If number of frames specified was less than zero, then
			// continue capturing indefinitely.
			if ((pFrameTransformFilter->filesSaved() >= frames) &&
				(frames >= 0)) break;
			
			// Check if capture time has elapsed
			current_time = GetTickCount();
			if (current_time < start_time + delay) continue;
			if (current_time < last_frame_time + period) continue;
			
			// Save next frame to PGM file
			if (number_files)
			{
				sprintf(filename, "frame%04d.%s",
					pFrameTransformFilter->filesSaved() + 1,
					filetype_string);
			}
			else
			{
				sprintf(filename, "frame.%s", filetype_string);
			}
			pFrameTransformFilter->saveNextFrameToFile(filename);
			last_frame_time = current_time;
		}
		else
		{
			// A message was available, so process it now
			if (msg.message == WM_QUIT) break;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	// Stop the graph
	hr = pMediaControl->Stop();
	if (hr != S_OK) exit_message("Error stopping graph", 1);

	// Clean up and exit
	fprintf(stderr, "Stopped capturing. Now exiting.");
	exit_message("", 0);
}
