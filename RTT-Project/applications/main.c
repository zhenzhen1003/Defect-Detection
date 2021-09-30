/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-09-02     RT-Thread    first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include "drv_common.h"
#include "drv_spi_ili9488.h"  // spi lcd driver
#include <lcd_spi_port.h>  // lcd ports
#include <rt_ai_model_full_model.h>
#include <rt_ai.h>
#include <rt_ai_log.h>
#include <backend_cubeai.h>
#include <logo.h>
#include <rt_ai_runtime.h>
#include <util.h>
#include "bmp_gen.h"
#include "onenet.h"

#define PIN_KEY    GET_PIN(H, 4)  // ����
#define PIN_TUBE   GET_PIN(H, 2)  //���ܶ˿�
#define PIN_IN1    GET_PIN(B, 0)  // L298N�������ƶ�

//ȫ�ֱ�����ָʾ���鵽��
unsigned char flag_of_tube = 1;

void tube_on(void *args)
{
    rt_kprintf("����������\n");
    //���̵����ߵ�ƽ
    rt_pin_write(PIN_IN1, PIN_LOW);
    flag_of_tube = 1;
}


struct rt_event ov2640_event;
extern void wlan_autoconnect_init(void);
rt_thread_t onenet_update_thread;
extern struct rt_messagequeue onenet_mq;
static void onenet_mqtt_update_data_entry(uint8_t *image_data, int mode);
extern int rt_gc0328c_init(void);
extern void DCMI_Start(void);
rt_ai_buffer_t ai_flag = 0;
void ai_run_complete(void *arg){
    *(int*)arg = 1;
}

void ai_camera();
void addWeighted(uint8_t *src1, float alpha, uint8_t *src2, float beta, float gamma, uint8_t *dst);
void Normalization(uint8_t *src, float *dst, int height, int width, int channels);
void argmax(float *src, uint8_t *dst,int channels, int width, int height);
void bilinera_interpolation(uint8_t *in_array, short height, short width,
        uint8_t *out_array, short out_height, short out_width);
void image_seg(uint8_t *in_array, short height, short width,
        uint8_t *out_array, short out_height, short out_width);
int main(void)
{
    rt_pin_mode(PIN_KEY, PIN_MODE_INPUT);

    //���ü̵������ƶ˿�Ϊ���ģʽ
    rt_pin_mode(PIN_IN1, PIN_MODE_OUTPUT);
    //Ĭ������Ϊ�͵�ƽ
    rt_pin_write(PIN_IN1, PIN_HIGH);


    //���ù���Ϊ����ģʽ
    rt_pin_mode(PIN_TUBE, PIN_MODE_INPUT_PULLUP);
    //����Ϊ�����ش������жϺ���Ϊtube_on
    rt_pin_attach_irq(PIN_TUBE, PIN_IRQ_MODE_FALLING, tube_on, RT_NULL);
    //ʹ���ж�
    rt_pin_irq_enable(PIN_TUBE, PIN_IRQ_ENABLE);

    /*��ʼ�� wifi �Զ�����*/
    wlan_autoconnect_init();
    /* ʹ�� wifi �Զ����� */
    rt_wlan_config_autoreconnect(RT_TRUE);
    rt_thread_delay(rt_tick_from_millisecond(3* 1000));
    /* init spi data notify event */
    rt_event_init(&ov2640_event, "ov2640", RT_IPC_FLAG_FIFO);

    /* struct lcd init */
    struct drv_lcd_device *lcd;
    struct rt_device_rect_info rect_info = {0, 0, 320, 240};
    lcd = (struct drv_lcd_device *)rt_device_find("lcd");

    /* ai:��ȡģ�;��model*/
    static rt_ai_t model = NULL;
    model = rt_ai_find(RT_AI_MODEL_FULL_MODEL_NAME);
    if(!model) {rt_kprintf("ai model find err\r\n"); return -1;}

    /* Ϊ����ͷ��ȡ��ͼƬ����ռ� */
    uint8_t *input_image = rt_malloc(AI_MODEL_FULL_IN_1_SIZE);
    if (!input_image) {rt_kprintf("malloc err\n"); return -1;}

    /* ai:Ϊģ�����������Ӧ�ռ� */
    float *input = rt_malloc(AI_MODEL_FULL_IN_1_SIZE_BYTES);
    if (!input) {rt_kprintf("malloc err\n"); return -1;}

    /* ai:Ϊģ�ͷ������пռ� */
    uint8_t *work_buf = rt_malloc(RT_AI_MODEL_FULL_WORK_BUFFER_BYTES);
    if (!work_buf) {rt_kprintf("malloc err\n"); return -1;}

    /* ai:Ϊģ��������䴢��ռ� */
    float *_out = rt_malloc(AI_MODEL_FULL_OUT_7_SIZE_BYTES);
    if (!_out) {rt_kprintf("malloc err\n"); return -1;}

    // allocate output memory
    uint8_t *out_image = rt_malloc(AI_MODEL_FULL_IN_1_SIZE);
    if (!out_image) {rt_kprintf("malloc err\n"); return -1;}

    // ����ȱ����ɫ��Ĥ
    uint8_t *added_prediction = rt_malloc(AI_MODEL_FULL_IN_1_SIZE);
    if (!added_prediction) {rt_kprintf("malloc err\n"); return -1;}

    // ȱ����ɫ
    uint8_t *pseudo_color_prediction = rt_malloc(AI_MODEL_FULL_IN_1_SIZE);
    if (!pseudo_color_prediction) {rt_kprintf("malloc err\n"); return -1;}


    /* ai:��ʼ��������aiģ�͵���������ռ� */
    uint8_t model_init = rt_ai_init(model, work_buf);
    if (model_init != 0) {rt_kprintf("ai init err\n"); return -1;}
    rt_ai_config(model, CFG_INPUT_0_ADDR, input);
    rt_ai_config(model, CFG_OUTPUT_0_ADDR, _out);
    rt_kprintf("over");
    ai_camera();
    /* gc0328c & LCD working */
    while(1)
    {
        rt_event_recv(&ov2640_event,
                            1,
                            RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR,
                            RT_WAITING_FOREVER,
                            RT_NULL);

        lcd->parent.control(&lcd->parent, RTGRAPHIC_CTRL_RECT_UPDATE, &rect_info);
        if(flag_of_tube)
                {
                    rt_kprintf("start\n");
                    // resize
                    image_seg(lcd->lcd_info.framebuffer, 240, 320, input_image, 240, 240);
                    lcd_show_image(0, 0, 240, 240, input_image);
                    onenet_mqtt_update_data_entry(input_image, 1);
                    Normalization(input_image, input, 240, 240, 3);

                    rt_ai_run(model, NULL, NULL);
                    float *out = (float *)rt_ai_output(model, 6);

                    argmax(out, pseudo_color_prediction, 6, 240, 240);
                    addWeighted(input_image,0.6,pseudo_color_prediction,0.4,0,added_prediction);
                    //lcd_show_image(240,0,240,240, pseudo_color_prediction);
                    lcd_show_image(0,240,240,240, added_prediction);


                    onenet_mqtt_update_data_entry(added_prediction, 2);
                    rt_pin_write(PIN_IN1, PIN_HIGH);
                    flag_of_tube = 0;
                }
        DCMI_Start();

    }
    rt_free(input_image);
    rt_free(input);
    rt_free(added_prediction);
    rt_free(pseudo_color_prediction);
    rt_free(out_image);
        rt_free(work_buf);
        rt_free(_out);
        return 0;
}
static void onenet_mqtt_update_data_entry(uint8_t *image_data, int mode)
{
    //ͼƬ����������ָ��
    unsigned char* post_data;
    //rt_thread_delay(rt_tick_from_millisecond(15 * 1000));//�ȴ� wifi ���ӳɹ� ��������
    //����bmpͼƬ
    post_data = bmp_gen_test(240, 240, image_data);
    //POST��������
    webclient_post_test(post_data, mode);
    //�ͷ���bmp_gen_test��������ڴ棬�����´��ٴ�����ʹ��
    rt_free(post_data);

}
void ai_camera()
{
    rt_gc0328c_init();
    ai_flag = 1;
    DCMI_Start();
}
/**
 * ��ɫ��ԭʼͼ���ںϺ���
 */
void addWeighted(uint8_t *src1, float alpha, uint8_t *src2, float beta, float gamma, uint8_t *dst)
{
    int i = 0;
    int num = AI_MODEL_FULL_OUT_7_HEIGHT * AI_MODEL_FULL_OUT_7_WIDTH * 3;
    while (i < num)
    {
        if(src2[0] == 128 && src2[1] == 0 && src2[2] == 0)
        {
            for(int j = 0; j < 3; j++)
            {
                *dst++ = *src1++;
                src2++;
            }
            i = i + 3;
        }
        else
        {
            for(int j = 0; j < 3; j++)
            {
                *dst++ = (int)((float)*src1++ * alpha + (float)*src2++ * beta + gamma);
            }
            i = i + 3;
        }
    }
}
/**
 * ��һ��������-1~1
 */
void Normalization(uint8_t *src, float *dst, int height, int width, int channels)
{
    for(int i = 0; i < height;i++){
        for(int j = 0; j < width;j++){
            for(int k = 0; k < channels;k++){
                *dst++ = (float)*src++ / 127.5f - 1.0f;
            }
        }
    }
}
/**
 * ���ߺ���
 */
int is_in_array(short x, short y, short height, short width)
{
    if (x >= 0 && x < width && y >= 0 && y < height)
        return 1;
    else
        return 0;
}
/**
 * ��ͨ�������ֵ����ţ�ͬʱ�����ӳ�䵽��Ӧ��ɫ
 */
void argmax(float *src, uint8_t *dst, int channels, int width, int height)
{
    float temp[channels];
    for (int i = 0; i < width*height; i++)
    {
        //�õ�ÿ��ͨ����ֵ
        for(int j = 0; j < channels; j++)
        {
            temp[j] = *src++;
        }
        //��ȡ���ֵ���±�
        float max = temp[0];
        //���������������±�ֵ
        int index = 0;
        for (int k = 1; k < channels; k++)
        {
            if (max <= temp[k])
            {
                max = temp[k];
                index = k;
            }
        }
        //�õ�ÿ��ͨ����ֵ
        for(int j = 0; j < 3; j++)
        {
            *dst++ = color_map[index][j];
        }
    }
}

void image_seg(uint8_t* in_array, short height, short width,
        uint8_t* out_array, short out_height, short out_width)
{
    for(int i = 0; i < out_height; i++){
        for(int j = 0; j < out_width; j++){
            for(int k = 0; k < 3; k++){
                *out_array++ = *in_array++;
            }
        }
        for(int n = 0; n < width - out_width; n++){
            for(int m = 0; m < 3; m++){
                in_array++;
            }
        }
    }
}

/**
 * ˫���Բ�ֵ����
 */
void bilinera_interpolation(uint8_t* in_array, short height, short width,
        uint8_t* out_array, short out_height, short out_width)
{
    double h_times = (double)out_height / (double)height,
           w_times = (double)out_width / (double)width;
    short  x1, y1, x2, y2, f11, f12, f21, f22;
    double x, y;

    for (int i = 0; i < out_height; i++){
        for (int j = 0; j < out_width*3; j=j+3){
            for (int k =0; k <3; k++){
                x = j / w_times + k;
                y = i / h_times;

                x1 = (short)(x - 3);
                x2 = (short)(x + 3);
                y1 = (short)(y + 1);
                y2 = (short)(y - 1);
                f11 = is_in_array(x1, y1, height, width*3) ? in_array[y1*width*3+x1] : 0;
                f12 = is_in_array(x1, y2, height, width*3) ? in_array[y2*width*3+x1] : 0;
                f21 = is_in_array(x2, y1, height, width*3) ? in_array[y1*width*3+x2] : 0;
                f22 = is_in_array(x2, y2, height, width*3) ? in_array[y2*width*3+x2] : 0;
                out_array[i*out_width*3+j+k] = (float)(((f11 * (x2 - x) * (y2 - y)) +
                                           (f21 * (x - x1) * (y2 - y)) +
                                           (f12 * (x2 - x) * (y - y1)) +
                                           (f22 * (x - x1) * (y - y1))) / ((x2 - x1) * (y2 - y1)));
            }
        }
    }
}

#include "stm32h7xx.h"
static int vtor_config(void)
{
    /* Vector Table Relocation in Internal QSPI_FLASH */
    SCB->VTOR = QSPI_BASE;
    return 0;
}
INIT_BOARD_EXPORT(vtor_config);


