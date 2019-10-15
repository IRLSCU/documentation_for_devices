#include "control.h"	
#include "filter.h"	
  /**************************************************************************
���ߣ�ƽ��С��֮��
�ҵ��Ա�С�꣺http://shop114407458.taobao.com/
**************************************************************************/

u8 Flag_Target,Flag_Change;                             //��ر�־λ
u8 temp1;                                               //��ʱ����
float Voltage_Count,Voltage_All; 											  //��ѹ������ر���
float Gyro_K=0.004;     				  											//�����Ǳ���ϵ��
int j;
#define a_PARAMETER          (0.275f)                     
/**************************************************************************
�������ܣ�С���˶���ѧģ��
��ڲ�����X Y Z �����ٶȻ���λ��
����  ֵ����
**************************************************************************/
void Kinematic_Analysis(float Vy,float Vz)
{
        Target_A   = -Vy-Vz*(a_PARAMETER);
        Target_B   = -Vy+Vz*(a_PARAMETER);
}
/**************************************************************************
�������ܣ����еĿ��ƴ��붼��������
         5ms��ʱ�ж���MPU6050��INT���Ŵ���
         �ϸ�֤���������ݴ����ʱ��ͬ��				 
**************************************************************************/
int EXTI15_10_IRQHandler(void) 
{    
	 if(INT==0)		
	{     
		  EXTI->PR=1<<15;                                                      //���LINE5�ϵ��жϱ�־λ  		
		   Flag_Target=!Flag_Target;
		  if(delay_flag==1)
			 {
				 if(++delay_50==10)	 delay_50=0,delay_flag=0;                     //���������ṩ50ms�ľ�׼��ʱ
			 }
		  if(Flag_Target==1)                                                  //5ms��ȡһ�������Ǻͼ��ٶȼƵ�ֵ
			{
					if(Usart_Flag==0&&PS2_ON_Flag==0&&Usart_ON_Flag==1)  memcpy(rxbuf,Urxbuf,8*sizeof(u8));	//��������˴��ڿ��Ʊ�־λ�����봮�ڿ���ģʽ
					Read_DMP();                                                           //===������̬		
			  	Key();//ɨ�谴���仯	
			    return 0;	                                               
			}                                                                     	 //===10ms����һ�Σ�Ϊ�˱�֤M�����ٵ�ʱ���׼�����ȶ�ȡ����������
			UA_Encoder=Read_Encoder(2);                                          //===��ȡ��������ֵ		
			Encoder_A=UA_Encoder/8;
			Position_A+=Encoder_A;                                                 //===���ֵõ��ٶ�   
			UB_Encoder=-Read_Encoder(3);                                          //===��ȡ��������ֵ		
			Encoder_B=UB_Encoder/8;
			Position_B+=Encoder_B;                                                 //===���ֵõ��ٶ�   
	  	Read_DMP();                                                            	//===������̬	
  		Led_Flash(100);                                                       	 //===LED��˸;����ģʽ 1s�ı�һ��ָʾ�Ƶ�״̬	
			Voltage_All+=Get_battery_volt();                                      	 //��β����ۻ�
			if(++Voltage_Count==100) Voltage=Voltage_All/100,Voltage_All=0,Voltage_Count=0;//��ƽ��ֵ ��ȡ��ص�ѹ	
		  if(PS2_KEY==4)PS2_ON_Flag=1,CAN_ON_Flag=0,Usart_ON_Flag=0;									
		  if(CAN_ON_Flag==1||Usart_ON_Flag==1||PS2_ON_Flag==1) CAN_N_Usart_Control();    //�ӵ����ڻ���CANң�ؽ���ָ��֮��ʹ��CAN�ʹ��ڿ�������
			if(Turn_Off(Voltage)==0)               //===�����ص�ѹ�������쳣
			 { 			 	
				 if(CAN_ON_Flag==0&&Usart_ON_Flag==0&&PS2_ON_Flag==0)      Get_RC(Run_Flag);//===���ں�CAN���ƶ�δʹ�ܣ����������ң��ָ
				 Motor_A=Incremental_PI_A(Encoder_A,Target_A);                         //===�ٶȱջ����Ƽ�����A����PWM
				 Motor_B=Incremental_PI_B(Encoder_B,Target_B);                         //===�ٶȱջ����Ƽ�����B����PWM
				 Xianfu_Pwm(6900);                     //===PWM�޷�
				 Set_Pwm(-Motor_B,Motor_A);     //===��ֵ��PWM�Ĵ���  
			 }
	 }
	 return 0;	 
} 

/**************************************************************************
�������ܣ���ֵ��PWM�Ĵ���
��ڲ�����PWM
����  ֵ����
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
�������ܣ�����PWM��ֵ 
��ڲ�������ֵ
����  ֵ����
**************************************************************************/
void Xianfu_Pwm(int amplitude)
{	
    if(Motor_A<-amplitude) Motor_A=-amplitude;	
		if(Motor_A>amplitude)  Motor_A=amplitude;	
	  if(Motor_B<-amplitude) Motor_B=-amplitude;	
		if(Motor_B>amplitude)  Motor_B=amplitude;		
}
/**************************************************************************
�������ܣ������޸�С������״̬ 
��ڲ�������
����  ֵ����
**************************************************************************/
void Key(void)
{	
	u8 tmp;
	tmp=click_N_Double(100); 
	if(tmp==2)Flag_Show=!Flag_Show;//˫��������ʾģʽ                  
}

/**************************************************************************
�������ܣ��쳣�رյ��
��ڲ�������ѹ
����  ֵ��1���쳣  0������
**************************************************************************/
u8 Turn_Off( int voltage)
{
	    u8 temp;
			if(voltage<2000||EN==0)//��ص�ѹ����22.2V�رյ��
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
�������ܣ�����ֵ����
��ڲ�����long int
����  ֵ��unsigned int
**************************************************************************/
u32 myabs(long int a)
{ 		   
	  u32 temp;
		if(a<0)  temp=-a;  
	  else temp=a;
	  return temp;
}
/**************************************************************************
�������ܣ�����PI������
��ڲ���������������ֵ��Ŀ���ٶ�
����  ֵ�����PWM
��������ʽ��ɢPID��ʽ 
pwm+=Kp[e��k��-e(k-1)]+Ki*e(k)+Kd[e(k)-2e(k-1)+e(k-2)]
e(k)������ƫ�� 
e(k-1)������һ�ε�ƫ��  �Դ����� 
pwm�����������
�����ǵ��ٶȿ��Ʊջ�ϵͳ���棬ֻʹ��PI����
pwm+=Kp[e��k��-e(k-1)]+Ki*e(k)
**************************************************************************/
int Incremental_PI_A (int Encoder,int Target)
{ 	
	 static int Bias,Pwm,Last_bias;
	 Bias=Encoder-Target;                //����ƫ��
	 Pwm+=Velocity_KP*(Bias-Last_bias)+Velocity_KI*Bias;   //����ʽPI������
	 if(Pwm>7200)Pwm=7200;
	 if(Pwm<-7200)Pwm=-7200;
	 Last_bias=Bias;	                   //������һ��ƫ�� 
	 return Pwm;                         //�������
}
int Incremental_PI_B (int Encoder,int Target)
{ 	
	 static int Bias,Pwm,Last_bias;
	 Bias=Encoder-Target;                //����ƫ��
	 Pwm+=Velocity_KP*(Bias-Last_bias)+Velocity_KI*Bias;   //����ʽPI������
	 if(Pwm>7200)Pwm=7200;
	 if(Pwm<-7200)Pwm=-7200;
	 Last_bias=Bias;	                   //������һ��ƫ�� 
	 return Pwm;                         //�������
}
/**************************************************************************
�������ܣ�ͨ������ָ���С������ң��
��ڲ���������ָ��
����  ֵ����
**************************************************************************/
void Get_RC(u8 mode)
{

	  if(mode==0)//�ٶ�
		{	
				 switch(Flag_Direction)   //�������
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
		 Kinematic_Analysis(Move_Y,Move_Z);//�õ�����Ŀ��ֵ�������˶�ѧ����
}

/**************************************************************************
�������ܣ�����CAN���ߴ��ڿ���ָ����д���
��ڲ�������
����  ֵ����
**************************************************************************/
void CAN_N_Usart_Control(void)
{
			int flag_1, flag_2;
			int LX,LY;
		  int Yuzhi=20;
			if(CAN_ON_Flag==1||Usart_ON_Flag==1) 
			{
			 if(rxbuf[2])flag_1=1;  else flag_1=-1;  //�������λ
			 if(rxbuf[3])flag_2=1;  else flag_2=-1;  //�������λ
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
		  Kinematic_Analysis(Move_Y*0.7882,Move_Z*1.3757);//�õ�����Ŀ��ֵ�������˶�ѧ����
	}
}
