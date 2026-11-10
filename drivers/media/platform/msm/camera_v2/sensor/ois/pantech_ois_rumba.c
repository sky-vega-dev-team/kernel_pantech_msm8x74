/* Copyright (c) 2011-2013, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/*
 * OIS of the RUMBA SA closed loop modules of the Pantech IMX214 cameras.
 * It sits behind the I2C address of the actuator, and is driven through
 * the same registers the Pantech actuator driver used.
 */

#include <linux/delay.h>
#include <linux/module.h>
#include "msm_ois.h"
#include "msm_ois_i2c.h"
#include "pantech_ois_rumba.h"

#define RUMBA_SID		(0x48 >> 1)

#define RUMBA_OISCTRL_ADDR	0x0000
#define RUMBA_OISSTS_ADDR	0x0001
#define RUMBA_OISMODE_ADDR	0x0002
#define RUMBA_FWINFO_CTRL_ADDR	0x0080
#define RUMBA_GRX_ADDR		0x0086
#define RUMBA_GRY_ADDR		0x0088
#define RUMBA_HAX_ADDR		0x008E
#define RUMBA_HAY_ADDR		0x0090

#define RUMBA_OISSTS_INIT	0
#define RUMBA_OISSTS_IDLE	1
#define RUMBA_OISSTS_RUN	2

#define RUMBA_MODE_STILL	0
#define RUMBA_MODE_MOVIE	1
#define RUMBA_MODE_CENTERING	5

static struct msm_ois_fn_t rumba_ois_func_tbl;

/* The gyro and hall values are little endian words */
static int rumba_ois_read_le(uint16_t addr, int16_t *val)
{
	uint16_t data = 0;
	int rc = ois_i2c_read(addr, &data, MSM_CAMERA_I2C_WORD_DATA);

	*val = (int16_t)(((data & 0xFF) << 8) | ((data & 0xFF00) >> 8));
	return rc;
}

static int rumba_ois_stop(void)
{
	uint16_t status = 0;
	int i, rc;

	rc = ois_i2c_write(RUMBA_OISCTRL_ADDR, 0x00, MSM_CAMERA_I2C_BYTE_DATA);
	if (rc < 0)
		return rc;
	mdelay(1);
	for (i = 0; i < 10; i++) {
		rc = ois_i2c_read(RUMBA_OISSTS_ADDR, &status,
			MSM_CAMERA_I2C_BYTE_DATA);
		if (rc >= 0 && (status == RUMBA_OISSTS_IDLE ||
			status == RUMBA_OISSTS_INIT))
			return 0;
		mdelay(10);
	}
	pr_err("%s: OIS did not stop, status %d\n", __func__, status);
	return OIS_FAIL;
}

static int rumba_ois_set_mode(uint16_t mode)
{
	int rc = rumba_ois_stop();

	if (rc < 0)
		return rc;
	rc = ois_i2c_write(RUMBA_OISMODE_ADDR, mode, MSM_CAMERA_I2C_BYTE_DATA);
	if (rc < 0)
		return rc;
	return ois_i2c_write(RUMBA_OISCTRL_ADDR, 0x01,
		MSM_CAMERA_I2C_BYTE_DATA);
}

static int rumba_ois_mode(enum ois_mode_t mode)
{
	int rc;

	switch (mode) {
	case OIS_MODE_PREVIEW_CAPTURE:
	case OIS_MODE_CAPTURE:
		rc = rumba_ois_set_mode(RUMBA_MODE_STILL);
		break;
	case OIS_MODE_VIDEO:
		rc = rumba_ois_set_mode(RUMBA_MODE_MOVIE);
		break;
	case OIS_MODE_CENTERING_ONLY:
		rc = rumba_ois_set_mode(RUMBA_MODE_CENTERING);
		break;
	case OIS_MODE_CENTERING_OFF:
		rc = rumba_ois_stop();
		break;
	default:
		return OIS_FAIL;
	}
	if (rc >= 0)
		rumba_ois_func_tbl.ois_cur_mode = mode;
	return rc;
}

static int rumba_ois_on(enum ois_ver_t ver)
{
	int rc;

	/* Let the gyro and hall values be read back */
	rc = ois_i2c_write(RUMBA_FWINFO_CTRL_ADDR, 1, MSM_CAMERA_I2C_BYTE_DATA);
	if (rc < 0) {
		pr_err("%s: OIS not responding, rc %d\n", __func__, rc);
		return OIS_INIT_I2C_ERROR;
	}
	return rumba_ois_mode(OIS_MODE_PREVIEW_CAPTURE);
}

static int rumba_ois_off(void)
{
	int rc = rumba_ois_stop();

	ois_i2c_write(RUMBA_FWINFO_CTRL_ADDR, 0, MSM_CAMERA_I2C_BYTE_DATA);
	return rc;
}

static int rumba_ois_stat(struct msm_sensor_ois_info_t *ois_stat)
{
	uint16_t status = 0;
	int rc;

	snprintf(ois_stat->ois_provider, ARRAY_SIZE(ois_stat->ois_provider),
		"PANTECH_RUMBA_SA");
	rc = ois_i2c_read(RUMBA_OISSTS_ADDR, &status, MSM_CAMERA_I2C_BYTE_DATA);
	if (rc < 0 || status != RUMBA_OISSTS_RUN)
		return rc;

	rc = rumba_ois_read_le(RUMBA_GRX_ADDR, &ois_stat->gyro[0]);
	rc |= rumba_ois_read_le(RUMBA_GRY_ADDR, &ois_stat->gyro[1]);
	rc |= rumba_ois_read_le(RUMBA_HAX_ADDR, &ois_stat->hall[0]);
	rc |= rumba_ois_read_le(RUMBA_HAY_ADDR, &ois_stat->hall[1]);
	ois_stat->is_stable = 1;
	return rc;
}

/* The module has no lens offset control */
static int rumba_ois_move_lens(int16_t offset_x, int16_t offset_y)
{
	return OIS_FAIL;
}

void pantech_ois_rumba_init(struct msm_ois_ctrl_t *msm_ois_t)
{
	rumba_ois_func_tbl.ois_on = rumba_ois_on;
	rumba_ois_func_tbl.ois_off = rumba_ois_off;
	rumba_ois_func_tbl.ois_mode = rumba_ois_mode;
	rumba_ois_func_tbl.ois_stat = rumba_ois_stat;
	rumba_ois_func_tbl.ois_move_lens = rumba_ois_move_lens;
	rumba_ois_func_tbl.ois_cur_mode = OIS_MODE_CENTERING_OFF;

	msm_ois_t->sid_ois = RUMBA_SID;
	msm_ois_t->ois_func_tbl = &rumba_ois_func_tbl;
}
