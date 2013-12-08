/******************************************************************************
Filename    : kernel.c
Author      : pry
Date        : 10/11/2012                 
Description : The kernel of the RMV RTOS.
              NOTE : In the 8051 microcontroller, the data pointer is expressed 
                     as follows:
              type_identifier  memory_region   *   ptr_stor_region  ptr_name
              ---------------  -------------  ---  ---------------  --------
                    (1)             (2)       (3)        (4)          (5)

                   char            xdata       *       idata          Ptr     
              A pointer named "Ptr", points to a variable in the memory region 
              xdata, and the pointer itself is storaged in the idata region.
******************************************************************************/

/* Includes ******************************************************************/
#include "sysconfig.h"
#define __KERNEL_MEMBERS__
#include "kernel.h"

#undef __KERNEL_MEMBERS__
/* End Includes **************************************************************/

/*-------------------------- Scheduler Module ---------------------------------
The system scheduler module is one that:
1> Supports 120 number of threads (not processes and pseudo-processes), and the
   implemented number is only limited by the available RAM of the MCU;
2> Support dynamic thread management including deletion and setup
3> Does not require a system timer, just like virus don't have their independent
   metabolism. 
4> DOES NOT support priority, but you can get the same functionality by playing
   some tricks on the signal system.
5> The system also does not support zombie thread and thread return value.
   
In very tiny places, these features have advantages as follows:
1> Save a system timer, which is especially important in the 8051 or AVR systems where
   the system timer is a rare resource;
2> Save the thread stack, making it possible to run RTOS-MV on a standard 8052
   microcontroller (implements on 8051 is impossible due to its tiny stack).
3> Quick in thread context switching.
4> Makes rapid simple system development possible.
-----------------------------------------------------------------------------*/

/* Begin Function:DISABLE_ALL_INTS ********************************************
Description : Disable all interrupts. This function is no longer a assembly one.         
Input       : None. 
Output      : None.
Return      : None.
******************************************************************************/
void DISABLE_ALL_INTS(void)                                               
{
    MCS51_Set_Interrupt_Mode(0);                                            
}
/* End Function:DISABLE_ALL_INTS *********************************************/

/* Begin Function:ENABLE_ALL_INTS *********************************************
Description : Enable all interrupts. This function is no longer a assembly one.           
Input       : None. 
Output      : None.
Return      : None.
******************************************************************************/
void ENABLE_ALL_INTS(void)                                               
{
    MCS51_Set_Interrupt_Mode(ENABLE_GLOBAL_INTS);                                            
}
/* End Function:ENABLE_ALL_INTS **********************************************/

/* Begin Function:SYS_LOAD_SP *************************************************
Description : Disable all interrupts.Critical.           
Input       : None. 
Output      : None.
Return      : None.
******************************************************************************/
#define SYS_LOAD_SP() SP=TCB_SP_Now[Current_TID]                                          
/* End Function:SYS_LOAD_SP **************************************************/

/* Begin Function:SYS_SAVE_SP *************************************************
Description : Enable all interrupts.Critical.           
Input       : None. 
Output      : None.
Return      : None.
******************************************************************************/
#define SYS_SAVE_SP()  TCB_SP_Now[Current_TID]=SP;                           
/* End Function:SYS_SAVE_SP **************************************************/

/* Begin Function:_Sys_Int_Init ***********************************************
Description : Initialize the system interrupt.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
void _Sys_Int_Init(void)							        	  
{	
    Interrupt_Lock_Cnt=0;
}
/* End Function:_Sys_Int_Init ************************************************/

/* Begin Function:Sys_Lock_Interrupt ******************************************
Description : The function locks the interrupt. The locking can be stacked.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
void Sys_Lock_Interrupt(void)
{
    if(Interrupt_Lock_Cnt==0)
    {
        /* Disable first before registering it. If an switch occurs between 
         * registering and disabling, then register-and-disable will cause 
         * fault.
         */
        DISABLE_ALL_INTS();
        Interrupt_Lock_Cnt=1;
    }
    else
        Interrupt_Lock_Cnt++;
}
/* End Function:Sys_Lock_Interrupt********************************************/

/* Begin Function:Sys_Unlock_Interrupt ****************************************
Description : The function unlocks the interrupt. The unlocking can be stacked.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
void Sys_Unlock_Interrupt(void)
{
    if(Interrupt_Lock_Cnt==1)
    {
        /* Clear the count before enabling, or it will cause fault in the same
         * sense as above.
         */
        Interrupt_Lock_Cnt=0;
        ENABLE_ALL_INTS();
    }
    else if(Interrupt_Lock_Cnt!=0)
        Interrupt_Lock_Cnt--;
}
/* End Function:Sys_Unlock_Interrupt******************************************/

/* Begin Function:Sys_Create_List *********************************************
Description : Create a doubly linkled list.
Input       : struct List_Head* Head - The list head pointer.
Output      : None.
Return      : None.
******************************************************************************/
void Sys_Create_List(struct List_Head* Head)
{
	Head->Prev=Head;
	Head->Next=Head;
}
/* End Function:Sys_Create_List **********************************************/

/* Begin Function:Sys_List_Delete_Node ****************************************
Description : Delete a node from the doubly-linked list.
Input       : struct List_Head* Prev-The Node Before The Node To Be Deleted.
              struct List_Head* Next-The Node After The Node To Be Deleted.
Output      : None.
Return      : None.
******************************************************************************/
void Sys_List_Delete_Node(struct List_Head* Prev,struct List_Head* Next)
{
    Next->Prev=Prev;
    Prev->Next=Next;
}
/* End Function:Sys_List_Delete_Node *****************************************/

/* Begin Function:Sys_List_Insert_Node ****************************************
Description : Insert A Node to The doubly-linked list.
Input       : struct List_Head* New-The new node to insert.
			  struct List_Head* Prev-The Previous Node.
			  struct List_Head* Next-The Next Node.
Output      : None.
Return      : None.
******************************************************************************/
void Sys_List_Insert_Node(struct List_Head* New,struct List_Head* Prev,struct List_Head* Next)
{
	Next->Prev=New;
	New->Next=Next;
	New->Prev=Prev;
	Prev->Next=New;
}
/* End Function:Sys_List_Insert_Node *****************************************/

/* Begin Function:Sys_Memset **************************************************
Description : The memset function. Fills a certain memory area with
              the intended character.
Input       : ptr_int_t Address -The Fill Start Address.
			  s8 Char -The character to fill.
			  size_t Size -The size to fill.
Output      : None. 
Return      : None.
******************************************************************************/
void Sys_Memset(ptr_int_t Address,s8 Char,size_t Size)		                         
{
	s8 xdata* Address_Ptr=(s8*)Address;
	cnt_t Size_Count;
	for(Size_Count=Size;Size_Count>0;Size_Count--)
		*Address_Ptr++=Char;						                         
}
/* End Function:Sys_Memset ***************************************************/

/* Begin Function:_Sys_Scheduler_Init *****************************************
Description : Initialize the system scheduler.
Input       : None. 
Output      : None.
Return      : None.
******************************************************************************/
void _Sys_Scheduler_Init(void)                                                   
{
    cnt_t Thread_Cnt;
    
    /* Initialize the ready list and the empty list */
    Sys_Create_List(&Thread_Ready_List_Head);
    Sys_Create_List(&Thread_Empty_List_Head);
    
    /* Clear the system variables */
    Sys_Memset((ptr_int_t)TCB,0,MAX_THREADS*sizeof(struct Thread_Control_Block));
    
    /* Insert all the nodes into the empty list */
    for(Thread_Cnt=0;Thread_Cnt<MAX_THREADS;Thread_Cnt++)
    {
        Sys_List_Insert_Node(&TCB[Thread_Cnt].Head,
                             Thread_Empty_List_Head.Prev,
                             &Thread_Empty_List_Head);
        
        TCB[Thread_Cnt].TID=Thread_Cnt;
    }
    
    /* Clear the statistical variable */
    Thread_In_Sys=0;
}
/* End Function:_Sys_Scheduler_Init ******************************************/

/* Begin Function:_Sys_Thread_Stack_Init **************************************
Description : Initialize a thread's stack given the TID.
Input       : tid_t TID - The thread's TID.
Output      : None.
Return      : None.
******************************************************************************/
void _Sys_Thread_Stack_Init(tid_t TID)
{      
    /* Set the thread entrance */                                                                              
    *((u8 idata*)(TCB_SP_Now[TID]))=((u16)(TCB[TID].Entrance))>>8;
    *((u8 idata*)(TCB_SP_Now[TID]-1))=((u16)(TCB[TID].Entrance))&0xff; 
}
/* End Function:_Sys_Thread_Stack_Init ***************************************/

/* Begin Function:_Sys_Thread_Load ********************************************
Description : The thread/task loader.
Input       : struct Thread_Init_Struct* Thread - The thread init struct
Output      : None.
Return      : None.
******************************************************************************/
void _Sys_Thread_Load(struct Thread_Init_Struct* Thread)
{    
    tid_t TID=Thread->TID;
    /* Indicates that this TID is in use */
    TCB[TID].Status=OCCUPY;   
    TCB[TID].Thread_Name=Thread->Thread_Name;  
    TCB[TID].Entrance=(ptr_int_t)(Thread->Entrance);    
    TCB_SP_Now[TID]=Thread->Init_SP+1;  
    
    /* Now delete this thread from the empty list,but not into the running list */
    Sys_List_Delete_Node(TCB[TID].Head.Prev,TCB[TID].Head.Next);
    
    _Sys_Thread_Stack_Init(TID);
}
/* End Function:_Sys_Thread_Load *********************************************/

/* Begin Function:Sys_Start_Thread ********************************************
Description : The thread/task loader.
Input       : struct Thread_Init_Struct* Thread - The thread init struct
Output      : None.
Return      : tid_t - If successful,0; else -1.
******************************************************************************/
tid_t Sys_Start_Thread(struct Thread_Init_Struct* Thread)
{    
    tid_t TID;
    
    /* See if the TID member is "AUTO_PID". If not, abort */
    if(Thread->TID!=AUTO_PID)
        return -1;   
    /* Find an empty slot to put the thread in */
    if(&Thread_Empty_List_Head==Thread_Empty_List_Head.Next)
        return -1;
        
    TID=((struct Thread_Control_Block xdata*)(Thread_Empty_List_Head.Next))->TID;
    
    /* Indicates that this TID is in use. */
    TCB[TID].Status=OCCUPY;   
    TCB[TID].Thread_Name=Thread->Thread_Name;  
    TCB[TID].Entrance=(ptr_int_t)(Thread->Entrance);    
    TCB_SP_Now[TID]=Thread->Init_SP+1;  
    
    /* Now delete this thread from the empty list,but not into the running list */
    Sys_List_Delete_Node(TCB[TID].Head.Prev,TCB[TID].Head.Next);
    
    /* Initialize the thread stack */
    _Sys_Thread_Stack_Init(TID);
    
    return (TID);
}
/* End Function:Sys_Start_Thread *********************************************/

/* Begin Function:Sys_Set_Ready ***********************************************
Description : Specify a thread as ready.
Input       : tid_t TID - The thread that you want to set as ready.
Output      : None.
Return      : retval_t - If the function fail, it will return -1.
******************************************************************************/
retval_t Sys_Set_Ready(tid_t TID)
{    
    Sys_Lock_Interrupt();
    
    /* See if the thread exists */
    if(TCB[TID].Status&OCCUPY==0)
        return -1;
    /* See if the thread is already ready */
    if(TCB[TID].Status&READY==1)
        return -1;
    /* See if the thread is sleeping */
    if(TCB[TID].Status&SLEEP==1)
        return -1;
    
    /* Now set the thread as ready */
    TCB[TID].Status|=READY;
    Sys_List_Insert_Node(&TCB[TID].Head,&Thread_Ready_List_Head,Thread_Ready_List_Head.Next);
    
    Sys_Unlock_Interrupt();
    return 0;
}
/* End Function:Sys_Set_Ready ************************************************/

/* Begin Function:_Sys_Load_Init **********************************************
Description : The function for loading the "Init" thread, the first thread of 
              the operating system.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
void _Sys_Load_Init(void)
{
    xdata struct Thread_Init_Struct Init;
    Init.TID=0;                                                       
    Init.Thread_Name="Init";
    Init.Init_SP=Kernel_Stack;
    
    _Sys_Thread_Load(&Init);
    Sys_Set_Ready(0);
    
    /* Set the current TID */
    Current_TID=0;
    
    /* Load its stack pointer */
    SYS_LOAD_SP();
    
    /* Will never return */    
    _Sys_Init();
}
/* End Function:_Sys_Load_Init ***********************************************/

/* Begin Function:_Sys_Init_Initial *******************************************
Description : The function initializing the system and loading the startup 
              processes.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
void _Sys_Init_Initial(void)
{
    struct Thread_Init_Struct Thread;                                    

    Thread.TID=1;  
    Thread.Thread_Name="Thread_1";                                                    
    Thread.Init_SP=App_Stack_1;                                        
    Thread.Entrance=Task1;                                            
    _Sys_Thread_Load(&Thread); 
    Sys_Set_Ready(1);
}
/* End Function:_Sys_Init_Initial ********************************************/

/* Begin Function:_Sys_Init_Always ********************************************
Description : The function initializing the system and loading the startup 
              processes.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
void _Sys_Init_Always(void)
{
    /* Do nothing now */;
}
/* End Function:_Sys_Init_Always *********************************************/

/* Begin Function:_Sys_Init ***************************************************
Description : This thread will initiate all the latter tasks. 
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
void _Sys_Init(void)    	    	    	                                   
{
    _Sys_Init_Initial();
    while(1)
    {
        _Sys_Init_Always();
        /* Switch out at direct */
        Sys_Switch_Now();
    }
}
/* End Function:_Sys_Init ****************************************************/

/* Begin Function:Sys_Switch_Now **********************************************
Description : Call the function to trigger a context switch. In the RMV, this is
              the only way to cause a context switch.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
void Sys_Switch_Now(void)
{
    Sys_Lock_Interrupt();
    SYS_SAVE_SP();
    
    /* We need to see if the current task is deleted from ths list.
     * NOTE: See if the task list is empty. If yes, we will still run the same task 
     */
    if((TCB[Current_TID].Status&READY)==0)
        Current_TID=((struct Thread_Control_Block xdata*)(Thread_Ready_List_Head.Next))->TID;
    else
    {
        /* If we have reached the end of the list, then rewind the pointer */
		if(TCB[Current_TID].Head.Next==&Thread_Ready_List_Head)
			Current_TID=((struct Thread_Control_Block xdata*)(Thread_Ready_List_Head.Next))->TID;
        else
			Current_TID=((struct Thread_Control_Block xdata*)(TCB[Current_TID].Head.Next))->TID;
	}
        
    
    _Sys_Signal_Handler(Current_TID);       
    
    SYS_LOAD_SP(); 
    Sys_Unlock_Interrupt();
}
/* End Function:Sys_Switch_Now ***********************************************/

/* Begin Function:Sys_Get_TID *************************************************
Description : Get the current thread ID.
Input       : None.
Output      : None.
Return      : tid_t - The Current TID.
******************************************************************************/
tid_t Sys_Get_TID(void)
{
    return(Current_TID);
}
/* End Function:Sys_Get_TID **************************************************/

/* Begin Function:main ********************************************************
Description : The entrance of the OS.
Input       : None.
Output      : None.
Return      : int-dummy.
******************************************************************************/
int main(void)    	                                                    
{    
    /* Initialize the system interrupt control */
    _Sys_Int_Init();
    
    /* Initialize system memory management module */
    _Sys_Memory_Init();
    
    /* Initialize system scheduler */
    _Sys_Scheduler_Init();
    
    /* Load the first process - The init process */
    _Sys_Load_Init();                           
    
    /* The function above should never return */                                               	    	    	    	    	    	               
    return(0);  
}
/* End Function:main *********************************************************/

/*--------------------------- Signal Module -----------------------------------
The signal module supports 7 signals: 3 system signals and 4 use signals. Each
user signal can be registered a handler function. The signals and their description
are as follows:
SIGKILL  Kill the thread instantly.
SIGSLEEP Make the thread sleep instantly.
SIGWAKE  Wakeup the thread instantly.
SIGUSR1  User signal 1.
SIGUSR2  User signal 1.
SIGUSR3  User signal 1.
SIGUSR4  User signal 1.
-----------------------------------------------------------------------------*/

/* Begin Function:_Sys_Signal_Handler *****************************************
Description : The signal handler.
Input       : tid_t TID -The thread ID.
Output      : None.
Return      : None.
******************************************************************************/
void _Sys_Signal_Handler(tid_t TID)           	    	    	    	    
{
    /* See if there are signals */
    if(TCB[TID].Signal==0)
        return;
    
    /* We don't scan SIGKILL, SIGSLEEP and SIGWAKE here. They are dealt with directly
     * when they are send. See if there are user signals.
     */
    if(((TCB[TID].Signal&SIGUSR1)!=0)&&(TCB[TID].Signal_Handler[0]!=0))
    {
        _Sys_Signal_Handler_Exe=(void(*)(void))TCB[TID].Signal_Handler[0];
        _Sys_Signal_Handler_Exe();
    }

    if(((TCB[TID].Signal&SIGUSR2)!=0)&&(TCB[TID].Signal_Handler[1]!=0))
    {
        _Sys_Signal_Handler_Exe=(void(*)(void))TCB[TID].Signal_Handler[1];
        _Sys_Signal_Handler_Exe();
    }

    if(((TCB[TID].Signal&SIGUSR3)!=0)&&(TCB[TID].Signal_Handler[2]!=0))
    {
        _Sys_Signal_Handler_Exe=(void(*)(void))TCB[TID].Signal_Handler[2];
        _Sys_Signal_Handler_Exe();
    }  

    if(((TCB[TID].Signal&SIGUSR4)!=0)&&(TCB[TID].Signal_Handler[3]!=0))
    {
        _Sys_Signal_Handler_Exe=(void(*)(void))TCB[TID].Signal_Handler[3];
        _Sys_Signal_Handler_Exe();

    }
    
    /* Clear its signal */
    TCB[TID].Signal=NOSIG;    	    	    	    	    	    	   
}
/* End Function:_Sys_Signal_Handler ******************************************/

/* Begin Function:_Sys_Thread_Kill ********************************************
Description : The SIGKILL handler, to kill a certain thread.
Input       : tid_t TID - The thread ID.
Output      : None.
Return      : None.
******************************************************************************/
void _Sys_Thread_Kill(tid_t TID)    	    	    	    	    	  
{
    /* It doesn't matter if the TID is the Current_TID */
    Sys_List_Delete_Node(TCB[TID].Head.Prev,TCB[TID].Head.Next);
    Sys_Memset((ptr_int_t)(&TCB[TID]),0,sizeof(struct Thread_Control_Block));
    Sys_List_Insert_Node(&TCB[TID].Head,&Thread_Empty_List_Head,Thread_Empty_List_Head.Next);
    /* We need the TID marker preserved */
    TCB[TID].TID=TID;
}

/* End Function:_Sys_Thread_Kill *********************************************/

/* Begin Function:_Sys_Thread_Sleep *******************************************
Description : The SIGSLEEP handler. In fact executed directly after the signal is
              send, to make a certain thread sleep.
Input       : tid_t TID - The thread ID.
Output      : None.
Return      : None.
******************************************************************************/
void _Sys_Thread_Sleep(tid_t TID)    	    	    	    	    	  
{
    /* See if the thread is already sleeping */
    if(TCB[TID].Status&SLEEP!=0)
        return;
    
    TCB[TID].Status|=SLEEP;
    TCB[TID].Status&=~READY;
    Sys_List_Delete_Node(TCB[TID].Head.Prev,TCB[TID].Head.Next);
}
/* End Function:_Sys_Thread_Sleep ********************************************/

/* Begin Function:_Sys_Thread_Wake ********************************************
Description : The SIGWAKE handler. In fact executed directly after the signal is

              send, to wake up a certain thread.
Input       : tid_t TID - The thread ID.
Output      : None.
Return      : None.
******************************************************************************/
void _Sys_Thread_Wake(tid_t TID)    	    	    	    	    	
{
    /* See if the thread is sleeping */
    if(TCB[TID].Status&SLEEP==0)
        return;
    
    TCB[TID].Status&=~(SLEEP);
    TCB[TID].Status|=READY;

    Sys_List_Insert_Node(&TCB[TID].Head,&Thread_Ready_List_Head,Thread_Ready_List_Head.Next);
}
/* End Function:_Sys_Thread_Wake *********************************************/

/* Begin Function:Sys_Send_Signal *********************************************
Description : The function for sending signals to the threads. You can also send
              a signal to the callee itself.
Input       : tid_t TID - The thread ID.
              signal_t Signal - The signal to send.
Output      : None.
Return      : retval_t - If the operation is invalid, it will return -1; else 0.
******************************************************************************/
retval_t Sys_Send_Signal(tid_t TID,signal_t Signal)
{    	    	   
    /* See if the TID is valid in the signal system */   
    if((TID==0)||TID>=MAX_THREADS)
        return -1;

    /* See if the thread exists in the system */
    if(TCB[TID].Status&OCCUPY==0)
        return -1;  
    
    switch(Signal)
    {
        /* The system signals will be dealt on send */
        case SIGKILL:_Sys_Thread_Kill(TID);break;
        case SIGSLEEP:_Sys_Thread_Sleep(TID);break;
        case SIGWAKE:_Sys_Thread_Wake(TID);break;

        case SIGUSR1:TCB[TID].Signal|=SIGUSR1;break;
        case SIGUSR2:TCB[TID].Signal|=SIGUSR2;break;
        case SIGUSR3:TCB[TID].Signal|=SIGUSR3;break;
        case SIGUSR4:TCB[TID].Signal|=SIGUSR4;break;
        /* The input is not a signal */
        default:return -1;
    }
    return 0;
}
/* End Function:Sys_Send_Signal **********************************************/

/* Begin Function:Sys_Register_Signal_Handler *********************************
Description : Register the user signal's signal handler.
Input       : tid_t TID - The thread ID.
              signal_t Signal - The signal to register the handler.
              void (*Signal_Handler)(void) - The function pointer to the handler.
Output      : None.
Return      ; retval_t - If succeeded,0; else -1.
******************************************************************************/
retval_t Sys_Reg_Signal_Handler(tid_t TID,signal_t Signal,void (*Signal_Handler)(void))    	    	    
{
    /* See if the TID is valid in the system */   
    if((TID==0)||(TID>=MAX_THREADS))

        return -1;

    /* See if the thread exists in the system */
    if(TCB[TID].Status&OCCUPY==0)
        return -1;    

    switch(Signal)
    {
        /* Other signals and non-signal patterns cannot be registered a handler */        
        case SIGUSR1:TCB[TID].Signal_Handler[0]=(ptr_int_t)Signal_Handler;break;
        case SIGUSR2:TCB[TID].Signal_Handler[1]=(ptr_int_t)Signal_Handler;break;
        case SIGUSR3:TCB[TID].Signal_Handler[2]=(ptr_int_t)Signal_Handler;break;
        case SIGUSR4:TCB[TID].Signal_Handler[3]=(ptr_int_t)Signal_Handler;break;

        /* The input is not a signal */
        default:return -1;
    }    
    return 0;
}
/* End Function:Sys_Register_Signal_Handler **********************************/

/*--------------------------- Memory Management -------------------------------
The memory management module utilize the paging method. When you allocate memory,
the minimum amount allocated is 2 pages - Why is that? See the description below.
Assume we have a memory region (1K) as follows, divided into 20 pages, the Mem_CB is:
0x0000 [0][0][0][0][0] [0][0][0][0][0] [0][0][0][0][0] [0][0][0][0][0] 0x03FF

When the thread A (TID=1) want to allocate 200 bytes of memory, the Mem_CB becomes:
0x0000 [1][1][1][0][0] [0][0][0][0][0] [0][0][0][0][0] [0][0][0][0][0] 0x03FF
Here we can see 3 slots are filled by "1". The "1" means that the corresponding 
memory area is assigned to the thread A. However, there is a single block marked
"0" after the "1"s, and this block is also assigned to the thread A. Why do we need this?

When the thread A allocates memory(100 bytes) again:
0x0000 [1][1][1][0][1] [0][0][0][0][0] [0][0][0][0][0] [0][0][0][0][0] 0x03FF
       ************=======
           A.1st    A.2nd
This means that the thread A has allocated the memory twice. The "0" acted as a
segregation marker between the two allocations.

Now the thread B (TID=2) want to allocate 1K bytes of memory. The Mem_CB is:
0x0000 [1][1][1][0][1] [0][2][2][2][2] [2][2][2][2][2] [0][0][0][0][0] 0x03FF
       ************=======++++++++++++++++++++++++++++++++
           A.1st    A.2nd              B.1st           
           
The simple memory control block works as the above desctiption. The memory allocator
is simple in both space and time (when managing a small memory region).
-----------------------------------------------------------------------------*/

/* Begin Function:_Sys_Memory_Init ********************************************
Description : Initialize the system memory management module. The Memory module
              uses paging memory pool method to manage a very limited amount of
              memory.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
void _Sys_Memory_Init(void)
{   
#if(ENABLE_MEMM==TRUE) 
    Sys_Memset((ptr_int_t)(&Mem),0,sizeof(struct Memory));
#endif
}
/* End Function:_Sys_Memory_Init *********************************************/

/* Begin Function:__Sys_Malloc ************************************************
Description : Allocate some memory in the name of a certain thread. This function
              will not check if the TID is valid.
Input       : tid_t - The thread ID.
              size_t Bytes - The amount of RAM that the application need.
Output      : None.
Return      : void xdata* - The pointer to the memory. If the function fails, it will
                            return 0.
******************************************************************************/
#if(ENABLE_MEMM==TRUE)
void xdata* __Sys_Malloc(tid_t TID,size_t Size)
{    
    cnt_t Mem_Page_Cnt;
    cnt_t Page_Amount_Cnt;
    cnt_t Total_Pages;
    s8 Find_Flag=0;
    
    /* See if the size is valid */
    if(Size==0)
        return ((void*)0);
    
    /* See if the TID is valid in the system */   
    if(TID>=MAX_THREADS)
        return ((void*)0);
    
    /* Decide how many pages to allocate */
    if(Size%PAGE_SIZE==0)
        Total_Pages=Size/PAGE_SIZE;
    else
        Total_Pages=Size/PAGE_SIZE+1;
    
    Page_Amount_Cnt=0;

    /* Try to find continuous page to allocate */
    for(Mem_Page_Cnt=0;Mem_Page_Cnt<DMEM_PAGES;Mem_Page_Cnt++)
    {
        if(Mem.Mem_CB[Mem_Page_Cnt]==0)
            Page_Amount_Cnt++;
        else
            Page_Amount_Cnt=0;
        
        if(Page_Amount_Cnt==Total_Pages)
        {
            Find_Flag=1;
            break;
        }
    }

    /* See if we have found any */
    if(Find_Flag==0)
        return ((void*)0);
    
    /* If the page amount is 1, use special method */
    if(Page_Amount_Cnt==1)
    {
        Mem.Mem_CB[Mem_Page_Cnt]=TID; 
    }
    else
    {
        for(Page_Amount_Cnt--;Page_Amount_Cnt>0;Page_Amount_Cnt--)
        {
            Mem_Page_Cnt--;
            Mem.Mem_CB[Mem_Page_Cnt]=TID;   
        }
    }
    /* Now the pointer must have rewinded to the start address */
    return (void xdata*)(&Mem.DMEM_Heap[Mem_Page_Cnt*PAGE_SIZE]);		
}
#endif
/* End Function:_Sys_Malloc **************************************************/

/* Begin Function:Sys_Malloc **************************************************
Description : Allocate some memory. For application use.
Input       : size_t Bytes - The amount of RAM that the application need.
Output      : None.
Return      : void xdata* - The pointer to the memory. If the function fails, it will
                            return 0.
******************************************************************************/
#if(ENABLE_MEMM==TRUE)
void xdata* Sys_Malloc(size_t Size)
{    
    return __Sys_Malloc(Current_TID,Size);
}
#endif
/* End Function:Sys_Malloc ***************************************************/

/* Begin Function:__Sys_Mfree *************************************************
Description : Free the allocated memory. In the name of a certain thread.
Input       : tid_t - The thread ID.
              void xdata* Mem_Ptr - The pointer to the memory region that you want to free.
Output      : None.
Return      : None.
******************************************************************************/
#if(ENABLE_MEMM==TRUE)
void __Sys_Mfree(tid_t TID,void xdata* Mem_Ptr)
{    
    cnt_t Page_Cnt;
    
    /* See if the pointer is valid - A valid pointer must not be null and point
     * to the start address of a certain memory page.
     */
    if((Mem_Ptr==0)||((ptr_int_t)(Mem_Ptr-(Mem.DMEM_Heap)))%PAGE_SIZE!=0)
        return;
    
    /* See if the TID is valid in the system */   
    if(TID>=MAX_THREADS)
        return;
    
    /* Calculate which page it is in */
    Page_Cnt=(cnt_t)(((ptr_int_t)(Mem_Ptr-(Mem.DMEM_Heap)))/PAGE_SIZE);
    
    /* See if this memory region can be freed by this thread */
    if(Mem.Mem_CB[Page_Cnt]!=TID)
        return;
    if(Page_Cnt>0)
        if(Mem.Mem_CB[Page_Cnt-1]!=0)
            return;
    
    /* Mark the area as free */
	while(Mem.Mem_CB[Page_Cnt]==TID)
    {
        Mem.Mem_CB[Page_Cnt]=0; 
        Page_Cnt++;
    }
}
#endif
/* End Function:__Sys_Mfree **************************************************/

/* Begin Function:Sys_Mfree ***************************************************
Description : Free the allocated memory. For application use.
Input       : void xdata* Mem_Ptr -The pointer to the memory region that you want to free.
Output      : None.
Return      : None.
******************************************************************************/
#if(ENABLE_MEMM==TRUE)
void Sys_Mfree(void xdata* Mem_Ptr)
{    
    __Sys_Mfree(Current_TID,Mem_Ptr);
}
#endif
/* End Function:Sys_Mfree ****************************************************/

/* Begin Function:__Sys_Mfree_All *********************************************
Description : Free all allocated memory of a certain thread.
Input       : tid_t - The thread ID.
Output      : None.
Return      : None.
******************************************************************************/
#if(ENABLE_MEMM==TRUE)
void __Sys_Mfree_All(tid_t TID)
{    
    cnt_t Mem_Page_Cnt;
    
    /* See if the TID is valid in the system */   
    if(TID>=MAX_THREADS)
        return;
    
    /* Mark all the memory allocated by it as free */
    for(Mem_Page_Cnt=0;Mem_Page_Cnt<DMEM_PAGES;Mem_Page_Cnt++)
    {
        if(Mem.Mem_CB[Mem_Page_Cnt]==TID)
            Mem.Mem_CB[Mem_Page_Cnt]=0;
    }
}
#endif
/* End Function:__Sys_Mfree_All **********************************************/

/* Begin Function:Sys_Mfree_All ***********************************************
Description : Free all allocated memory.
Input       : None.
Output      : None.
Return      : None.
******************************************************************************/
#if(ENABLE_MEMM==TRUE)
void Sys_Mfree_All(void)
{    
    __Sys_Mfree_All(Current_TID);
}
#endif
/* End Function:Sys_Mfree_All ************************************************/

/* End Of File ***************************************************************/

/* Copyright (C) 2011-2013 Evo-Devo Instrum. All rights reserved *************/

