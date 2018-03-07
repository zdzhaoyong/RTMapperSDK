/**
******************************************************************************
*
* @file       mercatorprojection.cpp
* @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
* @brief      
* @see        The GNU Public License (GPL) Version 3
* @defgroup   OPMapWidget
* @{
* 
*****************************************************************************/
/* 
* This program is free software; you can redistribute it and/or modify 
* it under the terms of the GNU General Public License as published by 
* the Free Software Foundation; either version 3 of the License, or 
* (at your option) any later version.
* 
* This program is distributed in the hope that it will be useful, but 
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
* or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License 
* for more details.
* 
* You should have received a copy of the GNU General Public License along 
* with this program; if not, write to the Free Software Foundation, Inc., 
* 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/
#include "mercatorprojection.h"
#include <qmath.h>
 
namespace projections {
MercatorProjection::MercatorProjection():MinLatitude(-85.05112878), MaxLatitude(85.05112878),MinLongitude(-177),
MaxLongitude(177), tileSize(256, 256)
{
}
Point MercatorProjection::FromLatLngToPixel(double lat, double lng, const int &zoom)
{
    Point ret;// = Point.Empty;

    lat = Clip(lat, MinLatitude, MaxLatitude);
    lng = Clip(lng, MinLongitude, MaxLongitude);

    double x = (lng + 180) / 360;
    double sinLatitude = sin(lat * M_PI / 180);
    double y = 0.5 - log((1 + sinLatitude) / (1 - sinLatitude)) / (4 * M_PI);

    Size s = GetTileMatrixSizePixel(zoom);
    int mapSizeX = s.Width();
    int mapSizeY = s.Height();

    ret.SetX((int) Clip(x * mapSizeX + 0.5, 0, mapSizeX - 1));
    ret.SetY((int) Clip(y * mapSizeY + 0.5, 0, mapSizeY - 1));

    return ret;
}
internals::PointLatLng MercatorProjection::FromPixelToLatLng(const int &x, const int &y, const int &zoom)
{
    internals::PointLatLng ret;// = internals::PointLatLng.Empty;

    Size s = GetTileMatrixSizePixel(zoom);
    double mapSizeX = s.Width();
    double mapSizeY = s.Height();

    double xx = (Clip(x, 0, mapSizeX - 1) / mapSizeX) - 0.5;
    double yy = 0.5 - (Clip(y, 0, mapSizeY - 1) / mapSizeY);

    ret.SetLat(90 - 360 * atan(exp(-yy * 2 * M_PI)) / M_PI);
    ret.SetLng(360 * xx);

    return ret;
}
double MercatorProjection::Clip(const double &n, const double &minValue, const double &maxValue) const
{
    return qMin(qMax(n, minValue), maxValue);
}
Size MercatorProjection::TileSize() const
{
    return tileSize;
}
double MercatorProjection::Axis() const
{
    return 6378137;
}
double MercatorProjection::Flattening() const
{
    return (1.0 / 298.257223563);
}
Size MercatorProjection::GetTileMatrixMaxXY(const int &zoom)
{
    Q_UNUSED(zoom);
    int xy = (1 << zoom);
    return  Size(xy - 1, xy - 1);
}
Size MercatorProjection::GetTileMatrixMinXY(const int &zoom)
{
    Q_UNUSED(zoom);
    return Size(0, 0);
}


class GPSConverter
{
public:
    typedef internals::PointLatLng Gps;
    constexpr const static double pi = 3.1415926535897932384626;
    constexpr const static double a  =  6378245.0;
    constexpr const static double ee = 0.00669342162296594323;

    /**
     * 84 to 火星坐标系 (GCJ-02) World Geodetic System ==> Mars Geodetic System
     *
     * @param lat
     * @param lon
     * @return
     */
    static Gps gps84_To_Gcj02(double lat, double lon) {
        if (outOfChina(lat, lon)) {
            return Gps(lat,lon);
        }
        double dLat = transformLat(lon - 105.0, lat - 35.0);
        double dLon = transformLon(lon - 105.0, lat - 35.0);
        double radLat = lat / 180.0 * pi;
        double magic = sin(radLat);
        magic = 1 - ee * magic * magic;
        double sqrtMagic = sqrt(magic);
        dLat = (dLat * 180.0) / ((a * (1 - ee)) / (magic * sqrtMagic) * pi);
        dLon = (dLon * 180.0) / (a / sqrtMagic * cos(radLat) * pi);
        double mgLat = lat + dLat;
        double mgLon = lon + dLon;
        return Gps(mgLat, mgLon);
    }

    /**
     * * 火星坐标系 (GCJ-02) to 84 * * @param lon * @param lat * @return
     * */
    static Gps gcj_To_Gps84(double lat, double lon) {
        Gps gps = transform(lat, lon);
        double lontitude = lon * 2 - gps.Lng();
        double latitude = lat * 2 - gps.Lat();
        return Gps(latitude, lontitude);
    }

    /**
     * 火星坐标系 (GCJ-02) 与百度坐标系 (BD-09) 的转换算法 将 GCJ-02 坐标转换成 BD-09 坐标
     *
     * @param gg_lat
     * @param gg_lon
     */
    static Gps gcj02_To_Bd09(double gg_lat, double gg_lon) {
        double x = gg_lon, y = gg_lat;
        double z = sqrt(x * x + y * y) + 0.00002 * sin(y * pi);
        double theta = atan2(y, x) + 0.000003 * cos(x * pi);
        double bd_lon = z * cos(theta) + 0.0065;
        double bd_lat = z * sin(theta) + 0.006;
        return Gps(bd_lat, bd_lon);
    }

    /**
     * * 火星坐标系 (GCJ-02) 与百度坐标系 (BD-09) 的转换算法 * * 将 BD-09 坐标转换成GCJ-02 坐标 * * @param
     * bd_lat * @param bd_lon * @return
     */
    static Gps bd09_To_Gcj02(double bd_lat, double bd_lon) {
        double x = bd_lon - 0.0065, y = bd_lat - 0.006;
        double z = sqrt(x * x + y * y) - 0.00002 * sin(y * pi);
        double theta = atan2(y, x) - 0.000003 * cos(x * pi);
        double gg_lon = z * cos(theta);
        double gg_lat = z * sin(theta);
        return Gps(gg_lat, gg_lon);
    }

    /**
     * (BD-09)-->84
     * @param bd_lat
     * @param bd_lon
     * @return
     */
    static Gps bd09_To_Gps84(double bd_lat, double bd_lon) {

        Gps gcj02 = bd09_To_Gcj02(bd_lat, bd_lon);
        Gps map84 = gcj_To_Gps84(gcj02.Lat(),gcj02.Lng());
        return map84;

    }

    static Gps gps84_To_Bd09(double lat,double lon){
        Gps gcj02=gps84_To_Gcj02(lat,lon);
        return gcj02_To_Bd09(gcj02.Lat(),gcj02.Lng());
    }

    static bool outOfChina(double lat, double lon) {
        if (lon < 72.004 || lon > 137.8347)
            return true;
        if (lat < 0.8293 || lat > 55.8271)
            return true;
        return false;
    }

    static Gps transform(double lat, double lon) {
        if (outOfChina(lat, lon)) {
            return Gps(lat, lon);
        }
        double dLat = transformLat(lon - 105.0, lat - 35.0);
        double dLon = transformLon(lon - 105.0, lat - 35.0);
        double radLat = lat / 180.0 * pi;
        double magic = sin(radLat);
        magic = 1 - ee * magic * magic;
        double sqrtMagic = sqrt(magic);
        dLat = (dLat * 180.0) / ((a * (1 - ee)) / (magic * sqrtMagic) * pi);
        dLon = (dLon * 180.0) / (a / sqrtMagic * cos(radLat) * pi);
        double mgLat = lat + dLat;
        double mgLon = lon + dLon;
        return Gps(mgLat, mgLon);
    }

    static double transformLat(double x, double y) {
        double ret = -100.0 + 2.0 * x + 3.0 * y + 0.2 * y * y + 0.1 * x * y
                + 0.2 * sqrt(abs(x));
        ret += (20.0 * sin(6.0 * x * pi) + 20.0 * sin(2.0 * x * pi)) * 2.0 / 3.0;
        ret += (20.0 * sin(y * pi) + 40.0 * sin(y / 3.0 * pi)) * 2.0 / 3.0;
        ret += (160.0 * sin(y / 12.0 * pi) + 320 * sin(y * pi / 30.0)) * 2.0 / 3.0;
        return ret;
    }

    static double transformLon(double x, double y) {
        double ret = 300.0 + x + 2.0 * y + 0.1 * x * x + 0.1 * x * y + 0.1
                * sqrt(abs(x));
        ret += (20.0 * sin(6.0 * x * pi) + 20.0 * sin(2.0 * x * pi)) * 2.0 / 3.0;
        ret += (20.0 * sin(x * pi) + 40.0 * sin(x / 3.0 * pi)) * 2.0 / 3.0;
        ret += (150.0 * sin(x / 12.0 * pi) + 300.0 * sin(x / 30.0
                * pi)) * 2.0 / 3.0;
        return ret;
    }
};


core::Point GCJ02Projection::FromLatLngToPixel(double lat, double lng, int const& zoom)
{
    auto gcj=GPSConverter::gps84_To_Gcj02(lat,lng);
    return MercatorProjection::FromLatLngToPixel(gcj.Lat(),gcj.Lng(),zoom);
}

internals::PointLatLng GCJ02Projection::FromPixelToLatLng(const int &x,const int &y,const int &zoom)
{
    auto gcj=MercatorProjection::FromPixelToLatLng(x,y,zoom);
    return GPSConverter::gcj_To_Gps84(gcj.Lat(),gcj.Lng());
}


core::Point BaiduProjection::FromLatLngToPixel(double lat, double lng, int const& zoom)
{
    auto gcj=GPSConverter::gps84_To_Bd09(lat,lng);
    return MercatorProjection::FromLatLngToPixel(gcj.Lat(),gcj.Lng(),zoom);
}

internals::PointLatLng BaiduProjection::FromPixelToLatLng(const int &x,const int &y,const int &zoom)
{
    auto gcj=MercatorProjection::FromPixelToLatLng(x,y,zoom);
    return GPSConverter::bd09_To_Gps84(gcj.Lat(),gcj.Lng());
}

}
