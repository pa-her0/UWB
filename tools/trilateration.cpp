// -------------------------------------------------------------------------------------------------------------------
//
//  File: trilateration.cpp
//
//  Copyright 2016 (c) Decawave Ltd, Dublin, Ireland.
//
//  All rights reserved.
//
//  Author:
//
// -------------------------------------------------------------------------------------------------------------------


#include "stdio.h"
#include "math.h"
#include "stdlib.h"
#include "time.h"
#include <tuple>
#include <vector>
#include "trilateration.h"
#include <math.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_linalg.h>
#include <gsl/gsl_math.h>
#include <gsl/gsl_multifit_nlinear.h>
#include <Eigen/Dense> // 使用 Eigen 库来解决线性方程组
#include <eigen/Eigenvalues>
#include <QDebug>
#include <algorithm>
using Eigen::SelfAdjointEigenSolver;
/* Largest nonnegative number still considered zero */
#define   MAXZERO  0.001


#define		ERR_TRIL_CONCENTRIC						-1
#define		ERR_TRIL_COLINEAR_2SOLUTIONS			-2
#define		ERR_TRIL_SQRTNEGNUMB					-3
#define		ERR_TRIL_NOINTERSECTION_SPHERE4			-4
#define		ERR_TRIL_NEEDMORESPHERE					-5

#define CM_ERR_ADDED (10) //was 5

bool stationList[MAX_ANC_NUM] = {true,true,true,false,false,false,false,false};//通过数组记录各个基站显示状态，默认A0-A2三基站显示
int stationNumber = 3;//记录当前基站显示数量。默认A0-A2三基站显示
int maxStationID = 0;   //记录激活基站最大ID（0-7）
QVector<QVector<int>> g_range_anchor;
QVector<QVector<int>> g_range_anchor_backup;
QVector<QVector<double>> g_range_anchor_avg;

typedef struct stationSt
{

}_stationSt;

// 计算残差函数
int residual_func(const gsl_vector *x, void *data, gsl_vector *f) {
    UserData *user_data = (UserData *)data;
    size_t n = user_data->n;
    std::vector<vec3d> anchors = user_data->anchors;
    std::vector<double> distances = user_data->distance;

    double tx = gsl_vector_get(x, 0);
    double ty = gsl_vector_get(x, 1);
    double tz = gsl_vector_get(x, 2);

    for (size_t i = 0; i < n; i++) {
        double dx = anchors[i].x - tx;
        double dy = anchors[i].y - ty;
        double dz = anchors[i].z - tz;
        double model_distance = sqrt(dx * dx + dy * dy + dz * dz);
        gsl_vector_set(f, i, model_distance - distances[i]);
    }

    return GSL_SUCCESS;
}

void callback(const size_t iter, void *params, const gsl_multifit_nlinear_workspace *w)
{
    gsl_vector *f = gsl_multifit_nlinear_residual(w);
    gsl_vector *x = gsl_multifit_nlinear_position(w);
    double rcond;

    /* compute reciprocal condition number of J(x) */
    gsl_multifit_nlinear_rcond(&rcond, w);
//    qDebug() << "iter" << iter << ": X =" << gsl_vector_get(x, 0) << ", Y =" << gsl_vector_get(x, 1)<<", Z=" << gsl_vector_get(x, 2) << ", cond(J) =" << 1.0 / rcond << ", |f(x)| =" << gsl_blas_dnrm2(f);

}
int taylor(vec3d *best_solution, int n,const std::vector<vec3d>& anchors, const std::vector<double>& distances, vec3d initGuess, double weights[]){
//        size_t n = sizeof(anchors) / sizeof(anchors[0]);
        printf("基站数量%d\n",n);
        double weights1[] = {1.0, 1.0, 1.0, 1.0}; // 示例权重，可以根据需要调整
        double initial_guess[] = {initGuess.x, initGuess.y, initGuess.z}; //坐标初始猜测//理论值2,3,3
//        double initial_guess[] = {initGuess.x, initGuess.y, 1.5f}; //坐标初始猜测//理论值2,3,3
        for(int i = 0 ; i < n ; i++){

//            distances[i] = distances[i]/100.0f
             qDebug()<<"distance"<<n<<distances[i];
        }
        // 初始化权重向量
        gsl_vector_view wts = gsl_vector_view_array(weights, n);
        gsl_vector_view x = gsl_vector_view_array(initial_guess,3);

        // 初始化用户数据
        UserData user_data = {n, anchors, distances};

        // 初始化GSL结构
        gsl_multifit_nlinear_fdf fdf;
        fdf.f = residual_func;
        fdf.df = NULL;  // 使用数值计算导数
        fdf.fvv = NULL;
        fdf.n = static_cast<size_t>(n);
        fdf.p = 3;
        fdf.params = &user_data;

        const gsl_multifit_nlinear_type *T = gsl_multifit_nlinear_trust;

        gsl_multifit_nlinear_workspace *w;
        gsl_multifit_nlinear_parameters fdf_params = gsl_multifit_nlinear_default_parameters();
        fdf_params.trs = gsl_multifit_nlinear_trs_ddogleg;


        // 设置驱动参数
        const double xtol = 1e-6;
        const double gtol = 1e-6;
        const double ftol = 0.0;
        size_t max_iter = 100;
        int status, info;

        // 初始化工作空间
        w = gsl_multifit_nlinear_alloc(T, &fdf_params, n, 3);
        gsl_multifit_nlinear_winit (&x.vector, &wts.vector, &fdf, w);
//        gsl_multifit_nlinear_init(&x.vector, &fdf, w);

        // 进行优化
        status = gsl_multifit_nlinear_driver(max_iter, xtol, gtol, ftol, callback, NULL, &info, w);
        gsl_vector *x_opt = gsl_multifit_nlinear_position(w);
        best_solution->x = gsl_vector_get(x_opt, 0);
        best_solution->y = gsl_vector_get(x_opt, 1);
        best_solution->z = gsl_vector_get(x_opt, 2);
        qDebug("Taylor Result=%f,%f,%f",best_solution->x,best_solution->y,best_solution->z);
        // 释放GSL资源
        gsl_multifit_nlinear_free(w);
        return UWB_OK;
}

int multilateration(vec3d *best_solution, const std::vector<vec3d>& anchors, const std::vector<double>& distances) {


    // 筛选有效的锚点和距离
    std::vector<vec3d> validAnchors;
    std::vector<double> validDistances;

    for (size_t i = 0; i < distances.size(); ++i) {
        if (distances[i] > 0) { // 过滤掉无效距离（假设无效距离为非正值）
            validAnchors.push_back(anchors[i]);
            validDistances.push_back(distances[i]/1000.0f);
            qDebug()<<"A"<<i<<":"<<distances[i]/1000.0f;
        }
    }

    size_t numValidAnchors = validAnchors.size();
    if (numValidAnchors < 3) {
        qDebug()<<"numValidAnchors : "<<numValidAnchors+"anchors.size():"<<anchors.size();
        return UWB_ANC_BELOW_THREE; // 至少需要3个有效锚点
    }

    // 判断是否所有有效锚点的z坐标相同
    bool isTwoDimensional = true;
    double commonZ = validAnchors[0].z;

    for (const auto& anchor : validAnchors) {
        if (anchor.z != commonZ || numValidAnchors < 4 ) {
            isTwoDimensional = false;
            break;
        }
    }

    // 判断基站坐标是否线性独立（且不全为同一个高度）
    bool lin_independent = false;
    for (size_t i = 0; i < numValidAnchors - 2; ++i) {
        for (size_t j = i + 1; j < numValidAnchors - 1; ++j) {
            for (size_t k = j + 1; k < numValidAnchors; ++k) {
                double det_xy = (validAnchors[j].x - validAnchors[i].x) * (validAnchors[k].y - validAnchors[i].y) -
                        (validAnchors[j].y - validAnchors[i].y) * (validAnchors[k].x - validAnchors[i].x);

                double det_xz = (validAnchors[j].x - validAnchors[i].x) * (validAnchors[k].z - validAnchors[i].z) -
                        (validAnchors[j].z - validAnchors[i].z) * (validAnchors[k].x - validAnchors[i].x);

                double det_yz = (validAnchors[j].y - validAnchors[i].y) * (validAnchors[k].z - validAnchors[i].z) -
                        (validAnchors[j].z - validAnchors[i].z) * (validAnchors[k].y - validAnchors[i].y);

                if (det_xy != 0 || det_xz != 0 || det_yz != 0) {
                    lin_independent = true;
                    break;
                }
            }

            if (lin_independent) break;
        }

        if (lin_independent) break;
    }

    if (!lin_independent) {
        return UWB_LIN_DEP_FOR_THREE; // 不满足线性独立条件
    }

    // 使用最小二乘法
    int dimension = (validDistances.size() == 3 && !isTwoDimensional) ? 2 : (isTwoDimensional ? 2 : 3);
    Eigen::MatrixXd A(numValidAnchors - 1, dimension);
    Eigen::VectorXd b(numValidAnchors - 1);

    for (size_t i = 1; i < numValidAnchors; ++i) {
        A(i-1, 0) = 2 * (validAnchors[i].x - validAnchors[0].x);
        A(i-1, 1) = 2 * (validAnchors[i].y - validAnchors[0].y);

        if (dimension == 3) {
            A(i-1, 2) = 2 * (validAnchors[i].z - validAnchors[0].z);
        }

        b(i-1) = std::pow(validDistances[0], 2) - std::pow(validDistances[i], 2)
                + std::pow(validAnchors[i].x, 2) - std::pow(validAnchors[0].x, 2)
                + std::pow(validAnchors[i].y, 2) - std::pow(validAnchors[0].y, 2);

        if (dimension == 3) {
            b(i-1) += std::pow(validAnchors[i].z, 2) - std::pow(validAnchors[0].z, 2);
        }
    }

    // 使用最小二乘法求解
    Eigen::VectorXd position = A.colPivHouseholderQr().solve(b);
    best_solution->x = position(0);
    best_solution->y = position(1);
    best_solution->z = (dimension == 3) ? position(2) : commonZ;

//    if (dimension == 3)
//    {
//        vec3d init_guess = {best_solution->x,best_solution->y,best_solution->z};
//        std::vector<double> weights(numValidAnchors);

//        // Define a reasonable minimum threshold to avoid division by zero or extremely large weights
//        double min_distance_threshold = 0.1; // adjust this threshold as necessary

//        // Compute weights based on the inverse of the distance
//        for (int i = 0; i < numValidAnchors; i++) {
//            weights[i] = 1.0 / std::max(min_distance_threshold, validDistances[i]);
//        }

//        // Normalize weights
//        double total_weight = std::accumulate(weights.begin(), weights.end(), 0.0);
//        for (int i = 0; i < numValidAnchors; i++) {
//            weights[i] =1.0f;///= total_weight;  // Normalize each weight
//            qDebug()<< "权重：A"<<i<<weights[i] << "总权重："<<total_weight;

//        }
//        taylor(best_solution, numValidAnchors, validAnchors, validDistances, init_guess, weights.data());
//    }
    qDebug()<< "计算结果 维度："<<dimension<<",坐标"<<best_solution->x<<best_solution->y<<best_solution->z;

    return UWB_OK;
}
int leastSquaresMethod(vec3d *best_solution, vec3d* anchorArray, int *distanceArray)
{
	/*!@brief: This function calculates the 3D position of the initiator from the anchor distances and positions using least squared errors.
	 *	 	   The function expects more than 4 anchors. The used equation system looks like follows:\n
	 \verbatim
	  		    -					-
	  		   | M_11	M_12	M_13 |	 x	  b[0]
	  		   | M_12	M_22	M_23 | * y	= b[1]
	  		   | M_23	M_13	M_33 |	 z	  b[2]
	  		    -					-
	 \endverbatim
	 * @param distances_cm_in_pt: 			Pointer to array that contains the distances to the anchors in cm (including invalid results)
	 * @param no_distances: 				Number of valid distances in distance array (it's not the size of the array)
	 * @param anchor_pos: 	                Pointer to array that contains anchor positions in cm (including positions related to invalid results)
	 * @param no_anc_positions: 			Number of valid anchor positions in the position array (it's not the size of the array)
	 * @param position_result_pt: 			Pointer toposition. position_t variable that holds the result of this calculation
	 * @return: The function returns a status code. */

	/* 		Algorithm used:
	 *		Linear Least Sqaures to solve Multilateration
	 * 		with a Special case if there are only 3 Anchors.
	 * 		Output is the Coordinates of the Initiator in relation to Anchor 0 in NEU (North-East-Up) Framing
	 * 		In cm
	 */

	/* Resulting Position Vector*/
	double x_pos = 0;
	double y_pos = 0;
	double z_pos = 0;
	/* Matrix components (3*3 Matrix resulting from least square error method) [cm^2] */
	double M_11 = 0;
	double M_12 = 0;																						// = M_21
	double M_13 = 0;																						// = M_31
	double M_22 = 0;
	double M_23 = 0;																						// = M_23
	double M_33 = 0;

	/* Vector components (3*1 Vector resulting from least square error method) [cm^3] */
	double b[3] = {0};

	/* Miscellaneous variables */
	double temp = 0;
	double temp2 = 0;
	double nominator = 0;
	double denominator = 0;
	bool   anchors_on_x_y_plane = true;																		// Is true, if all anchors are on the same height => x-y-plane
	bool   lin_dep = true;																						// All vectors are linear dependent, if this variable is true
	uint8_t ind_y_indi = 0;	//numberr of independet vectors																					// First anchor index, for which the second row entry of the matrix [(x_1 - x_0) (x_2 - x_0) ... ; (y_1 - x_0) (y_2 - x_0) ...] is non-zero => linear independent

	/* Arrays for used distances and anchor positions (without rejected ones) */
    uint8_t 	no_distances = MAX_ANC_NUM;
	int 	distances_cm[no_distances];
	position_t 	anchor_pos[no_distances]; //position in CM
	uint8_t		no_valid_distances = 0;

	/* Reject invalid distances (including related anchor position) */
	for (int i = 0; i < no_distances; i++) 
    {
		if (distanceArray[i] > 0) 
        {
			//excludes any distance that is 0xFFFFU (int16 Maximum Value)
			distances_cm[no_valid_distances] = distanceArray[i]/10;
			anchor_pos[no_valid_distances].x = anchorArray[i].x*100;
            anchor_pos[no_valid_distances].y = anchorArray[i].y*100;
            anchor_pos[no_valid_distances].z = anchorArray[i].z*100;
            no_valid_distances++;
		}
		// if (_dis[i] != -1) 
        // {
		// 	//excludes any distance that is 0xFFFFU (int16 Maximum Value)
		// 	distances_cm[no_valid_distances] = _dis[i];
		// 	anchor_pos[no_valid_distances] = _ancarray[i];
        //     no_valid_distances++;
		// }
        else
        {
            //printf("%d = -1\n", i);
        }
	}

    //printf("no_valid_distances = %d\n", no_valid_distances);

	/* Check, if there are enough valid results for doing the localization at all */
	if (no_valid_distances < 3) {
		return UWB_ANC_BELOW_THREE;
	}

	/* Check, if anchors are on the same x-y plane */
	for (int i = 1; i < no_valid_distances; i++) 
    {
		if (anchor_pos[i].z != anchor_pos[0].z) 
        {
			anchors_on_x_y_plane = false;
			break;
		}
	}

	/**** Check, if there are enough linear independent anchor positions ****/

	/* Check, if the matrix |(x_1 - x_0) (x_2 - x_0) ... | has rank 2
	 * 			|(y_1 - y_0) (y_2 - y_0) ... | 				*/

	for (ind_y_indi = 2; ((ind_y_indi < no_valid_distances) && (lin_dep == true)); ind_y_indi++) {
		temp = ((int64_t)anchor_pos[ind_y_indi].y - (int64_t)anchor_pos[0].y) * ((int64_t)anchor_pos[1].x -
				(int64_t)anchor_pos[0].x);
		temp2 = ((int64_t)anchor_pos[1].y - (int64_t)anchor_pos[0].y) * ((int64_t)anchor_pos[ind_y_indi].x -
				(int64_t)anchor_pos[0].x);

		if ((temp - temp2) != 0) {
			lin_dep = false;
			break;
		}
	}

	/* Leave function, if rank is below 2 */
	if (lin_dep == true) 
    {
		return UWB_LIN_DEP_FOR_THREE;
	}

	/* If the anchors are not on the same plane, three vectors must be independent => check */
	if (!anchors_on_x_y_plane) 
    {
		/* Check, if there are enough valid results for doing the localization */
		if (no_valid_distances < 4) 
        {
			return UWB_ANC_ON_ONE_LEVEL;
		}

		/* Check, if the matrix |(x_1 - x_0) (x_2 - x_0) (x_3 - x_0) ... | has rank 3 (Rank y, y already checked)
		 * 			|(y_1 - y_0) (y_2 - y_0) (y_3 - y_0) ... |
		 * 			|(z_1 - z_0) (z_2 - z_0) (z_3 - z_0) ... |											*/
		lin_dep = true;

		for (int i = 2; ((i < no_valid_distances) && (lin_dep == true)); i++) {
			if (i != ind_y_indi) {
				/* (x_1 - x_0)*[(y_2 - y_0)(z_n - z_0) - (y_n - y_0)(z_2 - z_0)] */
				temp 	= ((int64_t)anchor_pos[ind_y_indi].y - (int64_t)anchor_pos[0].y) * ((int64_t)anchor_pos[i].z -
						(int64_t)anchor_pos[0].z);
				temp 	-= ((int64_t)anchor_pos[i].y - (int64_t)anchor_pos[0].y) * ((int64_t)anchor_pos[ind_y_indi].z -
						(int64_t)anchor_pos[0].z);
				temp2 	= ((int64_t)anchor_pos[1].x - (int64_t)anchor_pos[0].x) * temp;

				/* Add (x_2 - x_0)*[(y_n - y_0)(z_1 - z_0) - (y_1 - y_0)(z_n - z_0)] */
				temp 	= ((int64_t)anchor_pos[i].y - (int64_t)anchor_pos[0].y) * ((int64_t)anchor_pos[1].z - (int64_t)anchor_pos[0].z);
				temp 	-= ((int64_t)anchor_pos[1].y - (int64_t)anchor_pos[0].y) * ((int64_t)anchor_pos[i].z - (int64_t)anchor_pos[0].z);
				temp2 	+= ((int64_t)anchor_pos[ind_y_indi].x - (int64_t)anchor_pos[0].x) * temp;

				/* Add (x_n - x_0)*[(y_1 - y_0)(z_2 - z_0) - (y_2 - y_0)(z_1 - z_0)] */
				temp 	= ((int64_t)anchor_pos[1].y - (int64_t)anchor_pos[0].y) * ((int64_t)anchor_pos[ind_y_indi].z -
						(int64_t)anchor_pos[0].z);
				temp 	-= ((int64_t)anchor_pos[ind_y_indi].y - (int64_t)anchor_pos[0].y) * ((int64_t)anchor_pos[1].z -
						(int64_t)anchor_pos[0].z);
				temp2 	+= ((int64_t)anchor_pos[i].x - (int64_t)anchor_pos[0].x) * temp;

				if (temp2 != 0) { lin_dep = false; }
			}
		}

		/* Leave function, if rank is below 3 */
		if (lin_dep == true) {
			return UWB_LIN_DEP_FOR_FOUR;
		}
	}

	/************************************************** Algorithm ***********************************************************************/

	/* Writing values resulting from least square error method (A_trans*A*x = A_trans*r; row 0 was used to remove x^2,y^2,z^2 entries => index starts at 1) */
	for (int i = 1; i < no_valid_distances; i++) {
		/* Matrix (needed to be multiplied with 2, afterwards) */
		M_11 += (int64_t)pow((int64_t)(anchor_pos[i].x - anchor_pos[0].x), 2);
		M_12 += (int64_t)((int64_t)(anchor_pos[i].x - anchor_pos[0].x) * (int64_t)(anchor_pos[i].y - anchor_pos[0].y));
		M_13 += (int64_t)((int64_t)(anchor_pos[i].x - anchor_pos[0].x) * (int64_t)(anchor_pos[i].z - anchor_pos[0].z));
		M_22 += (int64_t)pow((int64_t)(anchor_pos[i].y - anchor_pos[0].y), 2);
		M_23 += (int64_t)((int64_t)(anchor_pos[i].y - anchor_pos[0].y) * (int64_t)(anchor_pos[i].z - anchor_pos[0].z));
		M_33 += (int64_t)pow((int64_t)(anchor_pos[i].z - anchor_pos[0].z), 2);

		/* Vector */
		temp = (int64_t)((int64_t)pow(distances_cm[0], 2) - (int64_t)pow(distances_cm[i], 2)
				 + (int64_t)pow(anchor_pos[i].x, 2) + (int64_t)pow(anchor_pos[i].y, 2)
				 + (int64_t)pow(anchor_pos[i].z, 2) - (int64_t)pow(anchor_pos[0].x, 2)
				 - (int64_t)pow(anchor_pos[0].y, 2) - (int64_t)pow(anchor_pos[0].z, 2));

		b[0] += (int64_t)((int64_t)(anchor_pos[i].x - anchor_pos[0].x) * temp);
		b[1] += (int64_t)((int64_t)(anchor_pos[i].y - anchor_pos[0].y) * temp);
		b[2] += (int64_t)((int64_t)(anchor_pos[i].z - anchor_pos[0].z) * temp);
	}

	M_11 = 2 * M_11;
	M_12 = 2 * M_12;
	M_13 = 2 * M_13;
	M_22 = 2 * M_22;
	M_23 = 2 * M_23;
	M_33 = 2 * M_33;

	/* Calculating the z-position, if calculation is possible (at least one anchor at z != 0) */
	if (anchors_on_x_y_plane == false) {
		nominator = b[0] * (M_12 * M_23 - M_13 * M_22) + b[1] * (M_12 * M_13 - M_11 * M_23) + b[2] *
			    (M_11 * M_22 - M_12 * M_12);			// [cm^7]
		denominator = M_11 * (M_33 * M_22 - M_23 * M_23) + 2 * M_12 * M_13 * M_23 - M_33 * M_12 * M_12 - M_22 * M_13 *
			      M_13;				// [cm^6]

		/* Check, if denominator is zero (Rank of matrix not high enough) */
		if (denominator == 0) {
			return UWB_RANK_ZERO;
		}

		z_pos = ((nominator * 10) / denominator + 5) / 10;	// [cm]
	}

	/* Else prepare for different calculation approach (after x and y were calculated) */
	else {
		z_pos = 0;
	}

	/* Calculating the y-position */
	nominator = b[1] * M_11 - b[0] * M_12 - (z_pos * (M_11 * M_23 - M_12 * M_13));	// [cm^5]
	denominator = M_11 * M_22 - M_12 * M_12;// [cm^4]

	/* Check, if denominator is zero (Rank of matrix not high enough) */
	if (denominator == 0) {
		return UWB_RANK_ZERO;
	}

	y_pos = ((nominator * 10) / denominator + 5) / 10;	// [cm]

	/* Calculating the x-position */
	nominator = b[0] - z_pos * M_13 - y_pos * M_12;	// [cm^3]
	denominator = M_11;	// [cm^2]

	x_pos = ((nominator * 10) / denominator + 5) / 10;// [cm]

	/* Calculate z-position form x and y coordinates, if z can't be determined by previous steps (All anchors at z_n = 0) */
	if (anchors_on_x_y_plane == true) 
    {
		/* Calculate z-positon relative to the anchor grid's height */
		//for (int i = 0; i < no_distances; i++) {
        for (int i = 0; i < no_valid_distances; i++) {
			/* z² = dis_meas_n² - (x - x_anc_n)² - (y - y_anc_n)² */
			temp = (int64_t)((int64_t)pow(distances_cm[i], 2)
					 - (int64_t)pow((x_pos - (int64_t)anchor_pos[i].x), 2)
					 - (int64_t)pow((y_pos - (int64_t)anchor_pos[i].y), 2));

			/* z² must be positive, else x and y must be wrong => calculate positive sqrt and sum up all calculated heights, if positive */
			if (temp >= 0) {
				z_pos += (int64_t)sqrt(temp);

			} else {
				z_pos = 0;
			}
		}

        //printf("z_pos1 = %ld\n", z_pos);

		//z_pos = z_pos / no_distances;	// Divide sum by number of distances to get the average
        z_pos = z_pos / no_valid_distances;	// Divide sum by number of distances to get the average

        ///printf("z_pos2 = %ld\n", z_pos);

		/* Add height of the anchor grid's height */
		//z_pos += anchor_pos[0].z;
	}

//    vec3d init_guess = {x_pos,y_pos,z_pos};
//    double weights[] = {1.0,1.0,1.0};
//    taylor(best_solution, no_valid_distances, anchor_pos, distances_cm, init_guess, weights);

    //printf("x=%f, y=%f, z=%f\n",x_pos, y_pos, z_pos);

	return UWB_OK;
}

/* Return the difference of two vectors, (vector1 - vector2). */
vec3d vdiff(const vec3d vector1, const vec3d vector2)
{
    vec3d v;
    v.x = vector1.x - vector2.x;
    v.y = vector1.y - vector2.y;
    v.z = vector1.z - vector2.z;
    return v;
}

/* Return the sum of two vectors. */
vec3d vsum(const vec3d vector1, const vec3d vector2)
{
    vec3d v;
    v.x = vector1.x + vector2.x;
    v.y = vector1.y + vector2.y;
    v.z = vector1.z + vector2.z;
    return v;
}

/* Multiply vector by a number. */
vec3d vmul(const vec3d vector, const double n)
{
    vec3d v;
    v.x = vector.x * n;
    v.y = vector.y * n;
    v.z = vector.z * n;
    return v;
}

/* Divide vector by a number. */
vec3d vdiv(const vec3d vector, const double n)
{
    vec3d v;
    v.x = vector.x / n;
    v.y = vector.y / n;
    v.z = vector.z / n;
    return v;
}

/* Return the Euclidean norm. */
double vdist(const vec3d v1, const vec3d v2)
{
    double xd = v1.x - v2.x;
    double yd = v1.y - v2.y;
    double zd = v1.z - v2.z;
    return sqrt(xd * xd + yd * yd + zd * zd);
}

/* Return the Euclidean norm. */
double vnorm(const vec3d vector)
{
    return sqrt(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z);
}

/* Return the dot product of two vectors. */
double dot(const vec3d vector1, const vec3d vector2)
{
    return vector1.x * vector2.x + vector1.y * vector2.y + vector1.z * vector2.z;
}

/* Replace vector with its cross product with another vector. */
vec3d cross(const vec3d vector1, const vec3d vector2)
{
    vec3d v;
    v.x = vector1.y * vector2.z - vector1.z * vector2.y;
    v.y = vector1.z * vector2.x - vector1.x * vector2.z;
    v.z = vector1.x * vector2.y - vector1.y * vector2.x;
    return v;
}

double gdoprate(const vec3d& tag,
                const vec3d& p1,
                const vec3d& p2,
                const vec3d& p3)
{
    constexpr double epsilon = 1e-8;
    constexpr double GDOP_MIN = 1.0;    // 理论最优GDOP值（对应返回0）
    constexpr double GDOP_MAX = 10.0;   // 系统容忍的最差GDOP值（对应返回1）

    // Use dynamic-size Eigen matrices here.
    // This avoids MinGW 32-bit Debug crashes caused by fixed-size Eigen
    // objects requiring stack alignment on the 4-anchor GDOP path.
    Eigen::MatrixXd H(3, 3);
    bool has_singularity = false;

    // 带异常检测的锚点处理
    auto process_anchor = [&](int row, const vec3d& anchor) {
        const double dx = anchor.x - tag.x;
        const double dy = anchor.y - tag.y;
        const double dz = anchor.z - tag.z;
        const double h_norm = std::sqrt(dx * dx + dy * dy + dz * dz);

        if (h_norm < epsilon) {
            has_singularity = true;  // 标记奇异状态
            return;
        }

        H(row, 0) = dx / h_norm;
        H(row, 1) = dy / h_norm;
        H(row, 2) = dz / h_norm;
    };

    process_anchor(0, p1);
    process_anchor(1, p2);
    process_anchor(2, p3);

    // 存在锚点与tag重合时直接返回最差值1
    if (has_singularity) return 1.0;

    // ============== 矩阵特征值分解 ================
    SelfAdjointEigenSolver<Eigen::MatrixXd> solver(H.transpose() * H);
    if (solver.info() != Eigen::Success) return 1.0; // 分解失败返回最差

    const auto& eig = solver.eigenvalues();
    if (eig.minCoeff() < epsilon) return 1.0;  // 秩不足返回最差

    // ============ GDOP计算与反向归一化 ================
    const double raw_gdop = sqrt(eig.cwiseInverse().sum());

    /* 归一化公式推导（反向映射）：
       normalized = (raw_gdop - GDOP_MIN) / (GDOP_MAX - GDOP_MIN)
       边界行为：
       - raw_gdop ≤ GDOP_MIN → 0.0 (最优)
       - raw_gdop ≥ GDOP_MAX → 1.0 (最差)
    */
    double normalized = (raw_gdop - GDOP_MIN) / (GDOP_MAX - GDOP_MIN);

    // 增强中间区域的灵敏度（反向幂律调节）
    normalized = std::pow(normalized, 0.6);  // 指数<1增强低值区分辨率

    // 严格边界保护
    if (normalized < 0.0) return 0.0;
    if (normalized > 1.0) return 1.0;
    return normalized;
}


/* Intersecting a sphere sc with radius of r, with a line p1-p2.
 * Return zero if successful, negative error otherwise.
 * mu1 & mu2 are constant to find points of intersection.
*/
int sphereline(const vec3d p1, const vec3d p2, const vec3d sc, double r, double *const mu1, double *const mu2)
{
   double a,b,c;
   double bb4ac;
   vec3d dp;

   dp.x = p2.x - p1.x;
   dp.y = p2.y - p1.y;
   dp.z = p2.z - p1.z;

   a = dp.x * dp.x + dp.y * dp.y + dp.z * dp.z;

   b = 2 * (dp.x * (p1.x - sc.x) + dp.y * (p1.y - sc.y) + dp.z * (p1.z - sc.z));

   c = sc.x * sc.x + sc.y * sc.y + sc.z * sc.z;
   c += p1.x * p1.x + p1.y * p1.y + p1.z * p1.z;
   c -= 2 * (sc.x * p1.x + sc.y * p1.y + sc.z * p1.z);
   c -= r * r;

   bb4ac = b * b - 4 * a * c;

   if (fabs(a) == 0 || bb4ac < 0) {
      *mu1 = 0;
      *mu2 = 0;
      return -1;
   }

   *mu1 = (-b + sqrt(bb4ac)) / (2 * a);
   *mu2 = (-b - sqrt(bb4ac)) / (2 * a);

   return 0;
}

/* Return TRIL_3SPHERES if it is performed using 3 spheres and return
 * TRIL_4SPHERES if it is performed using 4 spheres
 * For TRIL_3SPHERES, there are two solutions: result1 and result2
 * For TRIL_4SPHERES, there is only one solution: best_solution
 *
 * Return negative number for other errors
 *
 * To force the function to work with only 3 spheres, provide a duplicate of
 * any sphere at any place among p1, p2, p3 or p4.
 *
 * The last parameter is the largest nonnegative number considered zero;
 * it is somewhat analogous to machine epsilon (but inclusive).
*/

int trilateration(vec3d *const result1,
                  vec3d *const result2,
                  vec3d *const best_solution,
                  const vec3d p1, const double r1,
                  const vec3d p2, const double r2,
                  const vec3d p3, const double r3,
                  const vec3d p4, const double r4,
                  const double maxzero)
{
    vec3d    ex, ey, ez, t1, t2;
    double    h, i, j, x, y, z, t;
    vec3d    t3;
    double    mu1, mu2, mu;
    int result;

    /*********** FINDING TWO POINTS FROM THE FIRST THREE SPHERES **********/

    // if there are at least 2 concentric spheres within the first 3 spheres
    // then the calculation may not continue, drop it with error -1

    /* h = |p3 - p1|, ex = (p3 - p1) / |p3 - p1| */
    ex = vdiff(p3, p1); // vector p13
    h = vnorm(ex); // scalar p13
    if (h <= maxzero) {
        /* p1 and p3 are concentric, not good to obtain a precise intersection point */
        //diag_printf("concentric13 return -1\n");
        return ERR_TRIL_CONCENTRIC;
    }

    /* h = |p3 - p2|, ex = (p3 - p2) / |p3 - p2| */
    ex = vdiff(p3, p2); // vector p23
    h = vnorm(ex); // scalar p23
    if (h <= maxzero) {
        /* p2 and p3 are concentric, not good to obtain a precise intersection point */
        //diag_printf("concentric23 return -1\n");
        return ERR_TRIL_CONCENTRIC;
    }

    /* h = |p2 - p1|, ex = (p2 - p1) / |p2 - p1| */
    ex = vdiff(p2, p1); // vector p12
    h = vnorm(ex); // scalar p12
    if (h <= maxzero) {
        /* p1 and p2 are concentric, not good to obtain a precise intersection point */
        //diag_printf("concentric12 return -1\n");
        return ERR_TRIL_CONCENTRIC;
    }
    ex = vdiv(ex, h); // unit vector ex with respect to p1 (new coordinate system)

    /* t1 = p3 - p1, t2 = ex (ex . (p3 - p1)) */
    t1 = vdiff(p3, p1); // vector p13
    i = dot(ex, t1); // the scalar of t1 on the ex direction
    t2 = vmul(ex, i); // colinear vector to p13 with the length of i

    /* ey = (t1 - t2), t = |t1 - t2| */
    ey = vdiff(t1, t2); // vector t21 perpendicular to t1
    t = vnorm(ey); // scalar t21
    if (t > maxzero) {
        /* ey = (t1 - t2) / |t1 - t2| */
        ey = vdiv(ey, t); // unit vector ey with respect to p1 (new coordinate system)

        /* j = ey . (p3 - p1) */
        j = dot(ey, t1); // scalar t1 on the ey direction
    } else
        j = 0.0;

    /* Note: t <= maxzero implies j = 0.0. */
    if (fabs(j) <= maxzero) {

        /* Is point p1 + (r1 along the axis) the intersection? */
        t2 = vsum(p1, vmul(ex, r1));
        if (fabs(vnorm(vdiff(p2, t2)) - r2) <= maxzero &&
            fabs(vnorm(vdiff(p3, t2)) - r3) <= maxzero) {
            /* Yes, t2 is the only intersection point. */
            if (result1)
                *result1 = t2;
            if (result2)
                *result2 = t2;
            return TRIL_3SPHERES;
        }

        /* Is point p1 - (r1 along the axis) the intersection? */
        t2 = vsum(p1, vmul(ex, -r1));
        if (fabs(vnorm(vdiff(p2, t2)) - r2) <= maxzero &&
            fabs(vnorm(vdiff(p3, t2)) - r3) <= maxzero) {
            /* Yes, t2 is the only intersection point. */
            if (result1)
                *result1 = t2;
            if (result2)
                *result2 = t2;
            return TRIL_3SPHERES;
        }
        /* p1, p2 and p3 are colinear with more than one solution */
        return ERR_TRIL_COLINEAR_2SOLUTIONS;
    }

    /* ez = ex x ey */
    ez = cross(ex, ey); // unit vector ez with respect to p1 (new coordinate system)

    x = (r1*r1 - r2*r2) / (2*h) + h / 2;
    y = (r1*r1 - r3*r3 + i*i) / (2*j) + j / 2 - x * i / j;
    z = r1*r1 - x*x - y*y;
    if (z < -maxzero) {
        /* The solution is invalid, square root of negative number */
        return ERR_TRIL_SQRTNEGNUMB;
    } else
    if (z > 0.0)
        z = sqrt(z);
    else
        z = 0.0;

    /* t2 = p1 + x ex + y ey */
    t2 = vsum(p1, vmul(ex, x));
    t2 = vsum(t2, vmul(ey, y));

    /* result1 = p1 + x ex + y ey + z ez */
    if (result1)
        *result1 = vsum(t2, vmul(ez, z));

    /* result1 = p1 + x ex + y ey - z ez */
    if (result2)
        *result2 = vsum(t2, vmul(ez, -z));

    /*********** END OF FINDING TWO POINTS FROM THE FIRST THREE SPHERES **********/
    /********* RESULT1 AND RESULT2 ARE SOLUTIONS, OTHERWISE RETURN ERROR *********/
    //return TRIL_3SPHERES;
    /************* FINDING ONE SOLUTION BY INTRODUCING ONE MORE SPHERE ***********/

    // check for concentricness of sphere 4 to sphere 1, 2 and 3
    // if it is concentric to one of them, then sphere 4 cannot be used
    // to determine the best solution and return -1

    /* h = |p4 - p1|, ex = (p4 - p1) / |p4 - p1| */
    ex = vdiff(p4, p1); // vector p14
    h = vnorm(ex); // scalar p14
    if (h <= maxzero) {
        /* p1 and p4 are concentric, not good to obtain a precise intersection point */
        //diag_printf("concentric14 return 0\n");
        return TRIL_3SPHERES;
    }
    /* h = |p4 - p2|, ex = (p4 - p2) / |p4 - p2| */
    ex = vdiff(p4, p2); // vector p24
    h = vnorm(ex); // scalar p24
    if (h <= maxzero) {
        /* p2 and p4 are concentric, not good to obtain a precise intersection point */
        //diag_printf("concentric24 return 0\n");
        return TRIL_3SPHERES;
    }
    /* h = |p4 - p3|, ex = (p4 - p3) / |p4 - p3| */
    ex = vdiff(p4, p3); // vector p34
    h = vnorm(ex); // scalar p34
    if (h <= maxzero) {
        /* p3 and p4 are concentric, not good to obtain a precise intersection point */
        //diag_printf("concentric34 return 0\n");
        return TRIL_3SPHERES;
    }

    // if sphere 4 is not concentric to any sphere, then best solution can be obtained
    /* find i as the distance of result1 to p4 */
    t3 = vdiff(*result1, p4);
    i = vnorm(t3);
    /* find h as the distance of result2 to p4 */
    t3 = vdiff(*result2, p4);
    h = vnorm(t3);

    /* pick the result1 as the nearest point to the center of sphere 4 */
    if (i > h) {
        *best_solution = *result1;
        *result1 = *result2;
        *result2 = *best_solution;
    }

    int count4 = 0;
    double rr4 = r4;
    result = 1;
    /* intersect result1-result2 vector with sphere 4 */
    while(result && count4 < 10)
    {
        result=sphereline(*result1, *result2, p4, rr4, &mu1, &mu2);
        rr4+=0.1;
        count4++;
    }

    if (result) {

        /* No intersection between sphere 4 and the line with the gradient of result1-result2! */
        *best_solution = *result1; // result1 is the closer solution to sphere 4
        //return ERR_TRIL_NOINTERSECTION_SPHERE4;

    } else {

        if (mu1 < 0 && mu2 < 0) {

            /* if both mu1 and mu2 are less than 0 */
            /* result1-result2 line segment is outside sphere 4 with no intersection */
            if (fabs(mu1) <= fabs(mu2)) mu = mu1; else mu = mu2;
            /* h = |result2 - result1|, ex = (result2 - result1) / |result2 - result1| */
            ex = vdiff(*result2, *result1); // vector result1-result2
            h = vnorm(ex); // scalar result1-result2
            ex = vdiv(ex, h); // unit vector ex with respect to result1 (new coordinate system)
            /* 50-50 error correction for mu */
            mu = 0.5*mu;
            /* t2 points to the intersection */
            t2 = vmul(ex, mu*h);
            t2 = vsum(*result1, t2);
            /* the best solution = t2 */
            *best_solution = t2;

        } else if ((mu1 < 0 && mu2 > 1) || (mu2 < 0 && mu1 > 1)) {

            /* if mu1 is less than zero and mu2 is greater than 1, or the other way around */
            /* result1-result2 line segment is inside sphere 4 with no intersection */
            if (mu1 > mu2) mu = mu1; else mu = mu2;
            /* h = |result2 - result1|, ex = (result2 - result1) / |result2 - result1| */
            ex = vdiff(*result2, *result1); // vector result1-result2
            h = vnorm(ex); // scalar result1-result2
            ex = vdiv(ex, h); // unit vector ex with respect to result1 (new coordinate system)
            /* t2 points to the intersection */
            t2 = vmul(ex, mu*h);
            t2 = vsum(*result1, t2);
            /* vector t2-result2 with 50-50 error correction on the length of t3 */
            t3 = vmul(vdiff(*result2, t2),0.5);
            /* the best solution = t2 + t3 */
            *best_solution = vsum(t2, t3);

        } else if (((mu1 > 0 && mu1 < 1) && (mu2 < 0 || mu2 > 1))
                || ((mu2 > 0 && mu2 < 1) && (mu1 < 0 || mu1 > 1))) {

            /* if one mu is between 0 to 1 and the other is not */
            /* result1-result2 line segment intersects sphere 4 at one point */
            if (mu1 >= 0 && mu1 <= 1) mu = mu1; else mu = mu2;
            /* add or subtract with 0.5*mu to distribute error equally onto every sphere */
            if (mu <= 0.5) mu-=0.5*mu; else mu-=0.5*(1-mu);
            /* h = |result2 - result1|, ex = (result2 - result1) / |result2 - result1| */
            ex = vdiff(*result2, *result1); // vector result1-result2
            h = vnorm(ex); // scalar result1-result2
            ex = vdiv(ex, h); // unit vector ex with respect to result1 (new coordinate system)
            /* t2 points to the intersection */
            t2 = vmul(ex, mu*h);
            t2 = vsum(*result1, t2);
            /* the best solution = t2 */
            *best_solution = t2;

        } else if (mu1 == mu2) {

            /* if both mu1 and mu2 are between 0 and 1, and mu1 = mu2 */
            /* result1-result2 line segment is tangential to sphere 4 at one point */
            mu = mu1;
            /* add or subtract with 0.5*mu to distribute error equally onto every sphere */
            if (mu <= 0.25) mu-=0.5*mu;
            else if (mu <=0.5) mu-=0.5*(0.5-mu);
            else if (mu <=0.75) mu-=0.5*(mu-0.5);
            else mu-=0.5*(1-mu);
            /* h = |result2 - result1|, ex = (result2 - result1) / |result2 - result1| */
            ex = vdiff(*result2, *result1); // vector result1-result2
            h = vnorm(ex); // scalar result1-result2
            ex = vdiv(ex, h); // unit vector ex with respect to result1 (new coordinate system)
            /* t2 points to the intersection */
            t2 = vmul(ex, mu*h);
            t2 = vsum(*result1, t2);
            /* the best solution = t2 */
            *best_solution = t2;

        } else {

            /* if both mu1 and mu2 are between 0 and 1 */
            /* result1-result2 line segment intersects sphere 4 at two points */

            //return ERR_TRIL_NEEDMORESPHERE;

            mu = mu1 + mu2;
            /* h = |result2 - result1|, ex = (result2 - result1) / |result2 - result1| */
            ex = vdiff(*result2, *result1); // vector result1-result2
            h = vnorm(ex); // scalar result1-result2
            ex = vdiv(ex, h); // unit vector ex with respect to result1 (new coordinate system)
            /* 50-50 error correction for mu */
            mu = 0.5*mu;
            /* t2 points to the intersection */
            t2 = vmul(ex, mu*h);
            t2 = vsum(*result1, t2);
            /* the best solution = t2 */
            *best_solution = t2;

        }

    }

    return TRIL_4SPHERES;

    /******** END OF FINDING ONE SOLUTION BY INTRODUCING ONE MORE SPHERE *********/
}
/* This function calls trilateration to get the best solution.
 *
 * If any three spheres does not produce valid solution,
 * then each distance is increased to ensure intersection to happens.
 *
 * Return the selected trilateration mode between TRIL_3SPHERES or TRIL_4SPHERES
 * For TRIL_3SPHERES, there are two solutions: solution1 and solution2
 * For TRIL_4SPHERES, there is only one solution: best_solution
 *
 * nosolution_count = the number of failed attempt before intersection is found
 * by increasing the sphere diameter.
*/
int deca_3dlocate ( vec3d     *const solution1,
                    vec3d     *const solution2,
                    vec3d     *const best_solution,
                    int       *const nosolution_count,
                    double    *const best_3derror,
                    double    *const best_gdoprate,
                    vec3d p1, double r1,
                    vec3d p2, double r2,
                    vec3d p3, double r3,
                    vec3d p4, double r4,
                    int *combination)
{
    vec3d    o1, o2, solution, ptemp;
    // vec3d    solution_compare1, solution_compare2;
    double    /*error_3dcompare1, error_3dcompare2,*/ rtemp;
    double    gdoprate_compare1, gdoprate_compare2;
    double    ovr_r1, ovr_r2, ovr_r3, ovr_r4;
    int        overlook_count, combination_counter;
    int        trilateration_errcounter, trilateration_mode34;
    int        success, concentric, result;

    trilateration_errcounter = 0;
    trilateration_mode34 = 0;

    combination_counter = 4; /* four spheres combination */

    *best_gdoprate = 1; /* put the worst gdoprate init */
    gdoprate_compare1 = 1; gdoprate_compare2 = 1;
    //solution_compare1.x = 0; solution_compare1.y = 0; solution_compare1.z = 0;
    //error_3dcompare1 = 0;

    do {
        success = 0;
        concentric = 0;
        overlook_count = 0;
        ovr_r1 = r1; ovr_r2 = r2; ovr_r3 = r3; ovr_r4 = r4;

        do {
            result = trilateration(&o1, &o2, &solution, p1, ovr_r1, p2, ovr_r2, p3, ovr_r3, p4, ovr_r4, MAXZERO);
            switch (result)
            {
                case TRIL_3SPHERES: // 3 spheres are used to get the result
                    trilateration_mode34 = TRIL_3SPHERES;
                    success = 1;
                    break;

                case TRIL_4SPHERES: // 4 spheres are used to get the result
                    trilateration_mode34 = TRIL_4SPHERES;
                    success = 1;
                    break;

                case ERR_TRIL_CONCENTRIC:
                    concentric = 1;
                    break;

                default: // any other return value goes here
                    ovr_r1 += 0.10;
                    ovr_r2 += 0.10;
                    ovr_r3 += 0.10;
                    ovr_r4 += 0.10;
                    overlook_count++;
                    break;
            }

            //qDebug() << "while(!success)" << overlook_count << concentric << "result" << result;

        } while (!success && (overlook_count <= CM_ERR_ADDED) && !concentric);

//        if(success)
//            qDebug() << "Location" << ovr_r1 << ovr_r2 << ovr_r3 << ovr_r4 << "+err=" << overlook_count;
//        else
//            qDebug() << "No Location" << ovr_r1 << ovr_r2 << ovr_r3 << ovr_r4 << "+err=" << overlook_count;

        if (success)
        {
            switch (result)
            {
            case TRIL_3SPHERES:
                *solution1 = o1;
                *solution2 = o2;
                *nosolution_count = overlook_count;
                combination_counter = 0;
                qDebug()<<"仅3基站参与计算成功直接退出,扩径次数："<< overlook_count;
                break;

            case TRIL_4SPHERES:
                /* calculate the new gdop */
                gdoprate_compare1    = gdoprate(solution, p1, p2, p3);
                qDebug()<<"当前组合:"<<(combination_counter==4?"1,2,3":combination_counter==3?"2,3,4":combination_counter==2?"3,4,1":combination_counter==1?"4,1,2":0)<<"计算成功，扩径次数："<<overlook_count;
                /* compare and swap with the better result */
                if (gdoprate_compare1 <= gdoprate_compare2) {

                    *solution1 = o1;
                    *solution2 = o2;
                    qDebug()<<"上次sol:"<<(*best_solution).x<<(*best_solution).y<<(*best_solution).z<<"更换 best_solution =" <<solution.x<<solution.y<<solution.z;
                    *best_solution    = solution;
                    *nosolution_count = overlook_count;
                    *best_3derror    = sqrt((vnorm(vdiff(solution, p1))-r1)*(vnorm(vdiff(solution, p1))-r1) +
                                        (vnorm(vdiff(solution, p2))-r2)*(vnorm(vdiff(solution, p2))-r2) +
                                        (vnorm(vdiff(solution, p3))-r3)*(vnorm(vdiff(solution, p3))-r3) +
                                        (vnorm(vdiff(solution, p4))-r4)*(vnorm(vdiff(solution, p4))-r4));
                    *best_gdoprate    = gdoprate_compare1;

                    /* save the previous result */
                    //solution_compare2 = solution_compare1;
                    //error_3dcompare2 = error_3dcompare1;
                    gdoprate_compare2 = gdoprate_compare1;
                    *combination = 5 - combination_counter;
                }
                combination_counter--;
                if(combination_counter<0)
                 {
                       combination_counter=0;
                 }
                 break;

            default:
                break;
            }
        }
        else
        {
            trilateration_errcounter++;
            combination_counter--;
            qDebug()<<"当前组合:"<<(combination_counter==4?"1,2,3":combination_counter==3?"2,3,4":combination_counter==2?"3,4,1":combination_counter==1?"4,1,2":0)<<"计算失败，扩径次数："<<overlook_count<<"剩余组合："<<4-combination_counter;
           if(combination_counter<0)
           {
              combination_counter=0;
           }
        }
        ptemp = p1; p1 = p2; p2 = p3; p3 = p4; p4 = ptemp;
        rtemp = r1; r1 = r2; r2 = r3; r3 = r4; r4 = rtemp;


    } while (combination_counter);

    // if it gives error for all 4 sphere combinations then no valid result is given
    // otherwise return the trilateration mode used
    if (trilateration_errcounter >= 4) 
    {
        qDebug()<<"当前组合全部失败";
        return -1; 
    }
    else {
        return trilateration_mode34;
    }
}



double distance(double x1, double y1, double x2, double y2) 
{
    return sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
}

void findPointA(vec3d *best_solution, float x1, float y1, float x2, float y2, float d1, float d2) 
{
    //printf("x1=%f, y1=%f, x2=%f, y2=%f, d1=%f, d2=%f\n", x1,y1,x2,y2,d1,d2);
    float d = distance(x1, y1, x2, y2);
    float d_delta;
    float fabsD = fabs(d1 - d2);
    if (d1 + d2 < d ||  fabsD > d) {
        //printf("No solution exists. Calculating the midpoint of the two potential intersection points...\n");
        // Extend the radii to make them intersect
        d_delta = d-d2-d1;
        if (d_delta>=0){ //tag位于2基站之间
            d1 += d_delta/2.0;
            d2 += d_delta/2.0;
        }
        else  //tag位于两点之外，使两距离分别延伸到相同比距离并相交
        {
            
            d_delta = (fabs(d1-d2)-d)/2.0f;
            int i = d1>d2?-1:1;
            d1+=(d_delta*i);
            d2-=(d_delta*i);
        }
    }

    float a = (d1 * d1 - d2 * d2 + d * d) / (2 * d);
    float h = sqrt(fabs(d1 * d1 - a * a));

    float x3 = x1 + a * (x2 - x1) / d;
    float y3 = y1 + a * (y2 - y1) / d;

    float x4 = x3 + h * (y2 - y1) / d;
    float y4 = y3 - h * (x2 - x1) / d;

    float x5 = x3 - h * (y2 - y1) / d;
    float y5 = y3 + h * (x2 - x1) / d;

    if (h == 0) 
    {
        //printf("One solution: (%.2lf, %.2lf)\n", x3, y3);
        best_solution->x = x3;
        best_solution->y = y3;
    } 
    else 
    {
        float xi = (x4 + x5) / 2;
        float yi = (y4 + y5) / 2;
        //printf("Midpoint of the two potential intersection points: (%.2lf, %.2lf)\n", xi, yi);
        best_solution->x = xi;
        best_solution->y = yi;
    }
}




struct num
{
    int anc_ID;
    int distance;
}valid_anc_num[(MAX_ANC_NUM+1)]; //因为存8个数 所以从0到8 数组大小开到9

int cmp(const void *m,const void *n) //定义返回值返回方式
{
    return ((struct num*)m) ->distance - ((struct num*)n) ->distance;
}
// 使用引用参数将结果传出
void convertToVectors(const vec3d* anchorArray, const int* distances, size_t validCount, std::vector<vec3d>& anchorVector, std::vector<double>& distanceVector) {
//    // 清空目标向量以确保结果正确
//    anchorVector.assign(anchorArray, anchorArray + size);
//    distanceVector.assign(distanceArray, distanceArray + size);
    anchorVector.clear();
    distanceVector.clear();
    for (size_t i = 0; i < validCount; i++) {
        // 根据 validAncArray[j].anc_ID 添加对应的 anchor 和 distance
        anchorVector.push_back(anchorArray[valid_anc_num[i].anc_ID]);
        distanceVector.push_back(valid_anc_num[i].distance);
    }
}
int GetLocation(vec3d *best_solution, vec3d* anchorArray, int *distanceArray, int dimensional)
{

    vec3d	o1, o2, p1, p2, p3, p4;
    double	r1 = 0, r2 = 0, r3 = 0, r4 = 0, best_3derror, best_gdoprate;
    int		result;
    int     error, combination;
    int     valid_anc_count=0;
    int     j=0;
    int     use3anc=0;

    //qDebug("dimensional = %d", dimensional);

    for(int i=0;i<(MAX_ANC_NUM+1);i++)//清空结构体数组
    {
        valid_anc_num[i].anc_ID=0;
        valid_anc_num[i].distance=0;
    }

    //容错，保证当前显示基站点大于3个
    int number = 0;
    for (int i = 0; i < MAX_ANC_NUM; ++i)
    {
        if(stationList[i])
        {
            number++;
        }
    }
    if((number < 3) && (dimensional > 1))
    {
        return -1;
    }

    for(int i=0;i<MAX_ANC_NUM;i++)//验证几个有效距离值
    {
        if(distanceArray[i]>0 && stationList[i]==true)
        {
            valid_anc_count++;
            valid_anc_num[j].anc_ID=i;//记录有效基站编号
            valid_anc_num[j].distance=distanceArray[i];//记录有效基站距离
            j++;
        }
    }

    if(dimensional == 3)
    {
        int vailAncNums = 0;
        for(int i=0;i<MAX_ANC_NUM;i++)//未勾选的距离值给无效，不参与计算
        {
            if(stationList[i] == false)
            {
                distanceArray[i] = -1;
            }
            else
                vailAncNums++;
        }
//        result = leastSquaresMethod(best_solution, anchorArray, distanceArray);

        std::vector<vec3d> anchorVector;
        std::vector<double> distanceVector;
        convertToVectors(anchorArray, distanceArray, vailAncNums, anchorVector, distanceVector);

        result = multilateration(best_solution, anchorVector, distanceVector);

        //qDebug("dimensional = 3,result=%d", result);
        return result;
    }
    else if(dimensional == 1)
    {
        if(valid_anc_count < 2)
        {
            return -1;
        }

        else //valid_anc_count >= 2
        {
            qsort(valid_anc_num, (valid_anc_count+1), sizeof(valid_anc_num[0]), cmp);  //将有效距离值进行从小到大排序
            // for(int i=1;i<=valid_anc_count;i++) //输出结果
            //     printf("No%d DIS=%d,ID=A%d\n",i,valid_anc_num[i].distance,valid_anc_num[i].anc_ID);

            findPointA( best_solution,
                        anchorArray[valid_anc_num[1].anc_ID].x, 
                        anchorArray[valid_anc_num[1].anc_ID].y, 
                        anchorArray[valid_anc_num[2].anc_ID].x, 
                        anchorArray[valid_anc_num[2].anc_ID].y, 
                        (double) distanceArray[valid_anc_num[1].anc_ID] / 1000.0,
                        (double) distanceArray[valid_anc_num[2].anc_ID] / 1000.0
                    );

            return 1;
        }
    }

    else if(dimensional == 2)
    {
        //qDebug("dimensional = 22222");
        if(valid_anc_count < 3)
        {
            return -1;
        }

        else if(valid_anc_count==3)//直接执行三基站定位
        {
            use3anc=1;
            /* Anchors coordinate */
            p1.x = anchorArray[valid_anc_num[0].anc_ID].x;		p1.y = anchorArray[valid_anc_num[0].anc_ID].y;	    p1.z = anchorArray[valid_anc_num[0].anc_ID].z;
            p2.x = anchorArray[valid_anc_num[1].anc_ID].x;		p2.y = anchorArray[valid_anc_num[1].anc_ID].y;	    p2.z = anchorArray[valid_anc_num[1].anc_ID].z;
            p3.x = anchorArray[valid_anc_num[2].anc_ID].x;		p3.y = anchorArray[valid_anc_num[2].anc_ID].y;	    p3.z = anchorArray[valid_anc_num[2].anc_ID].z;
            p4.x = p1.x;		                                p4.y = p1.y;	                                    p4.z = p1.z;

            r1 = (double) distanceArray[valid_anc_num[0].anc_ID] / 1000.0;
            r2 = (double) distanceArray[valid_anc_num[1].anc_ID] / 1000.0;
            r3 = (double) distanceArray[valid_anc_num[2].anc_ID] / 1000.0;
            r4 = r1;

            // printf("anc1=%d,anc2=%d,anc3=%d\n",valid_anc_num[0],valid_anc_num[1],valid_anc_num[2]);
            // printf("dis1=%f,dis2=%f,dis3=%f\n",r1,r2,r3);
            // printf("P1X=%f,P1Y=%f,P1Z=%f\n",p1.x,p1.y,p1.z);
        }

        else if(valid_anc_count==4)//直接执行4基站定位
        {
            /* Anchors coordinate */
            p1.x = anchorArray[valid_anc_num[0].anc_ID].x;		p1.y = anchorArray[valid_anc_num[0].anc_ID].y;	    p1.z = anchorArray[valid_anc_num[0].anc_ID].z;
            p2.x = anchorArray[valid_anc_num[1].anc_ID].x;		p2.y = anchorArray[valid_anc_num[1].anc_ID].y;	    p2.z = anchorArray[valid_anc_num[1].anc_ID].z;
            p3.x = anchorArray[valid_anc_num[2].anc_ID].x;		p3.y = anchorArray[valid_anc_num[2].anc_ID].y;	    p3.z = anchorArray[valid_anc_num[2].anc_ID].z;
            p4.x = anchorArray[valid_anc_num[3].anc_ID].x;		p4.y = anchorArray[valid_anc_num[3].anc_ID].y;	    p4.z = anchorArray[valid_anc_num[3].anc_ID].z;


            r1 = (double) distanceArray[valid_anc_num[0].anc_ID] / 1000.0;
            r2 = (double) distanceArray[valid_anc_num[1].anc_ID] / 1000.0;
            r3 = (double) distanceArray[valid_anc_num[2].anc_ID] / 1000.0;
            r4 = (double) distanceArray[valid_anc_num[3].anc_ID] / 1000.0;

        }

        //valid_anc_count 有效基站个数
        //valid_anc_num[0] 有效基站编号
        else if(valid_anc_count>4)//执行基站选取机制，选取最近的4个基站1234进行计算
        {
            qsort(valid_anc_num, (valid_anc_count+1), sizeof(valid_anc_num[0]), cmp);  //将有效距离值进行从小到大排序
            for(int i=1;i<=valid_anc_count;i++) //输出结果
            {
                //printf("No%d DIS=%d,ID=A%d\n",i,valid_anc_num[i].distance,valid_anc_num[i].anc_ID);
            // qDebug() << "No=" <<i<<",DIS="<<valid_anc_num[i].distance<<",ID=A"<<valid_anc_num[i].anc_ID;
            }

            p1.x = anchorArray[valid_anc_num[1].anc_ID].x;		p1.y = anchorArray[valid_anc_num[1].anc_ID].y;	    p1.z = anchorArray[valid_anc_num[1].anc_ID].z;
            p2.x = anchorArray[valid_anc_num[2].anc_ID].x;		p2.y = anchorArray[valid_anc_num[2].anc_ID].y;	    p2.z = anchorArray[valid_anc_num[2].anc_ID].z;
            p3.x = anchorArray[valid_anc_num[3].anc_ID].x;		p3.y = anchorArray[valid_anc_num[3].anc_ID].y;	    p3.z = anchorArray[valid_anc_num[3].anc_ID].z;
            p4.x = anchorArray[valid_anc_num[4].anc_ID].x;		p4.y = anchorArray[valid_anc_num[4].anc_ID].y;	    p4.z = anchorArray[valid_anc_num[4].anc_ID].z;

            r1 = (double) distanceArray[valid_anc_num[1].anc_ID] / 1000.0;
            r2 = (double) distanceArray[valid_anc_num[2].anc_ID] / 1000.0;
            r3 = (double) distanceArray[valid_anc_num[3].anc_ID] / 1000.0;
            r4 = (double) distanceArray[valid_anc_num[4].anc_ID] / 1000.0;

            //printf("use1=A%d,use2=A%d,use3=A%d,use4=A%d\n",valid_anc_num[1].anc_ID,valid_anc_num[2].anc_ID,valid_anc_num[3].anc_ID,valid_anc_num[4].anc_ID);
        //  qDebug() << "use1=" <<valid_anc_num[1].anc_ID;
        //  qDebug() << "use2=" <<valid_anc_num[2].anc_ID;
        // qDebug() << "use3=" <<valid_anc_num[3].anc_ID;
    //        qDebug() << "use4=" <<valid_anc_num[4].anc_ID;
        }


        result = deca_3dlocate (&o1, &o2, best_solution, &error, &best_3derror, &best_gdoprate,
                                p1, r1, p2, r2, p3, r3, p4, r4, &combination);


        if((result==0)&&(valid_anc_count>4))//多于4基站选取后计算失败，把第1舍掉用第2345计算
        {
            //puts("Second calculation");
            p1.x = anchorArray[valid_anc_num[2].anc_ID].x;		p1.y = anchorArray[valid_anc_num[2].anc_ID].y;	    p1.z = anchorArray[valid_anc_num[2].anc_ID].z;
            p2.x = anchorArray[valid_anc_num[3].anc_ID].x;		p2.y = anchorArray[valid_anc_num[3].anc_ID].y;	    p2.z = anchorArray[valid_anc_num[3].anc_ID].z;
            p3.x = anchorArray[valid_anc_num[4].anc_ID].x;		p3.y = anchorArray[valid_anc_num[4].anc_ID].y;	    p3.z = anchorArray[valid_anc_num[4].anc_ID].z;
            p4.x = anchorArray[valid_anc_num[5].anc_ID].x;		p4.y = anchorArray[valid_anc_num[5].anc_ID].y;	    p4.z = anchorArray[valid_anc_num[5].anc_ID].z;

            r1 = (double) distanceArray[valid_anc_num[2].anc_ID] / 1000.0;
            r2 = (double) distanceArray[valid_anc_num[3].anc_ID] / 1000.0;
            r3 = (double) distanceArray[valid_anc_num[4].anc_ID] / 1000.0;
            r4 = (double) distanceArray[valid_anc_num[5].anc_ID] / 1000.0;

            //printf("use1=A%d,use2=A%d,use3=A%d,use4=A%d\n",valid_anc_num[2].anc_ID,valid_anc_num[3].anc_ID,valid_anc_num[4].anc_ID,valid_anc_num[5].anc_ID);

    //        qDebug() << "use1=" <<valid_anc_num[2].anc_ID;
    //        qDebug() << "use2=" <<valid_anc_num[3].anc_ID;
    //        qDebug() << "use3=" <<valid_anc_num[4].anc_ID;
    //        qDebug() << "use4=" <<valid_anc_num[5].anc_ID;


            result = deca_3dlocate (&o1, &o2, best_solution, &error, &best_3derror, &best_gdoprate,
                            p1, r1, p2, r2, p3, r3, p4, r4, &combination);
            //qDebug("dimensional = 2,result=%d", result);
        }


        if (result >= 0)
        {
            if(o1.z <= o2.z) best_solution->z = o1.z; else best_solution->z = o2.z;
            if (use3anc == 1 || result == TRIL_3SPHERES)
            {
                if(o1.z < p1.z) *best_solution = o1; else *best_solution = o2; //assume tag is below the anchors (1, 2, and 3)
            }

            return result;
        }

        return -1;
    }
    else
    {
        return -1;
    }
}



void setStationList(int index, bool visible)
{
    //更新勾选基站状态
    if(index < MAX_ANC_NUM)
    {
        stationList[index] = visible;
    }

    //重新计算勾选基站数量
    stationNumber = 0;
    for (int i = 0; i < MAX_ANC_NUM; ++i)
    {
        if(stationList[i])
        {
            stationNumber++;
            maxStationID = i;
        }
    }

    //根据勾选基站数量，重置矩阵大小
    g_range_anchor.resize(stationNumber);
    for (int i = 0; i < stationNumber; i++)
    {
        g_range_anchor[i].resize(stationNumber);
//        qInfo()<<" ============ "<<g_range_anchor.size()<<g_range_anchor[i].size();
    }

    g_range_anchor_backup.resize(stationNumber);
    for (int i = 0; i < stationNumber; i++)
    {
        g_range_anchor_backup[i].resize(stationNumber);
    }

    g_range_anchor_avg.resize(stationNumber);
    for (int i = 0; i < stationNumber; i++)
    {
        g_range_anchor_avg[i].resize(stationNumber);
    }

}
