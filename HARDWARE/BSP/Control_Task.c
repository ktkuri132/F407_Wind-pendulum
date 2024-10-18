/*


这个文件 装载了  风力摆的核心部分---控制部分



*/

//使用前取消注释

//使用定时器2中断
//#define __TIM2_IRQn__
//使用外部中断15
#define __EXTI15_10_IRQn__

#include <stm32f4xx.h>
#include <bsp.h>
#include <control.h>
#include <usart.h>
#include <mpu6050.h>
#include <inv_mpu.h>
#include <stm32f4xx_exti.h>
#include <OLED.h>
#include <math.h>


extern float pitch,roll,yaw,dis,def,polar,Opolar;





/*

15cm 500,0,-65.5,00983
25cm 395,0,0.0001,0.0104223  <---这个具有普遍性

750,0,-350,0.0104223

(17+0.73333*Target_angle)
(140.625-Target_angle)/3.75+Target_angle

*/

int de[7]={0,-5,-7,/*150度*/+12.5,/*135度*/ +10.5,/*120度*/ +9.9,/*90度*/-10 };

float Target_dis=0.15;
float Target_angle = 80;
int mode_chose = 5;

#ifdef __EXTI15_10_IRQn__

/// @brief MPU6050读值触发的外部中断函数      
void EXTI15_10_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line15) != RESET)
    {
        EXTI_ClearITPendingBit(EXTI_Line15);
        
        mpu_dmp_get_data(&pitch, &roll, &yaw);      //读取MPU6050数据

        GetPolar(roll,pitch);

        switch (mode_chose)
        {
            case 1:
                Task1_LineMove(Target_dis,Target_angle);
                break;    
                
            case 3:
                Task3_AngleMove(Target_angle,Target_dis);
                break;

            case 4:
                Task4_StopFast();    //调用控制函数--->第4项
                break;

            case 5:
                Task5_CircleMove(0.2);
                break;

            default:break;
                
        }

    }
}



#endif

/*
    第四项  快速制动
*/
void Task4_StopFast()
{
    /*
	
    //垂直制动输出
    static float VerticalOutput;
    //水平制动输出
    static float LevelOutput;
    */
    static float Output;
    static struct PID Taks4Pid={900,-1,0,PidControl_Stop};
    
    Output = Taks4Pid.PIDControl(TargetDis,dis, &Taks4Pid);
    //printf("Output:%f\n",Output);
    //PWM_Allocation(Output);
  
}




float target_angle;

/*
    第一,二项  单摆运动+幅度可调
*/
void Task1_LineMove(float R,uint8_t forward)
{
    static float VOutput,LOutput;
    static float A;
    static float time=0;
	static uint8_t flag_1=0,flag_2=0,flag_3=0;
    /*0度 - 15cm以内*/
    static struct PID Taks1Pid1={750,0,-295,PidControl_LineMove};
    /*0度 - 20~25cm*/
    static struct PID Taks1Pid2={950,0,-320,PidControl_LineMove};
    /*90度 - 15cm*/
    static struct PID Taks1Pid3={720,0,-420,PidControl_LineMove};
    /*90度 - 20~25cm*/
    static struct PID Taks1Pid4={830,0,-200,PidControl_LineMove};


    A = atanf(R/0.86)*180.0f/PI;
    target_angle = A*sinf(2*PI*time/T);
    
    //printf("%f,%f\r\n",target_angle,roll);
    if(forward==0)
    {
        if(R<=0.16)
        {
            VOutput = Taks1Pid1.PIDControl(target_angle,roll, &Taks1Pid1);
            LOutput = Taks1Pid1.PIDControl(0,pitch, &Taks1Pid1);
        }
        else if(R>=0.17)
        {
            VOutput = Taks1Pid2.PIDControl(target_angle,roll, &Taks1Pid2);
            LOutput = Taks1Pid2.PIDControl(0,pitch, &Taks1Pid2);
        }
        
    }
    

//0.00983---0.15
    time+=0.0104220;//到位，到位
    if(time<T)
    {
        if(forward==0)
        {
            if(VOutput>0)
            {
                Motor_Cmd(VerticalOut,ENABLE);
                Motor_Cmd(VerticalIn,DISABLE);
                Motor->MVerticalOut =(uint32_t)VOutput;
            }
            else
            {
                Motor_Cmd(VerticalOut,DISABLE);
                Motor_Cmd(VerticalIn,ENABLE);
                Motor->MVerticalIn =(uint32_t)(-VOutput);
            }
        }
    }
    else time=0;
    
}



/*
    第三项  角度可调的单摆运动
*/
void Task3_AngleMove(float angle,float R)
{

    static float VOutput,LOutput;
    static float target_roll,target_pitch;
    static float time=0;
    static float Vtarget_angle,Ltarget_angle;
    
    static struct PID Taks3Pid1={380,0,-110,PidControl_LineMove};
    static struct  PID Taks3Pid2={380,0,-110,PidControl_LineMove};
    
    static struct PID Taks3Pid3={340,0,-90,PidControl_LineMove};
    static struct  PID Taks3Pid4={340,0,-90,PidControl_LineMove};

    static struct PID Taks3Pid5={380,0,-110,PidControl_LineMove};
    static struct  PID Taks3Pid6={380,0,-110,PidControl_LineMove};

    static struct PID Taks3Pid7={340,0,-90,PidControl_LineMove};
    static struct  PID Taks3Pid8={340,0,-90,PidControl_LineMove};

    int a;
    if(angle>90)
    {   
        //angle = (140.625-angle)/3.75+angle,0.15;
        angle = 180 - angle;
        //这里搞多点是因为浮点型读值不确定
        if(angle<=31)   
        {
            angle += de[3];
        }
        else if((angle>31)&&(angle<=46))
        {
            angle += de[4];
        }
        else if((angle>46)&&(angle<=61))
        {
            angle += de[5];
        }
        a=2;
    }
    
    else if(angle<90)
    {
        //angle = (17+0.73333*angle);
        if(angle<=31)
        {
            angle += de[0];
        }
        else if((angle>31)&&(angle<=46))
        {
            angle += de[1];
        }
        else if((angle>46)&&(angle<=61))
        {
            angle += de[2];
        }

        a=1;
    }
    else
    {
        angle +=de[6];
    }
    
   // angle = angle+5;
    angle = angle*PI/180.0f;

    target_roll = atanf(R*cosf(angle)/0.86)/PI*180;
    target_pitch = atanf(R*sinf(angle)/0.86)/PI*180;
	
//	printf("%f,%f\r\n",A1,A2);

    float wt = 2*PI*(time/T);

    Vtarget_angle =sinf(wt)*target_roll;
    Ltarget_angle =sinf(wt)*target_pitch;
 
    angle = (angle * 180) / PI;
	//printf("%f,%f\r\n",VOutput,LOutput);
/**************************************小于90度**************************************************************** */
    if(a==1)
    {
        printf("%f,%f\r\n",angle,PI/6);
        if(angle<=30+de[0])
        {
            VOutput = Taks3Pid1.PIDControl(Vtarget_angle,roll,&Taks3Pid1);
            LOutput = Taks3Pid2.PIDControl(Ltarget_angle,-pitch,&Taks3Pid2);

            time +=0.0104223;
            if(time<T)
            {
                T3Motor_CmdCombination(VOutput,LOutput,1);
            }
            else
            {
                time=0;
            }
        }
        else if((angle>30+de[0])&&(angle<=45+de[1]))
        {
            VOutput = Taks3Pid3.PIDControl(Vtarget_angle,roll,&Taks3Pid3);
            LOutput = Taks3Pid4.PIDControl(Ltarget_angle,-pitch,&Taks3Pid4);

            time +=0.0101223;
            if(time<T)
            {
                T3Motor_CmdCombination(VOutput,LOutput,1);
            }
            else
            {
                time=0;
            }
        }
        else if((angle>45+de[1])&&(angle<=60+de[2]))
        {
            VOutput = Taks3Pid5.PIDControl(Vtarget_angle,roll,&Taks3Pid5);
            LOutput = Taks3Pid6.PIDControl(Ltarget_angle,-pitch,&Taks3Pid6);

            time +=0.0102323;
            if(time<T)
            {
                T3Motor_CmdCombination(VOutput,LOutput,1);
            }
            else
            {
                time=0;
            }
        }
        else if((angle>60+de[2])&&(angle<=80))//90度
        {
            VOutput = Taks3Pid7.PIDControl(Vtarget_angle,roll,&Taks3Pid7);
            LOutput = Taks3Pid8.PIDControl(Ltarget_angle,-pitch,&Taks3Pid8);

            time +=0.0103423;
            if(time<T)
            {
                T3Motor_CmdCombination(VOutput,LOutput,1);
            }
            else
            {
                time=0;
            }
        }
        
    }
/*****************************************大于90度************************************************************* */
    else if(a==2)
    {
        if(angle<=30+de[3])     //150   
        {
            VOutput = Taks3Pid1.PIDControl(Vtarget_angle,-roll,&Taks3Pid1);
            LOutput = Taks3Pid2.PIDControl(Ltarget_angle,-pitch,&Taks3Pid2);

            time +=0.0104123;
            if(time<T)
            {
                T3Motor_CmdCombination(VOutput,LOutput,0);
            }
            else
            {
                time=0;
            }
        }
        else if((angle>30+de[3])&&(angle<=45+de[4]))     //135 
        {
            VOutput = Taks3Pid3.PIDControl(Vtarget_angle,-roll,&Taks3Pid3);
            LOutput = Taks3Pid4.PIDControl(Ltarget_angle,-pitch,&Taks3Pid4);

            time +=0.0103423;
            if(time<T)
            {
                T3Motor_CmdCombination(VOutput,LOutput,0);
            }
            else
            {
                time=0;
            }
        }
        else if((angle>45+de[4])&&(angle<=60+de[5]))     //120 
        {
            VOutput = Taks3Pid5.PIDControl(Vtarget_angle,-roll,&Taks3Pid5);
            LOutput = Taks3Pid6.PIDControl(Ltarget_angle,-pitch,&Taks3Pid6);

            time +=0.0103323;
            if(time<T)
            {
                T3Motor_CmdCombination(VOutput,LOutput,0);
            }
            else
            {
                time=0;
            }
        }
        
        
    }
/******************************************等于90*********************************************************** */

    
}


/*
    画圆
*/
void Task5_CircleMove(float R)
{
    static float VOutput,LOutput;
    static float A;
    static float time=0;
    static float Vtarget_angle,Ltarget_angle;
    
    static struct PID Taks5Pid={300,0,0,PidControl_LineMove};
    static struct PID Taks5Pid2={300,0,0,PidControl_LineMove};

    A = asinf(R/0.86)*180.0f/PI;

    Vtarget_angle = A*sinf(2*PI*(time/T));
    Ltarget_angle = A*cosf(2*PI*(time/T));

    float feedback_angle = A*sinf(polar/180*PI);

    int sit;

    printf("%f,%f\r\n",Vtarget_angle,feedback_angle);

    time+=0.006;
    if(time<T)
    {
        if((roll>0)&&(pitch<0))  //1   
        {
            sit=1;
            VOutput = Taks5Pid.PIDControl(Vtarget_angle,roll,&Taks5Pid);
            LOutput = Taks5Pid2.PIDControl(Ltarget_angle,-pitch,&Taks5Pid2);
            
        }
        else if((roll<0)&&(pitch<0))    //2
        {
            sit=2;
            VOutput = Taks5Pid.PIDControl(Vtarget_angle,-roll,&Taks5Pid);
            LOutput = Taks5Pid2.PIDControl(Ltarget_angle,pitch,&Taks5Pid2);
        }
        else if((roll<0)&&(pitch>0))    //3
        {
            sit=3;
            VOutput = Taks5Pid.PIDControl(Vtarget_angle,roll,&Taks5Pid);
            LOutput = Taks5Pid2.PIDControl(Ltarget_angle,-pitch,&Taks5Pid2);
        }
        else if((roll>0)&&(pitch>0))    //4
        {
            sit=4;
            VOutput = Taks5Pid.PIDControl(Vtarget_angle,-roll,&Taks5Pid);
            LOutput = Taks5Pid2.PIDControl(Ltarget_angle,pitch,&Taks5Pid2);
        }
        T5Motor_CmdCombination(sit,VOutput,LOutput);
    }

    else time=0;



    

    
}