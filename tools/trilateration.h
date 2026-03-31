// -------------------------------------------------------------------------------------------------------------------
//
//  File: trilateration.h
//
//  Copyright 2016 (c) Decawave Ltd, Dublin, Ireland.
//
//  All rights reserved.
//
//
// -------------------------------------------------------------------------------------------------------------------
//

#ifndef __TRILATERATION_H__
#define __TRILATERATION_H__

#include "basetsd.h"
#include "stdio.h"
#include <QVector>
//#define SHOW_PRINTS

#define TRILATERATION (1)

#define REGRESSION_NUM (10)
#define SPEED_OF_LIGHT      (299702547.0)   // in m/s in air
#define NUM_ANCHORS (5)
#define REF_ANCHOR (5)	//anchor IDs are 1,2,3,4,5 etc. (don't start from 0!)

#define     MAX_ANC_NUM     8

#define		TRIL_3SPHERES							3
#define		TRIL_4SPHERES							4

typedef struct vec3d	vec3d;
struct vec3d {
	double	x;
	double	y;
	double	z;
};

typedef enum {
	UWB_OK = 3,
	UWB_ANC_BELOW_THREE = -1,
	UWB_LIN_DEP_FOR_THREE = -2,
	UWB_ANC_ON_ONE_LEVEL = -3,
	UWB_LIN_DEP_FOR_FOUR = -4,
	UWB_RANK_ZERO = -5
}UWB_POS_ERROR_CODES;

typedef struct {
	int x, y, z; //axis in cm
} position_t; // Position of a device or target in 3D space
// 用户数据结构体，传递给拟合函数
typedef struct {
    int n;             // 基站数量
//    position_t *anchors;  // 基站坐标
//    int *distances;    // 标签到基站的距离
    std::vector<vec3d> anchors;
    std::vector<double> distance;
} UserData;
extern bool stationList[MAX_ANC_NUM];
extern int stationNumber;
extern int maxStationID;
extern QVector<QVector<int>> g_range_anchor;
extern QVector<QVector<int>> g_range_anchor_backup;
extern QVector<QVector<double>> g_range_anchor_avg;
/* Return the difference of two vectors, (vector1 - vector2). */
vec3d vdiff(const vec3d vector1, const vec3d vector2);

/* Return the sum of two vectors. */
vec3d vsum(const vec3d vector1, const vec3d vector2);

/* Multiply vector by a number. */
vec3d vmul(const vec3d vector, const double n);

/* Divide vector by a number. */
vec3d vdiv(const vec3d vector, const double n);

/* Return the Euclidean norm. */
double vdist(const vec3d v1, const vec3d v2);

/* Return the Euclidean norm. */
double vnorm(const vec3d vector);

/* Return the dot product of two vectors. */
double dot(const vec3d vector1, const vec3d vector2);

/* Replace vector with its cross product with another vector. */
vec3d cross(const vec3d vector1, const vec3d vector2);

int GetLocation(vec3d *best_solution, vec3d* anchorArray, int *distanceArray, int dimensional);

void setStationList(int index, bool visible);
int taylor(vec3d *best_solution, int n,position_t *anchors, int *distances, vec3d initGuess, double weights[]);
double vdist(const vec3d v1, const vec3d v2);
int multilateration(vec3d *best_solution,const std::vector<vec3d>& anchors, const std::vector<double>& distances);
#endif
