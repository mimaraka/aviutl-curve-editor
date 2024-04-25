#include "wndproc_grapheditor.hpp"
#include <windowsx.h>
#include <mkaul/include/point.hpp>
#include "config.hpp"
#include "curve_editor.hpp"
#include "drag_and_drop.hpp"
#include "global.hpp"
#include "menu_graph.hpp"
#include "menu_curve.hpp"
#include "menu_bezier_handle.hpp"
#include "my_messagebox.hpp"
#include "string_table.hpp"
#include "resource.h"



namespace cved {
	LRESULT CALLBACK wndproc_grapheditor(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
	{
		using StringId = global::StringTable::StringId;

		constexpr float CURVE_THICKNESS_VELOCITY = 1.f;
		constexpr float HANDLE_THICKNESS = 2.f;
		constexpr float POINT_RADIUS = 4.4f;
		constexpr float HANDLE_TIP_RADIUS = 5.f;
		constexpr float HANDLE_TIP_THICKNESS = 2.2f;
		constexpr float BRIGHTNESS_GRAPH_INVALID = -0.04f;

		static const auto hinst = reinterpret_cast<HINSTANCE>(::GetWindowLongPtrA(hwnd, GWLP_HINSTANCE));

		static DragAndDrop dnd;

		static GraphView view;
		static GraphMenu menu_graph{ hinst };
		static CurveMenu menu_curve{ hinst };
		static BezierHandleMenu menu_bezier_handle{ hinst };

		auto& editor = global::editor.editor_graph();
		const auto& config = global::config;

		RECT rect_tmp;
		::GetClientRect(hwnd, &rect_tmp);
		mkaul::WindowRectangle rect_wnd{ rect_tmp };


		// pointer
		using namespace mkaul::graphics;
		static std::unique_ptr<Graphics> p_graphics = Factory::create_graphics();
		static std::unique_ptr<Bitmap> p_bitmap_bg = nullptr;

		mkaul::Point<long> pt_client = {
			GET_X_LPARAM(lparam),
			GET_Y_LPARAM(lparam)
		};

		auto ptf_client = pt_client.to<mkaul::Point<float>>();
		auto pt_view = view.client_to_view(ptf_client, rect_wnd);

		switch (message) {
			// ウィンドウ初期化時
		case WM_CREATE:
			if (!p_graphics->init(hwnd)) {
				my_messagebox(global::string_table[StringId::ErrorGraphicsInitFailed], hwnd, MessageBoxIcon::Error);
				::DestroyWindow(hwnd);
				return 0;
			}
			if (config.get_set_bg_image()) {
				p_bitmap_bg = p_graphics->load_bitmap_from_filename(config.get_bg_image_path());
			}

			dnd.init();
			view.fit(rect_wnd);
			return 0;

			// ウィンドウ終了・破棄時
		case WM_CLOSE:
		case WM_DESTROY:
			p_graphics->release();
			if (p_bitmap_bg != nullptr) {
				p_bitmap_bg->release();
			}
			dnd.exit();
			return 0;

		case WM_SIZE:
			p_graphics->resize();
			return 0;

			// ウィンドウ描画時
		case WM_PAINT:
		{
			float left = view.view_to_client_x(0., rect_wnd);
			float right = view.view_to_client_x(1., rect_wnd);
			auto color_overlay = config.get_theme().bg_graph;

			p_graphics->begin_draw();
			p_graphics->fill_background(config.get_theme().bg_graph);

			// 背景画像の描画
			if (config.get_set_bg_image() and p_bitmap_bg != nullptr) {
				double aspect_wnd = (double)rect_wnd.get_width() / (double)rect_wnd.get_height();
				double aspect_bitmap = (double)p_bitmap_bg->get_width() / (double)p_bitmap_bg->get_height();
				mkaul::Rectangle<float> rect_f;
				// ビットマップの方が横長
				if (aspect_wnd < aspect_bitmap) {
					float scale = (float)rect_wnd.get_height() / (float)p_bitmap_bg->get_height();
					// 上下の辺を合わせる
					rect_f = mkaul::Rectangle{
						(rect_wnd.get_width() - (float)p_bitmap_bg->get_width() * scale) * 0.5f,
						0.f,
						(rect_wnd.get_width() + (float)p_bitmap_bg->get_width() * scale) * 0.5f,
						(float)rect_wnd.get_height()
					};
				}
				// ウィンドウの方が横長
				else {
					float scale = (float)rect_wnd.get_width() / (float)p_bitmap_bg->get_width();
					// 左右の辺を合わせる
					rect_f = mkaul::Rectangle{
						0.f,
						(rect_wnd.get_height() - (float)p_bitmap_bg->get_height() * scale) * 0.5f,
						(float)rect_wnd.get_width(),
						(rect_wnd.get_height() + (float)p_bitmap_bg->get_height() * scale) * 0.5f
					};
				}
				p_graphics->draw_bitmap(p_bitmap_bg.get(), rect_f, config.get_bg_image_opacity());
			}

			view.draw_grid(p_graphics.get(), rect_wnd);

			color_overlay.change_brightness(BRIGHTNESS_GRAPH_INVALID);
			color_overlay.set_a(0.5f);

			if (0 < left) {
				p_graphics->fill_rectangle(
					mkaul::Rectangle{ 0.f, 0.f, left, (float)rect_wnd.bottom },
					0.f, 0.f, color_overlay
				);
			}
			if (right < rect_wnd.right) {
				p_graphics->fill_rectangle(
					mkaul::Rectangle{ right, 0.f, (float)rect_wnd.right, (float)rect_wnd.bottom },
					0.f, 0.f, color_overlay
				);
			}

			// 速度グラフの描画
			if (config.get_show_velocity_graph()) {
				editor.current_curve()->draw_curve(
					p_graphics.get(),
					view,
					CURVE_THICKNESS_VELOCITY,
					config.get_curve_drawing_interval(),
					config.get_theme().curve_velocity,
					true
				);
			}
			// カーブの描画
			editor.current_curve()->draw_curve(
				p_graphics.get(),
				view,
				config.get_curve_thickness(),
				config.get_curve_drawing_interval(),
				config.get_curve_color()
			);
			// ハンドル・ポイントの描画
			if (config.get_show_handle()) {
				editor.current_curve()->draw_handle(
					p_graphics.get(),
					view,
					HANDLE_THICKNESS,
					POINT_RADIUS,
					HANDLE_TIP_RADIUS,
					HANDLE_TIP_THICKNESS,
					true,
					config.get_theme().handle
				);
				editor.current_curve()->draw_pt(p_graphics.get(), view, POINT_RADIUS, config.get_theme().handle);
			}
			p_graphics->end_draw();
			return 0;
		}

		// マウスの左ボタン押下時
		case WM_LBUTTONDOWN:
			// ハンドルにホバーしていた時
			if (
				editor.current_curve()->handle_check_hover(pt_view, view)
				or editor.current_curve()->pt_check_hover(pt_view, view) != GraphCurve::ActivePoint::Null
				) {
				::SetCursor(::LoadCursorA(NULL, IDC_HAND));
				::SetCapture(hwnd);
			}
			// Altキーを押していた場合、ビューの移動・拡大を行う
			else if (::GetAsyncKeyState(VK_MENU) & 0x8000) {
				if (::GetAsyncKeyState(VK_CONTROL) & 0x8000) {
					view.begin_scale(ptf_client);
				}
				else {
					view.begin_move(ptf_client);
				}
				::SetCursor(::LoadCursorA(NULL, IDC_SIZEALL));
				::SetCapture(hwnd);
			}
			// D&D
			else {
				if (dnd.drag()) {
					::SetCapture(hwnd);
				}
			}
			return 0;

			// マウスの左ボタンリリース時
		case WM_LBUTTONUP:
			// D&Dの処理
			dnd.drop();

			editor.current_curve()->handle_end_control();
			editor.current_curve()->pt_end_control();

			// Auto Copy
			if (global::config.get_auto_copy() and editor.numeric_curve()) {
				global::window_toolbar.send_command((WPARAM)WindowCommand::Copy);
			}

			// Auto Apply
			if (global::config.get_auto_apply() and !editor.numeric_curve()) {
				::SendMessageA(::GetParent(global::window_main.get_hwnd()), WM_COMMAND, (WPARAM)WindowCommand::RedrawAviutl, NULL);
			}

			// パラメータボタンの更新
			if (editor.numeric_curve()) {
				global::window_toolbar.send_command((WPARAM)WindowCommand::UpdateParamPanel);
			}

			view.end_move();
			view.end_scale();
			::ReleaseCapture();
			return 0;

			// マウスの左ボタンダブルクリック時
		case WM_LBUTTONDBLCLK:
			// 現在のカーブがNormalCurveのとき
			if (config.get_edit_mode() == EditMode::Normal) {
				if (editor.curve_normal()->delete_curve(pt_view, view)
					or editor.curve_normal()->add_curve(pt_view, view)
					) {
					editor.current_curve()->pt_check_hover(pt_view, view);
					::SetCursor(::LoadCursorA(NULL, IDC_HAND));
					::InvalidateRect(hwnd, NULL, FALSE);
				}
			}
			return 0;

			// マウスの中央ボタン押下時
		case WM_MBUTTONDOWN:
			if (::GetAsyncKeyState(VK_CONTROL) & 0x8000) {
				view.begin_scale(ptf_client);
			}
			else {
				view.begin_move(ptf_client);
			}
			::SetCursor(::LoadCursorA(NULL, IDC_SIZEALL));
			::SetCapture(hwnd);
			return 0;

			// マウスの中央ボタンリリース時
		case WM_MBUTTONUP:
			view.end_move();
			view.end_scale();
			::ReleaseCapture();
			return 0;

			// マウスカーソル移動時
		case WM_MOUSEMOVE:
			// ハンドルホバー時のカーソル変更
			if (editor.current_curve()->is_hovered(pt_view, view)) {
				::SetCursor(::LoadCursorA(NULL, IDC_HAND));
			}

			// ハンドルアップデート時に再描画
			if (
				editor.current_curve()->handle_update(pt_view, view)
				or editor.current_curve()->pt_update(pt_view, view) != GraphCurve::ActivePoint::Null
				) {
				::InvalidateRect(hwnd, NULL, FALSE);
			}

			// ビュー移動時に再描画
			if (view.move(ptf_client, rect_wnd) or view.scale(ptf_client)) {
				::SetCursor(::LoadCursorA(NULL, IDC_SIZEALL));
				::InvalidateRect(hwnd, NULL, FALSE);
			}

			// ドラッグ時
			if (dnd.is_dragging()) {
				// カーソルを変更
				if (!rect_wnd.pt_in_rect(pt_client)) {
					::SetCursor(::LoadCursorA(hinst, MAKEINTRESOURCEA(IDC_DRAG)));
				}
				// カーソルホバー時にトラックバーのボタンをハイライト
				dnd.highlight();
			}
			return 0;

			// マウスホイール回転時
		case WM_MOUSEWHEEL:
		{
			double scroll_amount = (double)GET_Y_LPARAM(wparam);
			if (config.get_reverse_wheel()) {
				scroll_amount *= -1.;
			}
			// Ctrlキーが押されているとき(横に移動)
			if (::GetAsyncKeyState(VK_CONTROL) & 0x8000 and not (GetAsyncKeyState(VK_SHIFT) & 0x8000)) {
				view.move(scroll_amount, false);
			}
			// Shiftキーが押されているとき(縦に移動)
			else if (::GetAsyncKeyState(VK_SHIFT) < 0 and GetAsyncKeyState(VK_CONTROL) >= 0) {
				view.move(scroll_amount, true);
			}
			// 縮尺の上限下限を設定
			else {
				view.scale(scroll_amount);
			}
			// 再描画
			::InvalidateRect(hwnd, NULL, FALSE);
			return 0;
		}

		// コンテクストメニュー展開時
		case WM_RBUTTONDOWN:
			// 標準・値指定モードのとき
			// TODO: 値指定モードに対応させる
			if (config.get_edit_mode() == EditMode::Normal) {
				auto p_curve_normal = editor.curve_normal();
				for (auto it = p_curve_normal->get_curve_segments().begin(), end = p_curve_normal->get_curve_segments().end(); it != end; it++) {
					// ベジェのハンドルにホバーしていた場合
					if (typeid(**it) == typeid(BezierCurve) and (*it)->is_handle_hovered(pt_view, view)) {
						auto p_curve_bezier = dynamic_cast<BezierCurve*>(it->get());
						if (p_curve_bezier->handle_left()->is_hovered(pt_view, view)) {
							menu_bezier_handle.show(p_curve_bezier->handle_left(), view, hwnd);
						}
						else if (p_curve_bezier->handle_right()->is_hovered(pt_view, view)) {
							menu_bezier_handle.show(p_curve_bezier->handle_right(), view, hwnd);
						}
						return 0;
					}
					// カーブの始点にホバーしていた場合
					if ((*it)->pt_start().is_hovered(pt_view, view)) {
						size_t idx = std::distance(p_curve_normal->get_curve_segments().begin(), it);
						menu_curve.show(idx, hwnd);
						return 0;
					}
				}
			}
			menu_graph.show(hwnd);
			return 0;

			// コマンド
		case WM_COMMAND:
			switch (wparam) {
			case (UINT)WindowCommand::Update:
				::InvalidateRect(hwnd, NULL, FALSE);
				break;

			case (UINT)WindowCommand::Fit:
				view.fit(rect_wnd);
				::InvalidateRect(hwnd, NULL, FALSE);
				break;

			case (UINT)WindowCommand::Reverse:
			{
				auto curve = editor.current_curve();
				if (curve) {
					curve->reverse();
					global::window_main.send_command((WPARAM)WindowCommand::Update);
				}
			}

			// 背景画像の設定
			case (UINT)WindowCommand::SetBackgroundImage:
				// 新しいビットマップの割り当て/古いビットマップの解放
				p_bitmap_bg = p_graphics->load_bitmap_from_filename(config.get_bg_image_path());
				break;
			}
			return 0;
		}

		return ::DefWindowProc(hwnd, message, wparam, lparam);
	}
}