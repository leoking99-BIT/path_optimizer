//
// Created by ljn on 19-8-16.
//

#ifndef MPC_PATH_OPTIMIZER__FGEVALFRENET_HPP_
#define MPC_PATH_OPTIMIZER__FGEVALFRENET_HPP_
#include <vector>

namespace PathOptimizationNS {
using CppAD::AD;

class FgEvalFrenet {
public:
    FgEvalFrenet(const std::vector<double> &seg_x_list,
                 const std::vector<double> &seg_y_list,
                 const std::vector<double> &seg_angle_list,
                 const std::vector<double> &seg_k_list,
                 const std::vector<double> &seg_s_list,
                 const int &N,
                 const std::vector<double> &cost_func,
                 const hmpl::State &first_state,
                 const std::vector<double> &car_geometry,
                 const std::vector<std::vector<double>> &bounds,
                 const double epsi) :
        N(N),
        seg_s_list_(seg_s_list),
        seg_x_list_(seg_x_list),
        seg_y_list_(seg_y_list),
        seg_angle_list_(seg_angle_list),
        seg_k_list_(seg_k_list),
        cost_func_curvature_weight_(cost_func[0]),
        cost_func_curvature_rate_weight_(cost_func[1]),
        cost_func_bound_weight_(cost_func[2]),
        cost_func_offset_weight_(cost_func[3]),
        first_state_(first_state),
        car_geometry_(car_geometry),
        bounds_(bounds),
        epsi_(epsi) {}
public:

    AD<double> constraintAngle(AD<double> angle) {
        if (angle > M_PI) {
            angle -= 2 * M_PI;
            return constraintAngle(angle);
        } else if (angle < -M_PI) {
            angle += 2 * M_PI;
            return constraintAngle(angle);
        } else {
            return angle;
        }
    }
    size_t N;
    const std::vector<double> &seg_s_list_;
    const std::vector<double> &seg_x_list_;
    const std::vector<double> &seg_y_list_;
    const std::vector<double> &seg_k_list_;
    const std::vector<double> &seg_angle_list_;

    double cost_func_curvature_weight_;
    double cost_func_curvature_rate_weight_;
    double cost_func_bound_weight_;
    double cost_func_offset_weight_;

    const hmpl::State &first_state_;
    const std::vector<double> &car_geometry_;
    const std::vector<std::vector<double> > &bounds_;
    double epsi_;

    typedef CPPAD_TESTVECTOR(AD<double>) ADvector;

    void operator()(ADvector &fg, const ADvector &vars) {
        const size_t pq_range_begin = 0;
        const size_t psi_range_begin = pq_range_begin + N;
        const size_t steer_range_begin = psi_range_begin + N;
        const size_t cons_pq_range_begin = 1;
        const size_t cons_psi_range_begin = cons_pq_range_begin + N;
        const size_t cons_c0_range_begin = cons_psi_range_begin + N;
        const size_t cons_c1_range_begin = cons_c0_range_begin + N;
        const size_t cons_c2_range_begin = cons_c1_range_begin + N;
        const size_t cons_c3_range_begin = cons_c2_range_begin + N;
//        const size_t cons_steer_change_range_begin = cons_front_range_begin + N;
        AD<double> rear_axle_to_center_circle = car_geometry_[5];
        AD<double> rear_axle_to_circle_0 = rear_axle_to_center_circle + car_geometry_[0];
        AD<double> rear_axle_to_circle_1 = rear_axle_to_center_circle + car_geometry_[1];
        AD<double> rear_axle_to_circle_2 = rear_axle_to_center_circle + car_geometry_[2];
        AD<double> rear_axle_to_circle_3 = rear_axle_to_center_circle + car_geometry_[3];
        AD<double> wheel_base = car_geometry_[6];
        fg[cons_pq_range_begin] = vars[pq_range_begin];
        fg[cons_psi_range_begin] = vars[psi_range_begin];
        for (size_t i = 0; i != N - 1; ++i) {
            AD<double> pq0 = vars[pq_range_begin + i];
            AD<double> pq1 = vars[pq_range_begin + i + 1];
            AD<double> psi0 = vars[psi_range_begin + i];
            AD<double> psi1 = vars[psi_range_begin + i + 1];
            AD<double> steer0 = vars[steer_range_begin + i];
            AD<double> steer1 = vars[steer_range_begin + i + 1];
            AD<double> ds = seg_s_list_[i + 1] - seg_s_list_[i];
            AD<double> next_pq = pq0 + ds * psi0;
            fg[cons_pq_range_begin + i + 1] = pq1 - next_pq;
            AD<double> steer_ref = atan(wheel_base * seg_k_list_[i]);
            AD<double> next_psi = psi0 - ds * pow(seg_k_list_[i], 2) * pq0
                + ds / wheel_base / pow(cos(steer_ref), 2) * (steer0 - steer_ref);
            fg[cons_psi_range_begin + i + 1] = psi1 - next_psi;
            AD<double> d0 = rear_axle_to_circle_0 * (psi0) + pq0;
            AD<double> d1 = rear_axle_to_circle_1 * (psi0) + pq0;
            AD<double> d2 = rear_axle_to_circle_2 * (psi0) + pq0;
            AD<double> d3 = rear_axle_to_circle_3 * (psi0) + pq0;
            fg[cons_c0_range_begin + i] = d0;
            fg[cons_c1_range_begin + i] = d1;
            fg[cons_c2_range_begin + i] = d2;
            fg[cons_c3_range_begin + i] = d3;
            fg[0] += cost_func_curvature_weight_ * pow(steer0, 2);
            fg[0] += cost_func_curvature_rate_weight_ * pow(steer1 - steer0, 2);
//            AD<double> middle_rear = (bounds_[i][0] + bounds_[i][1]) / 2;
//            fg[0] += cost_func_bound_weight_ * pow(d0 - middle_rear, 2);
            fg[0] += cost_func_offset_weight_ * pow(pq0, 2);
//            AD<double> delta_s = ds * (1 - pq0 * seg_k_list_[i]);
//            fg[0] += 1 * pow(delta_s, 2);
            if (i == N - 2) {
                d0 = rear_axle_to_circle_0 * (psi1) + pq1;
                d1 = rear_axle_to_circle_1 * (psi1) + pq1;
                d2 = rear_axle_to_circle_2 * (psi1) + pq1;
                d3 = rear_axle_to_circle_3 * (psi1) + pq1;
                fg[cons_c0_range_begin + i + 1] = d0;
                fg[cons_c1_range_begin + i + 1] = d1;
                fg[cons_c2_range_begin + i + 1] = d2;
                fg[cons_c3_range_begin + i + 1] = d3;
            }
        }
    }
};
}

#endif //MPC_PATH_OPTIMIZER__FGEVALFRENET_HPP_