//----------------------------------------------------------------------------------
//		Curve Editor
//		�\�[�X�t�@�C��(�R���g���[��)
//		Visual C++ 2022
//----------------------------------------------------------------------------------

#include "cve_header.hpp"



//---------------------------------------------------------------------
//		�R���g���[�����쐬
//---------------------------------------------------------------------
BOOL cve::Button::initialize(
	HWND			hwnd_p,
	LPCTSTR			name,
	LPCTSTR			desc,
	Content_Type	cont_type,
	LPCTSTR			ico_res_dark,
	LPCTSTR			ico_res_light,
	LPCTSTR			lb,
	int				ct_id,
	const RECT&		rect,
	int				flag
)
{
	id = ct_id;
	description = (LPTSTR)desc;
	edge_flag = flag;
	hwnd_parent = hwnd_p;

	content_type = cont_type;

	if (content_type == Button::String) {
		if (lb != nullptr)
			strcpy_s(label, CVE_CT_LABEL_MAX, lb);
		else
			label[0] = NULL;
	}
	else if (content_type == Button::Icon) {
		icon_res_dark = (LPTSTR)ico_res_dark;
		icon_res_light = (LPTSTR)ico_res_light;
	}

	return create(
		hwnd_p,
		name,
		wndproc_static,
		NULL,
		NULL,
		rect,
		this
	);
}



//---------------------------------------------------------------------
//		�R���g���[���`��p�̊֐�
//---------------------------------------------------------------------
void cve::Button::draw_content(COLORREF bg, RECT* rect_content, LPCTSTR content, bool change_color)
{
	if (change_color) {
		if (clicked)
			bg = CHANGE_BRIGHTNESS(bg, CVE_CT_BR_CLICKED);
		else if (hovered)
			bg = CHANGE_BRIGHTNESS(bg, CVE_CT_BR_HOVERED);
	}

	bitmap_buffer.d2d_setup(TO_BGR(bg));

	::SetBkColor(bitmap_buffer.hdc_memory, bg);

	// �������`��
	if (content_type == Button::String && content != nullptr) {
		::SelectObject(bitmap_buffer.hdc_memory, font);

		::DrawText(
			bitmap_buffer.hdc_memory,
			content,
			strlen(content),
			rect_content,
			DT_CENTER | DT_VCENTER | DT_NOCLIP | DT_SINGLELINE
		);

		::DeleteObject(font);
	}
	// �A�C�R����`��
	else if (content_type == Button::Icon) {
		::DrawIcon(
			bitmap_buffer.hdc_memory,
			(rect_content->right - CVE_ICON_SIZE) / 2,
			(rect_content->bottom - CVE_ICON_SIZE) / 2,
			g_config.theme ? icon_light : icon_dark
		);
	}
}



//---------------------------------------------------------------------
//		���E���h�G�b�W��`��
//---------------------------------------------------------------------
void cve::Button::draw_edge()
{
	if (g_render_target != nullptr) {
		g_render_target->BeginDraw();

		bitmap_buffer.draw_rounded_edge(edge_flag, CVE_ROUND_RADIUS);

		g_render_target->EndDraw();
	}
}



//---------------------------------------------------------------------
//		�t�H���g��ݒ�
//---------------------------------------------------------------------
void cve::Button::set_font(int font_height, LPTSTR font_name)
{
	font = ::CreateFont(
		font_height, 0,
		0, 0,
		FW_REGULAR,
		FALSE, FALSE, FALSE,
		SHIFTJIS_CHARSET,
		OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY,
		NULL,
		font_name
	);
}



//---------------------------------------------------------------------
//		�E�B���h�E�v���V�[�W��(static�ϐ��g�p�s��)
//---------------------------------------------------------------------
LRESULT cve::Button::wndproc(HWND hw, UINT msg, WPARAM wparam, LPARAM lparam)
{
	RECT rect_wnd;

	::GetClientRect(hw, &rect_wnd);

	switch (msg) {
	case WM_CREATE:
		bitmap_buffer.init(hw);
		bitmap_buffer.set_size(rect_wnd);

		// �A�C�R���������ꍇ
		if (content_type == Button::Icon) {
			icon_dark = ::LoadIcon(g_fp->dll_hinst, icon_res_dark);
			icon_light = ::LoadIcon(g_fp->dll_hinst, icon_res_light);

			hwnd_tooltip = ::CreateWindowEx(
				0, TOOLTIPS_CLASS,
				NULL, TTS_ALWAYSTIP,
				CW_USEDEFAULT, CW_USEDEFAULT,
				CW_USEDEFAULT, CW_USEDEFAULT,
				hw, NULL, g_fp->dll_hinst,
				NULL
			);

			tool_info.cbSize = sizeof(TOOLINFO);
			tool_info.uFlags = TTF_SUBCLASS;
			tool_info.hwnd = hw;
			tool_info.uId = id;
			tool_info.lpszText = (LPTSTR)description;
			::SendMessage(hwnd_tooltip, TTM_ADDTOOL, 0, (LPARAM)&tool_info);
		}

		tme.cbSize = sizeof(tme);
		tme.dwFlags = TME_LEAVE;
		tme.hwndTrack = hw;

		set_font(CVE_CT_FONT_H, CVE_FONT_SEMIBOLD);
		return 0;

	case WM_CLOSE:
		bitmap_buffer.exit();
		return 0;

	case WM_SIZE:
		bitmap_buffer.set_size(rect_wnd);

		if (content_type == Button::Icon) {
			tool_info.rect = rect_wnd;
			::SendMessage(hwnd_tooltip, TTM_NEWTOOLRECT, 0, (LPARAM)&tool_info);
		}
		return 0;

	// �`��
	case WM_PAINT:
	{
		COLORREF bg = g_theme[g_config.theme].bg;

		::SetTextColor(bitmap_buffer.hdc_memory, g_theme[g_config.theme].bt_tx);

		draw_content(bg, &rect_wnd, label, true);
		draw_edge();
		bitmap_buffer.transfer();

		return 0;
	}

	// �}�E�X���������Ƃ�
	case WM_MOUSEMOVE:
		hovered = true;
		::InvalidateRect(hw, NULL, FALSE);
		::TrackMouseEvent(&tme);
		return 0;

	case WM_SETCURSOR:
		::SetCursor(::LoadCursor(NULL, IDC_HAND));
		return 0;

	// ���N���b�N�����ꂽ�Ƃ�
	case WM_LBUTTONDOWN:
		::SetCursor(::LoadCursor(NULL, IDC_HAND));
		clicked = true;
		::InvalidateRect(hw, NULL, FALSE);
		::TrackMouseEvent(&tme);
		return 0;

	// ���N���b�N���I������Ƃ�
	case WM_LBUTTONUP:
		::SetCursor(::LoadCursor(NULL, IDC_HAND));
		clicked = false;
		::SendMessage(hwnd_parent, WM_COMMAND, id, 0);
		::InvalidateRect(hw, NULL, FALSE);
		return 0;

	// �}�E�X���E�B���h�E���痣�ꂽ�Ƃ�
	case WM_MOUSELEAVE:
		clicked = false;
		hovered = false;
		::InvalidateRect(hw, NULL, FALSE);
		return 0;

	// �R�}���h
	case WM_COMMAND:
		switch (wparam) {
		case CVE_CM_REDRAW:
			::InvalidateRect(hw, NULL, FALSE);
			return 0;

		case CVE_CM_CHANGE_LABEL:
		{
			int len_src = strlen((LPTSTR)lparam);
			int len_dst = strlen(label);

			strcpy_s(label, CVE_CT_LABEL_MAX, (LPTSTR)lparam);
			::InvalidateRect(hw, NULL, FALSE);
			return 0;
		}
		}
		return 0;

	default:
		return ::DefWindowProc(hw, msg, wparam, lparam);
	}
}



//---------------------------------------------------------------------
//		�E�B���h�E�v���V�[�W��(�X�C�b�`)
//---------------------------------------------------------------------
LRESULT cve::Button_Switch::wndproc(HWND hw, UINT msg, WPARAM wparam, LPARAM lparam)
{
	RECT rect_wnd;

	::GetClientRect(hw, &rect_wnd);

	switch (msg) {
	// �`��
	case WM_PAINT:
	{
		COLORREF bg;
		// �I����
		if (is_selected) {
			if (clicked)
				bg = CHANGE_BRIGHTNESS(g_theme[g_config.theme].bt_selected, CVE_CT_BR_CLICKED);
			else if (hovered)
				bg = CHANGE_BRIGHTNESS(g_theme[g_config.theme].bt_selected, CVE_CT_BR_HOVERED);
			else
				bg = g_theme[g_config.theme].bt_selected;

			::SetTextColor(bitmap_buffer.hdc_memory, g_theme[g_config.theme].bt_tx_selected);
		}
		// ��I����
		else {
			if (clicked)
				bg = CHANGE_BRIGHTNESS(g_theme[g_config.theme].bt_unselected, CVE_CT_BR_CLICKED);
			else if (hovered)
				bg = CHANGE_BRIGHTNESS(g_theme[g_config.theme].bt_unselected, CVE_CT_BR_HOVERED);
			else
				bg = g_theme[g_config.theme].bt_unselected;

			::SetTextColor(bitmap_buffer.hdc_memory, g_theme[g_config.theme].bt_tx);
		}

		draw_content(bg, &rect_wnd, label, false);
		draw_edge();
		bitmap_buffer.transfer();

		return 0;
	}

	// ���N���b�N���I������Ƃ�
	case WM_LBUTTONUP:
		::SetCursor(LoadCursor(NULL, IDC_HAND));
		clicked = false;
		if (!is_selected) {
			is_selected = true;
			::SendMessage(hwnd_parent, WM_COMMAND, id, CVE_CM_SELECTED);
		}
		::InvalidateRect(hw, NULL, FALSE);
		return 0;
	}
	return Button::wndproc(hw, msg, wparam, lparam);
}



//---------------------------------------------------------------------
//		�I����Ԃ�ύX(�X�C�b�`)
//---------------------------------------------------------------------
void cve::Button_Switch::set_status(BOOL bl)
{
	is_selected = bl;
	::InvalidateRect(hwnd, NULL, FALSE);
}



//---------------------------------------------------------------------
//		�E�B���h�E�v���V�[�W��(Value)
//---------------------------------------------------------------------
LRESULT cve::Button_Value::wndproc(HWND hw, UINT msg, WPARAM wparam, LPARAM lparam)
{
	RECT rect_wnd;

	::GetClientRect(hw, &rect_wnd);

	switch (msg) {
	case WM_PAINT:
	{
		COLORREF bg = g_theme[g_config.theme].bg;
		std::string str_value_4d = g_curve_normal.create_parameters();
		LPTSTR value_4d = const_cast<LPTSTR>(str_value_4d.c_str());

		::SetTextColor(bitmap_buffer.hdc_memory, g_theme[g_config.theme].bt_tx);

		draw_content(bg, &rect_wnd, value_4d, true);
		draw_edge();
		bitmap_buffer.transfer();

		return 0;
	}
	}
	return Button::wndproc(hw, msg, wparam, lparam);
}



//---------------------------------------------------------------------
//		�E�B���h�E�v���V�[�W��(ID)
//---------------------------------------------------------------------
LRESULT cve::Button_ID::wndproc(HWND hw, UINT msg, WPARAM wparam, LPARAM lparam)
{
	RECT rect_wnd;
	POINT pt_client = { GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };

	::GetClientRect(hw, &rect_wnd);

	switch (msg) {
	case WM_PAINT:
	{
		COLORREF bg = g_theme[g_config.theme].bg;
		TCHAR id_text[5];

		::_itoa_s(g_config.current_id, id_text, 5, 10);

		::SetTextColor(bitmap_buffer.hdc_memory, g_theme[g_config.theme].bt_tx);

		draw_content(bg, &rect_wnd, id_text, true);
		draw_edge();
		bitmap_buffer.transfer();

		return 0;
	}

	// ���N���b�N�����ꂽ�Ƃ�
	case WM_LBUTTONDOWN:
		pt_lock = pt_client;
		id_buffer = g_config.current_id;

		::SetCursor(LoadCursor(NULL, IDC_HAND));

		if (::GetAsyncKeyState(VK_CONTROL) < 0)
			coef_move = CVE_COEF_MOVE_FAST;
		else
			coef_move = CVE_COEF_MOVE_DEFAULT;

		clicked = true;

		::InvalidateRect(hw, NULL, FALSE);
		::SetCapture(hw);

		return 0;

	// �J�[�\�����������Ƃ�
	case WM_MOUSEMOVE:
		if (clicked && g_config.edit_mode == Mode_Multibezier) {
			is_scrolling = true;

			::SetCursor(LoadCursor(NULL, IDC_SIZEWE));

			g_config.current_id = MINMAX_LIMIT(id_buffer + (pt_client.x - pt_lock.x) / coef_move, 0, CVE_CURVE_MAX - 1);
			g_curve_mb_previous = g_curve_mb[g_config.current_id];

			::SendMessage(g_window_editor.hwnd, WM_COMMAND, CVE_CM_REDRAW, 0);
		}
		else ::SetCursor(::LoadCursor(NULL, IDC_HAND));

		hovered = true;

		::InvalidateRect(hw, NULL, FALSE);
		::TrackMouseEvent(&tme);

		return 0;

	case WM_LBUTTONUP:
		if (!is_scrolling && clicked && g_config.edit_mode == Mode_Multibezier)
			::DialogBox(g_fp->dll_hinst, MAKEINTRESOURCE(IDD_ID), hw, dialogproc_id);
		
		is_scrolling = false;
		clicked = false;

		::ReleaseCapture();
		::InvalidateRect(hw, NULL, FALSE);

		return 0;
	}
	return Button::wndproc(hw, msg, wparam, lparam);
}



//---------------------------------------------------------------------
//		�E�B���h�E�v���V�[�W��(�J�e�S���[)
//---------------------------------------------------------------------
LRESULT cve::Button_Category::wndproc(HWND hw, UINT msg, WPARAM wparam, LPARAM lparam)
{
	RECT rect_wnd;
	POINT pt_client = { GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };

	::GetClientRect(hw, &rect_wnd);

	switch (msg) {
	case WM_CREATE:
	{
		LRESULT result = Button::wndproc(hw, msg, wparam, lparam);

		set_font(14, CVE_FONT_REGULAR);

		return result;
	}

	case WM_PAINT:
	{
		COLORREF bg = g_theme[g_config.theme].bg;
		COLORREF text_color;

		if (clicked)
			text_color = CHANGE_BRIGHTNESS(g_theme[g_config.theme].preset_tx, CVE_CT_BR_CLICKED);
		else if (hovered)
			text_color = CHANGE_BRIGHTNESS(g_theme[g_config.theme].preset_tx, CVE_CT_BR_HOVERED);
		else
			text_color = g_theme[g_config.theme].preset_tx;

		::SetTextColor(bitmap_buffer.hdc_memory, text_color);

		draw_content(bg, &rect_wnd, "�� Default(23)", false);
		bitmap_buffer.transfer();

		return 0;
	}

	case WM_LBUTTONUP:
		if (clicked) {

		}
		return 0;
	}
	return Button::wndproc(hw, msg, wparam, lparam);
}



//---------------------------------------------------------------------
//		�E�B���h�E�v���V�[�W��(�F)
//---------------------------------------------------------------------
LRESULT cve::Button_Color::wndproc(HWND hw, UINT msg, WPARAM wparam, LPARAM lparam)
{
	RECT rect_wnd;

	::GetClientRect(hwnd, &rect_wnd);

	switch (msg) {
	case WM_CREATE:
		bg_color = NULL;
		break;

	case WM_PAINT:
	{
		COLORREF bg = bg_color;

		::SetTextColor(bitmap_buffer.hdc_memory, g_theme[g_config.theme].bt_tx);

		draw_content(bg, &rect_wnd, label, true);
		draw_edge();
		bitmap_buffer.transfer();

		return 0;
	}

	case WM_COMMAND:
		switch (wparam) {
		case CVE_CM_CHANGE_COLOR:
			bg_color = (COLORREF)lparam;
			::InvalidateRect(hw, NULL, FALSE);

			return 0;
		}
		break;
	}
	return Button::wndproc(hw, msg, wparam, lparam);
}