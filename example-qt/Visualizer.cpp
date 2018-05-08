#include "Visualizer.h"
#include <QVBoxLayout>
#include "opmapcontrol/mapwidget/opmapwidget.h"

class TileLayerItem: public QObject, public QGraphicsItem
{
    Q_INTERFACES(QGraphicsItem)
public:
    TileLayerItem(mapcontrol::MapGraphicItem* parent,GSLAM::TileManagerPtr manager=GSLAM::TileManagerPtr())
        : QGraphicsItem(parent),_map(parent),_tileManager(manager)
    {

    }

    virtual QRectF boundingRect() const{
        return _map->boundingRect();
    }

    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                       QWidget  *widget)
    {
        if(!_tileManager) return;
        int zoom=std::ceil(_map->Zoom());
        int maxZoom=_tileManager->maxZoomLevel();
        if(maxZoom>0&&zoom>maxZoom)
            zoom=maxZoom;
        QRectF box=_map->boundingRect();
        internals::PointLatLng topleft=_map->FromLocalToLatLng(0,0);
        internals::PointLatLng rightBottom=_map->FromLocalToLatLng(box.width(),box.height());

        core::Point pt1=_map->Projection()->FromLatLngToPixel(internals::PointLatLng(topleft.Lat(),topleft.Lng()),zoom);
        core::Point pt2=_map->Projection()->FromLatLngToPixel(internals::PointLatLng(rightBottom.Lat(),rightBottom.Lng()),zoom);
        pt1=_map->Projection()->FromPixelToTileXY(pt1);
        pt2=_map->Projection()->FromPixelToTileXY(pt2);

        for(int x=pt1.X();x<=pt2.X();x++)
            for(int y=pt1.Y();y<=pt2.Y();y++)
            {
                GSLAM::TilePtr tile=_tileManager->getTile(x,y,zoom);
                if(!tile) continue;
                GSLAM::GImage img=tile->getTileImage().clone();
                if(img.empty()) continue;

//                pi::Byte<4>* sp=(pi::Byte<4>*)img.data;
//                for(int i=0;i<img.total();i++,sp++)
//                    *sp=pi::Byte<4>({sp->data[0],sp->data[1],sp->data[2],(uchar)(sp->data[3]?255:0)});

                QImage qimg(img.data,img.cols,img.rows,QImage::Format_ARGB32);
                auto pixel=_map->Projection()->FromTileXYToPixel(core::Point(x,y));
                auto ll=_map->Projection()->FromPixelToLatLng(pixel,zoom);
                core::Point pt=_map->FromLatLngToLocal(ll);

                auto pixel1=_map->Projection()->FromTileXYToPixel(core::Point(x+1,y+1));
                auto ll1=_map->Projection()->FromPixelToLatLng(pixel1,zoom);
                core::Point pt1=_map->FromLatLngToLocal(ll1);

                painter->drawImage(QRect(pt.X(),pt.Y(),pt1.X()-pt.X(),pt1.Y()-pt.Y()),qimg);
            }
    }

    mapcontrol::MapGraphicItem* _map;
    GSLAM::TileManagerPtr _tileManager;
};

class VisualizerOPM : public Visualizer
{
public:
    VisualizerOPM(QWidget* parent=NULL)
        : Visualizer(parent),_homeGPS(108.918389,34.2457760,0),_radius(200),_tileLayer(NULL){
        QVBoxLayout* wPathLayout=new QVBoxLayout(this);
        wPathLayout->setMargin(0);
        wPathLayout->setContentsMargins(0,0,0,0);

        _opmap=new mapcontrol::OPMapWidget(this);
        wPathLayout->addWidget(_opmap);

        _opmap->setMinimumHeight(256);
        _opmap->setAutoFillBackground(true);
        _opmap->configuration->SetAccessMode(core::AccessMode::ServerAndCache);
        _opmap->configuration->SetTileMemorySize(200);

        _opmap->configuration->SetCacheLocation(".");

        _opmap->SetZoom(18);
        _opmap->SetMinZoom(4);
        _opmap->SetMaxZoom(19);
        _opmap->SetMapType(MapType::BingSatellite);

        goHome();

        _tileLayer=new TileLayerItem(_opmap->GetMapItem());
    }

    virtual void closeEvent(QCloseEvent *){
        svar.i["ShouldStop"]=1;
    }

    bool setHomeGPSPosition(const GSLAM::Point3d &gpsPosition,float radius=1e3){
        _homeGPS=gpsPosition;
        return goHome();
    }

    bool goHome(){
        _opmap->SetCurrentPosition(internals::PointLatLng(_homeGPS.y,_homeGPS.x));
        return true;
    }

    bool showTileManager(const GSLAM::TileManagerPtr& manager)
    {
        _tileLayer->_tileManager=manager;
        return true;
    }

    mapcontrol::OPMapWidget* _opmap;
    TileLayerItem*           _tileLayer;
    GSLAM::Point3d           _homeGPS;
    float                    _radius;
};

SPtr<Visualizer> Visualizer::create(QWidget* parent){return SPtr<Visualizer>(new VisualizerOPM(parent));}
