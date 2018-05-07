#ifndef __GIMAGE_IO_H__
#define __GIMAGE_IO_H__

#include "GSLAM/core/GImage.h"

namespace GSLAM {

GImage imread(const char* imFile);
int imwrite(const char* imFile, GImage& img);

} // end of namespace GSLAM


#endif // end of __GIMAGE_IO_H__
