//
// RobotEyez.cpp - A simple machine vision program
// Written by Ted Burke - last modified 23-11-2011
//
// Copyright Ted Burke, 2011, All rights reserved.
//
// Website: http://batchloaf.wordpress.com
//

// DirectShow header files
#include <dshow.h>

#include "FrameTransformFilter.h"

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
IMediaControl *pMediaControl = NULL;

void exit_message(const char* error_message, int error)
{
	// Print an error message
	fprintf(stderr, error_message);
	fprintf(stderr, "\n");
	
	// Clean up DirectShow / COM stuff
	if (pMediaControl != NULL) pMediaControl->Release();
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
	int capture_duration = 2000;
	int list_devices = 0;
	int device_number = 1;
	char device_name[100];
	
	// Other variables
	char char_buffer[100];

	// Default device name and output filename
	strcpy(device_name, "");
	
	// Information message
	fprintf(stderr, "\nRobotEyez.exe - http://batchloaf.wordpress.com\n");
	fprintf(stderr, "Written by Ted Burke - this version 23-11-2011\n");
	fprintf(stderr, "Copyright Ted Burke, 2011, All rights reserved.\n\n");
	
	// Parse command line arguments. Available options:
	//
	//		/duration DURATION_IN_MILLISECONDS
	//		/devnum DEVICE_NUMBER
	//		/devname DEVICE_NAME
	//		/devlist
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
		else if (strcmp(argv[n], "/duration") == 0)
		{
			// Set snapshot delay to specified value
			if (++n < argc) capture_duration = atoi(argv[n]);
			else exit_message("Error: invalid duration specified", 1);
			
			if (capture_duration <= 0)
				exit_message("Error: invalid duration specified", 1);
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
	hr = pFrameTransformFilter->QueryInterface(IID_IBaseFilter, reinterpret_cast<void**>(&pTransform));
	if (hr != S_OK)
		exit_message("Could not get IBaseFilter interface to frame transform filter", 1);
	
	// Add transform filter to graph
	hr = pGraph->AddFilter(pTransform, L"FrameTransform");
	if (hr != S_OK)
		exit_message("Could not add frame transform filter to filter graph", 1);
	
	// Connect up the filter graph's preview stream
	/*
	hr = pBuilder->RenderStream(
			&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video,
			pCap, pTransform, NULL);
	if (hr != S_OK && hr != VFW_S_NOPREVIEWPIN)
		exit_message("Could not render preview video stream", 1);
	*/
	hr = pBuilder->RenderStream(
			&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video,
			pCap, pTransform, NULL);
	if (hr != S_OK && hr != VFW_S_NOPREVIEWPIN)
		exit_message("Could not render preview video stream", 1);
	
	// Get media control interfaces to graph builder object
	hr = pGraph->QueryInterface(IID_IMediaControl,
					(void**)&pMediaControl);
	if (hr != S_OK) exit_message("Could not get media control interface", 1);
	
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
	
	// Wait for specified time delay (if any)
	Sleep(capture_duration);
	
	// Stop the graph
	hr = pMediaControl->Stop();
	if (hr != S_OK) exit_message("Error stopping graph", 1);

	// Clean up and exit
	fprintf(stderr, "Stopped capturing. Now exiting.");
	exit_message("", 0);
}
