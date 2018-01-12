/* Copyright (c) 2012 Patrick Ruoff
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 */

#include "wii_camera.h"
#include "compat/sleep.hpp"
#include "compat/camera-names.hpp"
#include "compat/math-imports.hpp"

#include "cv/video-property-page.hpp"

constexpr double WIICamera::dt_eps;

WIICamera::WIICamera(const QString& module_name) : dt_mean(0), fov(0), s(module_name) {}

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
    const int idx = camera_name_to_index(s.camera_name);

    if (bool(*this))
        video_property_page::show_from_capture(*cap, idx);
    else
    {
        video_property_page::show(idx);
    }
}

WIICamera::result WIICamera::get_info() const
{
    if (cam_info.res_x == 0 || cam_info.res_y == 0)
        return result(false, pt_camera_info());
    return result(true, cam_info);
}

WIICamera::result WIICamera::get_frame(cv::Mat& frame)
{
	cam_info.fps = 50;
	cam_info.res_x = 1024;
	cam_info.res_y = 768;
	cam_info.fov = 40;

	return result(true, cam_info);
}

pt_camera_open_status WIICamera::start(int idx, int fps, int res_x, int res_y)
{
	cam_info.fps = 70;
	cam_info.res_x = 1024;
	cam_info.res_y = 768;
	cam_info.fov = 40;
	return cam_open_ok_change;
}

void WIICamera::stop()
{
    cap = nullptr;
    desired_name = QString();
    active_name = QString();
    cam_info = pt_camera_info();
    cam_desired = pt_camera_info();
}


void WIICamera::camera_deleter::operator()(cv::VideoCapture* cap)
{
    if (cap)
    {
        if (cap->isOpened())
            cap->release();
        delete cap;
    }
}
