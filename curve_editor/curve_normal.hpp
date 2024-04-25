#pragma once

#include "curve_graph.hpp"
#include "enum.hpp"



namespace cved {
	// カーブ(標準)
	class NormalCurve : public GraphCurve {
		std::vector<std::unique_ptr<GraphCurve>> curve_segments_;

		template <class CurveType, class CurveDataType>
		static bool load_segment_data(
			const byte* data,
			size_t& idx,
			size_t size,
			std::unique_ptr<GraphCurve>& p_curve
		) noexcept {
			constexpr size_t n = sizeof(CurveDataType) / sizeof(byte);
			if (size < idx + n) return false;
			p_curve = std::make_unique<CurveType>();
			if (!p_curve->load_data(data + idx, n)) return false;
			idx += n;
			return true;
		}

	public:
		NormalCurve(
			const mkaul::Point<double>& pt_start = mkaul::Point{ 0., 0. },
			const mkaul::Point<double>& pt_end = mkaul::Point{ 1., 1. }
		);

		auto& get_curve_segments() noexcept { return curve_segments_; }

		double curve_function(double progress, double start, double end) const noexcept override;
		void clear() noexcept override;
		void reverse(bool fix_pt = false) noexcept override;
		void reverse_segment(size_t idx) noexcept;

		void create_data(std::vector<byte>& data) const noexcept override;
		bool load_data(const byte* data, size_t size) noexcept override;
		bool load_data_v1(const byte* data, size_t pt_n) noexcept;

		void draw_handle(
			mkaul::graphics::Graphics* p_graphics,
			const View& view,
			float thickness,
			float root_radius,
			float tip_radius,
			float tip_thickness,
			bool cutoff_line,
			const mkaul::ColorF& color = mkaul::ColorF{}
		) const noexcept override;

		bool add_curve(const mkaul::Point<double>& pt, const GraphView& view) noexcept;
		bool delete_curve(const mkaul::Point<double>& pt, const GraphView& view) noexcept;
		bool replace_curve(size_t idx, CurveSegmentType segment_type) noexcept;

		bool is_pt_hovered(const mkaul::Point<double>& pt, const GraphView& view) const noexcept override;
		bool is_handle_hovered(const mkaul::Point<double>& pt, const GraphView& view) const noexcept override;

		bool handle_check_hover(
			const mkaul::Point<double>& pt,
			const GraphView& view
		) noexcept override;

		bool handle_update(
			const mkaul::Point<double>& pt,
			const GraphView& view
		) noexcept override;
		void handle_end_control() noexcept override;

		// カーソルがポイントにホバーしているかを判定し、ホバーしていれば移動を開始
		ActivePoint pt_check_hover(const mkaul::Point<double>& pt, const GraphView& view) noexcept override;
		// ポイントの移動を開始
		bool pt_begin_move(ActivePoint) noexcept override { return false; }
		// ポイントの位置をアップデート
		ActivePoint pt_update(const mkaul::Point<double>& pt, const GraphView& view) noexcept override;
		// ポイントを強制的に動かす
		bool pt_move(ActivePoint, const mkaul::Point<double>&) noexcept override { return true; }
		// ポイントの移動を終了
		void pt_end_move() noexcept override;
		void pt_end_control() noexcept override;
	};
}