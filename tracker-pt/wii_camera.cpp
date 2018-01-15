/* Copyright (c) 2012 Patrick Ruoff
 * Copyright (c) 2015-2016 Stanislaw Halik <sthalik@misaki.pl>
 * Copyright (c) 2017-2018 Wei Shuai <cpuwolf@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 */

#include "wii_camera.h"
#include "wii_frame.hpp"

#include "compat/sleep.hpp"
#include "compat/camera-names.hpp"
#include "compat/math-imports.hpp"

#include <opencv2/imgproc.hpp>

#include "cv/video-property-page.hpp"

#include "point_tracker.h"

using namespace pt_module;

WIICamera::WIICamera(const QString& module_name) : s { module_name }
{
	cam_info.fps = 70;
	cam_info.res_x = 1024;
	cam_info.res_y = 768;
	cam_info.fov = 1.2f;
	//create a blank frame
	internalframe = cv::Mat(cam_info.res_x, cam_info.res_y, CV_8UC3, cv::Scalar(0, 0, 0));
	//cv::cvtColor(_frame, _frame2, cv::COLOR_BGR2BGRA);
}

QString WIICamera::get_desired_name() const
{
    return desired_name;
}

QString WIICamera::get_active_name() const
{
    return active_name;
}

void WIICamera::show_camera_settings()
{

}

WIICamera::result WIICamera::get_info() const
{
    if (cam_info.res_x == 0 || cam_info.res_y == 0)
        return result(false, pt_camera_info());
    return result(true, cam_info);
}

WIICamera::result WIICamera::get_frame(pt_frame& frame_)
{
    cv::Mat& frame = frame_.as<WIIFrame>()->mat;
	std::vector<vec2>& pts = frame_.as<WIIFrame>()->points;
	//create a blank frame
	cv::Mat blank_frame(cam_info.res_x, cam_info.res_y, CV_8UC3, cv::Scalar(0, 0, 0));

    const bool new_frame = _get_frame(frame);
	const bool new_points = _get_points(pts);

    if (new_frame)
    {
		_draw_bg();
        return result(true, cam_info);
    }
    else
        return result(false, pt_camera_info());
}

pt_camera_open_status WIICamera::start(int idx, int fps, int res_x, int res_y)
{
	m_pDev = new wiimote;
	if (m_pDev == NULL)
	{
		stop();
		return cam_open_error;
	}
	m_pDev->ChangedCallback = on_state_change;
	m_pDev->CallbackTriggerFlags = (state_change_flags)(CONNECTED |
		EXTENSION_CHANGED |
		MOTIONPLUS_CHANGED);
    return cam_open_ok_no_change;
}

void WIICamera::stop()
{
	onExit = false;
	m_pDev->ChangedCallback = NULL;
	m_pDev->Disconnect();
	Beep(1000, 200);
	if (m_pDev) {
		delete m_pDev;
		m_pDev = NULL;
	}

    desired_name = QString();
    active_name = QString();
    cam_info = pt_camera_info();
    cam_desired = pt_camera_info();
}

bool WIICamera::_get_frame(cv::Mat& frame)
{
	char txtbuf[64];
	sprintf(txtbuf, "%s", "wait for WIImote");
reconnect :
	qDebug() << "wii wait";
	while (!m_pDev->Connect(wiimote::FIRST_AVAILABLE)) {
		if (onExit)
			goto goodbye;
		Beep(500, 30); Sleep(1500);
		//cv::resize(blank_frame, preview_frame, cv::Size(preview_size.width(), preview_size.height()), 0, 0, cv::INTER_NEAREST);
		//draw wait text
		cv::putText(internalframe,
			txtbuf,
			cv::Point(cam_info.res_x / 10, cam_info.res_y / 2),
			cv::FONT_HERSHEY_SIMPLEX,
			1,
			cv::Scalar(255, 255, 255),
			1);
	}

	/* wiimote connected */
	m_pDev->SetLEDs(0x0f);
	Beep(1000, 300); Sleep(500);

	qDebug() << "wii connected";

	while (m_pDev->RefreshState() == NO_CHANGE) {
		Sleep(1); // don't hog the CPU if nothing changed
	}
	if (onExit)
		goto goodbye;

	// did we loose the connection?
	if (m_pDev->ConnectionLost())
	{
		goto reconnect;
	}

	return true;
goodbye:
	return false;
}

bool WIICamera::_get_points(std::vector<vec2>& points)
{
	points.reserve(4);
	points.clear();

	bool dot_sizes = (m_pDev->IR.Mode == wiimote_state::ir::EXTENDED);

	for (unsigned index = 0; index < 4; index++)
	{
		wiimote_state::ir::dot &dot = m_pDev->IR.Dot[index];
		if (dot.bVisible) {
			//qDebug() << "wii:" << dot.RawX << "+" << dot.RawY;

			const float W = 1024.0f;
			const float H = 768.0f;
			const float RX = W - dot.RawX;
			const float RY = H - dot.RawY;
			//vec2 dt((dot.RawX - W / 2.0f) / W, -(dot.RawY - H / 2.0f) / W);
			//anti-clockwise rotate 2D point
			vec2 dt((RX - W / 2.0f) / W, -(RY - H / 2.0f) / W);

			points.push_back(dt);

		}
	}
	const bool success = points.size() >= PointModel::N_POINTS;

	return success;
}

void WIICamera::_draw_bg()
{
	//draw battery status
	cv::line(internalframe,
		cv::Point(0, 0),
		cv::Point(cam_info.res_x*m_pDev->BatteryPercent / 100, 0),
		(m_pDev->bBatteryDrained ? cv::Scalar(255, 0, 0) : cv::Scalar(0, 140, 0)),
		2);
	{
		//draw horizon
		static int pdelta = 0;
		static int rdelta = 0;
		if (m_pDev->Nunchuk.Acceleration.Orientation.UpdateAge < 10)
		{
			pdelta = iround((cam_info.res_y / 2) * tan((m_pDev->Acceleration.Orientation.Pitch)* M_PI / 180.0f));
			rdelta = iround((cam_info.res_x / 4) * tan((m_pDev->Acceleration.Orientation.Roll)* M_PI / 180.0f));
		}
		cv::line(internalframe,
			cv::Point(0, cam_info.res_y / 2 + rdelta - pdelta),
			cv::Point(cam_info.res_x, cam_info.res_y / 2 - rdelta - pdelta),
			cv::Scalar(80, 80, 80),
			1);
	}
}

void WIICamera::on_state_change(wiimote &remote,
	state_change_flags  changed,
	const wiimote_state &new_state)
{
	// the wiimote just connected
	if (changed & CONNECTED)
	{
		if (new_state.ExtensionType != wiimote::BALANCE_BOARD)
		{
			if (new_state.bExtension)
				remote.SetReportType(wiimote::IN_BUTTONS_ACCEL_IR_EXT); // no IR dots
			else
				remote.SetReportType(wiimote::IN_BUTTONS_ACCEL_IR);		//    IR dots
		}
	}
	// another extension was just connected:
	else if (changed & EXTENSION_CONNECTED)
	{

		Beep(1000, 200);

		// switch to a report mode that includes the extension data (we will
		//  loose the IR dot sizes)
		// note: there is no need to set report types for a Balance Board.
		if (!remote.IsBalanceBoard())
			remote.SetReportType(wiimote::IN_BUTTONS_ACCEL_IR_EXT);
	}
	else if (changed & EXTENSION_DISCONNECTED)
	{

		Beep(200, 300);

		// use a non-extension report mode (this gives us back the IR dot sizes)
		remote.SetReportType(wiimote::IN_BUTTONS_ACCEL_IR);
	}
}
