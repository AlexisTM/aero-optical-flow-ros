/****************************************************************************
 *
 * Copyright (C) 2017  Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#include <getopt.h>

#include <ros/ros.h>
#include "log.h"
#include "mainloop.h"

/*
 * OV7251 only supports this resolutions each with a different FPS according
 * with datasheet but we are only being able to get 75fps in any of those
 * resolution, this is a problem in the ov7251 driver and the lack of
 * the documentation.
 */
static struct {
	uint32_t width;
	uint32_t height;
} resolutions[] = {
		{640, 480},// some glitches in the right and bottom
		{320, 240},
		{160, 120},
};

#define DEFAULT_RESOLUTION 0
#define DEFAULT_IMG_WIDTH resolutions[DEFAULT_RESOLUTION].width
#define DEFAULT_IMG_HEIGHT resolutions[DEFAULT_RESOLUTION].height
#define DEFAULT_IMG_CROP_WIDTH 128
#define DEFAULT_IMG_CROP_HEIGHT 128
#define DEFAULT_FLOW_OUTPUT_RATE 15
#define DEFAULT_FOCAL_LENGTH_X 216.6677
#define DEFAULT_FOCAL_LENGTH_Y 216.2457

#define DEFAULT_DEVICE_FILE "/dev/v4l/by-path/pci-0000:00:03.0-video-index2"
#define DEFAULT_DEVICE_ID 1

#define MAVLINK_TCP_IP "127.0.0.1"
#define MAVLINK_TCP_PORT 5760

static int safe_atoul(const char *s, unsigned long *ret)
{
	char *x = NULL;
	unsigned long l;

	errno = 0;
	l = strtoul(s, &x, 0);

	if (!x || x == s || *x || errno)
		return errno ? -errno : -EINVAL;

	*ret = l;

	return 0;
}

static int safe_atoi(const char *s, int *ret)
{
	char *x = NULL;
	long l;

	errno = 0;
	l = strtol(s, &x, 0);

	if (!x || x == s || *x || errno)
		return errno > 0 ? -errno : -EINVAL;

	if ((long) (int) l != l)
		return -ERANGE;

	*ret = (int) l;
	return 0;
}

static int safe_atof(const char *s, float *ret)
{
	char *x = NULL;
	float l;

	errno = 0;
	l = strtof(s, &x);

	if (!x || x == s || *x || errno)
		return errno > 0 ? -errno : -EINVAL;

	*ret = (float) l;
	return 0;
}

static void help()
{
	printf("%s [OPTIONS...]\n\n"
			"  -c --camera_device       Camera filepath\n"
			"                           Default %s\n"
			"  -i --camera_id           Camera id\n"
			"                           Default %u\n"
			"  -r --camera_resolution   Resolution of the video streaming from camera\n"
			"                           Default %ux%u\n"
			"  -x --crop_resolution     Resolution of the video that will be used to calculate optical flow\n"
			"                           Default %ux%u\n"
			"  -o --flow_output_rate    Output rate of the optical flow\n"
			"                           Default %u\n"
			"  -t --mavlink_tcp_ip    	MAVLink TCP IP where it will listen and send messages\n"
			"                           Default %s\n"
			"  -p --mavlink_tcp_port    MAVLink TCP port where it will listen and send messages\n"
			"                           Default %u\n"
			"  -f --focal_length        Set camera focal lenght in pixels\n"
			"                           Default %fx%f\n"
			,
			program_invocation_short_name,
			DEFAULT_DEVICE_FILE,
			DEFAULT_DEVICE_ID,
			DEFAULT_IMG_WIDTH,
			DEFAULT_IMG_HEIGHT,
			DEFAULT_IMG_CROP_WIDTH,
			DEFAULT_IMG_CROP_HEIGHT,
			DEFAULT_OUTPUT_RATE,
			MAVLINK_TCP_IP,
			MAVLINK_TCP_PORT,
			DEFAULT_FOCAL_LENGTH_X,
			DEFAULT_FOCAL_LENGTH_Y);
}

static int x_y_split(char *arg, unsigned long *x, unsigned long *y)
{
	char *divider = strchrnul(arg, 'x');
	const char *x_str, *y_str;

	if (!divider) {
		return -1;
	}

	x_str = arg;
	y_str = divider + 1;
	*divider = '\0';

	if (safe_atoul(x_str, x)) {
		return -1;
	}
	if (safe_atoul(y_str, y)) {
		return -1;
	}

	return 0;
}

static int x_y_float_split(char *arg, float *x, float *y)
{
	char *divider = strchrnul(arg, 'x');
	const char *x_str, *y_str;

	if (!divider) {
		return -1;
	}

	x_str = arg;
	y_str = divider + 1;
	*divider = '\0';

	if (safe_atof(x_str, x)) {
		return -1;
	}
	if (safe_atof(y_str, y)) {
		return -1;
	}

	return 0;
}

int main(int argc, char** argv)
{
    ros::init(argc, argv, "aero_optical_flow_node");
	Mainloop mainloop;
	int c;
	const struct option options[] = {
			{ "camera_device",			required_argument,	NULL,	'c' },
			{ "camera_id",				required_argument,	NULL,	'i' },
			{ "camera_width",			required_argument,	NULL,	'w' },
			{ "camera_height",			required_argument,	NULL,	'h' },
			{ "crop_width",				required_argument,	NULL,	'x' },
			{ "crop_height",			required_argument,	NULL,	'y' },
			{ "flow_output_rate",		required_argument,	NULL,	'o' },
			{ "mavlink_TCP_ip",			required_argument,	NULL,	't' },
			{ "mavlink_TCP_port",		required_argument,	NULL,	'p' },
			{ "focal_length",			required_argument,	NULL,	'f' },
			{ }
	};
	const char *camera_device = DEFAULT_DEVICE_FILE;
	unsigned long camera_id = DEFAULT_DEVICE_ID;
	unsigned long camera_width = DEFAULT_IMG_WIDTH;
	unsigned long camera_height = DEFAULT_IMG_HEIGHT;
	unsigned long crop_width = DEFAULT_IMG_CROP_WIDTH;
	unsigned long crop_height = DEFAULT_IMG_CROP_HEIGHT;
	unsigned long mavlink_tcp_port = MAVLINK_TCP_PORT;
	const char *mavlink_tcp_ip = MAVLINK_TCP_IP;
	int flow_output_rate = DEFAULT_FLOW_OUTPUT_RATE;
	float focal_length_x = DEFAULT_FOCAL_LENGTH_X;
	float focal_length_y = DEFAULT_FOCAL_LENGTH_Y;

	while ((c = getopt_long(argc, argv, "?c:i:r:x:o:p:t:f:", options, NULL)) >= 0) {
		switch (c) {
		case '?':
			help();
			return 0;
		case 'c':
			camera_device = optarg;
			break;
		case 'i':
			if (safe_atoul(optarg, &camera_id) < 0) {
				ERROR("Invalid argument for camera_id = %s", optarg);
				help();
				return -EINVAL;
			}
			break;
		case 'r':
			x_y_split(optarg, &camera_width, &camera_height);
			break;
		case 'x':
			x_y_split(optarg, &crop_width, &crop_height);
			break;
		case 'o':
			if (safe_atoi(optarg, &flow_output_rate) < 0) {
				ERROR("Invalid argument for flow_output_rate = %s", optarg);
				help();
				return -EINVAL;
			}
			break;
		case 't':
			mavlink_tcp_ip = optarg;
			break;
		case 'p':
			if (safe_atoul(optarg, &mavlink_tcp_port) < 0) {
				ERROR("Invalid argument for mavlink_tcp_port = %s", optarg);
				help();
				return -EINVAL;
			}
			break;
		case 'f':
			x_y_float_split(optarg, &focal_length_x, &focal_length_y);
			break;
		default:
			help();
			return -EINVAL;
		}
	}

	printf("Parameters:\n\tcamera_device=%s\n\tcamera_id=%lu\n\tcamera_width=%lu\n", camera_device, camera_id, camera_width);
	printf("\tcamera_height=%lu\n\tcrop_width=%lu\n\tcrop_height=%lu\n", camera_height, crop_width, crop_height);
	printf("\tflow_output_rate=%i\n\tmavlink_tcp_ip=%s\n\tmavlink_tcp_port=%lu\n", flow_output_rate, mavlink_tcp_ip, mavlink_tcp_port);
	printf("\tfocal_length_x=%f\n\tfocal_length_y=%f\n", focal_length_x, focal_length_y);

	int ret = mainloop.init(camera_device, camera_id, camera_width,
			camera_height, crop_width, crop_height, mavlink_tcp_ip, mavlink_tcp_port,
			flow_output_rate, focal_length_x, focal_length_y);
	if (ret) {
		return -1;
	}

	ret = mainloop.run();
	mainloop.shutdown();

	return ret;
}
