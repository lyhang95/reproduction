#include "tools.hpp"
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <limits>

void calcDarkChannel(const cv::Mat_<cv::Vec3b>& src, cv::Mat_<uchar>& dst, const int s)
{
    dst.create(src.size());

    for(int i = 0; i != src.rows; ++i)
    {
	for(int j = 0; j != src.cols; ++j)
	{
	    uchar min_value = 255;
	    int bi = i - (s >> 1), ei = bi + s,
		bj = j - (s >> 1), ej = bj + s;
	    for(int p = bi; p != ei; ++p)
	    {
		if(p < 0||p >= src.rows) continue;
		for(int q = bj; q != ej; ++q)
		{
		    if(q < 0||q >= src.cols) continue;
		    if(min_value > src(p, q)[0]) min_value = src(p, q)[0];
		    if(min_value > src(p, q)[1]) min_value = src(p, q)[1];
		    if(min_value > src(p, q)[2]) min_value = src(p, q)[2];
		}
	    }

	    dst(i, j) = min_value;
	}
    }
}

void estimateAtmosphericLight(const cv::Mat_<cv::Vec3b>& src, const cv::Mat_<uchar>& dark_channel, cv::Vec3b& A)
{
    int table[256] = { 0 };
    for(int i = 0; i != dark_channel.rows; ++i)
    {
	for(int j = 0; j != dark_channel.cols; ++j)
	{
	    table[dark_channel(i, j)] += 1;
	}
    }

    float n = dark_channel.rows*dark_channel.cols, cum = 0;
    uchar threshold = 0;
    for(int i = 0; i != 256; ++i)
    {
	cum += table[i]/n;
	if(cum >= 0.9)
	{
	    threshold = i;
	    break;
	}
    }

    cv::Mat_<uchar> mask(dark_channel.size());
    for(int i = 0; i != dark_channel.rows; ++i)
    {
	for(int j = 0; j != dark_channel.cols; ++j)
	{
	    mask(i, j) = dark_channel(i, j) > threshold ? 1 : 0;
	}
    }

    cv::Mat_<uchar> gray;
    uchar max_value = 0;
    cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
    for(int i = 0; i != gray.rows; ++i)
    {
	for(int j = 0; j != gray.cols; ++j)
	{
	    if(max_value < gray(i, j)&&mask(i, j) == 1)
	    {
		max_value = gray(i, j);
		A[0] = src(i, j)[0];
		A[1] = src(i, j)[1];
		A[2] = src(i, j)[2];
	    }
	}
    }
}

void initTransMap(const cv::Mat_<cv::Vec3b>& src, const cv::Vec3b A, cv::Mat& t, const int s, const float om)
{
    t.create(src.size(), CV_32F);
    cv::Vec3f Af(A[0], A[1], A[2]);

    for(int i = 0; i != src.rows; ++i)
    {
	for(int j = 0; j != src.cols; ++j)
	{
	    float min_value = std::numeric_limits<float>::max();
	    int bi = i - (s >> 1), ei = bi + s,
		bj = j - (s >> 1), ej = bj + s;
	    for(int p = bi; p != ei; ++p)
	    {
		if(p < 0||p >= src.rows) continue;
		for(int q = bj; q != ej; ++q)
		{
		    if(q < 0||q >= src.cols) continue;
		    if(min_value > src(p, q)[0]/Af[0]) min_value = src(p, q)[0]/Af[0];
		    if(min_value > src(p, q)[1]/Af[1]) min_value = src(p, q)[1]/Af[1];
		    if(min_value > src(p, q)[2]/Af[2]) min_value = src(p, q)[2]/Af[2];
		}
	    }

	    t.at<float>(i, j) = 1 - om*min_value;
	}
    }
}