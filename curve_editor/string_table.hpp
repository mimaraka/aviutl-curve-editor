#pragma once

#include <Windows.h>
#include <string>



namespace cved {
	namespace global {
		inline class StringTable {
		public:
			enum class StringId : size_t {
				ErrorLuaDLLNotFound,
				ErrorExeditNotFound,
				ErrorExeditHookFailed,
				ErrorGraphicsInitFailed,
				ErrorConnectionFailed,
				ErrorOutOfRange,
				ErrorInvalidInput,
				ErrorCodeCopyFailed,
				ErrorDataLoadFailed,
				WarningDeleteCurve,
				WarningDeleteAllCurves,
				WarningResetPreferences,
				WarningDeleteId,
				WarningDeleteAllIds,
				InfoLatestVersion,
				InfoUpdateAvailable,
				InfoRestartAviutl,
				LabelEditModeNormal,
				LabelEditModeValue,
				LabelEditModeBezier,
				LabelEditModeElastic,
				LabelEditModeBounce,
				LabelEditModeScript,
				LabelCurveSegmentTypeLinear,
				LabelTooltipCopy,
				LabelTooltipRead,
				LabelTooltipSave,
				LabelTooltipClear,
				LabelTooltipFit,
				LabelTooltipMore,
				LabelTooltipIdBack,
				LabelTooltipIdNext,
				LabelTooltipCurrentId,
				LabelThemeNameDark,
				LabelThemeNameLight,
				LabelPreferenceGeneral,
				LabelPreferenceAppearance,
				LabelPreferenceBehavior,
				LabelPreferenceEditing,
				LabelSelectBackgroundImage,
				WordVersion,
				WordEditMode,
				WordLanguageAutomatic,
				WordImageFiles,
				NumStringId
			};

		private:
			static constexpr size_t MAX_LEN = 0x200;
			bool loaded_ = false;

			std::string string_data[(size_t)StringId::NumStringId];
		public:
			StringTable() :
				string_data()
			{}

			bool load(HINSTANCE hinst);
			bool loaded() const noexcept { return loaded_; }
			const char* operator[] (StringId str_id);
		} string_table;
	}
}