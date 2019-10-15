#ifndef __MOTOR_H
#define __MOTOR_H
#include <sys.h>	 
  /**************************************************************************
作者：平衡小车之家
我的淘宝小店：http://shop114407458.taobao.com/
**************************************************************************/ 
#define PWMB   TIM8->CCR2 
#define PWMA   TIM8->CCR1 
#define INB1   PCout(12)  
#define INA1   PDout(2)  

 
#define INB2   PBout(5)  
#define INA2   PBout(4)  
#define EN     PAin(12)  
void MiniBalance_PWM_Init(u16 arr,u16 psc);
void MiniBalance_Motor_Init(void);
#endif
