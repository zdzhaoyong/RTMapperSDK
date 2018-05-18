#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdint.h>
#include <string>
#include <vector>


////////////////////////////////////////////////////////////////////////////////
/// string utils
////////////////////////////////////////////////////////////////////////////////

// string array type
typedef std::vector<std::string> StringArray;


/**
 * @brief split text by delims
 *
 * @param intext        - [in] input string
 * @param delims        - [in] delims, eg.: ' ', '-'
 *
 * @return StringArray data
 */
StringArray split_text(const std::string &intext, const std::string &delims);

/**
 * @brief join text to form a string
 *
 * @param sa            - [in] input StringArray
 * @param delims        - [in] delims, eg.: ' ', '-'
 *
 * @return string data
 */
std::string join_text(const StringArray& sa, const std::string& delims);


/**
 * @brief convert given string to upper case (use itself)
 *
 * @param s             - [in,out] string
 *
 * @return converted string
 */
std::string& str_toupper(std::string &s);


/**
 * @brief convert given string to lower case (use itself)
 *
 * @param s             - [in,out] string
 *
 * @return converted string
 */
std::string& str_tolower(std::string &s);


/**
 * @brief trim a const string, return a trimed string
 *
 * @param s             - [in] string
 *
 * @return trimed string
 */
std::string trim(const std::string &s);

/**
 * @brief trim a string, return itself
 * @param s             - [in,out] input string, and trimed string
 *
 * @return trimed string
 */
std::string& trim2(std::string& s);


/**
 * @brief trim a string in left side, return a trimed string
 * @param s             - [in] input string
 *
 * @return trimed string
 */
std::string ltrim(const std::string &s);

/**
 * @brief trim a string in right side, return a trimed string
 * @param s             - [in] input string
 *
 * @return trimed string
 */
std::string rtrim(const std::string &s);



// string to int, float, double
int    str_to_int(const std::string &s);
float  str_to_float(const std::string &s);
double str_to_double(const std::string &s);


////////////////////////////////////////////////////////////////////////////////
/// time utils
////////////////////////////////////////////////////////////////////////////////

// get milli-second
uint64_t tm_get_millis(void);

// get milli-second
uint64_t tm_get_ms(void);

// get micro-second
uint64_t tm_get_us(void);

// get timestamp since 1970 (unit is second)
double tm_getTimeStamp(void);

// get timestamp since 1970 (unit is second) from string with format of strptime
double tm_getTimeStamp(const char* dtStr, const char* fmt);

// get UNIX timestamp since 1970 (unit is second)
uint32_t tm_getTimeStampUnix(void);

// sleep for t milli-seconds
void tm_sleep(uint32_t t);

// sleep for t micro-seconds
void tm_sleep_us(uint64_t t);


////////////////////////////////////////////////////////////////////////////////
/// file utils
////////////////////////////////////////////////////////////////////////////////

// text file function
int readlines(const std::string &fn, StringArray &lns, int bufSize=8192);

// get file length
uint64_t filelength(FILE *fp);
uint64_t filelength(const char *fname);


////////////////////////////////////////////////////////////////////////////////
/// path utils
////////////////////////////////////////////////////////////////////////////////

// check whether given dir name is exist (true) or not (false)
bool path_exist(const std::string& fnpath);

// make a new directory (support sub-dir)
int path_mkdir(const std::string& path);

// remove a path (support sub-dir or files)
int path_rmdir(const std::string& path);

// remove a file
int path_rmfile(const std::string& path);

// determin given path is dir/file
int path_isdir(const std::string &p);
int path_isdir(const std::string &p);

// list given path
int path_lsdir(const std::string &dir_name, StringArray &dl, int sortFiles=1);



// get file name from a given path
std::string path_getFileName(const std::string& fname);
// get path name from a given path
std::string path_getPathName(const std::string& fname);

// get file base name (before '.')
std::string path_getFileBase(const std::string& fname);
// get file ext name (ex. '.jpg')
std::string path_getFileExt(const std::string& fname);


// split path & file name
StringArray path_split(const std::string &fname);
StringArray path_splitext(const std::string &fname);

std::string path_join(const std::string &p1, const std::string &p2);
std::string path_join(const std::string &p1, const std::string &p2, const std::string &p3);
std::string path_join(const StringArray &p);

#endif // end of __UTILS_H__

