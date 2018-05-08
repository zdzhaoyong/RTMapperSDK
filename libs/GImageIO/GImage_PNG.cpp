
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <iostream>

extern "C" {
#include "libpng/png.h"
}

#include "GImage_IO.h"

namespace GSLAM {

// https://gist.github.com/louisdx/998257
// http://zarb.org/~gc/html/libpng.html

GImage imread_png(const char* imFile)
{
    GImage img;

    png_structp png_ptr;
    png_infop info_ptr = NULL;
    png_infop end_ptr = NULL;
    png_bytep row_buf = NULL;

    // initialize PNG structure
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) return img;

    info_ptr = png_create_info_struct(png_ptr);
    end_ptr = png_create_info_struct(png_ptr);
    if ( !info_ptr || !end_ptr ) {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        return img;
    }

    // open file
    FILE *fp = fopen(imFile, "rb");
    if( !fp ) {
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_ptr);
        return img;
    }

    // begin png read
    png_init_io(png_ptr, fp);
    png_set_read_status_fn(png_ptr, NULL);		//for reporting stuff as the rows complete reading

    //getting a crash here with my windows library
    png_read_info(png_ptr, info_ptr);

    png_uint_32 width = 0;
    png_uint_32 height = 0;
    int bit_depth = 0;
    int color_type = 0;
    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, NULL, NULL, NULL);

    //do any conversions that we can
    if (color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png_ptr);
    //if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) png_set_gray_1_2_4_to_8(png_ptr);
    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png_ptr);
    if (bit_depth == 16) png_set_strip_16(png_ptr);
    if (bit_depth < 8) png_set_packing(png_ptr);


    int num_pass = png_set_interlace_handling(png_ptr);
    png_read_update_info(png_ptr, info_ptr);

    int rowbytes = png_get_rowbytes(png_ptr, info_ptr);
    int channels = png_get_channels(png_ptr, info_ptr);

    row_buf = (png_bytep) png_malloc(png_ptr, rowbytes);


    bool hasAlpha = color_type & PNG_COLOR_MASK_ALPHA;
    int bytespp = hasAlpha ? 4 : 3;
    std::vector<unsigned char> imgdata(height * rowbytes);

    if( channels == 1 ) {
        img = GImage::create(height, width, GImageType<uint8_t, 1>::Type);
        if( img.empty() ) return img;

        unsigned char *dst = img.data;
        for (int y = 0; y < (int)height; y++) {
            png_read_rows(png_ptr, (png_bytepp)&row_buf, NULL, 1);
            memcpy(dst, row_buf, rowbytes);
            dst += width;
        }
    } else {
        img = GImage::create(height, width, GImageType<uint8_t, 3>::Type);
        if( img.empty() ) return img;

        unsigned char *dst = img.data;

        for (int y = 0; y < (int)height; y++) {
            png_read_rows(png_ptr, (png_bytepp)&row_buf, NULL, 1);

            for(int x=0; x<width; x++) {
                dst[y*3*width + x*3 + 0] = row_buf[x*bytespp+2];
                dst[y*3*width + x*3 + 1] = row_buf[x*bytespp+1];
                dst[y*3*width + x*3 + 2] = row_buf[x*bytespp+0];
            }
        }
    }


    png_free_data(png_ptr, info_ptr, PNG_FREE_UNKN, -1);
    png_read_end(png_ptr, end_ptr);

    if( row_buf ) png_free(png_ptr, row_buf);
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_ptr);

    if( fp != NULL ) fclose(fp);

    return img;
}

int imwrite_png(const char* imFile, GImage& img)
{
    if( img.empty() ) return -1;

    int imgW, imgH, imgC, imgStride;
    imgW = img.cols;
    imgH = img.rows;
    imgC = img.channels();
    imgStride = img.elemSize()*imgW;

    // open output file
    FILE *fp = fopen(imFile, "wb");
    if( !fp ) return -1;

    // initialize png structures
    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if( !png_ptr ) {
        fclose(fp);
        return -2;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if( !info_ptr ) {
        fclose(fp);
        png_destroy_write_struct(&png_ptr, NULL);
        return -3;
    }

    png_init_io(png_ptr, fp);

    png_uint_32 width = imgW;
    png_uint_32 height = imgH;
    int bit_depth = 8;	//bits per channel, not bits per pixel
    int color_type = imgC == 3 ? PNG_COLOR_TYPE_RGB : PNG_COLOR_TYPE_GRAY;
    png_set_IHDR(png_ptr, info_ptr, width, height, bit_depth, color_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    png_write_info(png_ptr, info_ptr);

    if( imgC == 1 ) {
        std::vector<unsigned char> buf(width);

        unsigned char *src = img.data;
        unsigned char *dst = buf.data();
        for (int y = 0; y < (int)height; y++) {
            memcpy(dst, src+y*width, width);
            png_write_row(png_ptr, dst);
        }
    } else {
        std::vector<unsigned char> buf(width*3);

        unsigned char *src = img.data;
        unsigned char *dst = buf.data();

        for (int y = 0; y < (int)height; y++) {
            for(int i=0; i<width; i++) {
                dst[i*3 + 0] = src[y*imgStride + i*3 + 2];
                dst[i*3 + 1] = src[y*imgStride + i*3 + 1];
                dst[i*3 + 2] = src[y*imgStride + i*3 + 0];
            }

            png_write_row(png_ptr, dst);
        }
    }

    png_write_end(png_ptr, NULL);
    png_destroy_write_struct(&png_ptr, &info_ptr);

    return 0;
}

} // end of namespace GSLAM
