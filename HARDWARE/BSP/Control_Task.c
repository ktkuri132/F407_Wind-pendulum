/*


这个文件 装载了  风力摆的核心部分---控制部分



*/

//选择模式的宏定义
#define Task4

#include <stm32f4xx.h>
#include <bsp.h>
#include <control.h>
#include <usart.h>
#include <mpu6050.h>
#include <inv_mpu.h>
#include <stm32f4xx_exti.h>
#include <OLED.h>

//方便调试，定义电机方位
#define Motor           TIM8
#define MVerticalIn     CCR1    //垂直方向外侧电机,M--电机前缀，Vertical--电机方向垂直方向，Out--电机位置外侧电机
#define MVerticalOut    CCR2    //垂直方向内侧电机
#define MLevelOut       CCR3    //水平方向外侧电机
#define MLevelIn        CCR4    //水平方向内侧电机


#ifdef Task4
#define TargetRoll  0
#define TargetPitch 0
#define TargetDis   0
#endif


extern float pitch,roll,yaw;


//控制制动PID参数
float T4SKp=500,
      T4SKi=0,
      T4SKd=-300;



/// @brief MPU6050读值触发的外部中断函数      
void EXTI15_10_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line15) != RESET)
    {
        EXTI_ClearITPendingBit(EXTI_Line15);
        
        mpu_dmp_get_data(&pitch, &roll, &yaw);      //读取MPU6050数据
        
        GetPolar(roll, pitch);    

        //Task4_StopFast();    //调用控制函数--->第4项

    }
}


/*
    第四项  快速制动
*/
void Task4_StopFast()
{
    //垂直制动输出
    static float VerticalOutput;
    //水平制动输出
    static float LevelOutput;
    
    VerticalOutput=PidControl_Stop(TargetRoll, roll);
    LevelOutput= PidControl_Stop(TargetPitch, pitch);

    //VerticalOutput>8400?VerticalOutput=8400:(VerticalOutput<0?VerticalOutput=0:VerticalOutput);
    //LevelOutput>8400?LevelOutput=8400:(LevelOutput<0?LevelOutput=0:LevelOutput);

    VerticalOutput=VerticalOutput>8400?8400:(VerticalOutput<0?(-VerticalOut):VerticalOutput);
    LevelOutput=LevelOutput>8400?8400:(LevelOutput<0?(-LevelOutput):LevelOutput);
    //printf("VerticalOutput:%f  LevelOutput:%f\n",VerticalOutput,LevelOutput);
    Motor->MVerticalOut=(uint32_t)VerticalOutput;
    Motor->MVerticalIn=(uint32_t)VerticalOutput;
    Motor->MLevelOut=(uint32_t)LevelOutput;
    Motor->MLevelIn=(uint32_t)LevelOutput;
  
}