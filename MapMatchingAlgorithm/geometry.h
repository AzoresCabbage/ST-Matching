
#pragma once

#include "stdafx.h"

const double eps = 1e-18;//�ж������
const double EARTH_RADIUS = 6378.137;//����뾶��km
const double PI = acos(-1.0);//Բ����

//�жϸ�������0�Ĺ�ϵ
//x<0 --- -1 ; x==0 --- 0 ; x>0 --- 1
inline int dcmp(double x){
	return (x > eps) - (x < -eps);
}

//ȡƽ��
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


//���ض����Ļ���
inline double rad(double d);

//���
double xmult(Point p1,Point p2,Point p0);

//����ŷ����þ���
double getEucDis(Point,Point);

//��֪���㾭γ�ȣ�����������룬��λΪkm
double getGeoDis(Point,Point);

//������ֱ�߽���,ע�������ж�ֱ���Ƿ�ƽ��!
//�߶ν������������߶��ཻ(ͬʱ����Ҫ�ж��Ƿ�ƽ��!)
Point Intersection(Point ua,Point ub,Point va,Point vb);

//�㵽�߶������
Point pToseg(Point p,Point la,Point lb);

//�㵽�߶ξ���
double dispToseg(Point p,Point a,Point b);
