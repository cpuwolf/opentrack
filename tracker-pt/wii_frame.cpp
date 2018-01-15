/*
* Copyright (c) 2015-2016 Stanislaw Halik <sthalik@misaki.pl>
* Copyright (c) 2017-2018 Wei Shuai <cpuwolf@gmail.com>
*/
#include "point_tracker.h"
#include "wii_frame.hpp"

#include <cstring>
#include <tuple>

#include <opencv2/imgproc.hpp>

#pragma strict_gs_check(on)

using namespace pt_module;

WIIPreview& WIIPreview::operator=(const pt_frame& frame_)
{
    const cv::Mat& frame = frame_.as_const<const WIIFrame>()->mat;
    const int w = frame.cols, h = frame.rows;
    ensure_size(frame_copy, w, h, CV_8UC3);

    if (frame.channels() != 3)
    {
        once_only(qDebug() << "tracker/pt: camera frame depth: 3 !=" << frame.channels());
        return *this;
    }

    const bool need_resize = w != frame_out.cols || h != frame_out.rows;
    if (need_resize)
        cv::resize(frame, frame_copy, cv::Size(frame_resize.cols, frame_resize.rows), 0, 0, cv::INTER_NEAREST);
    else
        frame.copyTo(frame_copy);

    return *this;
}

WIIPreview::WIIPreview(int w, int h)
{
    ensure_size(frame_resize, w, h, CV_8UC4);
    ensure_size(frame_out, w, h, CV_8UC4);

    frame_out.setTo(cv::Scalar(0, 0, 0, 0));
}

QImage WIIPreview::get_bitmap()
{
    int stride = frame_out.step.p[0];

    if (stride < 64 || stride < frame_out.cols * 4)
    {
        once_only(qDebug() << "bad stride" << stride
                           << "for bitmap size" << frame_copy.cols << frame_copy.rows);
        return QImage();
    }

    cv::cvtColor(frame_copy, frame_out, cv::COLOR_BGR2BGRA);

    return QImage((const unsigned char*) frame_out.data,
                  frame_out.cols, frame_out.rows,
                  stride,
                  QImage::Format_ARGB32);
}

void WIIPreview::draw_head_center(double x, double y)
{
    double px_, py_;

    std::tie(px_, py_) = to_pixel_pos(x, y, frame_copy.cols, frame_copy.rows);

    int px = iround(px_), py = iround(py_);

    constexpr int len = 9;

    static const cv::Scalar color(0, 255, 255);
    cv::line(frame_copy,
             cv::Point(px - len, py),
             cv::Point(px + len, py),
             color, 1);
    cv::line(frame_copy,
             cv::Point(px, py - len),
             cv::Point(px, py + len),
             color, 1);
}

void WIIPreview::ensure_size(cv::Mat& frame, int w, int h, int type)
{
    if (frame.cols != w || frame.rows != h)
        frame = cv::Mat(h, w, type);
}
