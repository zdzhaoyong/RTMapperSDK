/*
  TinyEXIF.h -- A simple ISO C++ library to parse basic EXIF and XMP
                information from a JPEG file.

  Copyright (c) 2015-2016 Seacave
  cdc.seacave@gmail.com
  All rights reserved.

  Based on the easyexif library (2013 version)
    https://github.com/mayanklahiri/easyexif
  of Mayank Lahiri (mlahiri@gmail.com).
  
  Redistribution and use in source and binary forms, with or without 
  modification, are permitted provided that the following conditions are met:

  -- Redistributions of source code must retain the above copyright notice, 
     this list of conditions and the following disclaimer.
  -- Redistributions in binary form must reproduce the above copyright notice, 
     this list of conditions and the following disclaimer in the documentation 
   and/or other materials provided with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY EXPRESS 
   OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
   OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN 
   NO EVENT SHALL THE FREEBSD PROJECT OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
   INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
   BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY 
   OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
   EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/**
  string Make
  string Model

  string CameraName
  string CameraParameters
  double Timestamp
  double Longitude
  double Latitude
  double Altitude
  double RelativeAltitude
  double CameraPitch
  double CameraYaw
  double CameraRoll
  double LongitudeSigma
  double LatitudeSigma
  double AltitudeSigma
  double RelativeAltitudeSigma


  **/
#ifndef __TINYEXIF_H__
#define __TINYEXIF_H__

#include <string>
#include <vector>
#include <GSLAM/core/Svar.h>
#include <math.h>

namespace TinyEXIF {

enum ErrorCode {
	PARSE_EXIF_SUCCESS                 = 0, // Parse was successful
	PARSE_EXIF_ERROR_NO_JPEG           = 1, // No JPEG markers found in buffer, possibly invalid JPEG file
	PARSE_EXIF_ERROR_NO_EXIF           = 2, // No EXIF header found in JPEG file
	PARSE_EXIF_ERROR_NO_XMP            = 3, // No XMP header found in JPEG file
	PARSE_EXIF_ERROR_UNKNOWN_BYTEALIGN = 4, // Byte alignment specified in EXIF file was unknown (not Motorola or Intel)
	PARSE_EXIF_ERROR_CORRUPT           = 5, // EXIF header was found, but data was corrupted
};

//
// Class responsible for storing and parsing EXIF information from a JPEG blob
//
class EXIF {
public:
    EXIF() { }

    int parseFile(const std::string& filePath,GSLAM::Svar& myVar);
    int parseFrom(const uint8_t* data, unsigned length, GSLAM::Svar& myVar);
    int parseFrom(const std::string &data,GSLAM::Svar& myVar);

private:

	// Parsing function for an EXIF segment. This is used internally by parseFrom()
	// but can be called for special cases where only the EXIF section is 
	// available (i.e., a blob starting with the bytes "Exif\0\0").
    int parseFromEXIFSegment(const uint8_t* buf, unsigned len, GSLAM::Svar& myVar);

	// Parsing function for an XMP segment. This is used internally by parseFrom()
	// but can be called for special cases where only the XMP section is 
	// available (i.e., a blob starting with the bytes "http://ns.adobe.com/xap/1.0/\0").
    int parseFromXMPSegment(const uint8_t* buf, unsigned len, GSLAM::Svar& myVar);

	// Return true if the memory alignment is Intel format.
	bool alignIntel() const { return this->ByteAlign != 0; }
    void parseCoords(GSLAM::Svar& _svar);             // Convert Latitude/Longitude from deg/min/sec to decimal
    void parseCamera(GSLAM::Svar& _svar);
    void parsePitchYawRoll(GSLAM::Svar& _svar);

    // Data fields filled out by parseFrom()
    uint8_t ByteAlign;                  // 0: Motorola byte alignment, 1: Intel
};

} // namespace TinyEXIF

#endif // __TINYEXIF_H__
