
#pragma once

#include "stdafx.h"

const double eps = 1e-18;//判定的误差
const double EARTH_RADIUS = 6378.137;//地球半径，km
const double PI = acos(-1.0);//圆周率

//判断浮点数与0的关系
//x<0 --- -1 ; x==0 --- 0 ; x>0 --- 1
inline int dcmp(double x){
	return (x > eps) - (x < -eps);
}

//取平方
template <class T>
T SQR(T x) {return x*x;}

class Point{
public:
	double x, y;
	
	int id;

	Point() {}

	Point(double _x, double _y) : x(_x), y(_y) {}
	Point(double _x, double _y, int _id) : x(_x), y(_y),id(_id) {}
	Point operator + (const Point &p) const {return Point(x + p.x, y + p.y);}
	Point operator - (const Point &p) const {return Point(x - p.x, y - p.y);}
	Point operator * (const double &p) const {return Point(x * p, y * p);}
	Point operator / (const double &p) const {return Point (x / p, y / p);}

	bool operator == (const Point &p) const {return dcmp(x-p.x) == 0 && dcmp(y-p.y) == 0;}

	bool operator < (const Point &p) const {
        if (dcmp(y - p.y) == 0) return dcmp(x - p.x) <= 0;
        return dcmp(y - p.y) <= 0;
    }
};


//返回度数的弧度
inline double rad(double d);

//叉积
double xmult(Point p1,Point p2,Point p0);

//返回欧几里得距离
double getEucDis(Point,Point);

//已知两点经纬度，计算两点距离，单位为km
double getGeoDis(Point,Point);

//计算两直线交点,注意事先判断直线是否平行!
//线段交点请另外判线段相交(同时还是要判断是否平行!)
Point Intersection(Point ua,Point ub,Point va,Point vb);

//点到线段最近点
Point pToseg(Point p,Point la,Point lb);

//点到线段距离
double dispToseg(Point p,Point a,Point b);
