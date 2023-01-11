//----------------------------------------------------------------------------------
//		Curve Editor
//		�\�[�X�t�@�C���iDirect2D�`��j
//		Visual C++ 2022
//----------------------------------------------------------------------------------

#include "cve_header.hpp"

#define CVE_GR_GRID_TH_L				0.5f
#define CVE_GR_GRID_TH_B				1.0f
#define CVE_GR_GRID_MIN					36



//---------------------------------------------------------------------
//		�O���t�̃O���b�h��`��
//---------------------------------------------------------------------
void cve::My_D2D_Paint_Object::draw_grid()
{
	brush->SetColor(D2D1::ColorF(CHANGE_BRIGHTNESS(TO_BGR(g_theme[g_config.theme].bg_graph), CVE_BR_GRID)));
	// 
	int kx = (int)std::floor(std::log(CVE_GRAPH_RESOLUTION * g_view_info.scale.x / (double)CVE_GR_GRID_MIN) / std::log(CVE_GRAPH_GRID_NUM));
	int ky = (int)std::floor(std::log(CVE_GRAPH_RESOLUTION * g_view_info.scale.y / (double)CVE_GR_GRID_MIN) / std::log(CVE_GRAPH_GRID_NUM));
	// �O���t�̘g���ɕ\�������O���b�h�̖{��
	int nx = MIN_LIMIT((int)std::pow(CVE_GRAPH_GRID_NUM, kx), 1);
	int ny = MIN_LIMIT((int)std::pow(CVE_GRAPH_GRID_NUM, ky), 1);
	// 
	float dx = (float)(CVE_GRAPH_RESOLUTION * g_view_info.scale.x) / (float)nx;
	float dy = (float)(CVE_GRAPH_RESOLUTION * g_view_info.scale.y) / (float)ny;
	int lx, ly;

	if (to_graph(0, 0).x >= 0)
		lx = (int)std::floor(to_graph(0, 0).x * nx / (double)CVE_GRAPH_RESOLUTION);
	else
		lx = (int)std::ceil(to_graph(0, 0).x * nx / (double)CVE_GRAPH_RESOLUTION);

	if (to_graph(0, 0).y >= 0)
		ly = (int)std::floor(to_graph(0, 0).y * ny / (double)CVE_GRAPH_RESOLUTION);
	else
		ly = (int)std::ceil(to_graph(0, 0).y * ny / (double)CVE_GRAPH_RESOLUTION);

	float ax = to_client((int)(lx * CVE_GRAPH_RESOLUTION / (float)nx), 0).x;
	float ay = to_client(0, (int)(ly * CVE_GRAPH_RESOLUTION / (float)ny)).y;
	float thickness;

	// �c����
	for (int i = 0; ax + dx * i <= rect.right; i++) {
		if ((lx + i) % CVE_GRAPH_GRID_NUM == 0)
			thickness = CVE_GR_GRID_TH_B;
		else
			thickness = CVE_GR_GRID_TH_L;
		p_render_target->DrawLine(
			D2D1::Point2F(ax + dx * i, 0),
			D2D1::Point2F(ax + dx * i, (float)rect.bottom),
			brush, thickness, NULL
		);
	}

	// ������
	for (int i = 0; ay + dy * i <= rect.bottom; i++) {
		if ((ly - i) % CVE_GRAPH_GRID_NUM == 0)
			thickness = CVE_GR_GRID_TH_B;
		else
			thickness = CVE_GR_GRID_TH_L;

		p_render_target->DrawLine(
			D2D1::Point2F(0, ay + dy * i),
			D2D1::Point2F((float)rect.right, ay + dy * i),
			brush, thickness, NULL
		);
	}
}



//---------------------------------------------------------------------
//		���E���h�G�b�W��`��
//---------------------------------------------------------------------
void cve::My_D2D_Paint_Object::draw_rounded_edge(int flag, float radius) {
	ID2D1GeometrySink* sink;
	ID2D1PathGeometry* edge;
	D2D1_POINT_2F pt_1, pt_2, pt_3;

	D2D1_POINT_2F pts_1[] = {
		D2D1::Point2F((float)rect.left, (float)rect.top),
		D2D1::Point2F((float)rect.left, (float)rect.bottom),
		D2D1::Point2F((float)rect.right, (float)rect.top),
		D2D1::Point2F((float)rect.right, (float)rect.bottom)
	};

	D2D1_POINT_2F pts_2[] = {
		D2D1::Point2F((float)rect.left, (float)rect.top + radius),
		D2D1::Point2F((float)rect.left + radius, (float)rect.bottom),
		D2D1::Point2F((float)rect.right - radius, (float)rect.top),
		D2D1::Point2F((float)rect.right, (float)rect.bottom - radius)
	};

	D2D1_POINT_2F pts_3[] = {
		D2D1::Point2F((float)rect.left + radius, (float)rect.top),
		D2D1::Point2F((float)rect.left, (float)rect.bottom - radius),
		D2D1::Point2F((float)rect.right, (float)rect.top + radius),
		D2D1::Point2F((float)rect.right - radius, (float)rect.bottom)
	};

	(*pp_factory)->CreatePathGeometry(&edge);
	edge->Open(&sink);

	for (int i = 0; i < 4; i++) {
		if (flag & 1 << i) {
			pt_1 = pts_1[i];
			pt_2 = pts_2[i];
			pt_3 = pts_3[i];

			sink->BeginFigure(pt_1, D2D1_FIGURE_BEGIN_FILLED);
			sink->AddLine(pt_2);
			sink->AddArc(
				D2D1::ArcSegment(
					pt_3,
					D2D1::SizeF(radius, radius),
					0.0f,
					D2D1_SWEEP_DIRECTION_CLOCKWISE,
					D2D1_ARC_SIZE_SMALL
				)
			);
			sink->EndFigure(D2D1_FIGURE_END_CLOSED);
		}
	}

	sink->Close();
	brush->SetColor(D2D1::ColorF(TO_BGR(g_theme[g_config.theme].bg)));
	if (edge)
		p_render_target->FillGeometry(edge, brush, NULL);
}



//---------------------------------------------------------------------
//		���C���E�B���h�E��`��
//---------------------------------------------------------------------
void cve::My_D2D_Paint_Object::draw_panel_main(const RECT& rect_sepr)
{
	ID2D1StrokeStyle* style = nullptr;

	(*pp_factory)->CreateStrokeStyle(
		D2D1::StrokeStyleProperties(
			D2D1_CAP_STYLE_ROUND,
			D2D1_CAP_STYLE_ROUND,
			D2D1_CAP_STYLE_ROUND,
			D2D1_LINE_JOIN_MITER,
			10.0f,
			D2D1_DASH_STYLE_SOLID,
			0.0f),
		NULL, NULL,
		&style
	);

	const D2D1_POINT_2F line_start = {
		g_config.layout_mode == cve::Config::Vertical ?
			(rect_sepr.right + rect_sepr.left) * 0.5f - CVE_SEPARATOR_LINE_LENGTH :
			(float)(rect_sepr.left + CVE_SEPARATOR_WIDTH),
		g_config.layout_mode == cve::Config::Vertical ?
			(float)(rect_sepr.top + CVE_SEPARATOR_WIDTH) :
			(rect_sepr.top + rect_sepr.bottom) * 0.5f - CVE_SEPARATOR_LINE_LENGTH
	};

	const D2D1_POINT_2F line_end = {
		g_config.layout_mode == cve::Config::Vertical ?
			(rect_sepr.right + rect_sepr.left) * 0.5f + CVE_SEPARATOR_LINE_LENGTH :
			(float)(rect_sepr.left + CVE_SEPARATOR_WIDTH),
		g_config.layout_mode == cve::Config::Vertical ?
			(float)(rect_sepr.top + CVE_SEPARATOR_WIDTH) :
			(rect_sepr.top + rect_sepr.bottom) * 0.5f + CVE_SEPARATOR_LINE_LENGTH
	};

	//Direct2D������
	d2d_setup(TO_BGR(g_theme[g_config.theme].bg));

	if (p_render_target != nullptr) {
		p_render_target->BeginDraw();

		brush->SetColor(D2D1::ColorF(TO_BGR(g_theme[g_config.theme].sepr)));

		if (brush)
			p_render_target->DrawLine(
				line_start,
				line_end,
				brush, CVE_SEPARATOR_LINE_WIDTH, style
			);

		// brush->Release();

		p_render_target->EndDraw();
	}
}



//---------------------------------------------------------------------
//		�p�l����`��
//---------------------------------------------------------------------
void cve::My_D2D_Paint_Object::draw_panel()
{
	//Direct2D������
	d2d_setup(TO_BGR(g_theme[g_config.theme].bg));
}



//---------------------------------------------------------------------
//		�G�f�B�^�p�l����`��
//---------------------------------------------------------------------
void cve::My_D2D_Paint_Object::draw_panel_editor()
{
	D2D1_RECT_F rect_left = {
		0,
		0,
		(float)g_view_info.origin.x,
		(float)rect.bottom
	};

	D2D1_RECT_F rect_right = {
		(float)(g_view_info.origin.x + g_view_info.scale.x * CVE_GRAPH_RESOLUTION),
		0,
		(float)rect.right,
		(float)rect.bottom,
	};

	D2D1_RECT_F rect_up = {
		(float)g_view_info.origin.x,
		0,
		(float)(g_view_info.origin.x + g_view_info.scale.x * CVE_GRAPH_RESOLUTION),
		to_client(0, (int)(CVE_CURVE_VALUE_MAX_Y * CVE_GRAPH_RESOLUTION)).y
	};

	D2D1_RECT_F rect_down = {
		(float)g_view_info.origin.x,
		to_client(0, (int)(CVE_CURVE_VALUE_MIN_Y * CVE_GRAPH_RESOLUTION)).y,
		(float)(g_view_info.origin.x + g_view_info.scale.x * CVE_GRAPH_RESOLUTION),
		(float)rect.bottom
	};


	//Direct2D������
	d2d_setup(TO_BGR(g_theme[g_config.theme].bg_graph));

	//�`��
	if (is_safe(&p_render_target)) {
		p_render_target->BeginDraw();

		//�O���b�h
		draw_grid();

		brush->SetColor(D2D1::ColorF(CHANGE_BRIGHTNESS(TO_BGR(g_theme[g_config.theme].bg_graph), CVE_BR_GR_INVALID)));
		brush->SetOpacity(0.5f);
		if (brush) {
			// X��0����1����̕������Â�����
			p_render_target->FillRectangle(&rect_left, brush);
			p_render_target->FillRectangle(&rect_right, brush);
			// �x�W�F���[�h�̂Ƃ�
			if (g_config.edit_mode == cve::Mode_Bezier) {
				p_render_target->FillRectangle(&rect_up, brush);
				p_render_target->FillRectangle(&rect_down, brush);
			}
		}
		brush->SetOpacity(1);

		// �ҏW���[�h�U�蕪��
		//switch (g_config.edit_mode) {
		//	// �x�W�F���[�h�̂Ƃ�
		//case Mode_Bezier:
		//	if (g_config.trace)
		//		g_curve_bezier_trace.draw_curve(this, rect, CVE_DRAW_CURVE_TRACE);

		//	g_curve_bezier.draw_curve(this, rect, CVE_DRAW_CURVE_REGULAR);
		//	break;

		//	// �x�W�F(����)���[�h�̂Ƃ�
		//case Mode_Bezier_Multi:
		//	if (g_config.trace)
		//		g_curve_bezier_multi_trace.draw_curve(this, rect, CVE_DRAW_CURVE_TRACE);

		//	g_curve_bezier_multi[g_config.current_id.multi - 1].draw_curve(this, rect, CVE_DRAW_CURVE_REGULAR);
		//	break;

		//	// �l�w�胂�[�h�̂Ƃ�
		//case Mode_Bezier_Value:
		//	if (g_config.trace)
		//		g_curve_bezier_value_trace.draw_curve(this, rect, CVE_DRAW_CURVE_TRACE);

		//	g_curve_bezier_value[g_config.current_id.value].draw_curve(this, rect, CVE_DRAW_CURVE_REGULAR);
		//	break;

		//	// �U�����[�h�̂Ƃ�
		//case Mode_Elastic:
		//	if (g_config.trace)
		//		g_curve_elastic_trace.draw_curve(this, rect, CVE_DRAW_CURVE_TRACE);

		//	g_curve_elastic.draw_curve(this, rect, CVE_DRAW_CURVE_REGULAR);
		//	break;

		//	// �o�E���X���[�h�̂Ƃ�
		//case Mode_Bounce:
		//	if (g_config.trace)
		//		g_curve_bounce_trace.draw_curve(this, rect, CVE_DRAW_CURVE_TRACE);

		//	g_curve_bounce.draw_curve(this, rect, CVE_DRAW_CURVE_REGULAR);
		//	break;
		//}

		p_render_target->EndDraw();
	}
}