/* Copyright (c) 2012 Patrick Ruoff
 * Copyright (c) 2015-2016 Stanislaw Halik <sthalik@misaki.pl>
 * Copyright (c) 2017-2018 Wei Shuai <cpuwolf@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 */

#pragma once

#include "compat/ndebug-guard.hpp"
#include "pt-api.hpp"

#include "compat/util.hpp"
#include "compat/timer.hpp"

#include <functional>
#include <memory>
#include <tuple>

#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>

#include <QString>

#include <cstddef>
#include <wiiyourself/wiimote.h>

namespace pt_module {

struct WIICamera final : pt_camera
{
    WIICamera(const QString& module_name);

    pt_camera_open_status start(int idx, int fps, int res_x, int res_y) override;
    void stop() override;

    result get_frame(pt_frame& Frame) override;
    result get_info() const override;

    pt_camera_info get_desired() const override { return cam_desired; }
    QString get_desired_name() const override;
    QString get_active_name() const override;

    operator bool() const override { return cap && cap->isOpened(); }

    void set_fov(double value) override { fov = value; }
    void show_camera_settings() override;

	double get_focal_length() const { return 1.5f; }

private:
	wiimote * m_pDev;
	static void on_state_change(wiimote &remote,
		state_change_flags changed,
		const wiimote_state &new_state);
	bool onExit = false;
	cv::Mat internalframe;
	
	warn_result_unused bool _get_frame(cv::Mat& Frame);
	bool _get_points(std::vector<vec2>& points);
	void _draw_bg();

    double dt_mean = 0, fov = 30;

    Timer t;

    pt_camera_info cam_info;
    pt_camera_info cam_desired;
    QString desired_name, active_name;

    pt_settings s;

    static constexpr inline double dt_eps = 1./384;
};

} // ns pt_module