#include "sim900a.h"
#include "usart1.h"		
#include "delay.h"	
#include "string.h"    		
#include "lcd.h"



extern const char *Server_IpAddr;//服务器ip
extern const char *Server_Port;//服务器监听端口


/*sim900a模块复位----复位管脚PE4
**复位后等待设备就绪（返回"RDY"字符串）------------>可以发送AT指令
**继续等待网络就绪（返回"Call Ready"字符串）------->可以进行tcp连接
*/
void sim900a_reset( void)
{
	GPIO_SetBits( GPIOE,GPIO_Pin_4);
	delay_ms( 200);
	GPIO_ResetBits( GPIOE,GPIO_Pin_4);
	delay_ms( 200);
	GPIO_SetBits( GPIOE,GPIO_Pin_4);
	
	USART1_RX_STA=0;//初始状态为0
	while(sim900a_send_cmd("","RDY",500));//一直等待RDY字符串---3S
	USART1_RX_STA=0;//初始状态为0
	while(sim900a_send_cmd("","Call Ready",800));//一直等待Call Ready字符串---5S (需要稍微久点)
}
/*
**sim900a根据ip和port连接服务器
**输入： none
**返回值：0-连接成功			1-连接失败
*/
int sim900a_connect( void) 
{
	char server_info[100];		//用于存储服务器连接信息
	//连接模块
	while(sim900a_send_cmd("AT","OK",100))//检测是否应答AT指令 
	{
		LCD_ShowString(50,100,200,16,16,"not found device!");
		delay_ms(800);
		LCD_Fill(50,100,200,100+16,WHITE);
		LCD_ShowString(50,120,200,16,16,"try to connect...");
		delay_ms(400);  
	} 	 
	LCD_Fill(50,120,200,120+16,WHITE);
	sim900a_send_cmd("ATE0","OK",200);//不回显
	LCD_ShowString(50,120,200,16,16,"connect success!");
	//CIPMUX配置-----单点连接
	while(sim900a_send_cmd("AT+CIPMUX=0","OK",500))//检测是否应答AT指令 
	{
		LCD_ShowString(50,160,200,16,16,"CIPMUX SET TO 0");
		delay_ms(800);
		LCD_Fill(50,160,200,160+16,WHITE);
		LCD_ShowString(50,180,200,16,16,"try to SET...");
		delay_ms(400);  
	} 	 
	LCD_Fill(50,180,200,180+16,WHITE);
	LCD_ShowString(50,180,200,16,16,"SET success!");
	//CIPMODE配置---透传模式(后面的CIPCCFG暂时使用默认参数)
	while(sim900a_send_cmd("AT+CIPMODE=1","OK",500))//检测是否应答AT指令 
	{
		LCD_ShowString(50,160,200,16,16,"MODE SET TO 1");
		delay_ms(800);
		LCD_Fill(50,160,200,160+16,WHITE);
		LCD_ShowString(50,180,200,16,16,"try to SET...");
		delay_ms(400);  
	} 	 
	LCD_Fill(50,180,200,180+16,WHITE);
	LCD_ShowString(50,180,200,16,16,"SET success!");
	//AT+CSTT
	while(sim900a_send_cmd("AT+CSTT","OK",500))//检测是否应答AT指令 
	{
		LCD_ShowString(50,220,200,16,16,"SET CSTT");
		delay_ms(800);
		LCD_Fill(50,220,200,220+16,WHITE);
		LCD_ShowString(50,220,200,16,16,"try to SET...");
		delay_ms(400);  
	} 	 
	LCD_Fill(50,240,200,240+16,WHITE);
	LCD_ShowString(50,240,200,16,16,"CSTT SET success!");
	//AT+CIICR
	while(sim900a_send_cmd("AT+CIICR","OK",500))//检测是否应答AT指令 
	{
		LCD_ShowString(50,280,200,16,16,"SET CIICR");
		delay_ms(800);
		LCD_Fill(50,280,200,280+16,WHITE);
		LCD_ShowString(50,280,200,16,16,"try to SET...");
		delay_ms(400);  
	} 	 
	LCD_Fill(50,300,200,300+16,WHITE);
	LCD_ShowString(50,300,200,16,16,"CIICR SET success!");
		//AT+CIFSR
	while(sim900a_send_cmd("AT+CIFSR",".",500))//检测是否应答AT指令 
	{
		LCD_ShowString(50,340,200,16,16,"GET IP");
		delay_ms(800);
		LCD_Fill(50,340,200,340+16,WHITE);
		LCD_ShowString(50,340,200,16,16,"try to SET...");
		delay_ms(400);  
	} 	 
	LCD_Fill(50,360,200,360+16,WHITE);
	LCD_ShowString(50,360,200,16,16,"IP GET success!");
	
	//建立连接
	sprintf((char*)server_info,"AT+CIPSTART=\"%s\",\"%s\",\"%s\"","TCP",Server_IpAddr,Server_Port);
	if(sim900a_send_cmd(server_info,"CONNECT",2000)) return -1;		//发起连接,设备返回OK之后还有CONNECT，干脆直接等待CONNECT字符串
	LCD_ShowString(50,420,200,16,16,"server connect success!");
	
return 0;
}
/*sim900a模块初始化
**初始化成功（指与设备通信成功，不确保能成功连接服务器）前不会退出此函数
**输入： none
**返回值：0-连接服务器成功			1-连接服务器失败
*/
int sim900a_init( void)
{

	GPIO_InitTypeDef  GPIO_InitStructure;

  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);//使能GPIOE、F时钟
	
	/****************GPIOF9,F10作为数字量输入*********************/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4; 
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//输出模式
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100M
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
  GPIO_Init(GPIOE, &GPIO_InitStructure);//初始化GPIOE4
	//复位模块
	sim900a_reset( );
	//连接模块
	if( sim900a_connect( ))return -1;
return 0;
}


//sim900a发送命令后,检测接收到的应答
//str:期待的应答结果
//返回值:0,没有得到期待的应答结果
//    其他,期待应答结果的位置(str的位置)
u8* sim900a_check_cmd(u8 *str)
{
	char *strx=0;
	if(USART1_RX_STA&0X8000)		//接收到一次数据了
	{ 
		USART1_RX_BUF[USART1_RX_STA&0X7FFF]=0;//添加结束符
		strx=strstr((const char*)USART1_RX_BUF,(const char*)str);
	} 
	return (u8*)strx;
}
//向sim900a发送命令
//cmd:发送的命令字符串(自动添加'\r''\n'),当cmd<0XFF的时候,发送数字(比如发送0X1A),大于的时候发送字符串.
//ack:期待的应答结果,如果为空,则表示不需要等待应答
//waittime:等待时间(单位:10ms)
//返回值:0,发送成功(得到了期待的应答结果)
//       1,发送失败
u8 sim900a_send_cmd(u8 *cmd,u8 *ack,u16 waittime)
{
	u8 res=0; 
	USART1_RX_STA=0;
	if((u32)cmd<=0XFF)
	{
		USART_SendStr( USART1,(char*)cmd);
	}else	u1_printf("%s\r\n",cmd);//发送命令
	if(ack&&waittime)		//需要等待应答
	{
		while(--waittime)	//等待倒计时
		{
			delay_ms(10);
			if(USART1_RX_STA&0X8000)//接收到期待的应答结果
			{			
				if(sim900a_check_cmd(ack))break;//得到有效数据 
				USART1_RX_STA=0;
			} 
		}
		if(waittime==0)res=1; 
	}
	return res;
} 












