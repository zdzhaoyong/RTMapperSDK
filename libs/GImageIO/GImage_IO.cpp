#include <stdlib.h>
#include <string>

#include "GImage_IO.h"


namespace GSLAM {

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

extern GImage imread_jpeg(const char* imFile);
extern int imwrite_jpeg(const char* imFile, GImage& img);

extern GImage imread_bmp(const char* imFile);
extern int imwrite_bmp(const char* imFile, GImage& img);

extern GImage imread_png(const char* imFile);
extern int imwrite_png(const char* imFile, GImage& img);


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

std::string str_tolower(std::string& s)
{
    for(int i=0; i < s.size(); i++) {
        s[i] = tolower(s[i]);
    }

    return s;
}

std::string path_extname(const std::string& fname)
{
    std::string extname;
    size_t      found;

    // find .
    found = fname.find_last_of(".");
    if( found == std::string::npos ) {
        extname = "";
    } else {
        extname = fname.substr(found);
    }

    return extname;
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

GImage imread(const char* imFile)
{
    std::string extName_ = path_extname(imFile);
    std::string extname = str_tolower(extName_);

    if( extname == ".jpg" || extname == ".jpeg" )
        return imread_jpeg(imFile);
    else if( extname == ".png" )
        return imread_png(imFile);
    else if( extname == ".bmp" )
        return imread_bmp(imFile);
    else
        return GImage();
}

int imwrite(const char* imFile, GImage& img)
{
    std::string extName_ = path_extname(imFile);
    std::string extname = str_tolower(extName_);

    if( extname == ".jpg" || extname == ".jpeg" )
        return imwrite_jpeg(imFile, img);
    else if( extname == ".png" )
        return imwrite_png(imFile, img);
    else if( extname == ".bmp" )
        return imwrite_bmp(imFile, img);
    else
        return -10;
}


} // end of namespace GSLAM

