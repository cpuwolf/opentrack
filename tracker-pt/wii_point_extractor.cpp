/* Copyright (c) 2012 Patrick Ruoff
 * Copyright (c) 2015-2017 Stanislaw Halik <sthalik@misaki.pl>
 * Copyright (c) 2017-2018 Wei Shuai <cpuwolf@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 */

#include "wii_point_extractor.h"
#include "compat/util.hpp"
#include "point_tracker.h"
#include "wii_frame.hpp"

#include <QDebug>

#include "cv/numeric.hpp"
#include <opencv2/videoio.hpp>

#undef PREVIEW
//#define PREVIEW

#if defined PREVIEW
#   include <opencv2/highgui.hpp>
#endif

#include <cmath>
#include <algorithm>
#include <cinttypes>
#include <memory>

#include <QDebug>

using namespace types;
using namespace pt_module;


WIIPointExtractor::WIIPointExtractor(const QString& module_name) : s(module_name)
{
    blobs.reserve(max_blobs);
}


void WIIPointExtractor::extract_points(const pt_frame& frame_, pt_preview& preview_frame_, std::vector<vec2>& points)
{
    const cv::Mat& frame = frame_.as_const<WIIFrame>()->mat;
	std::vector<vec2>& pts = frame_.as<WIIFrame>()->points;
	const vec2* ptds = pts.data();
    cv::Mat& preview_frame = *preview_frame_.as<WIIPreview>();
	int point_count = points.size();

	auto fun = [&](const vec2& p, const cv::Scalar& color, int thinkness = 1)
	{
		static constexpr int len = 9;

		cv::Point p2(iround(p[0] * preview_frame.cols + preview_frame.cols / 2),
			iround(-p[1] * preview_frame.cols + preview_frame.rows / 2));

		cv::line(preview_frame,
			cv::Point(p2.x - len, p2.y),
			cv::Point(p2.x + len, p2.y),
			color,
			thinkness);
		cv::line(preview_frame,
			cv::Point(p2.x, p2.y - len),
			cv::Point(p2.x, p2.y + len),
			color,
			thinkness);
	};
	for (unsigned i = 0; i < PointModel::N_POINTS; ++i)
	{
		//if (dot_sizes)
		//	fun(ptds[i], cv::Scalar(0, 255, 0), dot.Size);
		//else
			fun(ptds[i], cv::Scalar(0, 255, 0));
	}
}

