#include "control.h"	
#include "filter.h"	
  /**************************************************************************
作者：平衡小车之家
我的淘宝小店：http://shop114407458.taobao.com/
**************************************************************************/

u8 Flag_Target,Flag_Change;                             //相关标志位
u8 temp1;                                               //临时变量
float Voltage_Count,Voltage_All; 											  //电压采样相关变量
float Gyro_K=0.004;     				  											//陀螺仪比例系数
int j;
#define a_PARAMETER          (0.275f)                     
/**************************************************************************
函数功能：小车运动数学模型
入口参数：X Y Z 三轴速度或者位置
返回  值：无
**************************************************************************/
void Kinematic_Analysis(float Vy,float Vz)
{
        Target_A   = -Vy-Vz*(a_PARAMETER);
        Target_B   = -Vy+Vz*(a_PARAMETER);
}
/**************************************************************************
函数功能：所有的控制代码都在这里面
         5ms定时中断由MPU6050的INT引脚触发
         严格保证采样和数据处理的时间同步				 
**************************************************************************/
int EXTI15_10_IRQHandler(void) 
{    
	 if(INT==0)		
	{     
		  EXTI->PR=1<<15;                                                      //清除LINE5上的中断标志位  		
		   Flag_Target=!Flag_Target;
		  if(delay_flag==1)
			 {
				 if(++delay_50==10)	 delay_50=0,delay_flag=0;                     //给主函数提供50ms的精准延时
			 }
		  if(Flag_Target==1)                                                  //5ms读取一次陀螺仪和加速度计的值
			{
					if(Usart_Flag==0&&PS2_ON_Flag==0&&Usart_ON_Flag==1)  memcpy(rxbuf,Urxbuf,8*sizeof(u8));	//如果解锁了串口控制标志位，进入串口控制模式
					Read_DMP();                                                           //===更新姿态		
			  	Key();//扫描按键变化	
			    return 0;	                                               
			}                                                                     	 //===10ms控制一次，为了保证M法测速的时间基准，首先读取编码器数据
			UA_Encoder=Read_Encoder(2);                                          //===读取编码器的值		
			Encoder_A=UA_Encoder/8;
			Position_A+=Encoder_A;                                                 //===积分得到速度   
			UB_Encoder=-Read_Encoder(3);                                          //===读取编码器的值		
			Encoder_B=UB_Encoder/8;
			Position_B+=Encoder_B;                                                 //===积分得到速度   
	  	Read_DMP();                                                            	//===更新姿态	
  		Led_Flash(100);                                                       	 //===LED闪烁;常规模式 1s改变一次指示灯的状态	
			Voltage_All+=Get_battery_volt();                                      	 //多次采样累积
			if(++Voltage_Count==100) Voltage=Voltage_All/100,Voltage_All=0,Voltage_Count=0;//求平均值 获取电池电压	
		  if(PS2_KEY==4)PS2_ON_Flag=1,CAN_ON_Flag=0,Usart_ON_Flag=0;									
		  if(CAN_ON_Flag==1||Usart_ON_Flag==1||PS2_ON_Flag==1) CAN_N_Usart_Control();    //接到串口或者CAN遥控解锁指令之后，使能CAN和串口控制输入
			if(Turn_Off(Voltage)==0)               //===如果电池电压不存在异常
			 { 			 	
				 if(CAN_ON_Flag==0&&Usart_ON_Flag==0&&PS2_ON_Flag==0)      Get_RC(Run_Flag);//===串口和CAN控制都未使能，则接收蓝牙遥控指
				 Motor_A=Incremental_PI_A(Encoder_A,Target_A);                         //===速度闭环控制计算电机A最终PWM
				 Motor_B=Incremental_PI_B(Encoder_B,Target_B);                         //===速度闭环控制计算电机B最终PWM
				 Xianfu_Pwm(6900);                     //===PWM限幅
				 Set_Pwm(-Motor_B,Motor_A);     //===赋值给PWM寄存器  
			 }
	 }
	 return 0;	 
} 

/**************************************************************************
函数功能：赋值给PWM寄存器
入口参数：PWM
返回  值：无
**************************************************************************/
void Set_Pwm(int motor_a,int motor_b)
{
   	if(motor_a<0)			INA2=1,			INA1=0;
			else 	          INA2=0,			INA1=1;
		PWMA=myabs(motor_a);
	
		if(motor_b<0)			INB2=1,			INB1=0;
		else 	            INB2=0,			INB1=1;
		PWMB=myabs(motor_b);
}

/**************************************************************************
函数功能：限制PWM赋值 
入口参数：幅值
返回  值：无
**************************************************************************/
void Xianfu_Pwm(int amplitude)
{	
    if(Motor_A<-amplitude) Motor_A=-amplitude;	
		if(Motor_A>amplitude)  Motor_A=amplitude;	
	  if(Motor_B<-amplitude) Motor_B=-amplitude;	
		if(Motor_B>amplitude)  Motor_B=amplitude;		
}
/**************************************************************************
函数功能：按键修改小车运行状态 
入口参数：无
返回  值：无
**************************************************************************/
void Key(void)
{	
	u8 tmp;
	tmp=click_N_Double(100); 
	if(tmp==2)Flag_Show=!Flag_Show;//双击控制显示模式                  
}

/**************************************************************************
函数功能：异常关闭电机
入口参数：电压
返回  值：1：异常  0：正常
**************************************************************************/
u8 Turn_Off( int voltage)
{
	    u8 temp;
			if(voltage<2000||EN==0)//电池电压低于22.2V关闭电机
			{	                                                
      temp=1;      
      PWMA=0;
      PWMB=0;						
      }
			else
      temp=0;
      return temp;			
}

/**************************************************************************
函数功能：绝对值函数
入口参数：long int
返回  值：unsigned int
**************************************************************************/
u32 myabs(long int a)
{ 		   
	  u32 temp;
		if(a<0)  temp=-a;  
	  else temp=a;
	  return temp;
}
/**************************************************************************
函数功能：增量PI控制器
入口参数：编码器测量值，目标速度
返回  值：电机PWM
根据增量式离散PID公式 
pwm+=Kp[e（k）-e(k-1)]+Ki*e(k)+Kd[e(k)-2e(k-1)+e(k-2)]
e(k)代表本次偏差 
e(k-1)代表上一次的偏差  以此类推 
pwm代表增量输出
在我们的速度控制闭环系统里面，只使用PI控制
pwm+=Kp[e（k）-e(k-1)]+Ki*e(k)
**************************************************************************/
int Incremental_PI_A (int Encoder,int Target)
{ 	
	 static int Bias,Pwm,Last_bias;
	 Bias=Encoder-Target;                //计算偏差
	 Pwm+=Velocity_KP*(Bias-Last_bias)+Velocity_KI*Bias;   //增量式PI控制器
	 if(Pwm>7200)Pwm=7200;
	 if(Pwm<-7200)Pwm=-7200;
	 Last_bias=Bias;	                   //保存上一次偏差 
	 return Pwm;                         //增量输出
}
int Incremental_PI_B (int Encoder,int Target)
{ 	
	 static int Bias,Pwm,Last_bias;
	 Bias=Encoder-Target;                //计算偏差
	 Pwm+=Velocity_KP*(Bias-Last_bias)+Velocity_KI*Bias;   //增量式PI控制器
	 if(Pwm>7200)Pwm=7200;
	 if(Pwm<-7200)Pwm=-7200;
	 Last_bias=Bias;	                   //保存上一次偏差 
	 return Pwm;                         //增量输出
}
/**************************************************************************
函数功能：通过串口指令对小车进行遥控
入口参数：串口指令
返回  值：无
**************************************************************************/
void Get_RC(u8 mode)
{

	  if(mode==0)//速度
		{	
				 switch(Flag_Direction)   //方向控制
				 {
				 case 0:      Move_Y=0; 			 			 	 Move_Z=0;        break;	 
				 case 1:      Move_Y=RC_Velocity;  	 	 Move_Z=0;       	break;
				 case 2:      Move_Y=RC_Velocity;  	 	 Move_Z=-RC_Velocity;   	break;
				 case 3:      Move_Y=0;      				 	 Move_Z=-RC_Velocity;   	break;
				 case 4:      Move_Y=-RC_Velocity;  	 Move_Z=RC_Velocity;  	break;
				 case 5:      Move_Y=-RC_Velocity;  	 Move_Z=0;        break;
				 case 6:      Move_Y=-RC_Velocity;  	 Move_Z=-RC_Velocity;    break;
				 case 7:      Move_Y=0;     	 			 	 Move_Z=RC_Velocity;    break;
				 case 8:      Move_Y=RC_Velocity; 	 	 Move_Z=RC_Velocity;    break; 
				 default:     break;	 
			 } 
	 }
		 Kinematic_Analysis(Move_Y,Move_Z);//得到控制目标值，进行运动学分析
}

/**************************************************************************
函数功能：接收CAN或者串口控制指令进行处理
入口参数：无
返回  值：无
**************************************************************************/
void CAN_N_Usart_Control(void)
{
			int flag_1, flag_2;
			int LX,LY;
		  int Yuzhi=20;
			if(CAN_ON_Flag==1||Usart_ON_Flag==1) 
			{
			 if(rxbuf[2])flag_1=1;  else flag_1=-1;  //方向控制位
			 if(rxbuf[3])flag_2=1;  else flag_2=-1;  //方向控制位
			 Target_A=flag_1*rxbuf[0];
			 Target_B=flag_2*rxbuf[1];
			}
			  else if (PS2_ON_Flag==1)
	{
	    LX=PS2_LX-128;
			LY=PS2_LY-128;
			if(LX>-Yuzhi&&LX<Yuzhi)LX=0;
			if(LY>-Yuzhi&&LY<Yuzhi)LY=0;
		  Move_Z= -LX*RC_Velocity/150;
		  Move_Y=-LY*RC_Velocity/150;	 
		  if(Move_Y<0)Move_Z=-Move_Z;
		  Kinematic_Analysis(Move_Y*0.7882,Move_Z*1.3757);//得到控制目标值，进行运动学分析
	}
}
