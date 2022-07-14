#include <time.h>

#include "rtthread.h"

//-------------------------------------------------------------------------------------------------
// BSP
//-------------------------------------------------------------------------------------------------

#include "bsp.h"
#include "src/hdc2080/hdc2080.h"
#include "ls1b_gpio.h"
#include "ls1x_rtc.h"


#if defined(BSP_USE_FB)
#include "ls1x_fb.h"
#ifdef XPT2046_DRV
char LCD_display_mode[] = LCD_800x480;
#elif defined(GT1151_DRV)
char LCD_display_mode[] = LCD_480x800;
#else
#error "��bsp.h��ѡ������ XPT2046_DRV ���� GT1151_DRV"
"XPT2046_DRV:  ����800*480 �����Ĵ�����."
"GT1151_DRV:   ����480*800 �����Ĵ�����."
"�������ѡ��, ע�͵��� error ��Ϣ, Ȼ���Զ���: LCD_display_mode[]"
#endif
#endif



/* ��Ϣ���п��ƿ� */
struct rt_messagequeue mq;
/* ��Ϣ�������õ��ķ�����Ϣ���ڴ�� */
static rt_uint8_t msg_pool[2048];


static rt_thread_t p_data_show_thread = NULL;

static void data_show_thread(void *arg)
{

    char str[50];
    
    fb_fillrect(50, 50, 600, 150, cidxBLACK);
    fb_textout(50, 50, "��ʪ�ȴ�����");
    fb_textout(50, 70, "CURRENT TEMPERATURE(��ǰ��ʪ��): ");

    for ( ; ; )
    {
        /* ����Ϣ�����н�����Ϣ */
        if (rt_mq_recv(&mq, &str, sizeof(str), 20) == RT_EOK)
        {
             /* ͨ�����ڴ�ӡ���յ������� */
            rt_kprintf(str);
            /* ��LCD����ʾ��ǰ�¶� */
            fb_fillrect(50, 90, 600, 150, cidxBLACK);
            fb_textout(50, 90, str);
        }
        rt_thread_delay(10);   
    }
}

int data_show_create()
{
    /* create the thread */
    p_data_show_thread = rt_thread_create("data_show_thread",
                                          data_show_thread,
                                          NULL,         // arg
                                          1024*20,   // TODO statck size
                                          12,           // TODO priority
                                          10);          // slice ticks

    if (p_data_show_thread == NULL)
    {
        rt_kprintf("create data_show thread fail!\r\n");
        return -1;
    }
    rt_thread_startup(p_data_show_thread);
    return 0;
}


static rt_thread_t sensor_data_get_thread = NULL;

static void sensor_data_get(void *arg)
{
    char str[50];
    rt_err_t result;
    float temp_value=0.0,hum_value=0.0;
    unsigned int tickcount;
    I2C1_init();
        /* ��ʼ����Ϣ���� */
    result = rt_mq_init(&mq,
                        "mqt",
                        &msg_pool[0],             /* �ڴ��ָ�� msg_pool */
                        50,                          /* ÿ����Ϣ�Ĵ�С�� 1 �ֽ� */
                        sizeof(msg_pool),        /* �ڴ�صĴ�С�� msg_pool �Ĵ�С */
                        RT_IPC_FLAG_PRIO);       /* ����ж���̵߳ȴ������ȼ���С�ķ���������Ϣ */
    for ( ; ; )
    {

        HDC_Get_Temp_Hum(&temp_value,&hum_value);
        sprintf(str,"temp %.02f �� hum %.02f% RH%%\r\n", temp_value,hum_value);

        /* ������Ϣ����Ϣ������ */
        result = rt_mq_send(&mq, &str, sizeof(str));
        if (result != RT_EOK)
        {
            rt_kprintf("rt_mq_send ERR\n");
        }

        rt_thread_delay(500);
    }
}

int sensor_data_get_create()
{
    sensor_data_get_thread = rt_thread_create("sensor_data_get_thread",
                             sensor_data_get,
                             NULL,         // arg
                             1024*4,       // statck size
                             11,           // priority
                             10);          // slice ticks

    if (sensor_data_get_thread == NULL)
    {
        rt_kprintf("sensor_data_get_ thread fail!\r\n");
    }
    else
    {
        rt_thread_startup(sensor_data_get_thread);
    }
    return 0;
}


int main(int argc, char** argv)
{
    rt_kprintf("\r\nWelcome to RT-Thread.\r\n\r\n");

    ls1x_drv_init();            /* Initialize device drivers */

    rt_ls1x_drv_init();         /* Initialize device drivers for RTT */

    sensor_data_get_create();

    data_show_create();

    gpio_enable(54,DIR_OUT);
    gpio_write(54,1);

    /*
     * Finsh as another thread...
     */
    return 0;
}

/*
 * @@ End
 */
