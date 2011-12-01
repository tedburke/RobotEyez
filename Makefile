#
# RobotEyez makefile
# Written by Ted Burke - last modified 30-11-2011
#
# Copyright Ted Burke, 2011, All rights reserved.
#
# Website: http://batchloaf.wordpress.com
#

RobotEyez.exe: RobotEyez.cpp FrameTransformFilter.cpp FrameTransformFilter.h
	cl RobotEyez.cpp FrameTransformFilter.cpp /I"C:\Program Files\Microsoft SDKs\Windows\v7.1\Samples\multimedia\directshow\baseclasses" /MD -link /LIBPATH:"C:\Program Files\Microsoft SDKs\Windows\v7.1\Samples\multimedia\directshow\baseclasses\Release" user32.lib ole32.lib strmiids.lib oleaut32.lib strmbase.lib winmm.lib
