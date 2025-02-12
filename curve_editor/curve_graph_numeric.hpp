#pragma once

#include "curve_graph.hpp"



namespace curve_editor {
	class NumericGraphCurve : public GraphCurve {
	public:
		using GraphCurve::GraphCurve;

		// カーブから一意な整数値を生成
		virtual int32_t encode() const noexcept = 0;
		// 整数値からカーブに変換
		virtual bool decode(int32_t code) noexcept = 0;

		virtual std::string create_params_str(size_t precision = 2) const noexcept = 0;
		virtual bool read_params(const std::vector<double>& params) = 0;

		template <class Archive>
		void save(Archive& archive, const std::uint32_t) const {
			archive(
				cereal::base_class<GraphCurve>(this)
			);
		}

		template <class Archive>
		void load(Archive& archive, const std::uint32_t) {
			archive(
				cereal::base_class<GraphCurve>(this)
			);
		}
	};
} // namespace curve_editor

CEREAL_CLASS_VERSION(curve_editor::NumericGraphCurve, 0)
CEREAL_REGISTER_TYPE(curve_editor::NumericGraphCurve)
CEREAL_REGISTER_POLYMORPHIC_RELATION(curve_editor::GraphCurve, curve_editor::NumericGraphCurve)