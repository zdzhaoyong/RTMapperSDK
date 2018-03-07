#include "GSLAM/core/GSLAM.h"

#define USE_RTMAPPER_PLUGIN(C) extern "C"{\
    SPtr<RTMapper> createRTMapperInstance(){return SPtr<RTMapper>(new C());}}

class RTMapper;
typedef SPtr<RTMapper> RTMapperPtr;
typedef RTMapperPtr (*funcCreateRTMapperInstance)();
class RTMapper : public GSLAM::SLAM
{
public:
    virtual std::string type()const{return "RTMapper";}
    virtual bool        valid()const{return false;}

    virtual bool        track(GSLAM::FramePtr& frame){return false;}

    virtual bool        isDrawable()const{return false;}
    virtual void        draw(){}

    virtual void        call(const std::string& command,void* arg=NULL){}
    virtual bool        setCallback(GSLAM::GObjectHandle* cbk){return false;}
    virtual bool        addLogSink(google::LogSink *sink){return false;}

    // added
    virtual bool        setSvar(GSLAM::Svar& var){return false;}
    virtual std::string version()const{return "0.0.0";}
    virtual bool        setLicense(const std::string &lic){return false;}
    virtual int         getProductID(std::string& email,std::string& pid){return -1;}
    virtual int         getLicenseStatus(){return -1;}
    virtual double      getExpiredTime(){return -1;}

    static RTMapperPtr create(const std::string& pluginPath="libRTMapperSDK"){
        SPtr<GSLAM::SharedLibrary> plugin=GSLAM::Registry::get(pluginPath);
        if(!plugin) return RTMapperPtr();
        funcCreateRTMapperInstance createFunc=(funcCreateRTMapperInstance)plugin->getSymbol("createRTMapperInstance");
        if(!createFunc) return RTMapperPtr();
        else return createFunc();
    }
};

extern "C"{
SPtr<RTMapper> createRTMapperInstance();
}

