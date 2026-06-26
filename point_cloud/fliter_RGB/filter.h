
#pragma once
#include "base.h"
#include "FileLibrary.h"
class point_base
{
public:
    point_base(){};
    ~point_base(){};

    void sor(PointTRGBNPtr src, PointTRGBNPtr &tgt, float, float, float);
    void ror(PointTRGBNPtr src, PointTRGBNPtr &tgt, float, float, float);
    void vox(PointTRGBNPtr src, PointTRGBNPtr &tgt, float);
    void mls(PointTRGBNPtr src, PointTRGBNPtr &tgt, float, float, float);
    void rg(PointTRGBPtr src, PointTRGBPtr &tgt);
};
