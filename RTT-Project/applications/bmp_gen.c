/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-09-23     zhenzhen1003       the first version
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "bmp_gen.h"

/*
��С���ֽ������洢�������ݿ�һ����֯��4�ֽڶ��롣
ͼ��������Ҳ�����⣬��ÿ��ͼ��������ֽڣ���4�ֽڶ��롣

ͼ�����ݰ��е����ţ��ȴ洢���һ��ͼ�����ݣ�Ȼ�����δ�ţ�ֱ����һ�����ݡ�
������ƣ�������Ϊ�˴��ļ�β����ǰ����ʱ���ܹ�ֱ��˳�����ͼ�����ݰɡ�
*/

typedef union {
    uint8_t bytes[4];
    uint32_t value;
}LITTLE;

/*
 * @fileName: bmp file name: test.bmp
 * @width   : bmp pixel width: 32bit
 * @height  : bmp pixel width: 32bit
 * @color   : R[8]/G[8]/B[8]
 * @note    : BMP is l endian mode
 */
uint8_t* bmp_gen_test(uint32_t width, uint32_t height, uint8_t* color)
{
    uint32_t i, j;
    LITTLE l_width, l_height, l_bfSize, l_biSizeImage;

    uint32_t width_r  =  (width * 24 / 8 + 3) / 4 * 4;
    uint32_t bfSize = width_r * height + 54 + 2;
    uint32_t biSizeImage = width_r * height;
    uint32_t size = width_r - width * 3 + 54 + width * height * 3 + 2;

    //rt_kprintf("%d ",size);
    uint8_t* data = (uint8_t*)rt_malloc(size);

    l_width.value = width;
    l_height.value = height;
    l_bfSize.value = bfSize;
    l_biSizeImage.value = biSizeImage;

    /* BMP file format: www.cnblogs.com/wainiwann/p/7086844.html */
    uint8_t bmp_head_map[54] = {
        /* bmp file header: 14 byte */
        0x42, 0x4d,
        // bmp pixel size: width * height * 3 + 54
        l_bfSize.bytes[0], l_bfSize.bytes[1], l_bfSize.bytes[2], l_bfSize.bytes[3],
        0, 0 , 0, 0,
        54, 0 , 0, 0,    /* 14+40=54 */

        /* bmp map info: 40 byte */
        40, 0, 0, 0,
        //width
        l_width.bytes[0], l_width.bytes[1], l_width.bytes[2], l_width.bytes[3],
        //height
        l_height.bytes[0], l_height.bytes[1], l_height.bytes[2], l_height.bytes[3],
        1, 0,
        24, 00,             /* 24 bit: R[8]/G[8]/B[8] */

        0, 0, 0, 0,     //biCompression:0
//        0, 0, 0, 0,     //biSizeImage：A2 00 00 00=162
        l_biSizeImage.bytes[0], l_biSizeImage.bytes[1], l_biSizeImage.bytes[2], l_biSizeImage.bytes[3],
        0, 0, 0, 0,     //biXPelsPerMeter: 60 0F 00 00
        0, 0, 0, 0,     //biYPelsPerMeter
        0, 0, 0, 0,     //biClrUsed
        0, 0, 0, 0      //biClrImportant
    };

    /* write in binary format */
    for(int k = 0; k < 54; k++){
        data[k] = bmp_head_map[k];
    }

    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            for (int k = 2; k >= 0; k--) {
                data[54 + i*width * 3 + j * 3 + (2 - k)] = color[(height - i - 1) * width * 3 + j * 3 + k];
            }
        }
    //4 byte align
        for(j = 0; j < width_r-width*3; j++)
            data[size - width_r-width*3 - 2 + j] = 0;
    }
    data[size - 2] = 0;
    data[size - 1] = 0;


    return data;
}
