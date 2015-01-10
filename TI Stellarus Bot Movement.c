//
//  EECE 437 S14 Lab Work Development Project
//  Hwk #3 - Interrupts, etc.
//
#include "includes.h"    
#include "string.h"

#include <bsp_display.h>

#define   MANUAL_DISPLAY
//#undef  MANUAL_DISPLAY

//#define NOT_NOW

#define   RT_EDGE     0
#define   LT_EDGE     48
#define   COL_MAX     96

#define   SCROLL_DELAY  20


#define MAX_RPM                 87
#define RIGHT_SIDE_SENSOR       SENSOR_A
#define LEFT_SIDE_SENSOR        SENSOR_A
#define RIGHT_SIDE_SENSOR_PORT  RIGHT_IR_SENSOR_A_PORT
#define RIGHT_SIDE_SENSOR_PIN   RIGHT_IR_SENSOR_A_PIN
#define LEFT_SIDE_SENSOR_PORT    LEFT_IR_SENSOR_A_PORT
#define LEFT_SIDE_SENSOR_PIN     LEFT_IR_SENSOR_A_PIN



#define  TASK_STK_SIZE                128u

#define  MAIN_TASK_PRIO                2u
#define  DRIVE_TASK_PRIO             4u

static  OS_TCB       MainTaskTCB;
static  OS_TCB       DriveTaskTCB;

static  CPU_STK       MainTaskStk[TASK_STK_SIZE];
static  CPU_STK       DriveTaskStk[TASK_STK_SIZE];

static  void   MainTask        (void  *p_arg);
static  void   DriveTask     (void  *p_arg);

static  CPU_BOOLEAN   AppRobotLeftWheelFirstEdge;
static  CPU_BOOLEAN   AppRobotRightWheelFirstEdge;

int   left_pct = 0, right_pct = 0;
int i=0;
unsigned int   left_counts = 0, right_counts = 0;

static  void  WheelSensorEnable (tSide );

int distance[10];

typedef struct
{
    CPU_INT32U  right_counts;
    CPU_INT32U  left_counts;
    CPU_INT32U  timestamp;
} Message;


void   ScrollMessage (const char *);
void   ShowTaskFlag(int );
void   ClearTaskFlag(int );
void   DisplayChar (char , int, int);


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int  main (void)
{
    OS_ERR  err;
    
    CPU_INT32U  clk_freq;
    CPU_INT32U  cnts;
    CPU_INT32U  ulPHYMR0;


    BSP_IntDisAll();                                            // Disable all interrupts.     

    OSInit(&err);                                               // Init uC/OS-III.  
    
    
    //printf(" Creating tasks...\n");
    
    
                
                
                OSTaskCreate((OS_TCB     *)& MainTaskTCB,                // Create Main Task                       
                 (CPU_CHAR   *)"Main Task",
                 (OS_TASK_PTR )  MainTask,
                 (void       *) 0,
                 (OS_PRIO     ) MAIN_TASK_PRIO,
                 (CPU_STK    *)& MainTaskStk[0],
                 (CPU_STK_SIZE) TASK_STK_SIZE / 10u,
                 (CPU_STK_SIZE) TASK_STK_SIZE,
                 (OS_MSG_QTY  ) 0u,
                 (OS_TICK     ) 0u,
                 (void       *) 0,
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *)&err);
  
    
                OSTaskCreate((OS_TCB     *)& DriveTaskTCB,                // Create Drive Task                       
                 (CPU_CHAR   *)"Drive Task",
                 (OS_TASK_PTR )  DriveTask,
                 (void       *) 2,
                 (OS_PRIO     ) DRIVE_TASK_PRIO,
                 (CPU_STK    *)& DriveTaskStk[0],
                 (CPU_STK_SIZE) TASK_STK_SIZE / 10u,
                 (CPU_STK_SIZE) TASK_STK_SIZE,
                 (OS_MSG_QTY  ) 100u,
                 (OS_TICK     ) 0u,
                 (void       *) 0,
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *)&err);
       
       
               
               
               
    distance[0]=65;
    distance[1]=72;
    distance[2]=137;
    distance[3]=158;
    distance[4]=223;
    distance[5]=247;
    distance[6]=280;
    distance[7]=283;

    
    BSP_Init();                                                 /* Initialize BSP functions                             */
    CPU_Init();                                                 /* Initialize the uC/CPU services                       */


    SysCtlPeripheralEnable(SYSCTL_PERIPH_ETH);                  /* Enable and Reset the Ethernet Controller.            */
    SysCtlPeripheralReset(SYSCTL_PERIPH_ETH);

    ulPHYMR0 = EthernetPHYRead(ETH_BASE, PHY_MR0);              /* Power Down PHY                                       */
    EthernetPHYWrite(ETH_BASE, PHY_MR0, ulPHYMR0 | PHY_MR0_PWRDN);
    SysCtlPeripheralDeepSleepDisable(SYSCTL_PERIPH_ETH);


    clk_freq = BSP_CPUClkFreq();                                /* Determine SysTick reference freq.                    */
    cnts     = clk_freq / (CPU_INT32U)OSCfg_TickRate_Hz;        /* Determine nbr SysTick increments                     */
    OS_CPU_SysTickInit(cnts);                                   /* Init uC/OS periodic time src (SysTick).              */
    CPU_TS_TmrFreqSet(clk_freq);

 
 
     //printf(" Running OS...\n");
    OSStart(&err);                                              /* Start multitasking (i.e. give control to uC/OS-III). */
}


//
//  Main Task
//

static  void   MainTask (void  *p_arg)
{
     OS_ERR  err;
  
    BSP_LED_On(1);
   // BSP_LED_Off(2);


     //printf("in MainTask ...\n");
     
     WheelSensorEnable(LEFT_SIDE);
     WheelSensorEnable(RIGHT_SIDE);
    
    while (DEF_ON) {                                            /* Task body, always written as an infinite loop.       */
        OSTimeDlyHMSM(0u, 0u, 1u, 0u,
                      OS_OPT_TIME_HMSM_STRICT,
                      &err);

        BSP_LED_Toggle(1);

        // Toggle LED 1
         //BSP_LED_Off(1);
       
        
        

    
    //BSP_DisplayClear();
    

    }
}


//
//  Drive task
//

static  void   DriveTask (void  *p_arg)
{
    OS_ERR  err;
    
    CPU_INT32U   pulse_time;
    OS_MSG_SIZE  msg_size;
    CPU_TS              ts;
    int toggle_switch=0;//used to toggle on or off.
    int toggle_activate=0;//interprets message and causes the toggle loop to go
        
        
             // Display strings for holding counts for buttons, touch sensors
    CPU_INT08S         left_disp[3] = "00\0";
    CPU_INT08S         right_disp[3]= "00\0";
    
    CPU_INT08S         left_count[5] = "0000\0";
    CPU_INT08S         right_count[5]= "0000\0";
     

    BSP_LED_On(2);
    
    BSP_MotorDir(LEFT_SIDE, FORWARD);
    BSP_MotorSpeed(LEFT_SIDE, 0);    
    BSP_MotorRun(LEFT_SIDE);   
    
    BSP_MotorDir(RIGHT_SIDE, FORWARD);
    BSP_MotorSpeed(RIGHT_SIDE, 0);    
    BSP_MotorRun(RIGHT_SIDE); 
    
    
    
    while (DEF_ON) {                                            /* Task body, always written as an infinite loop.       */
        //OSTimeDlyHMSM(0u, 0u, 0u, 200u,
        //              OS_OPT_TIME_HMSM_STRICT,
        //              &err);
             

        BSP_LED_Toggle(2);                                      // Toggle LED 2
        
        
        
#ifdef   MANUAL_DISPLAY
        
        BSP_DisplayClear();
        
       // Test pushbuttons for a press....                               
       if (!BSP_PushButtonGetStatus(1)) {
                    left_pct+=45;
                    left_pct %= 100;
                    right_pct +=45;
                    right_pct %= 100;
        }

        /*if (!BSP_PushButtonGetStatus(2)) {
                    right_pct+=5;
                    right_pct %= 100;
        }*/
       

       BSP_MotorSpeed(LEFT_SIDE, (left_pct*256));         
       
       BSP_MotorSpeed(RIGHT_SIDE, (right_pct*256));     
       
        sprintf((char *)left_disp, "%02i", left_pct);     
        sprintf((char *)right_disp, "%02i", right_pct);      
          
        sprintf((char *)left_count, "%04i", left_counts);
        sprintf((char *)right_count, "%04i", right_counts);   
             
        //BSP_DisplayClear();
        BSP_DisplayStringDraw("L:", RT_EDGE, 0u);
        BSP_DisplayStringDraw(left_disp, RT_EDGE+16, 0u);
        
        BSP_DisplayStringDraw(left_count, RT_EDGE+16, 1u);
                
        BSP_DisplayStringDraw("R:",LT_EDGE, 0u);
        BSP_DisplayStringDraw(right_disp, LT_EDGE+16, 0u);

        BSP_DisplayStringDraw(right_count, LT_EDGE+16, 1u);      
        
#endif 
    
        // Pend on task queue - to get message 
       

        
      toggle_activate = (CPU_INT32U)OSTaskQPend((OS_TICK      )OSCfg_TickRate_Hz/2,
                                 (OS_OPT       )OS_OPT_PEND_BLOCKING,
                                 (OS_MSG_SIZE *)&msg_size,
                                 (CPU_TS      *)&ts,
                                 (OS_ERR      *)&err);
          //toggle setup
        if(toggle_activate==1)
        {
          if(toggle_switch==0)
          {
            BSP_MotorDir(LEFT_SIDE, REVERSE);
            toggle_switch=1;
          }
          else
          {
            BSP_MotorDir(LEFT_SIDE, FORWARD);
            toggle_switch=0;
          }
          toggle_activate=0;
        }
        
    }
}





static  void  WheelSensorIntHandler (void)
{
    static CPU_INT32U  ulRightEdgePrevTime;
    static CPU_INT32U  ulLeftEdgePrevTime;
    CPU_INT32U         ulRightEdgeDiff;
    CPU_INT32U         ulLeftEdgeDiff;
    CPU_INT32U         ulRightEdgeTime;
    CPU_INT32U         ulLeftEdgeTime;
    CPU_INT32U         ulStatus;
    CPU_INT08U         ucCnt;
    OS_ERR             err;
    CPU_TS              ts;

    ulStatus = GPIOPinIntStatus(LEFT_SIDE_SENSOR_PORT, DEF_TRUE);
    if (ulStatus & LEFT_SIDE_SENSOR_PIN) {

        GPIOPinIntClear(LEFT_SIDE_SENSOR_PORT,                  /* Clear interrupt.                                     */
                        LEFT_SIDE_SENSOR_PIN);
        left_counts++;
        left_counts %= 1000;
        
                                                                /* {Workaround}                                         */
#ifdef NOT_NOW
        for (ucCnt = 0u; ucCnt < 100u; ucCnt++) {               /* Check for pin high (rising edge).                    */
            if (!GPIOPinRead(LEFT_SIDE_SENSOR_PORT, LEFT_SIDE_SENSOR_PIN)) {
                return;
            }
        }

                                                                /* Check to make sure that the time since the last ...  */
                                                                /* ... makes sense. This only matters if it is not ...  */
                                                                /* ... the first edge.                                  */
        if (!AppRobotLeftWheelFirstEdge) {
            ulLeftEdgeTime = OSTimeGet(&err);
            if(ulLeftEdgeTime <= ulLeftEdgePrevTime) {
                                                                /* Account for rollover.                                */
                ulLeftEdgeDiff = ulLeftEdgeTime + (0xFFFFFFFFu - ulLeftEdgePrevTime);
            } else {
                ulLeftEdgeDiff = ulLeftEdgeTime - ulLeftEdgePrevTime; 
            }

                                                                /* Is the time difference less than expected for ...    */
                                                                /* ... the MAX_RPM + some buffer.                       */
            if ((ulLeftEdgeDiff) < (OSCfg_TickRate_Hz / (((MAX_RPM + 10u) * 8u) / 60u))) {
                return;
            }
        }
                                                                /* {Workaround End}                                     */

        if (AppRobotLeftWheelFirstEdge) {
            ulLeftEdgePrevTime = OSTimeGet(&err);
            AppRobotLeftWheelFirstEdge = DEF_FALSE;
        } else {
            ulLeftEdgeTime = OSTimeGet(&err);
            if (ulLeftEdgeTime <= ulLeftEdgePrevTime) {
                                                                /* Account for rollover.                                */
                ulLeftEdgeDiff  = ulLeftEdgeTime +
                                 (0xFFFFFFFFu - ulLeftEdgePrevTime);
            } else {
                ulLeftEdgeDiff  = ulLeftEdgeTime - ulLeftEdgePrevTime; 
            }
            ulLeftEdgePrevTime = ulLeftEdgeTime;
            
            // code for valid pulse goes here....

         }
        
        
#endif
    }

    
    ulStatus = GPIOPinIntStatus(RIGHT_SIDE_SENSOR_PORT, DEF_TRUE);
    if (ulStatus & RIGHT_SIDE_SENSOR_PIN) {

        GPIOPinIntClear(RIGHT_SIDE_SENSOR_PORT,                 /* Clear interrupt.                                     */
                        RIGHT_SIDE_SENSOR_PIN);
        right_counts++;
        right_counts %= 1000;
                                                               /* {Workaround}                                         */
        for (ucCnt = 0u; ucCnt < 100u; ucCnt++) {               /* Check for pin high (rising edge).                    */
            if (!GPIOPinRead(RIGHT_SIDE_SENSOR_PORT, RIGHT_SIDE_SENSOR_PIN)) {
                return;
            }
        }
                                                                /* Check to make sure that the time since the last ...  */
                                                                /* ... makes sense. This only matters if it is not ...  */
                                                                /* ... the first edge.                                  */
        if (!AppRobotRightWheelFirstEdge) {
            ulRightEdgeTime = OSTimeGet(&err);
            if (ulRightEdgeTime <= ulRightEdgePrevTime) {
                                                                /* Account for rollover.                                */
                ulRightEdgeDiff  = ulRightEdgeTime + 
                                  (0xFFFFFFFFu - ulRightEdgePrevTime);
            } else {
                ulRightEdgeDiff  = ulRightEdgeTime - ulRightEdgePrevTime; 
            }
                                                                /* Is the time difference less than expected for ...    */
                                                                /* ... the MAX_RPM + some buffer.                       */
            if((ulRightEdgeDiff) < (OSCfg_TickRate_Hz / (((MAX_RPM + 10u) * 8u) / 60u))) {
                return;
            }
        }
                                                                /* {Workaround End}                                     */

        if (AppRobotRightWheelFirstEdge) {
            ulRightEdgePrevTime = OSTimeGet(&err);
            AppRobotRightWheelFirstEdge = DEF_FALSE;

        } else {

            ulRightEdgeTime = OSTimeGet(&err);
            if (ulRightEdgeTime <= ulRightEdgePrevTime) {
                                                                /* Account for rollover.                                */
                ulRightEdgeDiff  = ulRightEdgeTime +
                                  (0xFFFFFFFFu - ulRightEdgePrevTime) + 1u;

            } else {

                ulRightEdgeDiff = ulRightEdgeTime - ulRightEdgePrevTime; 
            }

            ulRightEdgePrevTime = ulRightEdgeTime;

            
            //right_counts++;
            //right_counts %= 1000;
 
            // Post message indicating valid pulse
            ts = OS_TS_GET();      
        if (left_counts==distance[i])//compares number of wheel rotations to the needed distance
        {
          i++;
          OSTaskQPost((OS_TCB    *)&DriveTaskTCB, //calls the subfunction to reverse wheel direction              
                        (void      *)1,
                        (OS_MSG_SIZE)sizeof(CPU_TS),
                        (OS_OPT     )OS_OPT_POST_FIFO,
                        (OS_ERR    *)&err);
        }
        else if(left_counts==380)//ends the run at the end.
        {
          left_pct=0;//kills the left and right wheels motion
          right_pct=0;
        }
    
        }

    }
}


static  void  WheelSensorEnable (tSide  eSide)
{
    if (eSide == LEFT_SIDE) {

        AppRobotLeftWheelFirstEdge = DEF_TRUE;                  /* Indicate the first edge has yet to occur.            */

        BSP_WheelSensorEnable();                                /* Enable wheel sensors.                                */

        BSP_WheelSensorIntEnable(LEFT_SIDE, LEFT_SIDE_SENSOR,   /* Enable wheel sensor interrupts.                      */
                                 WheelSensorIntHandler);

    } else {

        AppRobotRightWheelFirstEdge = DEF_TRUE;                 /* Indicate the first edge has yet to occur.            */

        BSP_WheelSensorEnable();                                /* Enable wheel sensors.                                */

        BSP_WheelSensorIntEnable(RIGHT_SIDE, RIGHT_SIDE_SENSOR, /* Enable wheel sensor interrupts.                      */
                                 WheelSensorIntHandler);
    }
}
