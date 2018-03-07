/*
  TinyEXIF.cpp -- A simple ISO C++ library to parse basic EXIF and XMP
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

#include "TinyEXIF.h"
#include <GSLAM/core/XML.h>
#include <algorithm>
#include <cstdint>
#include <stdio.h>
#include <vector>
#include <cfloat>

#ifdef _MSC_VER
#include <tchar.h>
#else
#include <strings.h>
#define _tcsncmp         	strncmp
#define _tcsicmp         	strcasecmp
#endif

#define _T(c) c

using namespace std;
namespace TinyEXIF {

enum JPEG_MARKERS {
	JM_START = 0xFF,
	JM_SOF0  = 0xC0,
	JM_SOF1  = 0xC1,
	JM_SOF2  = 0xC2,
	JM_SOF3  = 0xC3,
	JM_DHT   = 0xC4,
	JM_SOF5  = 0xC5,
	JM_SOF6  = 0xC6,
	JM_SOF7  = 0xC7,
	JM_JPG   = 0xC8,
	JM_SOF9  = 0xC9,
	JM_SOF10 = 0xCA,
	JM_SOF11 = 0xCB,
	JM_DAC   = 0xCC,
	JM_SOF13 = 0xCD,
	JM_SOF14 = 0xCE,
	JM_SOF15 = 0xCF,
	JM_RST0  = 0xD0,
	JM_RST1  = 0xD1,
	JM_RST2  = 0xD2,
	JM_RST3  = 0xD3,
	JM_RST4  = 0xD4,
	JM_RST5  = 0xD5,
	JM_RST6  = 0xD6,
	JM_RST7  = 0xD7,
	JM_SOI   = 0xD8,
	JM_EOI   = 0xD9,
	JM_SOS   = 0xDA,
	JM_DQT	 = 0xDB,
	JM_DNL	 = 0xDC,
	JM_DRI	 = 0xDD,
	JM_DHP	 = 0xDE,
	JM_EXP	 = 0xDF,
	JM_APP0	 = 0xE0,
	JM_APP1  = 0xE1, // EXIF and XMP
	JM_APP2  = 0xE2,
	JM_APP3  = 0xE3,
	JM_APP4  = 0xE4,
	JM_APP5  = 0xE5,
	JM_APP6  = 0xE6,
	JM_APP7  = 0xE7,
	JM_APP8  = 0xE8,
	JM_APP9  = 0xE9,
	JM_APP10 = 0xEA,
	JM_APP11 = 0xEB,
	JM_APP12 = 0xEC,
	JM_APP13 = 0xED, // IPTC
	JM_APP14 = 0xEE,
	JM_APP15 = 0xEF,
	JM_JPG0	 = 0xF0,
	JM_JPG1	 = 0xF1,
	JM_JPG2	 = 0xF2,
	JM_JPG3	 = 0xF3,
	JM_JPG4	 = 0xF4,
	JM_JPG5	 = 0xF5,
	JM_JPG6	 = 0xF6,
	JM_JPG7	 = 0xF7,
	JM_JPG8	 = 0xF8,
	JM_JPG9	 = 0xF9,
	JM_JPG10 = 0xFA,
	JM_JPG11 = 0xFB,
	JM_JPG12 = 0xFC,
	JM_JPG13 = 0xFD,
	JM_COM   = 0xFE
};


// Parser helper
class EntryParser {
private:
	const uint8_t* buf;
	const unsigned len;
	const unsigned tiff_header_start;
	const bool alignIntel; // byte alignment (defined in EXIF header)
	unsigned offs; // current offset into buffer
	uint16_t tag, format;
	uint32_t length;

public:
	EntryParser(const uint8_t* _buf, unsigned _len, unsigned _tiff_header_start, bool _alignIntel)
		: buf(_buf), tiff_header_start(_tiff_header_start), len(_len), alignIntel(_alignIntel), offs(0) {}

	void Init(unsigned _offs) {
		offs = _offs - 12;
	}

	uint16_t ParseTag() {
		offs  += 12;
		tag    = parse16(buf + offs, alignIntel);
		format = parse16(buf + offs + 2, alignIntel);
		length = parse32(buf + offs + 4, alignIntel);
		return tag;
	}
	uint16_t GetTag() const { return tag; }
	uint32_t GetLength() const { return length; }
	uint32_t GetData() const { return parse32(buf + offs + 8, alignIntel); }
	uint32_t GetSubIFD() const { return tiff_header_start + GetData(); }

	bool IsRational() const { return format == 5 || format == 10; }

	bool Fetch(std::string& val) const {
		if (format != 2)
			return false;
		val = parseEXIFString(buf, length, GetData(), tiff_header_start, len, alignIntel);
		return true;
	}
	bool Fetch(uint8_t& val) const {
		if (format != 1 && format != 2 && format != 6)
			return false;
		val = parse8(buf + offs + 8);
		return true;
	}
	bool Fetch(uint16_t& val) const {
		if (format != 3)
			return false;
		val = parse16(buf + offs + 8, alignIntel);
		return true;
	}
	bool Fetch(uint32_t& val) const {
		if (format != 4)
			return false;
		val = parse32(buf + offs + 8, alignIntel);
		return true;
	}
	bool Fetch(double& val) const {
		if (!IsRational())
			return false;
		val = parseEXIFRational(buf + GetSubIFD(), alignIntel);
		return true;
	}
	bool Fetch(double& val, uint32_t idx) const {
		if (!IsRational() || length <= idx)
			return false;
		val = parseEXIFRational(buf + GetSubIFD() + idx*8, alignIntel);
		return true;
	}

public:
	static uint8_t parse8(const uint8_t* buf) {
		return buf[0];
	}
	static uint16_t parse16(const uint8_t* buf, bool intel) {
		if (intel)
			return ((uint16_t)buf[1]<<8) | buf[0];
		return ((uint16_t)buf[0]<<8) | buf[1];
	}
	static uint32_t parse32(const uint8_t* buf, bool intel) {
		if (intel)
			return ((uint32_t)buf[3]<<24) |
				((uint32_t)buf[2]<<16) |
				((uint32_t)buf[1]<<8)  |
				buf[0];
		return ((uint32_t)buf[0]<<24) |
			((uint32_t)buf[1]<<16) |
			((uint32_t)buf[2]<<8)  |
			buf[3];
	}
	static double parseEXIFRational(const uint8_t* buf, bool intel) {
		const uint32_t denominator = parse32(buf+4, intel);
		if (denominator == 0)
			return 0.0;
		const uint32_t numerator = parse32(buf, intel);
		return (double)(int32_t)numerator/(double)(int32_t)denominator;
	}
	static std::string parseEXIFString(const uint8_t* buf,
		unsigned num_components,
		unsigned data,
		unsigned base,
		unsigned len,
		bool intel)
	{
		std::string value;
		if (num_components <= 4) {
			value.resize(num_components);
			char j = intel ? 0 : 24;
			char j_m = intel ? -8 : 8;
			for (unsigned i=0; i<num_components; ++i, j -= j_m)
				value[i] = data >> j & 0xff;
			if (value[num_components-1] == '\0')
				value.resize(num_components-1);
		} else
		if (base+data+num_components <= len) {
			const char* const sz((const char*)buf+base+data);
			if (sz[--num_components] == '\0')
				while (num_components && sz[num_components-1] == '\0')
					--num_components;
			value.assign(sz, num_components);
		}
		return value;
	}
};



int EXIF::parseFile(const std::string& filePath,GSLAM::Svar& myVar)
{
    std::ifstream file(filePath, std::ifstream::in|std::ifstream::binary);
    if(!file.is_open())
    {
        std::cout<<"Failed to open file ."<<std::endl;
        return -1;
    }
    file.seekg(0,std::ios::end);
    std::streampos length = file.tellg();
    file.seekg(0,std::ios::beg);
    std::vector<uint8_t> data(length);
    file.read((char*)data.data(), length);
    int ret= parseFrom(data.data(),length,myVar);

    parseCamera(myVar);
    parsePitchYawRoll(myVar);
    return ret;
}
//
// Locates the JM_APP1 segment and parses it using
// parseFromEXIFSegment() or parseFromXMPSegment()
//
int EXIF::parseFrom(const uint8_t* buf, unsigned len,GSLAM::Svar& myVar) {
	// Sanity check: all JPEG files start with 0xFFD8 and end with 0xFFD9
	// This check also ensures that the user has supplied a correct value for len.
	if (!buf || len < 16)
		return PARSE_EXIF_ERROR_NO_EXIF;
	if (buf[0] != JM_START || buf[1] != JM_SOI)
		return PARSE_EXIF_ERROR_NO_JPEG;
	// not always valid, sometimes 0xFF is added for padding
	//if (buf[len-2] != JM_START || buf[len-1] != JM_EOI)
	//  return PARSE_EXIF_ERROR_NO_JPEG;

	// Scan for JM_APP1 header (bytes 0xFF 0xE1) and parse its length.
	// Exit if both EXIF and XMP sections were parsed.
	enum {
		APP1_NA = 0,
		APP1_EXIF = (1 << 0),
		APP1_XMP = (1 << 1),
		APP1_ALL = APP1_EXIF|APP1_XMP
	};
	struct APP1S {
		uint32_t val;
		inline APP1S() : val(APP1_NA) {}
		inline operator uint32_t () const { return val; }
		inline operator uint32_t& () { return val; }
		inline int operator () (int code=PARSE_EXIF_ERROR_NO_EXIF) const { return (val&APP1_EXIF) == 0 ? code : (int)PARSE_EXIF_SUCCESS; }
	} app1s;
	for (unsigned pos=2; pos<len; ) {
		// find next marker
		uint8_t marker, prev(0);
		do {
			marker = buf[pos++];
			if (marker != JM_START && prev == JM_START)
				break;
			prev = marker;
		} while (pos<len);
		// select marker
		switch (marker) {
		case 0x00:
		case 0x01:
		case JM_RST0:
		case JM_RST1:
		case JM_RST2:
		case JM_RST3:
		case JM_RST4:
		case JM_RST5:
		case JM_RST6:
		case JM_RST7:
		case JM_SOI:
			break;
		case JM_SOS: // start of stream: and we're done
		case JM_EOI: // no data? not good
			return app1s();
		case JM_APP1: {
			const uint16_t section_length(EntryParser::parse16(buf + pos, false));
			int ret;
            switch (ret=parseFromEXIFSegment(buf + pos + 2, section_length - 2,myVar)) {
			case PARSE_EXIF_ERROR_NO_EXIF:
                switch (ret=parseFromXMPSegment(buf + pos + 2, section_length - 2,myVar)) {
				case PARSE_EXIF_ERROR_NO_XMP:
					break;
				case PARSE_EXIF_SUCCESS:
					if ((app1s|=APP1_XMP) == APP1_ALL)
						return PARSE_EXIF_SUCCESS;
					break;
				default:
					return app1s(ret); // some error
				}
				break;
			case PARSE_EXIF_SUCCESS:
				if ((app1s|=APP1_EXIF) == APP1_ALL)
					return PARSE_EXIF_SUCCESS;
				break;
			default:
				return app1s(ret); // some error
			}
		}
		default: {
			// read section length
			const uint16_t section_length(EntryParser::parse16(buf + pos, false));
			if (pos + section_length > len)
				return app1s(PARSE_EXIF_ERROR_NO_JPEG);
			// skip the section
			pos += section_length;
		}
		}
	}
	return app1s();
}

int EXIF::parseFrom(const std::string& data,GSLAM::Svar& myVar) {
    return parseFrom((const uint8_t*)data.data(), (unsigned)data.length(),myVar);
}

//
// Main parsing function for an EXIF segment.
// Do a sanity check by looking for bytes "Exif\0\0".
// The marker has to contain at least the TIFF header, otherwise the
// JM_APP1 data is corrupt. So the minimum length specified here has to be:
//   6 bytes: "Exif\0\0" string
//   2 bytes: TIFF header (either "II" or "MM" string)
//   2 bytes: TIFF magic (short 0x2a00 in Motorola byte order)
//   4 bytes: Offset to first IFD
// =========
//  14 bytes
//
// PARAM: 'buf' start of the EXIF TIFF, which must be the bytes "Exif\0\0".
// PARAM: 'len' length of buffer
//
int EXIF::parseFromEXIFSegment(const uint8_t* buf, unsigned len,GSLAM::Svar& _svar) {
	unsigned offs = 0;        // current offset into buffer
	if (!buf || len < 6)
		return PARSE_EXIF_ERROR_NO_EXIF;

	if (!std::equal(buf, buf+6, "Exif\0\0"))
		return PARSE_EXIF_ERROR_NO_EXIF;
	offs += 6;

	// Now parsing the TIFF header. The first two bytes are either "II" or
	// "MM" for Intel or Motorola byte alignment. Sanity check by parsing
	// the uint16_t that follows, making sure it equals 0x2a. The
	// last 4 bytes are an offset into the first IFD, which are added to 
	// the global offset counter. For this block, we expect the following
	// minimum size:
	//  2 bytes: 'II' or 'MM'
	//  2 bytes: 0x002a
	//  4 bytes: offset to first IDF
	// -----------------------------
	//  8 bytes
	if (offs + 8 > len)
		return PARSE_EXIF_ERROR_CORRUPT;
	const unsigned tiff_header_start = offs;
	if (buf[offs] == 'I' && buf[offs+1] == 'I')
		ByteAlign = 1;
	else {
		if (buf[offs] == 'M' && buf[offs+1] == 'M')
			ByteAlign = 0;
		else
			return PARSE_EXIF_ERROR_UNKNOWN_BYTEALIGN;
	}
	offs += 2;
	if (0x2a != EntryParser::parse16(buf + offs, alignIntel()))
		return PARSE_EXIF_ERROR_CORRUPT;
	offs += 2;
	const unsigned first_ifd_offset = EntryParser::parse32(buf + offs, alignIntel());
	offs += first_ifd_offset - 4;
	if (offs >= len)
		return PARSE_EXIF_ERROR_CORRUPT;

	// Now parsing the first Image File Directory (IFD0, for the main image).
	// An IFD consists of a variable number of 12-byte directory entries. The
	// first two bytes of the IFD section contain the number of directory
	// entries in the section. The last 4 bytes of the IFD contain an offset
	// to the next IFD, which means this IFD must contain exactly 6 + 12 * num
	// bytes of data.
	if (offs + 2 > len)
		return PARSE_EXIF_ERROR_CORRUPT;
	int num_entries = EntryParser::parse16(buf + offs, alignIntel());
	if (offs + 6 + 12 * num_entries > len)
		return PARSE_EXIF_ERROR_CORRUPT;
	unsigned exif_sub_ifd_offset = len;
	unsigned gps_sub_ifd_offset  = len;
	EntryParser parser(buf, len, tiff_header_start, alignIntel());
	parser.Init(offs+2);
	while (--num_entries >= 0) {
		switch (parser.ParseTag()) {
		case 0x0102:
			// Bits per sample
            uint16_t BitsPerSample;
			parser.Fetch(BitsPerSample);
            _svar.i["BitsPerSample"]=BitsPerSample;
			break;

		case 0x010e:
			// Image description
            parser.Fetch(_svar.GetString("ImageDescription",""));
			break;

		case 0x010f:
			// Camera maker
            parser.Fetch(_svar.GetString("Make",""));
			break;

		case 0x0110:
            // Camera model
            parser.Fetch(_svar.GetString("Model",""));
			break;

		case 0x0112:
			// Orientation of image
            uint16_t Orientation;
            if(parser.Fetch(Orientation))
                _svar.GetInt("Orientation",Orientation);
			break;

		case 0x011a:
			// XResolution 
            double XResolution;
            if(parser.Fetch(XResolution))
                _svar.GetDouble("XResolution",XResolution)=XResolution;
			break;

		case 0x011b:
			// YResolution 
            double YResolution;
            if(parser.Fetch(YResolution))
                _svar.GetDouble("YResolution",YResolution)=YResolution;
			break;

		case 0x0128:
			// Resolution Unit
            uint16_t ResolutionUnit;
            if(parser.Fetch(ResolutionUnit))
                _svar.GetInt("ResolutionUnit",ResolutionUnit)=ResolutionUnit;
			break;

		case 0x0131:
			// Software used for image
            parser.Fetch(_svar.GetString("Software",""));
			break;

		case 0x0132:
			// EXIF/TIFF date/time of image modification
            parser.Fetch(_svar.GetString("DateTime",""));
			break;

		case 0x1001:
			// Original Image width
            uint32_t RelatedImageWidth;
            if (!parser.Fetch(RelatedImageWidth)) {
				uint16_t _RelatedImageWidth;
				if (parser.Fetch(_RelatedImageWidth))
                    _svar.GetInt("RelatedImageWidth") = _RelatedImageWidth;
			}
            else _svar.GetInt("RelatedImageWidth")=RelatedImageWidth;
			break;

		case 0x1002:
			// Original Image height
            uint32_t RelatedImageHeight;
			if (!parser.Fetch(RelatedImageHeight)) {
				uint16_t _RelatedImageHeight;
				if (parser.Fetch(_RelatedImageHeight))
                    _svar.GetInt("RelatedImageHeight") = _RelatedImageHeight;
			}
			break;

		case 0x8298:
			// Copyright information
            parser.Fetch(_svar.GetString("Copyright",""));
			break;

		case 0x8769:
			// EXIF SubIFD offset
			exif_sub_ifd_offset = parser.GetSubIFD();
			break;

		case 0x8825:
			// GPS IFS offset
			gps_sub_ifd_offset = parser.GetSubIFD();
			break;

		case 0xa405:
			// Focal length in 35mm film
            double FocalLengthIn35mm;
            if (!parser.Fetch(FocalLengthIn35mm)) {
				uint16_t _FocalLengthIn35mm;
				if (parser.Fetch(_FocalLengthIn35mm))
                    FocalLengthIn35mm = (double)_FocalLengthIn35mm;
			}
            _svar.d["FocalLengthIn35mm"]=FocalLengthIn35mm;
			break;
		}
	}

	// Jump to the EXIF SubIFD if it exists and parse all the information
	// there. Note that it's possible that the EXIF SubIFD doesn't exist.
	// The EXIF SubIFD contains most of the interesting information that a
	// typical user might want.
	if (exif_sub_ifd_offset + 4 <= len) {
		offs = exif_sub_ifd_offset;
		int num_entries = EntryParser::parse16(buf + offs, alignIntel());
		if (offs + 6 + 12 * num_entries > len)
			return PARSE_EXIF_ERROR_CORRUPT;
		parser.Init(offs+2);
		while (--num_entries >= 0) {
			switch (parser.ParseTag()) {
			case 0x829a:
				// Exposure time in seconds
                parser.Fetch(_svar.d["ExposureTime"]);
				break;

			case 0x829d:
				// FNumber
                parser.Fetch(_svar.d["FNumber"]);
				break;

			case 0x8827:
				// ISO Speed Rating
                uint16_t ISOSpeedRatings;
                if(parser.Fetch(ISOSpeedRatings))
                    _svar.GetInt("ISOSpeedRatings",ISOSpeedRatings)=ISOSpeedRatings;
				break;

			case 0x9003:
				// Original date and time
                parser.Fetch(_svar.s["DateTimeOriginal"]);
				break;

			case 0x9004:
				// Digitization date and time
                parser.Fetch(_svar.s["DateTimeDigitized"]);
				break;

			case 0x9201:
				// Shutter speed value
                parser.Fetch(_svar.d["ShutterSpeedValue"]);
				break;

			case 0x9202:
				// Aperture value
                parser.Fetch(_svar.d["ApertureValue"]);
				break;

			case 0x9203:
				// Brightness value
                parser.Fetch(_svar.d["BrightnessValue"]);
				break;

			case 0x9204:
				// Exposure bias value 
                parser.Fetch(_svar.d["ExposureBiasValue"]);
				break;

			case 0x9206:
				// Subject distance
                parser.Fetch(_svar.d["SubjectDistance"]);
				break;

			case 0x9209:
				// Flash used
                uint16_t Flash;
				parser.Fetch(Flash);
                _svar.i["Flash"]=Flash;
				break;

			case 0x920a:
				// Focal length
                parser.Fetch(_svar.d["FocalLength"]);
				break;

			case 0x9207:
				// Metering mode
                uint16_t MeteringMode;
                if(parser.Fetch(MeteringMode))
                    _svar.i["MeteringMode"]=MeteringMode;
				break;

			case 0x9291:
				// Fractions of seconds for DateTimeOriginal
                parser.Fetch(_svar.s["SubSecTimeOriginal"]);
				break;

			case 0xa002:
            {
				// EXIF Image width
                uint32_t ImageWidth=0;
				if (!parser.Fetch(ImageWidth)) {
					uint16_t _ImageWidth;
					if (parser.Fetch(_ImageWidth))
						ImageWidth = _ImageWidth;
				}
                _svar.i["ImageWidth"]=ImageWidth;
            }
				break;

			case 0xa003:
            {
				// EXIF Image height
                uint32_t ImageHeight=0;
				if (!parser.Fetch(ImageHeight)) {
					uint16_t _ImageHeight;
					if (parser.Fetch(_ImageHeight))
						ImageHeight = _ImageHeight;
				}
                _svar.i["ImageHeight"]=ImageHeight;
                break;
            }

			case 0xa20e:
				// Focal plane X resolution
                parser.Fetch(_svar.d["FocalPlaneXResolution"]);
				break;

			case 0xa20f:
				// Focal plane Y resolution
                parser.Fetch(_svar.d["FocalPlaneYResolution"]);
				break;

			case 0xa210:
            {
				// Focal plane resolution unit
                uint16_t FocalPlaneResolutionUnit;
                if(parser.Fetch(FocalPlaneResolutionUnit))
                    _svar.i["FocalPlaneResolutionUnit"]=FocalPlaneResolutionUnit;
            }
				break;

			case 0xa405:
            {
				// Focal length in 35mm film
                double FocalLengthIn35mm=0;
                if (!parser.Fetch(FocalLengthIn35mm)) {
					uint16_t _FocalLengthIn35mm;
					if (parser.Fetch(_FocalLengthIn35mm))
                        FocalLengthIn35mm = (double)_FocalLengthIn35mm;
				}
                _svar.d["FocalLengthIn35mm"]=FocalLengthIn35mm;
				break;
            }

			case 0xa432:
            {
                // Focal length and FStop.
                if (parser.Fetch(_svar.d["FocalLengthMin"], 0))
                    if (parser.Fetch(_svar.d["FocalLengthMax"], 1))
                        if (parser.Fetch(_svar.d["FStopMin"], 2))
                            parser.Fetch(_svar.d["FStopMax"], 3);
            }
				break;

			case 0xa433:
				// Lens make.
                parser.Fetch(_svar.s["LensInfo.Make"]);
				break;

			case 0xa434:
				// Lens model.
                parser.Fetch(_svar.s["LensInfo.Model"]);
				break;
			}
		}
	}

	// Jump to the GPS SubIFD if it exists and parse all the information
	// there. Note that it's possible that the GPS SubIFD doesn't exist.
	if (gps_sub_ifd_offset + 4 <= len) {
		offs = gps_sub_ifd_offset;
		int num_entries = EntryParser::parse16(buf + offs, alignIntel());
		if (offs + 6 + 12 * num_entries > len)
			return PARSE_EXIF_ERROR_CORRUPT;
		parser.Init(offs+2);
		while (--num_entries >= 0) {
			switch (parser.ParseTag()) {
			case 1:
				// GPS north or south
                uint8_t LatComponents_direction;
                parser.Fetch(LatComponents_direction);
                _svar.i["LatComponents.direction"]=LatComponents_direction;
				break;

			case 2:
				// GPS latitude
				if (parser.IsRational() && parser.GetLength() == 3) {
                    parser.Fetch(_svar.d["LatComponents.degrees"], 0);
                    parser.Fetch(_svar.d["LatComponents.minutes"], 1);
                    parser.Fetch(_svar.d["LatComponents.seconds"], 2);
				}
				break;

			case 3:
				// GPS east or west
                uint8_t LonComponents_direction;
                if(parser.Fetch(LonComponents_direction))
                    _svar.i["LonComponents.direction"]=LonComponents_direction;
				break;

			case 4:
				// GPS longitude
				if (parser.IsRational() && parser.GetLength() == 3) {
                    parser.Fetch(_svar.d["LonComponents.degrees"], 0);
                    parser.Fetch(_svar.d["LonComponents.minutes"], 1);
                    parser.Fetch(_svar.d["LonComponents.seconds"], 2);
				}
				break;

			case 5:
				// GPS altitude reference (below or above sea level)
                uint8_t AltitudeRef;
                if(parser.Fetch(AltitudeRef))
                    _svar.i["AltitudeRef"]=AltitudeRef;
				break;

			case 6:
				// GPS altitude
                parser.Fetch(_svar.d["Altitude"]);
				break;

			case 7:
				// GPS timestamp
				if (parser.IsRational() && parser.GetLength() == 3) {
					double h,m,s;
					parser.Fetch(h, 0);
					parser.Fetch(m, 1);
					parser.Fetch(s, 2);
					char buffer[256];
					snprintf(buffer, 256, "%g %g %g", h, m, s);
                    _svar.s["GPSTimeStamp"]=buffer;
				}
				break;

			case 11:
				// Indicates the GPS DOP (data degree of precision)
                parser.Fetch(_svar.d["GPSDOP"]);
				break;

			case 18:
				// GPS geodetic survey data
                parser.Fetch(_svar.s["GPSMapDatum"]);
				break;

			case 29:
				// GPS date-stamp
                parser.Fetch(_svar.s["GPSDateStamp"]);
				break;

			case 30:
				// GPS differential indicates whether differential correction is applied to the GPS receiver
                uint16_t GPSDifferential;
                if(parser.Fetch(GPSDifferential))
                    _svar.i["GPSDifferential"]=GPSDifferential;
				break;
			}
        }
        parseCoords(_svar);
	}

	return PARSE_EXIF_SUCCESS;
}

//
// Main parsing function for a XMP segment.
// Do a sanity check by looking for bytes "http://ns.adobe.com/xap/1.0/\0".
// So the minimum length specified here has to be:
//  29 bytes: "http://ns.adobe.com/xap/1.0/\0" string
//
// PARAM: 'buf' start of the XMP header, which must be the bytes "http://ns.adobe.com/xap/1.0/\0".
// PARAM: 'len' length of buffer
//
int EXIF::parseFromXMPSegment(const uint8_t* buf, unsigned len,GSLAM::Svar& _svar) {
	struct Tools {
		static const char* strrnstr(const char* haystack, const char* needle, size_t len) {
			const size_t needle_len(strlen(needle));
			if (0 == needle_len)
				return haystack;
			if (len <= needle_len)
				return NULL;
			for (size_t i=len-needle_len; i-- > 0; ) {
				if (haystack[0] == needle[0] &&
					0 == _tcsncmp(haystack, needle, needle_len))
					return haystack;
				haystack++;
			}
			return NULL;
		}
	};

	unsigned offs = 0; // current offset into buffer
	if (!buf || len < 29)
		return PARSE_EXIF_ERROR_NO_XMP;

	if (!std::equal(buf, buf+29, "http://ns.adobe.com/xap/1.0/\0"))
		return PARSE_EXIF_ERROR_NO_XMP;
	offs += 29;
	if (offs >= len)
		return PARSE_EXIF_ERROR_CORRUPT;
	len -= offs;

	// Skip xpacket end section so that tinyxml2 lib parses the section correctly.
	const char* const strXMP((const char*)(buf + offs)), *strEnd;
	if ((strEnd=Tools::strrnstr(strXMP, "<?xpacket end=", len)) != NULL)
		len = (unsigned)(strEnd - strXMP);

	// Now try parsing the XML packet.
	tinyxml2::XMLDocument doc;
	const tinyxml2::XMLElement* document;
	if (doc.Parse(strXMP, len) != tinyxml2::XML_SUCCESS ||
		((document=doc.FirstChildElement(_T("x:xmpmeta"))) == NULL && (document=doc.FirstChildElement(_T("xmp:xmpmeta"))) == NULL) ||
		(document=document->FirstChildElement(_T("rdf:RDF"))) == NULL ||
		(document=document->FirstChildElement(_T("rdf:Description"))) == NULL)
		return PARSE_EXIF_SUCCESS;

    for( const tinyxml2::XMLAttribute* a = document->FirstAttribute(); a; a = a->Next() ) {
        _svar.insert(a->Name(),a->Value());
    }
	return PARSE_EXIF_SUCCESS;
}


void EXIF::parseCoords(GSLAM::Svar& _svar) {
    if(!_svar.d.exist("LatComponents.degrees")) return;
    if(!_svar.d.exist("LatComponents.minutes")) return;
    if(!_svar.d.exist("LatComponents.seconds")) return;
    if(!_svar.d.exist("LonComponents.degrees")) return;
    if(!_svar.d.exist("LonComponents.minutes")) return;
    if(!_svar.d.exist("LonComponents.seconds")) return;

    // convert GPS latitude
    double& Latitude=_svar.d["Latitude"];
    Latitude =
            _svar.d["LatComponents.degrees"] +
            _svar.d["LatComponents.minutes"] / 60 +
            _svar.d["LatComponents.seconds"] / 3600;
    if ('S' == _svar.i["LatComponents.direction"])
        Latitude = -Latitude;

    // convert GPS longitude
    double& Longitude =_svar.d["Longitude"];
    Longitude=
            _svar.d["LonComponents.degrees"] +
            _svar.d["LonComponents.minutes"] / 60 +
            _svar.d["LonComponents.seconds"] / 3600;
    if ('W' ==_svar.i["LonComponents.direction"])
        Longitude = -Longitude;
    // convert GPS altitude

    double& Altitude=_svar.d["Altitude"];
    if(_svar.i["AltitudeRef"] == 1)
        Altitude = -Altitude;

}

void EXIF::parseCamera(GSLAM::Svar& _svar)
{
    stringstream sst;
    sst<<_svar.GetString("Model","Model")
     <<"_"<<_svar.GetDouble("FocalLength",0)
    <<"_"<<_svar.i["ImageWidth"]
    <<"x"<<_svar.i["ImageHeight"];
    string cameraName=sst.str();
    int idx=-1;
    while((idx=cameraName.find(' '))!=string::npos)
        cameraName.at(idx)='_';
    _svar.s["CameraName"]=cameraName;
}

inline double Factor2Double(std::string factor)
{
    std::string v(factor);
    int idx=v.find('/');
    if(idx==std::string::npos)
    {
        return std::stod(v);
    }
    else
    {
        std::string up=v.substr(0,idx);
        std::string down=v.substr(idx+1);
        return std::stod(up)/std::stod(down);
    }
    return 0;
}

void EXIF::parsePitchYawRoll(GSLAM::Svar& _svar)
{
    if(_svar.s["Make"]=="DJI")
    {
        if(_svar.exist("drone-dji:GimbalPitchDegree"))
            _svar.d["CameraPitch"]=_svar.GetDouble("drone-dji:GimbalPitchDegree",-90);
        if(_svar.exist("drone-dji:GimbalYawDegree"))
        {
            double& yaw=_svar.d["CameraYaw"];
            yaw=_svar.GetDouble("drone-dji:GimbalYawDegree",90);
        }
        if(_svar.exist("drone-dji:GimbalRollDegree"))
        {
            double& roll=_svar.d["CameraRoll"];
            roll=_svar.GetDouble("drone-dji:GimbalRollDegree",0);
            if(fabs(180-fabs(roll))<10) roll+=180;
        }
        if(_svar.exist("drone-dji:RelativeAltitude"))
            _svar.d["RelativeAltitude"]=_svar.GetDouble("drone-dji:RelativeAltitude",0);
    }
    else if(_svar.s["Make"]=="Xaircraft")
    {
        if(_svar.exist("Camera:Pitch"))
            _svar.d["CameraPitch"]=(Factor2Double(_svar.GetString("Camera:Pitch","0"))-90);
        if(_svar.exist("Camera:Yaw"))
            _svar.d["CameraYaw"]=(Factor2Double(_svar.GetString("Camera:Yaw","0")));
        if(_svar.exist("Camera:Roll"))
            _svar.d["CameraRoll"]=(Factor2Double(_svar.GetString("Camera:Roll","0")));
        if(_svar.exist("Camera:GPSXYAccuracy"))
            _svar.d["LongitudeSigma"]=_svar.d["LatitudeSigma"]=(Factor2Double(_svar.GetString("Camera:GPSXYAccuracy","5")));
        if(_svar.exist("Camera:GPSZAccuracy"))
            _svar.d["AltitudeSigma"]=(Factor2Double(_svar.GetString("Camera:GPSZAccuracy","10")));
    }
    else if(_svar.s["Make"]=="Parrot")
    {
        if(_svar.exist("Camera:Pitch"))
            _svar.d["CameraPitch"]=(Factor2Double(_svar.GetString("Camera:Pitch","0"))-90);
        if(_svar.exist("Camera:Yaw"))
            _svar.d["CameraYaw"]=(Factor2Double(_svar.GetString("Camera:Yaw","0"))+90);
        if(_svar.exist("Camera:Roll"))
            _svar.d["CameraRoll"]=(Factor2Double(_svar.GetString("Camera:Roll","0")));
        if(_svar.exist("Camera:GPSXYAccuracy"))
            _svar.d["LongitudeSigma"]=_svar.d["LatitudeSigma"]=(Factor2Double(_svar.GetString("Camera:GPSXYAccuracy","5")));
        if(_svar.exist("Camera:GPSZAccuracy"))
            _svar.d["AltitudeSigma"]=(Factor2Double(_svar.GetString("Camera:GPSZAccuracy","10")));
    }
}

} // namespace TinyEXIF
