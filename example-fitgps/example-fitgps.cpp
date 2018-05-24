#include <GSLAM/core/Dataset.h>
#include <GSLAM/core/Timer.h>
#include <GSLAM/core/TileManager.h>
#include <GSLAM/core/Event.h>
#include <GSLAM/core/Estimator.h>
#include <GSLAM/core/GPS.h>
#include <RTMapper.h>


#include "../libs/utils.h"

#include <fstream>
#include <string>
#include <vector>

using namespace std;
using namespace GSLAM;

std::vector<std::pair<double,Point3d> > trackPath;

// a class for fast access to gps path, the path should be continuable
class FastPathTable
{
public:
    FastPathTable(double step)
            :min_time(-1),step_time(step),max_time(-1)
    {
    }

    inline bool Add(double timestamp,const pi::Point3d& pt)
    {
//        std::cout<<"PathTable:Adding "<<timestamp<<",PT:"<<pt<<std::endl;
        if(timestamp<=min_time) return false;
        if(min_time<0)//the first pt
        {
            min_time=timestamp;
            last=std::make_pair(timestamp,pt);
            path.push_back(last);
            return true;
        }

        double next_time=path.back().first;
        if(timestamp<=next_time) return false;

        next_time+=step_time;
        for(;next_time<=timestamp;next_time+=step_time)
        {
            pi::Point3d next_pose=last.second+((next_time-last.first)/(timestamp-last.first))
                                              *(pt-last.second);
            path.emplace_back(std::make_pair(next_time,next_pose));
        }
        last=std::make_pair(timestamp,pt);
        max_time=timestamp;
        return true;

    }

    inline bool Add(const std::pair<double,pi::Point3d>& pt)
    {
        return Add(pt.first,pt.second);
    }

    inline bool Get(const double& timestamp,pi::Point3d& pt)
    {
        if(timestamp<min_time||timestamp>max_time)
            return false;
        //compute index
        int idx=(timestamp-min_time)/step_time;
        std::pair<double,pi::Point3d> left,right;
        if(idx+1<path.size())//between idx--idx+1
        {
            left=path[idx];
            right=path[idx+1];
        }
        else //between idx--last
        {
            left=path[idx];
            right=last;
        }
        pt=left.second+((timestamp-left.first)/(right.first-left.first))
                       *(right.second-left.second);
        return true;
    }

    std::vector<std::pair<double,pi::Point3d> > path;
    std::pair<double,pi::Point3d> last;
    double min_time,step_time,max_time;
};

FastPathTable limitGPS(0.1);
FastPathTable gps(0.1);

struct PlyObject
{
    PlyObject(string file2save="out.ply"):_file2save(file2save){}
    ~PlyObject(){save(_file2save);}
    typedef pi::Point3f Vertex3f;
    typedef pi::Point3ub Color3b;

    std::string _file2save;
    std::vector<pi::Point3f>  vertices;
    std::vector<unsigned int> faces;
    std::vector<pi::Point3f>  normals;
    std::vector<pi::Point3ub> colors;
    std::vector<unsigned int> edges;

    void addPoint(Point3d pt,Color3b color=Color3b(255,255,255),pi::Point3f normal=Point3f(0,0,1))
    {
        vertices.push_back(pt);
        colors.push_back(color);
        normals.push_back(normal);
    }

    void addLine(Point3d first,Point3d second,Color3b color=Color3b(255,255,255),pi::Point3f normal=Point3f(0,0,1))
    {
        edges.push_back(vertices.size());
        edges.push_back(vertices.size()+1);
        addPoint(first,color,normal);
        addPoint(second,color,normal);
    }

    bool save(const string& filename)
    {
        if(filename.substr(filename.find_last_of('.')+1)!="ply") return false;
        std::fstream file;
        file.open(filename.c_str(),std::ios::out|std::ios::binary);
        if(!file.is_open()){
            fprintf(stderr,"\nERROR: Could not open File %s for writing!",(filename).c_str());
            return false;
        }

        int _verticesPerFace=3;
        bool binary=false;

        file << "ply";
        if(binary)file << "\nformat binary_little_endian 1.0";
        else file << "\nformat ascii 1.0";
        file << "\nelement vertex " << vertices.size();
        file << "\nproperty float32 x\nproperty float32 y\nproperty float32 z";
        if(normals.size())
            file << "\nproperty float32 nx\nproperty float32 ny\nproperty float32 nz";
        if(colors.size())
            file << "\nproperty uchar red\nproperty uchar green\nproperty uchar blue";
        if(faces.size()){
            file << "\nelement face " << faces.size()/_verticesPerFace;
            file << "\nproperty list uint8 int32 vertex_indices";
        }
        if(edges.size()){
            file << "\nelement edge " << edges.size()/2;
            file << "\nproperty int vertex1\nproperty int vertex2";
        }
        file << "\nend_header";
        if(binary) file << "\n";

        for(unsigned int i=0;i<vertices.size();i++){
            if(binary){
                file.write((char*)(&(vertices[i])),sizeof(Vertex3f));
            }
            else file << "\n" << vertices[i].x << " " << vertices[i].y << " " << vertices[i].z;

            if(normals.size())
            {
                if(binary){
                    file.write((char*)(&(normals[i])),sizeof(Vertex3f));
                }
                else file << " " << normals[i].x << " " << normals[i].y << " " << normals[i].z;
            }
            if(colors.size()){
                if(binary){
                    file.write((char*)(&(colors[i])),sizeof(Color3b));
                }
                else file << " " << (int)(colors[i].x) << " " << (int)(colors[i].y) << " " << (int)(colors[i].z);
            }
        }
        for(unsigned int i=0;i<faces.size();i+=_verticesPerFace){
            if(binary){
                file.write((char*)(&_verticesPerFace),sizeof(uchar));
            }
            else file << "\n" << (int)_verticesPerFace;
            for(unsigned int j=0;j<_verticesPerFace;j++)
                if(binary){
                    unsigned int idx = faces[i+j];
                    file.write((char*)(&idx),sizeof(unsigned int));
                }
                else file << " " << (faces[i+j]);
        }
        for(unsigned int i=0;i<edges.size();i+=2){
            if(binary){
                unsigned int idx = edges[i];
                file.write((char*)(&idx),sizeof(unsigned int));
                idx = edges[i+1]; file.write((char*)(&idx),sizeof(unsigned int));
            }
            else file << "\n " << edges[i] << " " << edges[i+1];
        }

        file.close();
        return true;
    }
};

PlyObject ply;

double CalibrateFromTrajectory(std::vector<std::pair<double,Point3d> > trackPath,pi::SIM3d& sim3Result,
                               double& minDif,double& maxDif,double precision,int FitNum,FastPathTable& gps)
{
    auto estimator=GSLAM::Estimator::create();
    if(maxDif-minDif<precision) return minDif;//no need to calibrate

    //compute time interval
    double minTrack=trackPath[0].first;
    double maxTrack=trackPath[trackPath.size()-1].first;
    double minGps =gps.path.front().first,maxGps=gps.path.back().first;
//    int frameNum=gps.path.size();
//    int MinIndex=0;
    if(minGps<0||maxGps<0||minGps>maxGps) return minDif-1;

    if(minGps-maxTrack>minDif) minDif=minGps-maxTrack;
    if(maxGps-minTrack<maxDif) maxDif=maxGps-minTrack;

    cerr<<"MinDiff:"<<minDif<<"\nMaxDiff:"<<maxDif<<endl;
    vector<pair<double,double> > diff_error;
    size_t minDiffFound;
    double minErrorFound=100000;

    //Coase calibrate:try diferrent diff
    for(double diff=minDif;diff<maxDif;diff+=precision)
    {

        vector<pi::Point3d> TrackPoints,GPSPoints;
        double minTime,midTime,maxTime;
        //find the minTrack timestamp and its corrospond gpsPose
        size_t i=0,iend=trackPath.size()-3;
        for(;i<iend;i++)
        {
            minTime=trackPath[i].first+diff;
            if(minTime>minGps)
            {
                Point3d pt;
                if(gps.Get(minTime,pt))
                {
                    GPSPoints.push_back(pt);
                    TrackPoints.push_back(trackPath[i].second);
                    break;
                }
            }
        }
        if(TrackPoints.size()!=1)
        {
            cerr<<"Error find minTime!\n";
            continue;
        }
        size_t min_i=i;
        //find the maxTrack timestamp and its corrospond gpsPose
        for(iend=min_i,i=trackPath.size()-1;i>iend;i--)
        {
            maxTime=trackPath[i].first+diff;
            if(maxTime<maxGps)
            {
                Point3d pt;
                if(gps.Get(maxTime,pt))
                {
                    GPSPoints.push_back(pt);
                    TrackPoints.push_back(trackPath[i].second);
                    break;
                }
            }
        }
        if(TrackPoints.size()!=2)
        {
            cerr<<"Error find maxTime!\n";
            continue ;
        }
        size_t max_i=i;
        if(max_i-min_i<2)
        {
            cerr<<"Error max_i-min_i too small!\n";
            continue ;
        }

        //find the middle Track timestamp and its corrospond gpsPose
        int fitNum=FitNum-2;
        if(max_i-min_i<fitNum+1) fitNum=max_i-min_i-1;
        int step=(max_i-min_i-1)/(fitNum+1);
        if(step<1) step=1;

        for(i=min_i;i<max_i;i+=step)
        {
            midTime=trackPath[i].first+diff;
            Point3d pt;
            if(gps.Get(midTime,pt))
            {
                GPSPoints.push_back(pt);
                TrackPoints.push_back(trackPath[i].second);
            }
        }
        if(TrackPoints.size()<3)
        {
            cerr<<"Error find midTime!\n";
            continue ;
        }

        pi::SIM3d sim3;

        estimator->findSIM3(sim3,TrackPoints,GPSPoints);

        vector<pi::Point3d> FitPoints;
        double error=0;
        pi::Point3d err_vec;
        for(int j=0;j<TrackPoints.size();j++)
        {
            FitPoints.push_back(sim3*TrackPoints[j]);
            err_vec=GPSPoints[j]-FitPoints[j];
            error+=err_vec*err_vec;
        }
        error/=TrackPoints.size();

        diff_error.emplace_back(make_pair(diff,error));

        if(error<minErrorFound)
        {
            minErrorFound=error;
            minDiffFound=diff_error.size();
            sim3Result=sim3;
        }
    }

    //Update min max diff
    if(diff_error.size()<3&&minErrorFound>10000) return minDif-1;
    if(minDiffFound>=1)
        for(int i=minDiffFound-1;i>=0;i--)
        {
            if(diff_error[i].second>2*minErrorFound)
            {
                minDif=diff_error[i].first;
                break;
            }
        }

    if(minDiffFound<diff_error.size()-1)
        for(size_t i=minDiffFound+1;i<diff_error.size();i++)
        {
            if(diff_error[i].second>2*minErrorFound)
            {
                maxDif=diff_error[i].first;
                break;
            }
        }

    return diff_error[minDiffFound].first;
}

bool loadGPSPath(const string& file,FastPathTable& gps){
    std::ifstream ifs(file);
    if(!ifs.is_open()) return false;

    string line;
    static bool   isFirst=true;
    static Point3d firstPos;
    int i=0;
    while(getline(ifs,line))
    {
        double  time;
        Point3d lla;
        stringstream sst(line);
        sst>>time>>lla;
        lla=GSLAM::GPS<>::GPS2XYZ(Point3d(lla.y,lla.x,lla.z));
        if(isFirst){
            isFirst=false;
            firstPos=lla;
        }
        gps.Add(time,lla-firstPos);
    }
    return !gps.path.empty();
}

class System : public GSLAM::GObjectHandle
{
public:
    System(){}

    void run(){
        std::string dataset=svar.GetString("Dataset","");
        if(dataset.empty()){
            LOG(ERROR)<<"Please set dataset with Dataset=dataset_path";
            return;
        }
        if(!_dataset.open(dataset))
        {
            LOG(ERROR)<<"Failed to open Dataset "<<svar.GetString("Dataset","");
            return ;
        }

        RTMapperPtr rtmapper=createRTMapperInstance();
        if(!rtmapper) {
            LOG(ERROR)<<"Failed to load RTMapperSDK.";
            return;
        }
        else LOG(INFO)<<"Loaded RTMapperSDK version "<<rtmapper->version();

        rtmapper->setCallback(this);
        rtmapper->setSvar(svar);

        bool        isFirstFrame=true;
        int&        shouldStop=svar.i["ShouldStop"];

        while(!shouldStop){
            GSLAM::FramePtr frame=_dataset.grabFrame();
            if(!frame) {
                LOG(INFO)<<"Dataset processed.";
                break;
            }
            if(frame->getImage().empty()) {
                LOG(ERROR)<<"Frame has no image.";
                break;
            }
            if(!frame->getCamera().isValid())
            {
                LOG(ERROR)<<"Camera not valid.";
                break;
            }
            rtmapper->track(frame);
        }
    }


    virtual void handle(const SPtr<GSLAM::GObject>& obj){
        GSLAM::FramePtr frame;
        if(auto e = std::dynamic_pointer_cast<GSLAM::CurrentFrameEvent>(obj)) {
            frame = e->_frame;
        }
        Point3d xyz;
        if(frame) {
            double time = frame->timestamp();
            SE3 pose = frame->getPose();
            xyz.x = pose.get_translation().x;
            xyz.y = pose.get_translation().y;
            xyz.z = pose.get_translation().z;
            trackPath.emplace_back(make_pair(time, xyz));
        }

        pi::SIM3d sim3;
        // when got 2000 visual pose, use these pose and limitGPS data to fit sim3.
        // this number should be set according to your own mission request.
        if(trackPath.size() == 2000) {
            double diff=CalibrateFromTrajectory(trackPath,sim3,
                                                svar.GetDouble("MinDiff",-60),
                                                svar.GetDouble("MaxDiff",60),
                                                svar.GetDouble("Precision",0.1),
                                                svar.GetInt("FitNum",10),limitGPS);
            cout << "============================" << endl;
            cout << "============================" << endl;
            cout << "time diff is " << diff << endl;
            cout << "============================" << endl;
            cout << "scale of sim3 is " << sim3.get_scale() << endl;
            cout << "============================" << endl;
            cout << "============================" << endl;
        }

        cout << sim3 * xyz << endl;

        ply.addPoint(sim3 * xyz, pi::Point3ub(255,0,0));

    }

    GSLAM::Dataset          _dataset;

};

int main(int argc,char** argv)
{
    svar.ParseMain(argc,argv);

    // limitGPS is a part of full gps sequence.
    // for example, if you have 10000 lines gps data,
    // you can use the data between 400~1200 to fit your visual pose, but not all of data.
    if(!loadGPSPath(svar.GetString("limitGPS","limitGPS.txt"),limitGPS)) return -1;

    // load all GPS data for drawing groundtruth, to make sure your fitting is correct.
    if(!loadGPSPath(svar.GetString("GPS","gps.txt"),gps)) return -2;

    System sys;
    sys.run();

    for(auto p:gps.path){
        ply.addPoint(p.second,Point3ub(0,255,0));
    }

    return 0;
}


