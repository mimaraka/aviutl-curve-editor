#pragma once

#include "constants.hpp"
#include "curve_graph_numeric.hpp"
#include "string_table.hpp"


namespace cved {
	// カーブ(ベジェ)
	class BezierCurve : public NumericGraphCurve {
		mkaul::Point<double> handle_left_;
		mkaul::Point<double> handle_right_;

	public:
		enum class HandleType {
			Left,
			Right
		};

		static constexpr double DEFAULT_HANDLE_XY = 0.3;
		// コンストラクタ
		BezierCurve(
			const mkaul::Point<double>& anchor_start = mkaul::Point{ 0., 0. },
			const mkaul::Point<double>& anchor_end = mkaul::Point{ 1., 1. },
			bool pt_fixed = false,
			GraphCurve* prev = nullptr,
			GraphCurve* next = nullptr,
			mkaul::Point<double> handle_left = mkaul::Point{ DEFAULT_HANDLE_XY, DEFAULT_HANDLE_XY },
			mkaul::Point<double> handle_right = mkaul::Point{ -DEFAULT_HANDLE_XY, -DEFAULT_HANDLE_XY }
		) noexcept;

		// コピーコンストラクタ
		BezierCurve(const BezierCurve& curve) noexcept;

		[[nodiscard]] constexpr std::string get_name() const noexcept override { return global::CURVE_NAME_BEZIER; }
		[[nodiscard]] std::string get_disp_name() const noexcept override { return global::string_table[global::StringTable::StringId::LabelEditModeBezier]; }

		[[nodiscard]] double get_handle_left_x() const noexcept { return anchor_start().x + handle_left_.x; }
		[[nodiscard]] double get_handle_left_y() const noexcept { return anchor_start().y + handle_left_.y; }
		[[nodiscard]] double get_handle_right_x() const noexcept { return anchor_end().x + handle_right_.x; }
		[[nodiscard]] double get_handle_right_y() const noexcept { return anchor_end().y + handle_right_.y; }

		void set_handle_left(double x, double y, bool keep_angle = false) noexcept;
		void set_handle_right(double x, double y, bool keep_angle = false) noexcept;

		// カーブの値を取得する
		[[nodiscard]] double curve_function(double progress, double start, double end) const noexcept override;
		void clear() noexcept override;
		[[nodiscard]] bool is_default() const noexcept override;
		void reverse(bool fix_pt = false) noexcept override;

		// カーブから一意な整数値を生成
		[[nodiscard]] int32_t encode() const noexcept override;
		// 整数値からカーブに変換
		bool decode(int32_t code) noexcept override;

		[[nodiscard]] std::string create_params_str(size_t precision = 2) const noexcept override;
		bool read_params(const std::vector<double>& params) noexcept override;

		[[nodiscard]] nlohmann::json create_json() const noexcept override;
		bool load_json(const nlohmann::json& data) noexcept override;

		template <class Archive>
		void save(Archive& archive, const std::uint32_t) const {
			archive(
				cereal::base_class<NumericGraphCurve>(this),
				handle_left_,
				handle_right_
			);
		}

		template <class Archive>
		void load(Archive& archive, const std::uint32_t) {
			archive(
				cereal::base_class<NumericGraphCurve>(this),
				handle_left_,
				handle_right_
			);
		}
	};
}

CEREAL_CLASS_VERSION(cved::BezierCurve, 0)
CEREAL_REGISTER_TYPE(cved::BezierCurve)
CEREAL_REGISTER_POLYMORPHIC_RELATION(cved::NumericGraphCurve, cved::BezierCurve)