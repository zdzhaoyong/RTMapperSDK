
#include <GSLAM/core/Dataset.h>
#include <GSLAM/core/VecParament.h>
#include <GSLAM/core/Timer.h>
#include <GSLAM/core/XML.h>
#include <GSLAM/core/GPS.h>

#include "TinyEXIF.h"

#include "GImageIO/GImage_IO.h"
#include "utils.h"

using namespace std;
using namespace GSLAM;

class RTMapperFrame : public GSLAM::MapFrame
{
public:
    RTMapperFrame(const GSLAM::FrameID& id=0,const double& timestamp=0)
        :GSLAM::MapFrame(id,timestamp){
        GSLAM::WriteMutex lock(_mutexPose);
    }

    virtual std::string type()const{return "RTMapperFrame";}

    virtual int     cameraNum()const{return 1;}                                      // Camera number
    virtual GSLAM::SE3     getCameraPose(int idx=0) const{return GSLAM::SE3();}                    // The transform from camera to local
    virtual int     imageChannels(int idx=0) const{return GSLAM::IMAGE_BGRA;}               // Default is a colorful camera
    virtual GSLAM::Camera  getCamera(int idx=0){return _camera;}                            // The camera model

    virtual bool           setImage(const GSLAM::GImage &img, int idx, int channalMask)
    {
        GSLAM::WriteMutex lock(_mutexPose);
        if(idx==0)
        {
            _image=img;
        }
        else _thumbnail=img;
        return true;
    }

    virtual GSLAM::GImage  getImage(int idx=0,int channalMask=GSLAM::IMAGE_UNDEFINED){
        GSLAM::WriteMutex lock(_mutexPose);
        if(idx==0)
        {
            return _image;
        }
        else return _thumbnail;
    }   // Just return the image if only one channel is available

    // When the frame contains IMUs or GPSs
    virtual int     getIMUNum()const{return (_gpshpyr.size()==11||_gpshpyr.size()==12||_gpshpyr.size()==14)?1:0;}
    virtual bool    getAcceleration(GSLAM::Point3d& acc,int idx=0)const{return false;}        // m/s^2
    virtual bool    getAngularVelocity(GSLAM::Point3d& angularV,int idx=0)const{return false;}// rad/s
    virtual bool    getMagnetic(GSLAM::Point3d& mag,int idx=0)const{return false;}            // gauss
    virtual bool    getAccelerationNoise(GSLAM::Point3d& accN,int idx=0)const{return false;}
    virtual bool    getAngularVNoise(GSLAM::Point3d& angularVN,int idx=0)const{return false;}
    virtual bool    getPitchYawRoll(GSLAM::Point3d& pyr,int idx=0)const{
        if(_gpshpyr.size()==11&&_gpshpyr[8]<20) {pyr=GSLAM::Point3d(_gpshpyr[5],_gpshpyr[6],_gpshpyr[7]);return true;}
        else if(_gpshpyr.size()==14&&_gpshpyr[11]) {pyr=GSLAM::Point3d(_gpshpyr[8],_gpshpyr[9],_gpshpyr[10]);return true;}
        else if(_gpshpyr.size()==12&&_gpshpyr[9]<20) {pyr=GSLAM::Point3d(_gpshpyr[6],_gpshpyr[7],_gpshpyr[8]);return true;}
        return false;
    }     // in rad
    virtual bool    getPYRSigma(GSLAM::Point3d& pyrSigma,int idx=0)const{
        if(_gpshpyr.size()==11&&_gpshpyr[8]<20) {pyrSigma=GSLAM::Point3d(_gpshpyr[8],_gpshpyr[9],_gpshpyr[10]);return true;}
        else if(_gpshpyr.size()==14&&_gpshpyr[11]) {pyrSigma=GSLAM::Point3d(_gpshpyr[11],_gpshpyr[12],_gpshpyr[13]);return true;}
        else if(_gpshpyr.size()==12&&_gpshpyr[9]<20) {pyrSigma=GSLAM::Point3d(_gpshpyr[9],_gpshpyr[10],_gpshpyr[11]);return true;}
        return false;
    }    // in rad

    virtual int     getGPSNum()const{return (_gpshpyr.size()>=6&&_gpshpyr[3]<10)?1:0;}
    virtual bool    getGPSLLA(GSLAM::Point3d& LonLatAlt,int idx=0)const{
        if(getGPSNum()==0) return false;
        LonLatAlt=GSLAM::Point3d(_gpshpyr[0],_gpshpyr[1],_gpshpyr[2]);
        return _gpshpyr[3]<100;
    }        // WGS84 [longtitude latitude altitude]
    virtual bool    getGPSLLASigma(GSLAM::Point3d& llaSigma,int idx=0)const{
        if(_gpshpyr.size()>=6||_gpshpyr.size()==8||_gpshpyr.size()==12||_gpshpyr.size()==14)
        {llaSigma=GSLAM::Point3d(_gpshpyr[3],_gpshpyr[4],_gpshpyr[5]);return true;}
        else if(_gpshpyr.size()==7) {llaSigma=GSLAM::Point3d(_gpshpyr[3],_gpshpyr[3],_gpshpyr[4]);return true;}
        return false;
    }    // meter
    virtual bool    getGPSECEF(GSLAM::Point3d& xyz,int idx=0)const{
        GSLAM::Point3d lla;
        if(!getGPSLLA(lla)) return false;
        xyz=GSLAM::GPS<>::GPS2XYZ(lla.y,lla.x,lla.z);
        return true;
    }             // meter
    virtual bool    getHeight2Ground(GSLAM::Point2d& height,int idx=0)const{
        if(_gpshpyr.size()==14||_gpshpyr.size()==8){height=GSLAM::Point2d(_gpshpyr[6],_gpshpyr[7]);return _gpshpyr[7]<100;}
        return false;
    }    // height against ground

    virtual void call(const std::string &command, void *arg)
    {
        if("GetImagePath"==command)
        {
            if(arg) *(std::string*)arg=_imagePath;
        }
        else if("ReleaseImage"==command)
        {
            _image=GSLAM::GImage();
        }
        else if("GetGPS"==command)
        {
            if((!arg)||_gpshpyr.empty()) return;
            std::vector<double>* dest=(std::vector<double>*)arg;
            *dest=_gpshpyr;
        }
    }


    static GImage imread(std::string _imagePath){
        return GSLAM::imread(_imagePath.c_str());
    }

    std::string imagePath(){return _imagePath;}
    GSLAM::GImage& thumbnail(){return _thumbnail;}

    std::string         _imagePath;       // where image come from or where to cache
    std::string         _cameraName;
    GSLAM::GImage       _image,_thumbnail;// should be RGBA
    GSLAM::Camera       _camera;
    std::vector<double> _gpshpyr;
    // 6 : long,lat,alt,sigmaX,sigmaY,sigmaZ
    // 8 : long,lat,alt,sigmaX,sigmaY,sigmaZ,Height,sigmaH
    // 11: long,lat,alt,sigmaH,sigmaV,pitch,yaw,roll,sigmaP,sigmaR,sigmaP
    // 12: long,lat,alt,sigmaX,sigmaY,sigmaZ,pitch,yaw,roll,sigmaP,sigmaR,sigmaP
    // 14: long,lat,alt,sigmaX,sigmaY,sigmaZ,Height,sigmaH,pitch,yaw,roll,sigmaP,sigmaR,sigmaP
};

class DatasetImages : public Dataset
{
public:
    DatasetImages():_curIdx(0){

    }
    std::string         name() const{return _name;}
    virtual std::string type() const{return "DatasetImages";}
    virtual bool        isOpened(){return _imageFiles.size();}

    virtual FramePtr    grabFrame()
    {
        if(_curIdx>=_imageFiles.size()) return FramePtr();

        string imgFile=_imageFiles[_curIdx++];
        string refPath = _topDir + "/" + imgFile;
        if( !path_exist(imgFile) && path_exist(refPath) ) imgFile = refPath;

        printf("load frame[%4d] %s\n", _curIdx, imgFile.c_str());

        TinyEXIF::EXIF exif;
        GSLAM::Svar    var;
        if(exif.parseFile(imgFile,var)<0) return FramePtr();

        string dataTime=var.GetString("DateTime","");
        double Timestamp=var.GetDouble("Timestamp",0);
        if( dataTime.size() && !Timestamp )
        {
            Timestamp = tm_getTimeStamp(dataTime.c_str(), "%Y:%m:%d %H:%M%S");
        }

        vector<double> gpsInfo;
        gpsInfo.reserve(14);
        // 6 : long,lat,alt,sigmaX,sigmaY,sigmaZ
        // 8 : long,lat,alt,sigmaX,sigmaY,sigmaZ,Height,sigmaH
        // 11: long,lat,alt,sigmaH,sigmaV,pitch,yaw,roll,sigmaP,sigmaR,sigmaP
        // 12: long,lat,alt,sigmaX,sigmaY,sigmaZ,pitch,yaw,roll,sigmaP,sigmaR,sigmaP
        // 14: long,lat,alt,sigmaX,sigmaY,sigmaZ,Height,sigmaH,pitch,yaw,roll,sigmaP,sigmaR,sigmaP
        if(var.d.exist("Longitude")&&var.d.exist("Latitude")&&var.d.exist("Altitude"))
        {
            gpsInfo.push_back(var.d["Longitude"]);
            gpsInfo.push_back(var.d["Latitude"]);
            gpsInfo.push_back(var.d["Altitude"]);
            gpsInfo.push_back(var.GetDouble("LongitudeSigma",5));
            gpsInfo.push_back(var.GetDouble("LatitudeSigma",5));
            gpsInfo.push_back(var.GetDouble("AltitudeSigma",10));
        }
        if(var.d.exist("RelativeAltitude"))
        {
            gpsInfo.push_back(var.d["RelativeAltitude"]);
            gpsInfo.push_back(var.GetDouble("RelativeAltitudeSigma",10));
        }
        if(var.d.exist("CameraPitch")&&var.d.exist("CameraYaw")&&var.d.exist("CameraRoll"))
        {
            gpsInfo.push_back(var.d["CameraPitch"]);
            gpsInfo.push_back(var.d["CameraYaw"]);
            gpsInfo.push_back(var.d["CameraRoll"]);
            gpsInfo.push_back(var.GetDouble("CameraPitchSigma",1));
            gpsInfo.push_back(var.GetDouble("CameraYawSigma",10));
            gpsInfo.push_back(var.GetDouble("CameraRollSigma",1));
        }

        SPtr<RTMapperFrame> frame(new RTMapperFrame(_curIdx,Timestamp));
        frame->_cameraName=var.s["CameraName"];
        if(frame->_cameraName.empty()) {
            LOG(ERROR)<<"Failed to obtain camera name.";
            return FramePtr();
        }
        if(!_cameraMap.count(frame->_cameraName))
        {
            VecParament<double> paras;
            paras=svar.get_var(frame->_cameraName+".Paraments",paras);
            if(!paras.size()) LOG(ERROR)<<"Please set "<<frame->_cameraName+".Paraments";
            _cameraMap[frame->_cameraName]=GSLAM::Camera(paras.data);
        }
        frame->_camera=_cameraMap[frame->_cameraName];
        frame->_gpshpyr=gpsInfo;
        frame->_image=RTMapperFrame::imread(imgFile);

        return frame;
    }

    bool                openImages(const std::string& dataset)
    {
        ifstream ifs(dataset.c_str());
        string line;
        while(getline(ifs,line)) {
            _imageFiles.push_back(line);
        }
        return _imageFiles.size();
    }

    virtual bool        open(const std::string& dataset)
    {
        _imageFiles.clear();
        _curIdx=0;

        if( !path_exist(dataset) ) return false;

        _name = path_getFileBase(dataset);
        string extname = path_getFileExt(dataset);
        if( extname == ".imgs" )
        {
            _topDir = path_getPathName(dataset);
            return openImages(dataset);
        }

        return false;
    }

    virtual float processedPercentage()const{
        if(_imageFiles.empty()) return 0;
        return _curIdx/(float)_imageFiles.size();
    } // -1-->1

    std::string                         _topDir;
    std::map<std::string,Camera>        _cameraMap;
    std::vector<std::string>            _imageFiles;
    int                                 _curIdx;

};


class DroneMapKFDataset : public GSLAM::Dataset
{
    struct DroneMapFrame
    {
        double      timestamp;
        std::string imagePath;
        pi::SE3d    pose;
    };
public:
    DroneMapKFDataset():curIdx(0){}
    virtual std::string type() const{return "DroneMapKFDataset";}
    virtual bool        isOpened(){return frames.size();}

    virtual bool        open(const std::string& dataset)
    {
        string path=Svar::getFolderPath(dataset);
        Svar var;
        var.ParseFile(path+"/config.cfg");
        plane=var.get_var<pi::SE3d>("Plane",plane);
        origin=var.get_var("GPS.Origin",origin);

        // Local to ECEF
        local2ECEF.get_translation()=GSLAM::GPS<>::GPS2XYZ(Point3d(origin.y,origin.x,origin.z));
        double D2R=3.1415925/180.;
        double lon=origin.x*D2R;
        double lat=origin.y*D2R;
        GSLAM::Point3d up(cos(lon)*cos(lat), sin(lon)*cos(lat), sin(lat));
        GSLAM::Point3d east(-sin(lon), cos(lon), 0);
        GSLAM::Point3d north=up.cross(east);
        double R[9]={east.x, north.x, up.x,
                     east.y, north.y, up.y,
                     east.z, north.z, up.z};
        local2ECEF.get_rotation().fromMatrix(R);

        camParameters=var.get_var<VecParament<double> >("Camera.Paraments",VecParament<double>()).data;
        camera=GSLAM::Camera(camParameters);
        if(!camera.isValid()) return false;

        ifstream ifs(path+"/trajectory.txt");
        if(!ifs.is_open()) {
            LOG(ERROR)<<"Can't open tracjectory file.";
            return false;
        }
        string line;
        DroneMapFrame frame;
        while(getline(ifs,line))
        {
            stringstream sst(line);
            string& imgfile=frame.imagePath;
            sst>>imgfile;

            stringstream st(imgfile);
            st>>frame.timestamp;
            imgfile=path+"/rgb/"+imgfile+".jpg";

            sst>>frame.pose;
            frames.push_back(frame);
        }
        return frames.size();
    }

    virtual FramePtr    grabFrame()
    {
        int frameId=curIdx++;
        if(frameId>=frames.size()) return FramePtr();
        DroneMapFrame& df=frames[frameId];
        SPtr<RTMapperFrame> fr(new RTMapperFrame(frameId,df.timestamp));

        printf("load frame[%4d] %s\n", frameId, df.imagePath.c_str());

        fr->_image=RTMapperFrame::imread(df.imagePath);
        fr->_camera=camera;

        GSLAM::SE3 pose=local2ECEF*df.pose;
        GSLAM::Point3d lla=GSLAM::GPS<>::XYZ2GPS(pose.get_translation());
        fr->_gpshpyr=std::vector<double>({lla.y,lla.x,lla.z,5.,5.,10.});

        return fr;
    }

    string          datasetPath;
    GSLAM::SE3      plane,local2ECEF;
    Point3d         origin;
    vector<double>  camParameters;
    GSLAM::Camera   camera;
    vector<DroneMapFrame> frames;
    int             curIdx;
};


REGISTER_DATASET(DatasetImages,imgs);
REGISTER_DATASET(DroneMapKFDataset,cfg);
