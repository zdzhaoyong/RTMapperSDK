
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <iostream>

#include "GImage_IO.h"

namespace GSLAM {

GImage imread_bmp(const char* imFile)
{
    GImage img;
    int ret = 0;
    int w, h;

    unsigned char bmp_header[] =
    {
        66,77,54,16,14,0,0,0,0,0,54,0,0,0,40,0,0,0,128,2,0,0,224,1,0,0,
        1,0,24,0,0,0,0,0,0,16,14,0,18,11,0,0,18,11,0,0,0,0,0,0,0,0,0,0
    };

    // read file
    FILE* fp = fopen(imFile, "rb");
    if( fp == NULL ) return img;

    ret = fread(bmp_header, 54, 1, fp);
    w = bmp_header[19]*256 + bmp_header[18];
    h = bmp_header[23]*256 + bmp_header[22];

    int bmpStride = w*3;
    if( bmpStride % 4 != 0 ) bmpStride = (bmpStride/4+1)*4;

    // read image data
    std::vector<uint8_t> buf_(h*bmpStride);
    ret = fread(buf_.data(), h*bmpStride, 1, fp);
    fclose(fp);

    // create image & copy data
    img = GImage::create(h, w, GImageType<uint8_t, 3>::Type);
    if( img.empty() ) return img;

    uint8_t *ps = buf_.data();
    uint8_t *pd = img.data;

    for(int j=0; j<h; j++) {
        int ji = h-1 - j;
        for(int i=0; i<w; i++) {
            pd[ji*w*3 + i*3 + 0] = ps[j*bmpStride + i*3 + 0];
            pd[ji*w*3 + i*3 + 1] = ps[j*bmpStride + i*3 + 1];
            pd[ji*w*3 + i*3 + 2] = ps[j*bmpStride + i*3 + 2];
        }
    }

    return img;
}

int imwrite_bmp(const char* imFile, GImage& img)
{
    if( img.empty() ) return -1;

    int imgW, imgH, imgC, imgStride;
    imgW = img.cols;
    imgH = img.rows;
    imgC = img.channels();
    imgStride = img.elemSize()*imgW;

    int bmpStride = imgW*3;
    if( bmpStride % 4 != 0 ) bmpStride = (bmpStride/4+1)*4;

    unsigned char bmp_header[] =
    {
        66,77,54,16,14,0,0,0,0,0,54,0,0,0,40,0,0,0,128,2,0,0,224,1,0,0,
        1,0,24,0,0,0,0,0,0,16,14,0,18,11,0,0,18,11,0,0,0,0,0,0,0,0,0,0
    };

    std::vector<unsigned char> buf_(bmpStride*imgH);
    unsigned char *pd = buf_.data();

    // set file header
    bmp_header[18] = imgW%256;
    bmp_header[19] = imgW/256;
    bmp_header[22] = imgH%256;
    bmp_header[23] = imgH/256;

    // convert BGR->RGB
    if( img.channels() == 1 ) {
        for(int j=0; j<imgH; j++) {
            int ji = imgH-1 - j;
            for(int i=0; i<imgW; i++) {
                pd[ji*bmpStride + i*3 + 0] = img.data[j*imgStride + i*img.elemSize() + 0];
                pd[ji*bmpStride + i*3 + 1] = img.data[j*imgStride + i*img.elemSize() + 0];
                pd[ji*bmpStride + i*3 + 2] = img.data[j*imgStride + i*img.elemSize() + 0];
            }
        }
    } else {
        for(int j=0; j<imgH; j++) {
            int ji = imgH-1 - j;
            for(int i=0; i<imgW; i++) {
                pd[ji*bmpStride + i*3 + 0] = img.data[j*imgStride + i*img.elemSize() + 0];
                pd[ji*bmpStride + i*3 + 1] = img.data[j*imgStride + i*img.elemSize() + 1];
                pd[ji*bmpStride + i*3 + 2] = img.data[j*imgStride + i*img.elemSize() + 2];
            }
        }
    }

    // write to file
    FILE* fp = fopen(imFile, "wb");
    if( fp == NULL ) return -2;

    fwrite(bmp_header, 54, 1, fp);
    fwrite(pd, imgH*bmpStride, 1, fp);
    fclose(fp);


    return 0;
}

} // end of namespace GSLAM
