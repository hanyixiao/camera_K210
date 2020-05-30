/* Copyright 2018 Canaan Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdio.h>
#include <string.h>
#include "dvp.h"
#include "fpioa.h"
#include "lcd.h"
#include "ov5640.h"
#include "ov2640.h"
#include "gc0328.h"
#include "cambus.h"
#include "plic.h"
#include "sysctl.h"
#include "uarths.h"
#include "nt35310.h"
#include "board_config.h"
#include "sleep.h"
#include "sdcard.h"
#include "ff.h"
#include "i2s.h"
#include "rgb2bmp.h"
#include <stdlib.h>
#include "picojpeg.h"
#include "picojpeg_util.h"

uint32_t g_lcd_gram0[38400] __attribute__((aligned(64)));
uint32_t g_lcd_gram1[38400] __attribute__((aligned(64)));

volatile uint8_t g_dvp_finish_flag;
volatile uint8_t g_ram_mux;

static int on_irq_dvp(void *ctx)
{
    if (dvp_get_interrupt(DVP_STS_FRAME_FINISH))
    {
        /* switch gram */
        dvp_set_display_addr(g_ram_mux ? (uint32_t)g_lcd_gram0 : (uint32_t)g_lcd_gram1);

        // dvp_clear_interrupt(DVP_STS_FRAME_FINISH);
        dvp_clear_interrupt(DVP_STS_FRAME_START | DVP_STS_FRAME_FINISH);
        g_dvp_finish_flag = 1;
    }
    else
    {
        if (g_dvp_finish_flag == 0)
            dvp_start_convert();
        dvp_clear_interrupt(DVP_STS_FRAME_START);
    }

    return 0;
}

/**
 *	dvp_pclk	io47
 *	dvp_xclk	io46
 *	dvp_hsync	io45
 *	dvp_pwdn	io44
 *	dvp_vsync	io43
 *	dvp_rst		io42
 *	dvp_scl		io41
 *	dvp_sda		io40
 * */

/**
 *  lcd_cs	    36
 *  lcd_rst	37
 *  lcd_dc	    38
 *  lcd_wr 	39
 * */

static void io_mux_init(void)
{

#if BOARD_LICHEEDAN
    /* Init DVP IO map and function settings */
    fpioa_set_function(42, FUNC_CMOS_RST);
    fpioa_set_function(44, FUNC_CMOS_PWDN);
    fpioa_set_function(46, FUNC_CMOS_XCLK);
    fpioa_set_function(43, FUNC_CMOS_VSYNC);
    fpioa_set_function(45, FUNC_CMOS_HREF);
    fpioa_set_function(47, FUNC_CMOS_PCLK);
    fpioa_set_function(41, FUNC_SCCB_SCLK);
    fpioa_set_function(40, FUNC_SCCB_SDA);

    /* Init SPI IO map and function settings */
    fpioa_set_function(37, FUNC_GPIOHS0 + RST_GPIONUM);
    fpioa_set_function(38, FUNC_GPIOHS0 + DCX_GPIONUM);
    fpioa_set_function(36, FUNC_SPI0_SS3);
    fpioa_set_function(39, FUNC_SPI0_SCLK);
    
    sysctl_set_spi0_dvp_data(1);
    /* Init sdcard IO map and function setttings */
    //io28--mosi
    //io26--miso
    //io29--cs
    //io27--clk
    // fpioa_set_function(27, FUNC_SPI0_SCLK); //clk
    // fpioa_set_function(28, FUNC_SPI0_D0);   //MOSI
    // fpioa_set_function(26, FUNC_SPI0_D1);   //MISO
    // fpioa_set_function(29, FUNC_GPIOHS7);   //cs
    fpioa_set_function(27, FUNC_SPI1_SCLK); //clk
    fpioa_set_function(28, FUNC_SPI1_D0);   //MOSI
    fpioa_set_function(26, FUNC_SPI1_D1);   //MISO
    fpioa_set_function(29, FUNC_GPIOHS7);   //cs
    // fpioa_set_function(29, FUNC_SPI0_SS3);

    // fpioa_set_function(32, FUNC_I2S0_OUT_D0);
    // fpioa_set_function(34, FUNC_I2S0_SCLK);
    // fpioa_set_function(31, FUNC_I2S0_WS);
#else
    /* Init DVP IO map and function settings */
    fpioa_set_function(11, FUNC_CMOS_RST);
    fpioa_set_function(13, FUNC_CMOS_PWDN);
    fpioa_set_function(14, FUNC_CMOS_XCLK);
    fpioa_set_function(12, FUNC_CMOS_VSYNC);
    fpioa_set_function(17, FUNC_CMOS_HREF);
    fpioa_set_function(15, FUNC_CMOS_PCLK);
    fpioa_set_function(10, FUNC_SCCB_SCLK);
    fpioa_set_function(9, FUNC_SCCB_SDA);

    /* Init SPI IO map and function settings */
    fpioa_set_function(8, FUNC_GPIOHS0 + DCX_GPIONUM);
    fpioa_set_function(6, FUNC_SPI0_SS3);
    fpioa_set_function(7, FUNC_SPI0_SCLK);

    sysctl_set_spi0_dvp_data(1);

#endif
}

static void io_set_power(void)
{
#if BOARD_LICHEEDAN
    /* Set dvp and spi pin to 1.8V */
    sysctl_set_power_mode(SYSCTL_POWER_BANK6, SYSCTL_POWER_V18);
    sysctl_set_power_mode(SYSCTL_POWER_BANK7, SYSCTL_POWER_V18);
#else
    /* Set dvp and spi pin to 1.8V */
    sysctl_set_power_mode(SYSCTL_POWER_BANK1, SYSCTL_POWER_V18);
    sysctl_set_power_mode(SYSCTL_POWER_BANK2, SYSCTL_POWER_V18);
#endif
}
static int sdcard_test(void)
{
    uint8_t status;

    printf("/******************sdcard test*****************/\r\n");
    status = sd_init();
    printf("sd init %d\r\n", status);
    if (status != 0)
    {
        return status;
    }

    printf("card info status %d\r\n", status);
    printf("CardCapacity:%ld\r\n", cardinfo.CardCapacity);
    printf("CardBlockSize:%d\r\n", cardinfo.CardBlockSize);
    return 0;
}

static int fs_test(void)
{
    static FATFS sdcard_fs;
    FRESULT status;
    DIR dj;
    FILINFO fno;

    printf("/********************fs test*******************/\r\n");
    status = f_mount(&sdcard_fs, _T("0:"), 1);
    printf("mount sdcard:%d\r\n", status);
    if (status != FR_OK)
        return status;

    printf("printf filename\r\n");
    status = f_findfirst(&dj, &fno, _T("0:"), _T("*"));
    while (status == FR_OK && fno.fname[0])
    {
        if (fno.fattrib & AM_DIR)
            printf("dir:%s\r\n", fno.fname);
        else
            printf("file:%s\r\n", fno.fname);
        status = f_findnext(&dj, &fno);
    }
    f_closedir(&dj);
    return 0;
}
FRESULT sd_read_img(TCHAR *path,uint8_t *src_img,int len)
{
    FIL file;
    FRESULT ret=FR_OK;
    uint32_t v_ret_len;
    if((ret=f_open(&file,path,FA_READ))==FR_OK)
    {
        ret = f_read(&file,(void *)src_img,len,&v_ret_len);
        if(ret !=FR_OK)
        {
            printf("[ERROR]:read file error\n");
        }
        else 
        {
            printf("[INFO]:read file is %s",src_img);
        }
        f_close(&file);
    }
    return ret;
}

int main(void)
{
    uint64_t core_id = current_coreid();

    if (core_id == 0)
    {
        /* Set CPU and dvp clk */
        sysctl_pll_set_freq(SYSCTL_PLL0, 800000000UL);
        sysctl_pll_set_freq(SYSCTL_PLL1, 300000000UL);
        sysctl_pll_set_freq(SYSCTL_PLL2, 45158400UL);
        uarths_init();

        io_mux_init();
       // dmac_init();
        io_set_power();
        plic_init();
        sysctl_enable_irq();

        /* LCD init */
        printf("LCD init\n");
        lcd_init();
#if BOARD_LICHEEDAN
#if OV5640
        lcd_set_direction(DIR_YX_RLUD);
#elif OV2640
        lcd_set_direction(DIR_YX_RLDU);
#else 
        printf("[INFO] change the direction\n");
        lcd_set_direction(DIR_YX_LRUD);
#endif

        lcd_clear(WHITE);
        lcd_draw_string(LCD_X_MAX/2,LCD_Y_MAX/2-40,"LCD_INIT",RED);
        while(sdcard_test())
        {
            printf("SD card err\r\n");
            return -1;
        }
        if (fs_test())
        {
            printf("FAT32 err\r\n");
            return -1;
        }
         /* DVP init */
        printf("DVP init\n");
#if OV5640
        dvp_init(16);
        dvp_enable_burst();
        dvp_set_output_enable(0, 1);
        dvp_set_output_enable(1, 1);
        dvp_set_image_format(DVP_CFG_RGB_FORMAT);
        dvp_set_image_size(320, 240);
        ov5640_init();
#elif OV2640
        dvp_init(8);
        dvp_set_xclk_rate(24000000);
        dvp_enable_burst();
        dvp_set_output_enable(0, 1);
        dvp_set_output_enable(1, 1);
        dvp_set_image_format(DVP_CFG_RGB_FORMAT);
        dvp_set_image_size(320, 240);
        ov2640_init();
#else
        printf("init gc0382\n");
        dvp_init(8);
        dvp_set_xclk_rate(12000000);
        dvp_enable_burst();
        dvp_set_output_enable(0, 1);
        dvp_set_output_enable(1, 1);
        dvp_set_image_format(DVP_CFG_YUV_FORMAT);
        dvp_set_image_size(320, 240);
        cambus_init(8, 2, 41, 40, 0, 0);

    	int id = cambus_scan_gc0328();
    	if (id != 0) {
    		printf("[INFO]: find gc3028\n");
    	}

        gc0328_reset();
        gc0328_init();
        lcd_clear(WHITE);
        lcd_draw_string(LCD_X_MAX/2,LCD_Y_MAX/2-40,"GC0328 Init",RED);
        msleep(200);
#endif
#endif
        printf("[INFO]:begin to read jpeg\n");
        uint64_t tm= sysctl_get_time_us();
        uint8_t *img=(uint8_t *)malloc(43756);
        sd_read_img("0:deshakase.jpg",img,43756);
        jpeg_image_t *jpeg = pico_jpeg_decode(img,43756);
        jpeg_display(0,0,jpeg);
        printf("[INFO]:decode use time %ld ms\n",(sysctl_get_time_us()-tm)/1000);
        msleep(5000);
        free(jpeg->img_data);
        free(jpeg);
        free(img);
        dvp_set_ai_addr((uint32_t)0x40600000, (uint32_t)0x40612C00, (uint32_t)0x40625800);
        dvp_set_display_addr((uint32_t)g_lcd_gram0);
        dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, 0);
        dvp_disable_auto();

        /* DVP interrupt config */
        printf("DVP interrupt config\r\n");
        plic_set_priority(IRQN_DVP_INTERRUPT, 1);
        plic_irq_register(IRQN_DVP_INTERRUPT, on_irq_dvp, NULL);
        plic_irq_enable(IRQN_DVP_INTERRUPT);
        /* enable global interrupt */
        // sysctl_enable_irq();
     

        /* system start */
        printf("system start\r\n");
        g_ram_mux = 0;
        g_dvp_finish_flag = 0;
        dvp_clear_interrupt(DVP_STS_FRAME_START | DVP_STS_FRAME_FINISH);
        dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, 1);
        lcd_draw_string(LCD_X_MAX/2,LCD_Y_MAX/2-40,"System Boot Up",RED);
        msleep(200);
        char *src_img=(char *)malloc(320*240*2);
        while (1)
        {
            
            /* ai cal finish*/
            while (g_dvp_finish_flag == 0)
                ;
            g_dvp_finish_flag = 0;
            /* display pic*/
            g_ram_mux ^= 0x01;
            lcd_draw_picture(0, 0, 320, 240, g_ram_mux ? g_lcd_gram0 : g_lcd_gram1);
            
            lcd_draw_string(0,0,"video mode",RED);
            static int number;
            char file_name[1024];
            sprintf(file_name,"0:photo%d.bmp",number++);
           // printf("[INFO] sizeof the g_lcd_gram0 %ld",sizeof(g_lcd_gram0));
            memcpy(src_img,(char *)g_lcd_gram0,sizeof(g_lcd_gram0));
          //  printf(src_img);
        //    sd_write_test(file_name,src_img);
            if(number>10&&number<15){
            rgb565tobmp(src_img,320,240,file_name);
            }

            // );
        }
        free(src_img);
    }
    while (1)
        ;

    return 0;
}
