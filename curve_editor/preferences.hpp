#pragma once

#include <nlohmann/json.hpp>
#include <mkaul/include/graphics.hpp>
#include "enum.hpp"



namespace cved {
	struct Preferences {
		Language language = Language::Automatic;
		ThemeId theme_id = ThemeId::Dark;
		mkaul::ColorF curve_color = mkaul::ColorF{};
		float curve_thickness = 0.f;
		float curve_drawing_interval = 0.f;
		mkaul::graphics::Factory::GraphicEngine graphic_method = mkaul::graphics::Factory::GraphicEngine::Directx;
		std::filesystem::path bg_image_path = "";
		float bg_image_opacity = 0.f;
		bool show_popup = false;
		bool show_trace = false;
		bool auto_copy = false;
		bool auto_apply = false;
		bool reverse_wheel = false;
		bool notify_update = false;
		bool set_bg_image = false;

		void reset() noexcept;
		void from_json(const nlohmann::json& data) noexcept;
		void to_json(nlohmann::json* p_data) const noexcept;
	};
}