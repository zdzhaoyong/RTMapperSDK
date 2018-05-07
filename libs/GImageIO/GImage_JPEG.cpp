
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <iostream>

extern "C" {
#include "libjpeg/jpeglib.h"
#include "libjpeg/jerror.h"
}

#include "GImage_IO.h"

namespace GSLAM {

GImage imread_jpeg(const char* imFile)
{
    GImage img;

    FILE *file = fopen(imFile, "rb");
    if ( file == NULL ) return img;

    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    JSAMPROW row_pointer[1];

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);


    jpeg_stdio_src(&cinfo, file);
    jpeg_read_header(&cinfo, TRUE);

    jpeg_start_decompress(&cinfo);
    int row_stride = cinfo.output_width * cinfo.output_components;

    if( cinfo.output_components == 1 ) {
        img = GImage::create(cinfo.output_height, cinfo.output_width,
                             GImageType<uint8_t, 1>::Type);
        if( img.empty() ) return img;

        unsigned char *dst = img.data;
        while (cinfo.output_scanline < cinfo.output_height) {
            row_pointer[0] = &dst[ cinfo.output_scanline * row_stride];
            jpeg_read_scanlines(&cinfo, row_pointer, 1);
        }
    } else {
        img = GImage::create(cinfo.output_height, cinfo.output_width,
                             GImageType<uint8_t, 3>::Type);
        if( img.empty() ) return img;

        unsigned char *dst = img.data;
        std::vector<unsigned char> buf_(row_stride);
        unsigned char *buf = buf_.data();

        while (cinfo.output_scanline < cinfo.output_height) {
            row_pointer[0] = buf;
            jpeg_read_scanlines(&cinfo, row_pointer, 1);

            for(int i=0; i<img.cols; i++) {
                dst[i*cinfo.output_components + 0] = buf[i*cinfo.output_components + 2];
                dst[i*cinfo.output_components + 1] = buf[i*cinfo.output_components + 1];
                dst[i*cinfo.output_components + 2] = buf[i*cinfo.output_components + 0];
            }

            dst += row_stride;
        }
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);

    fclose(file);

    return img;
}

int imwrite_jpeg(const char* imFile, GImage& img)
{
    if( img.empty() ) return -1;

    int imgW, imgH, imgC, imgStride;
    imgW = img.cols;
    imgH = img.rows;
    imgC = img.channels();
    imgStride = img.elemSize()*imgW;

    std::vector<unsigned char> buf_(imgStride*imgH);
    unsigned char *buf = buf_.data();

    // convert BGR->RGB
    if( img.channels() == 1 ) {
        memcpy(buf, img.data, imgH*imgStride);
    } else {
        for(int j=0; j<imgH; j++) {
            for(int i=0; i<imgW; i++) {
                buf[j*imgStride + i*img.elemSize() + 0] = img.data[j*imgStride + i*img.elemSize() + 2];
                buf[j*imgStride + i*img.elemSize() + 1] = img.data[j*imgStride + i*img.elemSize() + 1];
                buf[j*imgStride + i*img.elemSize() + 2] = img.data[j*imgStride + i*img.elemSize() + 0];
            }
        }
    }

    // begin write to jpeg file
    FILE* fp = fopen(imFile, "wb");
    if( fp == NULL ) return -2;

    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    JSAMPROW row_pointer[1];

    cinfo.err = jpeg_std_error( &jerr );
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, fp);

    cinfo.image_width = imgW;
    cinfo.image_height = imgH;
    cinfo.input_components = imgC;
    cinfo.in_color_space = JCS_RGB;
    jpeg_set_defaults( &cinfo );
    jpeg_set_quality (&cinfo, 90, true);

    jpeg_start_compress( &cinfo, TRUE );
    while( cinfo.next_scanline < cinfo.image_height ) {
        row_pointer[0] = &buf[cinfo.next_scanline * cinfo.image_width * cinfo.input_components];
        jpeg_write_scanlines( &cinfo, row_pointer, 1 );
    }

    jpeg_finish_compress( &cinfo );
    jpeg_destroy_compress( &cinfo );

    fclose( fp );

    return 0;
}

} // end of namespace GSLAM
