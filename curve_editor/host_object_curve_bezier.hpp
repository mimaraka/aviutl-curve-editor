#pragma once

#include "host_object_curve_graph.hpp"



namespace cved {
	class BezierCurveHostObject : public GraphCurveHostObject {
		static std::vector<double> get_handle_left(uint32_t id);
		static void begin_move_handle_left(uint32_t id, double scale_x, double scale_y);
		static void move_handle_left(uint32_t id, double x, double y, bool keep_angle);
		static std::vector<double> get_handle_right(uint32_t id);
		static void begin_move_handle_right(uint32_t id, double scale_x, double scale_y);
		static void move_handle_right(uint32_t id, double x, double y, bool keep_angle);
		static std::string get_param(uint32_t id);
		static uint32_t get_id(bool is_select_dialog);

	public:
		BezierCurveHostObject() {
			register_member(L"getId", DispatchType::Method, get_id);
			register_member(L"getHandleLeft", DispatchType::Method, get_handle_left);
			register_member(L"beginMoveHandleLeft", DispatchType::Method, begin_move_handle_left);
			register_member(L"moveHandleLeft", DispatchType::Method, move_handle_left);
			register_member(L"getHandleRight", DispatchType::Method, get_handle_right);
			register_member(L"beginMoveHandleRight", DispatchType::Method, begin_move_handle_right);
			register_member(L"moveHandleRight", DispatchType::Method, move_handle_right);
			register_member(L"getParam", DispatchType::Method, get_param);
		}
	};
} // namespace cved