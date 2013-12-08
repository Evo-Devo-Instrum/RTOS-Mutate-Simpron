/******************************************************************************
Filename    : kernel.h
Author      : pry 
Date        : 17/07/2013
Description : The header for the RTOS-MV.
******************************************************************************/

/* Defines *******************************************************************/
#ifdef __KERNEL_MEMBERS__
#undef EXTERN
#define EXTERN 
#endif

/* The TID value when starting a thread after booting is done */
#define AUTO_PID   0x00

/* Thread/Task Status */
/* Bit Assignment
   7       6        5         4          3         2         1         0
OCCUPY   READY    SLEEP    Reserved {4                 :               0] */
#define OCCUPY     0x80
#define READY      0x40
#define SLEEP      0x20    

/* Signals */
#define NOSIG      0x00    
#define SIGKILL    1<<0
#define SIGSLEEP   1<<1                                                      
#define SIGWAKE    1<<2  
/* #define SIGRESERVE 1<<3 reserved */
#define SIGUSR1    1<<4    
#define SIGUSR2    1<<5
#define SIGUSR3    1<<6
#define SIGUSR4    1<<7

/* Memory */
#define PAGE_SIZE  (DMEM_SIZE/DMEM_PAGES)

/* Error */
/* Not enough memory */
#define ENOMEM     0x00				 					                
/* End Defines ***************************************************************/

/* Basic Types ***************************************************************/
#if(DEFINE_BASIC_TYPES==TRUE)
typedef int  s32;
typedef short s16;
typedef char  s8;

typedef const int sc32; 
typedef const short sc16; 
typedef const char sc8;   

typedef volatile int  vs32;
typedef volatile short  vs16;
typedef volatile char   vs8;

typedef volatile const int vsc32;
typedef volatile const short vsc16;
typedef volatile const char vsc8;

typedef unsigned int  u32;
typedef unsigned short u16;
typedef unsigned char  u8;

typedef const unsigned int uc32;  
typedef const unsigned short uc16;  
typedef const unsigned char uc8;   

typedef volatile unsigned int  vu32;
typedef volatile unsigned short vu16;
typedef volatile unsigned char  vu8;

typedef volatile const unsigned int vuc32;
typedef volatile const unsigned short vuc16;  
typedef volatile const unsigned char vuc8;  
#endif
/* End Basic Types ***********************************************************/

/* Extended Types ************************************************************/
/* Thread ID type */
typedef s8 tid_t;
/* Signal type */
typedef u8 signal_t;
/* The pointer's integer type - for MCS51,16 bits */
typedef u16 ptr_int_t;
/* The count type */
typedef u16 cnt_t;
/* the size type */
typedef u16 size_t;
/* the return value common type */
typedef s8 retval_t;
/* End Extended Types ********************************************************/

/* Structs *******************************************************************/
/* Syslib */
struct List_Head															   
{                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       
	struct List_Head* Prev;
	struct List_Head* Next;
};

/* Kernel */
struct Thread_Control_Block
{   
    struct List_Head Head;
    tid_t TID;
    s8* Thread_Name;
    u8 Status;             
    ptr_int_t Entrance;    
    signal_t Signal;	
    ptr_int_t Signal_Handler[4];  
};

struct Thread_Init_Struct
{
    tid_t TID;  
    s8* Thread_Name;    
    ptr_int_t Init_SP;                                                              
    ptr_int_t Entrance; 
};

/* Memory */
struct Memory
{
    volatile tid_t Mem_CB[DMEM_PAGES];
    vu8 DMEM_Heap[DMEM_SIZE];
};
/* End Structs ***************************************************************/

/* Global Variables **********************************************************/
/* Scheduler */
EXTERN xdata volatile cnt_t Global_Thread_Spin_Lock;
EXTERN xdata volatile cnt_t Interrupt_Lock_Cnt;
EXTERN xdata tid_t Current_TID;                  	    	                 	         	                                               

EXTERN xdata volatile ptr_int_t TCB_SP_Now[MAX_THREADS];
EXTERN xdata volatile struct Thread_Control_Block TCB[MAX_THREADS];     
EXTERN xdata struct List_Head Thread_Ready_List_Head;   
EXTERN xdata struct List_Head Thread_Empty_List_Head;
EXTERN xdata volatile cnt_t Thread_In_Sys;

/* Signal module */
EXTERN xdata volatile void (*_Sys_Signal_Handler_Exe)(void);

/* Memory management module */
#if(ENABLE_MEMM==TRUE)
/* For 8051, these has to be in xdata. We use a struct and place the Mem_CB
 * before the DMEM_Heap to make sure the allocatable memory space don't start 
 * with address=0, which may cause confusion.
 */
EXTERN xdata struct Memory Mem;
#endif

/* Stacks */
EXTERN idata u8 Kernel_Stack[KERNEL_STACK_SIZE];
EXTERN idata u8 App_Stack_1[APP_STACK_1_SIZE];
EXTERN idata u8 App_Stack_2[APP_STACK_2_SIZE];
/* End Global Variables ******************************************************/

/* Pseudo-Assembly Functions Prototypes **************************************/
/* Disable and enable all interrupts */
EXTERN void DISABLE_ALL_INTS(void);    	                                   
EXTERN void ENABLE_ALL_INTS(void);    	                                  

/* EXTERN void SYS_LOAD_SP(void);    	    	                                  
 * EXTERN void SYS_SAVE_SP(void);  
 */ 
/* End Pseudo-Assembly Functions Prototypes **********************************/

/* Public Function Prototypes ************************************************/
/* Scheduler */
EXTERN void DISABLE_ALL_INTS(void);
EXTERN void ENABLE_ALL_INTS(void);
EXTERN void _Sys_Int_Init(void);
EXTERN void Sys_Lock_Interrupt(void);
EXTERN void Sys_Unlock_Interrupt(void);
EXTERN void Sys_Create_List(struct List_Head* Head);
EXTERN void Sys_List_Delete_Node(struct List_Head* Prev,struct List_Head* Next);
EXTERN void Sys_List_Insert_Node(struct List_Head* New,struct List_Head* Prev,struct List_Head* Next);
EXTERN void Sys_Memset(ptr_int_t Address,s8 Char,size_t Size);		                         
EXTERN void _Sys_Scheduler_Init(void);                                                   
EXTERN void _Sys_Thread_Stack_Init(tid_t TID);
EXTERN void _Sys_Thread_Load(struct Thread_Init_Struct* Thread);
EXTERN tid_t Sys_Start_Thread(struct Thread_Init_Struct* Thread);
EXTERN retval_t Sys_Set_Ready(tid_t TID);
EXTERN void _Sys_Load_Init(void);
EXTERN void _Sys_Init(void);	    	                                   
EXTERN void Sys_Switch_Now(void);
EXTERN tid_t Sys_Get_TID(void);

/* Signal module */
EXTERN void _Sys_Signal_Handler(tid_t TID);
EXTERN void _Sys_Thread_Kill(tid_t TID);
EXTERN void _Sys_Thread_Sleep(tid_t TID);
EXTERN void _Sys_Thread_Wake(tid_t TID);
EXTERN retval_t Sys_Send_Signal(tid_t TID,signal_t Signal);
EXTERN retval_t Sys_Reg_Signal_Handler(tid_t TID,signal_t Signal,void (*Signal_Handler)(void));

/* Memory management module */
EXTERN void _Sys_Memory_Init(void);
EXTERN void xdata* __Sys_Malloc(tid_t TID,size_t Size);
EXTERN void xdata* Sys_Malloc(size_t Size);
EXTERN void __Sys_Mfree(tid_t TID,void xdata* Mem_Ptr);
EXTERN void Sys_Mfree(void xdata* Mem_Ptr);
EXTERN void __Sys_Mfree_All(tid_t TID);
EXTERN void Sys_Mfree_All(void);


/* Stacks */
EXTERN void Task1(void);    	    	    	                          
EXTERN void Task2(void); 
EXTERN void Task3(void);
/* End Public Function Prototypes ********************************************/

/* End Of File ***************************************************************/

/* Copyright (C) 2011-2013 Evo-Devo Instrum. All rights reserved *************/
