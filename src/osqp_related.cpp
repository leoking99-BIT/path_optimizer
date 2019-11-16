//
// Created by ljn on 19-11-15.
//

#include "path_optimizer.hpp"
namespace PathOptimizationNS {

void PathOptimizer::setHessianMatrix(size_t horizon, Eigen::SparseMatrix<double> *matrix_h) {
    const size_t state_size = 2 * horizon;
    const size_t control_size = horizon - 1;
    const size_t matrix_size = state_size + control_size;
    double w_c = 10;
    double w_cr = 100;
    double w_pq = 0.01;
    Eigen::MatrixXd hessian = Eigen::MatrixXd::Constant(matrix_size, matrix_size, 0);
    // Populate hessian matrix
    Eigen::Matrix2d matrix_Q;
    matrix_Q << 0, 0,
        0, w_pq;
    Eigen::SparseMatrix<double> matrix_R;
    matrix_R.resize(control_size, control_size);
    for (size_t i = 0; i != control_size; ++i) {
        for (size_t j = 0; j != control_size; ++j) {
            if (i == j) {
                if (i == 0 || i == control_size - 1) {
                    matrix_R.insert(i, j) = w_c + w_cr;
                } else {
                    matrix_R.insert(i, j) = w_cr * 2 + w_c;
                }
            } else if (i == j - 1 || i == j + 1) {
                matrix_R.insert(i, j) = -w_cr;
            }
        }
    }
    for (size_t i = 0; i != horizon; ++i) {
        hessian.block(2 * i, 2 * i, 2, 2) = matrix_Q;
    }
    hessian.block(2 * horizon, 2 * horizon, horizon - 1, horizon - 1) = matrix_R;
    *matrix_h = hessian.sparseView();
}

void PathOptimizer::setDynamicMatrix(size_t i,
                                     Eigen::Matrix<double, 2, 2> *matrix_a,
                                     Eigen::Matrix<double, 2, 1> *matrix_b) {
    double ref_k = seg_k_list_[i];
    double ref_s = seg_s_list_[i + 1] - seg_s_list_[i];
    double ref_delta = atan(ref_k * wheel_base);
    Eigen::Matrix2d a;
    a << 1, -ref_s * pow(ref_k, 2),
        ref_s, 1;
    Eigen::Matrix<double, 2, 1> b;
    b << ref_s / wheel_base / pow(cos(ref_delta), 2), 0;
    *matrix_a = a;
    *matrix_b = b;
}

void PathOptimizer::setConstraintMatrix(size_t horizon,
                                        Eigen::SparseMatrix<double> *matrix_constraints,
                                        Eigen::VectorXd *lower_bound,
                                        Eigen::VectorXd *upper_bound,
                                        const std::vector<double> &init_state) {
    //TODO: how to initialize a zero matrix?
    Eigen::MatrixXd cons = Eigen::MatrixXd::Zero(9 * horizon - 1, 3 * horizon - 1);
    for (size_t i = 0; i != 2 * horizon; ++i) {
        cons(i, i) = -1;
    }
    Eigen::Matrix<double, 2, 2> a;
    Eigen::Matrix<double, 2, 1> b;
    for (size_t i = 0; i != horizon - 1; ++i) {
        setDynamicMatrix(i, &a, &b);
        cons.block(2 * (i + 1), 2 * i, 2, 2) = a;
        cons.block(2 * (i + 1), 2 * horizon + i, 2, 1) = b;
    }
    for (size_t i = 0; i != 3 * horizon - 1; ++i) {
        cons(2 * horizon + i, i) = 1;
    }
    Eigen::Matrix<double, 4, 2> collision;
    collision << car_geo_[0] + car_geo_[5], 1,
        car_geo_[1] + car_geo_[5], 1,
        car_geo_[2] + car_geo_[5], 1,
        car_geo_[3] + car_geo_[5], 1;
    for (size_t i = 0; i != horizon; ++i) {
        cons.block(5 * horizon - 1 + 4 * i, 2 * i, 4, 2) = collision;
    }
    *matrix_constraints = cons.sparseView();

    *lower_bound = Eigen::MatrixXd::Zero(9 * horizon - 1, 1);
    *upper_bound = Eigen::MatrixXd::Zero(9 * horizon - 1, 1);
//    lower_bound->resize(9 * horizon - 1);
//    upper_bound->resize(9 * horizon - 1);
    Eigen::Matrix<double, 2, 1> x0;
    x0 << init_state[0], init_state[1];
    lower_bound->block(0, 0, 2, 1) = -x0;
    upper_bound->block(0, 0, 2, 1) = -x0;
//    lower_bound->block(2, 0, 2 * horizon - 2, 1) = Eigen::VectorXd::Zero(2 * horizon - 2);
//    upper_bound->block(2, 0, 2 * horizon - 2, 1) = Eigen::VectorXd::Zero(2 * horizon - 2);
    for (size_t i = 0; i != horizon - 1; ++i) {
        double ds = seg_s_list_[i + 1] - seg_s_list_[i];
        double steer = atan(seg_k_list_[i] * wheel_base);
        Eigen::Vector2d c;
        c << ds * steer / wheel_base / pow(cos(steer), 2), 0;
        lower_bound->block(2 + 2 * i, 0, 2, 1) = c;
        upper_bound->block(2 + 2 * i, 0, 2, 1) = c;
    }
    lower_bound->block(2 * horizon, 0, 2 * horizon, 1) = Eigen::VectorXd::Constant(2 * horizon, -OsqpEigen::INFTY);
    upper_bound->block(2 * horizon, 0, 2 * horizon, 1) = Eigen::VectorXd::Constant(2 * horizon, OsqpEigen::INFTY);
    lower_bound->block(4 * horizon, 0, horizon - 1, 1) = Eigen::VectorXd::Constant(horizon - 1, -30 * M_PI / 180);
    upper_bound->block(4 * horizon, 0, horizon - 1, 1) = Eigen::VectorXd::Constant(horizon - 1, 30 * M_PI / 180);
    for (size_t i = 0; i != horizon; ++i) {
        Eigen::Vector4d ld, ud;
        ud
            << seg_clearance_list_[i][0], seg_clearance_list_[i][2], seg_clearance_list_[i][4], seg_clearance_list_[i][6];
        ld
            << seg_clearance_list_[i][1], seg_clearance_list_[i][3], seg_clearance_list_[i][5], seg_clearance_list_[i][7];
        lower_bound->block(5 * horizon - 1 + 4 * i, 0, 4, 1) = ld;
        upper_bound->block(5 * horizon - 1 + 4 * i, 0, 4, 1) = ud;
    }
}

}