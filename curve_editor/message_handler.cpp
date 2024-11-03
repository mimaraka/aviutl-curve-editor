#include "config.hpp"
#include "context_menu.hpp"
#include "curve_editor.hpp"
#include "dialog_about.hpp"
#include "dialog_id_jumpto.hpp"
#include "dialog_modifier.hpp"
#include "dialog_pref.hpp"
#include "dialog_update_notification.hpp"
#include "global.hpp"
#include "input_box.hpp"
#include "message_box.hpp"
#include "message_handler.hpp"
#include "my_webview2.hpp"
#include "preset_manager.hpp"
#include "string_table.hpp"
#include "util.hpp"
#include "update_checker.hpp"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <magic_enum/magic_enum.hpp>
#include <regex>
#include <strconv2.h>
#include <string_view>



namespace cved {
	using StringId = global::StringTable::StringId;
	constexpr auto EXT_COLLECTION = "cecl";

	/// <summary>
	/// WebView2からのメッセージを処理する関数
	/// </summary>
	/// <param name="message">オプションが格納されたjsonオブジェクト</param>
	/// <returns>メッセージが処理された場合: true, メッセージが処理されなかった場合: false</returns>
	bool MessageHandler::handle_message(const nlohmann::json& message) {
		try {
			auto command_str = message.at("command").get<std::string>();
			auto message_command = magic_enum::enum_cast<MessageCommand>(command_str);
			for (const auto& [command, callback] : handlers_) {
				if (message_command.has_value() and message_command.value() == command) {
					callback(message);
					return true;
				}
			}
		}
		catch (nlohmann::json::exception&) {}
		return false;
	}


	/// <summary>
	/// WebView2にメッセージを送信する関数
	/// </summary>
	/// <param name="command">送信するコマンド</param>
	/// <param name="options">オプションが格納されたjsonオブジェクト</param>
	void MessageHandler::send_command(MessageCommand command, const nlohmann::json& options) {
		if (p_webview_) {
			auto command_sv = magic_enum::enum_name(command);
			p_webview_->post_message(::ansi_to_wide(command_sv.data()), options);
		}
	}


	/// <summary>
	/// 「カーブのコード/IDをコピー」ボタンが押されたときに呼び出される関数
	/// </summary>
	void MessageHandler::button_copy() {
		auto tmp = std::to_string(global::editor.track_param());
		if (!util::copy_to_clipboard(hwnd_, tmp.c_str()) and global::config.get_show_popup()) {
			util::message_box(global::string_table[StringId::ErrorCodeCopyFailed], hwnd_, util::MessageBoxIcon::Error);
		}
	}

	/// <summary>
	/// 「カーブのコードを読み取り」ボタンが押されたときに呼び出される関数
	/// </summary>
	void MessageHandler::button_curve_code() {
		util::input_box(
			hwnd_,
			global::string_table[StringId::PromptCurveCode],
			global::string_table[StringId::CaptionCurveCode],
			[this](HWND hwnd, const std::string& text) {
				const std::regex regex_code{ R"(^-?\d+$)" };
				auto p_curve = global::editor.editor_graph().numeric_curve();
				if (p_curve) {
					if (std::regex_match(text, regex_code)) {
						try {
							if (p_curve->decode(std::stoi(text))) {
								if (p_webview_) p_webview_->send_command(MessageCommand::UpdateHandlePosition);
								return true;
							}
						}
						catch (std::out_of_range&) {
							// 入力値が範囲外の場合
							if (global::config.get_show_popup()) {
								util::message_box(global::string_table[StringId::ErrorOutOfRange], hwnd, util::MessageBoxIcon::Error);
							}
							return false;
						}
					}
					// 入力値が不正の場合
					else {
						if (global::config.get_show_popup()) {
							util::message_box(global::string_table[StringId::ErrorInvalidInput], hwnd, util::MessageBoxIcon::Error);
						}
						return false;
					}
				}
				return true;
			}
		);
	}


	/// <summary>
	/// 「プリセットに保存」ボタンが押されたときに呼び出される関数
	/// </summary>
	void MessageHandler::button_save() {
		util::input_box(
			hwnd_,
			global::string_table[StringId::PromptCreatePreset],
			global::string_table[StringId::CaptionCreatePreset],
			[this](HWND hwnd, const std::string& text) -> bool {
				if (text.empty()) {
					util::message_box(global::string_table[StringId::ErrorInvalidInput], hwnd, util::MessageBoxIcon::Error);
					return false;
				}

				auto curve = global::editor.current_curve();
				if (curve) {
					if (curve->get_name() == global::CURVE_NAME_NORMAL) {
						global::preset_manager.create_preset(*dynamic_cast<NormalCurve*>(curve), text);
					}
					else if (curve->get_name() == global::CURVE_NAME_BEZIER) {
						global::preset_manager.create_preset(*dynamic_cast<BezierCurve*>(curve), text);
					}
					else if (curve->get_name() == global::CURVE_NAME_ELASTIC) {
						global::preset_manager.create_preset(*dynamic_cast<ElasticCurve*>(curve), text);
					}
					else if (curve->get_name() == global::CURVE_NAME_BOUNCE) {
						global::preset_manager.create_preset(*dynamic_cast<BounceCurve*>(curve), text);
					}
					else if (curve->get_name() == global::CURVE_NAME_SCRIPT) {
						global::preset_manager.create_preset(*dynamic_cast<ScriptCurve*>(curve), text);
					}
					else {
						util::message_box(global::string_table[StringId::ErrorPresetCreateFailed], hwnd, util::MessageBoxIcon::Error);
					}
				}
				else {
					util::message_box(global::string_table[StringId::ErrorPresetCreateFailed], hwnd, util::MessageBoxIcon::Error);
				}

				if (p_webview_) p_webview_->send_command(MessageCommand::UpdatePresets);
				return true;
			}
		);
	}

	void MessageHandler::button_lock(const nlohmann::json& options) {
		bool sw = options.at("switch").get<bool>();
	}


	/// <summary>
	/// 「カーブを削除」ボタンが押されたときに呼び出される関数
	/// </summary>
	void MessageHandler::button_clear() {
		int response = IDOK;
		if (global::config.get_show_popup()) {
			response = util::message_box(
				global::string_table[StringId::WarningDeleteCurve],
				hwnd_,
				util::MessageBoxIcon::Warning,
				util::MessageBoxButton::OkCancel
			);
		}

		if (response == IDOK) {
			auto curve = global::editor.current_curve();
			if (curve) {
				curve->clear();
				if (p_webview_) p_webview_->send_command(MessageCommand::UpdateControl);
			}
		}
	}


	/// <summary>
	/// 「その他」ボタンが押されたときに呼び出される関数
	/// </summary>
	void MessageHandler::button_others() {
		std::vector<MenuItem> menu_items = {
			MenuItem{
				global::string_table[StringId::MenuOthersWindowLayout],
				MenuItem::Type::String,
				MenuItem::State::Null,
				nullptr,
				{
					MenuItem{
						global::string_table[StringId::MenuOthersWindowLayoutVertical],
						MenuItem::Type::RadioCheck,
						global::config.get_layout_mode() == LayoutMode::Vertical ? MenuItem::State::Checked : MenuItem::State::Null,
						[this]() {
							global::config.set_layout_mode(LayoutMode::Vertical);
							if (p_webview_) p_webview_->send_command(MessageCommand::ChangeLayoutMode);
						}
					},
					MenuItem{
						global::string_table[StringId::MenuOthersWindowLayoutHorizontal],
						MenuItem::Type::RadioCheck,
						global::config.get_layout_mode() == LayoutMode::Horizontal ? MenuItem::State::Checked : MenuItem::State::Null,
						[this]() {
							global::config.set_layout_mode(LayoutMode::Horizontal);
							if (p_webview_) p_webview_->send_command(MessageCommand::ChangeLayoutMode);
						}
					}
				}
			},
			MenuItem{
				global::string_table[StringId::MenuOthersExtension],
				MenuItem::Type::String,
				MenuItem::State::Null,
				nullptr,
				{
					MenuItem{
						global::string_table[StringId::MenuOthersExtensionInstall],
						MenuItem::Type::String,
						MenuItem::State::Null,
						[this]() {

						}
					}
				}
			},
			MenuItem{
				global::string_table[StringId::MenuOthersReloadWindow],
				MenuItem::Type::String,
				MenuItem::State::Null,
				[this]() { if (p_webview_) p_webview_->reload(); }
			},
			MenuItem{"", MenuItem::Type::Separator},
			MenuItem{
				global::string_table[StringId::MenuOthersPreferences],
				MenuItem::Type::String,
				MenuItem::State::Null,
				[this]() {
					PrefDialog dialog;
					dialog.show(hwnd_);
				}
			},
			MenuItem{"", MenuItem::Type::Separator},
			MenuItem{
				global::string_table[StringId::MenuOthersHelp],
				MenuItem::Type::String,
				MenuItem::State::Null,
				[this]() {
					::ShellExecuteA(nullptr, "open", std::format("{}/wiki", global::PLUGIN_GITHUB_URL).c_str(), nullptr, nullptr, SW_SHOWNORMAL);
				}
			},
			MenuItem{
				global::string_table[StringId::MenuOthersAbout],
				MenuItem::Type::String,
				MenuItem::State::Null,
				[this]() {
					AboutDialog dialog;
					dialog.show(hwnd_);
				}
			}
		};
		if (global::update_checker.is_update_available()) {
			std::vector<MenuItem> menu_items_update = {
				MenuItem{
					global::string_table[StringId::MenuOthersUpdateAvailable],
					MenuItem::Type::String,
					MenuItem::State::Null,
					[this]() {
						UpdateNotificationDialog dialog;
						dialog.show(hwnd_);
					}
				},
				MenuItem{"", MenuItem::Type::Separator}
			};
			menu_items.insert(menu_items.begin(), menu_items_update.begin(), menu_items_update.end());
		}
		ContextMenu{ menu_items }.show(hwnd_);
	}

	void MessageHandler::button_idx() {
		IdJumptoDialog dialog;
		dialog.show(hwnd_);
	}


	/// <summary>
	/// パラメータ設定ボタンが押されたときに呼び出される関数
	/// </summary>
	void MessageHandler::button_param(const nlohmann::json& options) {
		auto curve_id = options.at("curveId").get<uint32_t>();
		auto curve = global::id_manager.get_curve<NumericGraphCurve>(curve_id);
		if (!curve) {
			return;
		}
		util::input_box(
			hwnd_,
			global::string_table[StringId::PromptCurveParam],
			global::string_table[StringId::CaptionCurveParam],
			[this, curve](HWND hwnd, const std::string& text) -> bool {
				std::vector<std::string> params_str;
				std::vector<double> params;
				boost::algorithm::split(params_str, text, boost::is_any_of(","));
				for (const auto& param_str : params_str) {
					try {
						params.emplace_back(std::stod(param_str));
					}
					catch (std::invalid_argument&) {
						if (global::config.get_show_popup()) {
							util::message_box(global::string_table[StringId::ErrorInvalidInput], hwnd, util::MessageBoxIcon::Error);
						}
						return false;
					}
				}
				if (!curve->read_params(params)) {
					if (global::config.get_show_popup()) {
						util::message_box(global::string_table[StringId::ErrorInvalidInput], hwnd, util::MessageBoxIcon::Error);
					}
					return false;
				}
				if (p_webview_) p_webview_->send_command(MessageCommand::UpdateHandlePosition);
				return true;
			},
			curve->create_params_str(4).c_str()
		);
	}


	/// <summary>
	/// グラフエディタのコンテキストメニューを表示する関数
	/// </summary>
	/// <param name="options">オプションが格納されたjsonオブジェクト</param>
	void MessageHandler::context_menu_graph(const nlohmann::json& options) {
		auto mode = options.at("mode").get<EditMode>();
		auto curve_id = options.at("curveId").get<uint32_t>();

		ContextMenu{
			MenuItem{
				global::string_table[StringId::MenuGraphApplyMode],
				MenuItem::Type::String,
				MenuItem::State::Null,
				nullptr,
				{
					MenuItem{
						global::string_table[StringId::LabelApplyModeNormal],
						MenuItem::Type::RadioCheck,
						global::config.get_apply_mode(mode) == ApplyMode::Normal ? MenuItem::State::Checked : MenuItem::State::Null,
						[mode]() {
							global::config.set_apply_mode(mode, ApplyMode::Normal);
						}
					},
					MenuItem{
						global::string_table[StringId::LabelApplyModeIgnoreMidPoint],
						MenuItem::Type::RadioCheck,
						global::config.get_apply_mode(mode) == ApplyMode::IgnoreMidPoint ? MenuItem::State::Checked : MenuItem::State::Null,
						[mode]() {
							global::config.set_apply_mode(mode, ApplyMode::IgnoreMidPoint);
						}
					},
					MenuItem{
						global::string_table[StringId::LabelApplyModeInterpolate],
						MenuItem::Type::RadioCheck,
						global::config.get_apply_mode(mode) == ApplyMode::Interpolate ? MenuItem::State::Checked : MenuItem::State::Null,
						[mode]() {
							global::config.set_apply_mode(mode, ApplyMode::Interpolate);
						}
					},
				}
			},
			MenuItem{"", MenuItem::Type::Separator},
			MenuItem{global::string_table[StringId::MenuGraphAddAnchor]},
			MenuItem{global::string_table[StringId::MenuGraphReverseCurve], MenuItem::Type::String, MenuItem::State::Null, [curve_id, this]() {
				auto curve = global::id_manager.get_curve<GraphCurve>(curve_id);
				if (curve) {
					curve->reverse();
					if (p_webview_) p_webview_->send_command(MessageCommand::UpdateHandlePosition);
				}
			}},
			MenuItem{global::string_table[StringId::MenuGraphModifier], MenuItem::Type::String, MenuItem::State::Null, [curve_id, this]() {
				ModifierDialog dialog;
				dialog.show(hwnd_, static_cast<LPARAM>(curve_id));
			}},
			MenuItem{"", MenuItem::Type::Separator},
			MenuItem{
				global::string_table[StringId::MenuGraphAlignHandle],
				MenuItem::Type::String,
				global::config.get_align_handle() ? MenuItem::State::Checked : MenuItem::State::Null,
				[]() {
					global::config.set_align_handle(!global::config.get_align_handle());
				}
			},
			MenuItem{
				global::string_table[StringId::MenuGraphShowXLabel],
				MenuItem::Type::String,
				global::config.get_show_x_label() ? MenuItem::State::Checked : MenuItem::State::Null,
				[this]() {
					global::config.set_show_x_label(!global::config.get_show_x_label());
					if (p_webview_) p_webview_->send_command(MessageCommand::UpdateAxisLabelVisibility);
				}
			},
			MenuItem{
				global::string_table[StringId::MenuGraphShowYLabel],
				MenuItem::Type::String,
				global::config.get_show_y_label() ? MenuItem::State::Checked : MenuItem::State::Null,
				[this]() {
					global::config.set_show_y_label(!global::config.get_show_y_label());
					if (p_webview_) p_webview_->send_command(MessageCommand::UpdateAxisLabelVisibility);
				}
			},
			MenuItem{
				global::string_table[StringId::MenuGraphShowHandle],
				MenuItem::Type::String,
				global::config.get_show_handle() ? MenuItem::State::Checked : MenuItem::State::Null,
				[this]() {
					global::config.set_show_handle(!global::config.get_show_handle());
					if (p_webview_) p_webview_->send_command(MessageCommand::UpdateHandleVisibility);
				}
			},
			MenuItem{
				global::string_table[StringId::MenuGraphShowVelocityGraph],
				MenuItem::Type::String,
				global::config.get_show_velocity_graph() ? MenuItem::State::Checked : MenuItem::State::Null,
				[this]() {
					global::config.set_show_velocity_graph(!global::config.get_show_velocity_graph());
					if (p_webview_) p_webview_->send_command(MessageCommand::UpdateVelocityGraphVisibility);
				}
			},
		}.show(hwnd_);
	}


	/// <summary>
	/// セグメントのコンテキストメニューを表示する関数
	/// </summary>
	/// <param name="options">オプションが格納されたjsonオブジェクト</param>
	void MessageHandler::context_menu_segment(const nlohmann::json& options) {
		auto id = options.at("curveId").get<uint32_t>();
		auto segment_id = options.at("segmentId").get<uint32_t>();

		auto curve = global::id_manager.get_curve<NormalCurve>(id);
		auto segment = global::id_manager.get_curve<GraphCurve>(segment_id);
		if (!curve or !segment) {
			return;
		}

		ContextMenu{
			MenuItem{
				global::string_table[StringId::MenuCurveSegmentType],
				MenuItem::Type::String,
				MenuItem::State::Null,
				nullptr,
				{
					MenuItem{
						global::string_table[StringId::LabelCurveSegmentTypeLinear],
						MenuItem::Type::RadioCheck,
						segment->get_name() == global::CURVE_NAME_LINEAR ? MenuItem::State::Checked : MenuItem::State::Null,
						[this, curve, segment_id]() {
							curve->replace_curve(segment_id, CurveSegmentType::Linear);
							if (p_webview_) p_webview_->send_command(MessageCommand::UpdateControl);
						}
					},
					MenuItem{
						global::string_table[StringId::LabelEditModeBezier],
						MenuItem::Type::RadioCheck,
						segment->get_name() == global::CURVE_NAME_BEZIER ? MenuItem::State::Checked : MenuItem::State::Null,
						[this, curve, segment_id]() {
							curve->replace_curve(segment_id, CurveSegmentType::Bezier);
							if (p_webview_) p_webview_->send_command(MessageCommand::UpdateControl);
						}
					},
					MenuItem{
						global::string_table[StringId::LabelEditModeElastic],
						MenuItem::Type::RadioCheck,
						segment->get_name() == global::CURVE_NAME_ELASTIC ? MenuItem::State::Checked : MenuItem::State::Null,
						[this, curve, segment_id]() {
							curve->replace_curve(segment_id, CurveSegmentType::Elastic);
							if (p_webview_) p_webview_->send_command(MessageCommand::UpdateControl);
						}
					},
					MenuItem{
						global::string_table[StringId::LabelEditModeBounce],
						MenuItem::Type::RadioCheck,
						segment->get_name() == global::CURVE_NAME_BOUNCE ? MenuItem::State::Checked : MenuItem::State::Null,
						[this, curve, segment_id]() {
							curve->replace_curve(segment_id, CurveSegmentType::Bounce);
							if (p_webview_) p_webview_->send_command(MessageCommand::UpdateControl);
						}
					}
				},
			},
			MenuItem{
				global::string_table[StringId::MenuCurveSegmentReverse],
				MenuItem::Type::String,
				MenuItem::State::Null,
				[this, curve, segment_id]() {
					curve->reverse_segment(segment_id);
					if (p_webview_) p_webview_->send_command(MessageCommand::UpdateHandlePosition);
				}
			},
			MenuItem{
				global::string_table[StringId::MenuCurveSegmentModifier],
				MenuItem::Type::String,
				MenuItem::State::Null,
				[this, segment_id]() {
					ModifierDialog dialog;
					dialog.show(hwnd_, static_cast<LPARAM>(segment_id));
				}
			}
		}.show(hwnd_);
	}


	/// <summary>
	/// ベジェのハンドルのコンテキストメニューを表示する関数
	/// </summary>
	/// <param name="options">オプションが格納されたjsonオブジェクト</param>
	void MessageHandler::context_menu_bezier_handle(const nlohmann::json& options) {
		auto id = options.at("curveId").get<uint32_t>();
		auto type_str = options.at("handleType").get<std::string>();
		auto curve = global::id_manager.get_curve<BezierCurve>(id);
		if (!curve) {
			return;
		}
		bool has_adjacent = (type_str == "left" and curve->prev() != nullptr)
			or (type_str == "right" and curve->next() != nullptr);

		ContextMenu{
			MenuItem{
				global::string_table[StringId::MenuBezierHandleRoot],
				MenuItem::Type::String,
				MenuItem::State::Null,
				[this, curve, type_str]() {
					if (type_str == "left") {
						curve->set_handle_left(curve->get_anchor_start_x(), curve->get_anchor_start_y());
					}
					else {
						curve->set_handle_right(curve->get_anchor_end_x(), curve->get_anchor_end_y());
					}
					if (p_webview_) p_webview_->send_command(MessageCommand::UpdateHandlePosition);
				}
			},
			MenuItem{
				global::string_table[StringId::MenuBezierHandleAdjustAngle],
				MenuItem::Type::String,
				has_adjacent ? MenuItem::State::Null : MenuItem::State::Disabled,
				[]() {
					// TODO: Implement adjust angle
				}
			}
		}.show(hwnd_);
	}


	/// <summary>
	/// プリセットアイテムのコンテキストメニューを表示する関数
	/// </summary>
	/// <param name="options">オプションが格納されたjsonオブジェクト</param>
	void MessageHandler::context_menu_preset_item(const nlohmann::json& options) {
		auto curve_id = options.at("curveId").get<uint32_t>();

		ContextMenu{
			MenuItem{
				global::string_table[StringId::MenuPresetItemRename],
				MenuItem::Type::String,
				global::preset_manager.is_preset_default(curve_id) ? MenuItem::State::Disabled : MenuItem::State::Null,
				[this, curve_id]() {
					util::input_box(
						hwnd_,
						global::string_table[StringId::PromptRenamePreset],
						global::string_table[StringId::CaptionRenamePreset],
						[this, curve_id](HWND hwnd, const std::string& text) -> bool {
							if (text.empty()) {
								util::message_box(global::string_table[StringId::ErrorInvalidInput], hwnd, util::MessageBoxIcon::Error);
								return false;
							}
							global::preset_manager.rename_preset(curve_id, text);
							if (p_webview_) p_webview_->send_command(MessageCommand::UpdatePresets);
							return true;
						},
						global::preset_manager.get_preset_name(curve_id).c_str()
					);
				}
			},
			MenuItem{
				global::string_table[StringId::MenuPresetItemRemove],
				MenuItem::Type::String,
				global::preset_manager.is_preset_default(curve_id) ? MenuItem::State::Disabled : MenuItem::State::Null,
				[this, curve_id]() {
					int response = IDOK;
					if (global::config.get_show_popup()) {
						response = util::message_box(
							global::string_table[StringId::WarningRemovePreset],
							hwnd_,
							util::MessageBoxIcon::Warning,
							util::MessageBoxButton::OkCancel
						);
					}

					if (response == IDOK) {
						global::preset_manager.remove_preset(curve_id);
						if (p_webview_) p_webview_->send_command(MessageCommand::UpdatePresets);
					}
				}
			}
		}.show(hwnd_);
	}


	/// <summary>
	/// インデックスのコンテキストメニューを表示する関数
	/// </summary>
	void MessageHandler::context_menu_idx() {
		ContextMenu{
			MenuItem{
				global::string_table[StringId::MenuIdxJumpToFirst],
				MenuItem::Type::String,
				global::editor.is_idx_first() ? MenuItem::State::Disabled : MenuItem::State::Null,
				[this]() {
					global::editor.set_idx(0);
					if (p_webview_) p_webview_->send_command(MessageCommand::UpdateEditor);
				}
			},
			MenuItem{
				global::string_table[StringId::MenuIdxJumpToLast],
				MenuItem::Type::String,
				global::editor.is_idx_last() ? MenuItem::State::Disabled : MenuItem::State::Null,
				[this]() {
					global::editor.jump_to_last_idx();
					if (p_webview_) p_webview_->send_command(MessageCommand::UpdateEditor);
				}
			},
			MenuItem{"", MenuItem::Type::Separator},
			MenuItem{
				global::string_table[StringId::MenuIdxDelete],
				MenuItem::Type::String,
				global::editor.is_idx_last() and !global::editor.is_idx_first() ? MenuItem::State::Null : MenuItem::State::Disabled,
				[this]() {
					int response = IDOK;
					if (global::config.get_show_popup()) {
						response = util::message_box(
							global::string_table[StringId::WarningDeleteId],
							hwnd_,
							util::MessageBoxIcon::Warning,
							util::MessageBoxButton::OkCancel
						);
					}

					if (response == IDOK) {
						global::editor.pop_idx();
						if (p_webview_) p_webview_->send_command(MessageCommand::UpdateEditor);
					}
				}
			},
			MenuItem{
				global::string_table[StringId::MenuIdxDeleteAll],
				MenuItem::Type::String,
				MenuItem::State::Null,
				[this]() {
					/*int response = IDOK;
					if (global::config.get_show_popup()) {
						response = util::message_box(
							global::string_table[StringId::WarningDeleteAllId],
							hwnd_,
							util::MessageBoxIcon::Warning,
							util::MessageBoxButton::OkCancel
						);
					}

					if (response == IDOK) {
						global::editor.clear_idx();
						if (p_webview_) p_webview_->send_command(MessageCommand::UpdateEditor);
					}*/
				}
			}
		}.show(hwnd_);
	}


	/// <summary>
	/// コレクション追加ボタンが押されたときに呼び出される関数
	/// </summary>
	void MessageHandler::button_collection_add() {
		ContextMenu{
			MenuItem{
				global::string_table[StringId::MenuCollectionAddNew],
				MenuItem::Type::String,
				MenuItem::State::Null,
				[this]() {
					util::input_box(
						hwnd_,
						global::string_table[StringId::PromptCreateCollection],
						global::string_table[StringId::CaptionCreateCollection],
						[this](HWND hwnd, const std::string& text) -> bool {
							if (text.empty()) {
								util::message_box(global::string_table[StringId::ErrorInvalidInput], hwnd, util::MessageBoxIcon::Error);
								return false;
							}
							global::preset_manager.create_collection(text);
							if (p_webview_) p_webview_->send_command(MessageCommand::UpdatePresets);
							return true;
						}
					);
				}
			},
			MenuItem{
				global::string_table[StringId::MenuCollectionAddImport],
				MenuItem::Type::String,
				MenuItem::State::Null,
				[this]() {
					using namespace std::literals::string_view_literals;
					std::string str_filter = std::format(
						"{0} (*.{1})\0*.{1}\0"sv,
						global::string_table[StringId::WordCollectionFile],
						EXT_COLLECTION
					);
					char buffer[MAX_PATH + 1]{};
					OPENFILENAMEA ofn{
						.lStructSize = sizeof(OPENFILENAMEA),
						.hwndOwner = hwnd_,
						.lpstrFilter = str_filter.c_str(),
						.lpstrFile = buffer,
						.nMaxFile = MAX_PATH + 1,
						.lpstrTitle = global::string_table[StringId::CaptionImportCollection],
						.Flags = OFN_FILEMUSTEXIST
					};
					if (::GetOpenFileNameA(&ofn)) {
						global::preset_manager.import_collection(buffer);
						if (p_webview_) p_webview_->send_command(MessageCommand::UpdatePresets);
					}
				}
			}
		}.show(hwnd_);
	}


	/// <summary>
	/// コレクション管理ボタンが押されたときに呼び出される関数
	/// </summary>
	void MessageHandler::button_collection() {
		ContextMenu{
			MenuItem{
				global::string_table[StringId::MenuCollectionRename],
				MenuItem::Type::String,
				global::preset_manager.is_collection_custom() ? MenuItem::State::Null : MenuItem::State::Disabled,
				[this]() {
					util::input_box(
						hwnd_,
						global::string_table[StringId::PromptRenameCollection],
						global::string_table[StringId::CaptionRenameCollection],
						[this](HWND hwnd, const std::string& text) -> bool {
							if (text.empty()) {
								util::message_box(global::string_table[StringId::ErrorInvalidInput], hwnd, util::MessageBoxIcon::Error);
								return false;
							}
							global::preset_manager.rename_collection(global::preset_manager.get_current_collection_id(), text);
							if (p_webview_) p_webview_->send_command(MessageCommand::UpdatePresets);
							return true;
						},
						global::preset_manager.get_collection_info().at(global::preset_manager.get_current_collection_id()).second.c_str()
					);
				}
			},
			MenuItem{
				global::string_table[StringId::MenuCollectionRemove],
				MenuItem::Type::String,
				global::preset_manager.is_collection_custom() ? MenuItem::State::Null : MenuItem::State::Disabled,
				[this]() {
					int response = IDOK;
					if (global::config.get_show_popup()) {
						response = util::message_box(
							global::string_table[StringId::WarningRemoveCollection],
							hwnd_,
							util::MessageBoxIcon::Warning,
							util::MessageBoxButton::OkCancel
						);
					}

					if (response == IDOK) {
						global::preset_manager.remove_collection(global::preset_manager.get_current_collection_id());
						if (p_webview_) p_webview_->send_command(MessageCommand::UpdatePresets);
					}
				}
			},
			MenuItem{
				global::string_table[StringId::MenuCollectionExport],
				MenuItem::Type::String,
				MenuItem::State::Null,
				[this]() {
					using namespace std::literals::string_view_literals;
					std::string str_filter = std::format(
						"{0} (*.{1})\0*.{1}\0"sv,
						global::string_table[StringId::WordCollectionFile],
						EXT_COLLECTION
					);
					char buffer[MAX_PATH + 1]{};
					OPENFILENAMEA ofn{
						.lStructSize = sizeof(OPENFILENAMEA),
						.hwndOwner = hwnd_,
						.lpstrFilter = str_filter.c_str(),
						.lpstrFile = buffer,
						.nMaxFile = MAX_PATH + 1,
						.lpstrTitle = global::string_table[StringId::CaptionExportCollection],
						.Flags = OFN_OVERWRITEPROMPT,
						.lpstrDefExt = EXT_COLLECTION
					};
					if (::GetSaveFileNameA(&ofn)) {
						global::preset_manager.export_collection(global::preset_manager.get_current_collection_id(), buffer);
					}
				}
			}
		}.show(hwnd_);
	}


	/// <summary>
	/// フィルタボタンが押されたときに呼び出される関数
	/// </summary>
	/// <param name="options"></param>
	void MessageHandler::button_preset_filter(const nlohmann::json& options) {
		
	}


	/// <summary>
	/// 並び替えボタンが押されたときに呼び出される関数
	/// </summary>
	/// <param name="options"></param>
	void MessageHandler::button_preset_sort(const nlohmann::json& options) {
		
	}


	/// <summary>
	/// カーブ選択ダイアログでOKボタンが押されたときに呼び出される関数
	/// </summary>
	/// <param name="options">オプションが格納されたjsonオブジェクト</param>
	void MessageHandler::select_curve_ok(const nlohmann::json& options) {
		auto mode = options.at("mode").get<EditMode>();
		auto param = options.at("param").get<int32_t>();
		std::pair<std::string, int> ret = std::make_pair(global::editor.get_curve(mode)->get_name(), param);
		::SendMessageA(hwnd_, WM_COMMAND, (WPARAM)WindowCommand::SelectCurveOk, std::bit_cast<LPARAM>(&ret));
	}


	/// <summary>
	/// カーブ選択ダイアログでキャンセルボタンが押されたときに呼び出される関数
	/// </summary>
	void MessageHandler::select_curve_cancel() {
		::PostMessageA(hwnd_, WM_COMMAND, (WPARAM)WindowCommand::SelectCurveCancel, 0);
	}


	/// <summary>
	/// D&Dが開始されたときに呼び出される関数
	/// </summary>
	/// <param name="options">オプションが格納されたjsonオブジェクト</param>
	void MessageHandler::on_dnd_start(const nlohmann::json& options) {
		uint32_t curve_id = 0;
		if (options.contains("curveId")) {
			curve_id = options.at("curveId").get<uint32_t>();
		}
		::SendMessageA(hwnd_, WM_COMMAND, (WPARAM)WindowCommand::StartDnd, static_cast<LPARAM>(curve_id));
	}


	/// <summary>
	/// ハンドルのドラッグが終了したときに呼び出される関数
	/// </summary>
	/// <param name="options">オプションが格納されたjsonオブジェクト</param>
	void MessageHandler::on_handle_drag_end(const nlohmann::json& options) {
		auto curve_id = options.at("curveId").get<uint32_t>();
		auto curve_numeric = global::id_manager.get_curve<NumericGraphCurve>(curve_id);
		if (curve_numeric) {
			if (global::config.get_auto_copy()) {
				auto code = std::to_string(curve_numeric->encode());
				if (!util::copy_to_clipboard(hwnd_, code.c_str()) and global::config.get_show_popup()) {
					util::message_box(global::string_table[StringId::ErrorCodeCopyFailed], hwnd_, util::MessageBoxIcon::Error);
				}
			}
		}
		else {
			if (global::config.get_auto_apply()) {
				::SendMessageA(hwnd_, WM_COMMAND, (WPARAM)WindowCommand::RedrawAviutl, 0);
			}
		}
	}


	/// <summary>
	/// プリセットをエディタに適用する関数
	/// </summary>
	/// <param name="options">オプションが格納されたjsonオブジェクト</param>
	void MessageHandler::apply_preset(const nlohmann::json& options) {
		auto id = options.at("curveId").get<uint32_t>();
		auto curve_normal = global::id_manager.get_curve<NormalCurve>(id);
		auto curve_value = global::id_manager.get_curve<ValueCurve>(id);
		auto curve_bezier = global::id_manager.get_curve<BezierCurve>(id);
		auto curve_elastic = global::id_manager.get_curve<ElasticCurve>(id);
		auto curve_bounce = global::id_manager.get_curve<BounceCurve>(id);
		auto curve_script = global::id_manager.get_curve<ScriptCurve>(id);
		EditMode new_mode = EditMode::Normal;
		if (curve_normal) {
			new_mode = EditMode::Normal;
		}
		else if (curve_value) {
			new_mode = EditMode::Value;
		}
		else if (curve_bezier) {
			new_mode = EditMode::Bezier;
		}
		else if (curve_elastic) {
			new_mode = EditMode::Elastic;
		}
		else if (curve_bounce) {
			new_mode = EditMode::Bounce;
		}
		else if (curve_script) {
			new_mode = EditMode::Script;
		}
		bool animate = false;
		if (global::config.get_edit_mode() != new_mode) {
			global::config.set_edit_mode(new_mode);
			if (p_webview_) p_webview_->send_command(MessageCommand::ChangeEditMode);
		}
		else {
			animate = true;
		}
		switch (new_mode) {
		case EditMode::Normal:
			if (curve_normal) {
				global::editor.editor_graph().append_curve_normal(*curve_normal);
				global::editor.editor_graph().jump_to_last_idx_normal();
				if (p_webview_) p_webview_->send_command(MessageCommand::UpdateEditor);
			}
			break;

		case EditMode::Value:
			if (curve_value) {
				global::editor.editor_graph().append_curve_value(*curve_value);
				global::editor.editor_graph().jump_to_last_idx_value();
				if (p_webview_) p_webview_->send_command(MessageCommand::UpdateEditor);
			}
			break;

		case EditMode::Script:
			if (curve_script) {
				global::editor.editor_script().append_curve(*curve_script);
				global::editor.editor_script().jump_to_last_idx();
				if (p_webview_) p_webview_->send_command(MessageCommand::UpdateEditor);
			}
			break;

		case EditMode::Bezier:
		case EditMode::Elastic:
		case EditMode::Bounce:
			// TODO: コードを渡しているから若干劣化してしまう
			if (curve_bezier) {
				global::editor.editor_graph().curve_bezier()->decode(curve_bezier->encode());
			}
			else if (curve_elastic) {
				global::editor.editor_graph().curve_elastic()->decode(curve_elastic->encode());
			}
			else if (curve_bounce) {
				global::editor.editor_graph().curve_bounce()->decode(curve_bounce->encode());
			}
			if (animate) {
				if (p_webview_) p_webview_->send_command(MessageCommand::UpdateHandlePosition);
			}
			else {
				if (p_webview_) p_webview_->send_command(MessageCommand::UpdateControl);
			}
		}
	}
} // namespace cved