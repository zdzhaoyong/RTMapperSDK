#pragma once
#include <QWidget>
#include <GSLAM/core/TileManager.h>

class Visualizer : public QWidget
{
public:
    Visualizer(QWidget* parent=NULL):QWidget(parent){}

    virtual bool setHomeGPSPosition(const GSLAM::Point3d &gpsPosition,float radius=1e3){return false;}
    virtual bool goHome(){return false;}

    virtual bool showTileManager(const GSLAM::TileManagerPtr& manager)=0;

    static SPtr<Visualizer> create(QWidget* parent=NULL);
};
