/*
 * Realtek Semiconductor Corp.
 *
 * stack_s.c
 *      ARMv8-M RTOS Secure Process Stack Management - API
 *
 * Copyright (C) 20015-2016 Jethro Hsu (jethro@realtek.com)
 */
#include <arm_cmse.h>
#include <section_config.h>

#define NUMBER_OF_THREAD	32

#define TASKNAME_LEN 		12		// max string length is TASKNAME_LEN - 1

#if defined(NO_HEAP)
#define SECURE_STACK_SIZE	(1024/4)
struct secure_stack {
	int stack_s[SECURE_STACK_SIZE];
	int *sp;
	uint32_t owner;
	char name[TASKNAME_LEN];
}__attribute__ ((aligned (8)));
#else
struct secure_stack {
	int *stack_s;
	int *sp;
	uint32_t owner;
	char name[TASKNAME_LEN];
};
extern void *pvPortMalloc( size_t xWantedSize );
extern void vPortFree( void *pv );
#endif

//pvPortMalloc
static struct secure_stack stacks[NUMBER_OF_THREAD];

// set task name, created from secure domain
void TZ_Set_Task_Name(int id, char* name)
{
	int i;
	for(i=0;i<TASKNAME_LEN-1;i++){
		stacks[id].name[i] = name[i];
	}
}

char* TZ_Get_Task_Name(int id)
{
	return stacks[id].name;
}

#define ST_UNKNOWN 	0
#define ST_INITED	1
static int stack_status = ST_UNKNOWN;

uint32_t __Init_Stack_S(void)
{
	if(stack_status != ST_UNKNOWN)	return 0;
	
	for (int i = 0; i < NUMBER_OF_THREAD; i++) {
		#if defined(NO_HEAP)
		for (int j = 0; j < SECURE_STACK_SIZE; j ++)
			stacks[i].stack_s[j] = 0x0;
		stacks[i].sp = &stacks[i].stack_s[SECURE_STACK_SIZE - 5];
		#else
		stacks[i].stack_s = NULL;
		stacks[i].sp = NULL;
		#endif
		
		stacks[i].owner = 0x0;
		stacks[i].name[0] = 0x0;
	}
	stack_status = ST_INITED;
	return 0;  
}

SECTION_NS_ENTRY_FUNC
uint32_t NS_ENTRY TZ_Init_Stack_S (void)
{
	return __Init_Stack_S();
}

SECTION_NS_ENTRY_FUNC
uint32_t NS_ENTRY TZ_Alloc_Stack_S (uint32_t module_id, uint32_t stackdepth)
{
	uint32_t context_id = -1;
	int i;

	if(stack_status==ST_UNKNOWN){
		__Init_Stack_S();
	}
	
	if(stackdepth==0)	return 0xFFFFFFFF;
	
	for (i = 0; i < NUMBER_OF_THREAD; i++) {
		if (stacks[i].owner == 0x0) {
		#if !defined(NO_HEAP)
			stacks[i].stack_s = pvPortMalloc(stackdepth*sizeof(int));
			if(!stacks[i].stack_s) return 0xFFFFFFFF;
			stacks[i].sp = &stacks[i].stack_s[stackdepth - 5];
		#endif	
			stacks[i].owner = module_id;
			context_id = i;
			break;
		}
	}

	return context_id;
}

SECTION_NS_ENTRY_FUNC
uint32_t NS_ENTRY TZ_Free_Stack_S (uint32_t context_id)
{
#if defined(NO_HEAP)
	int i;
#endif
	if (context_id >= NUMBER_OF_THREAD)
		return -1;
	
	if(context_id == 0xFFFFFFFF)	return 0;
#if defined(NO_HEAP)
	for (i = 0; i < SECURE_STACK_SIZE; i++)
		stacks[context_id].stack_s[i] = 0;
#else
	vPortFree(stacks[context_id].stack_s);
	stacks[context_id].stack_s = NULL;
#endif
	stacks[context_id].owner = 0x0;
	stacks[context_id].sp = NULL;
	stacks[context_id].name[0] = 0x0;

	return 0;
}

SECTION_NS_ENTRY_FUNC
uint32_t NS_ENTRY TZ_Load_Context_S (uint32_t context_id)
{
	char *give_msp_s;
	char *stack_limit;
	
	if(context_id == 0xFFFFFFFF)	return 0;

	give_msp_s = (char*)stacks[context_id].sp;
	stack_limit = (char*)&stacks[context_id].stack_s[0];

        __asm volatile (
                        "msr msp, %0;                    \n\t"
                        "msr msplim, %1;                 \n\t"
                        : : "r" (give_msp_s), "r" (stack_limit) : "memory"
        );
		/*
        __asm volatile (
                        "msr msp, %0;                    \n\t"
                        : : "r" (give_msp_s), "r" (stack_limit) : "memory"
        );		
		*/
	return 0;
}

SECTION_NS_ENTRY_FUNC
uint32_t NS_ENTRY TZ_Store_Context_S (uint32_t context_id)
{
	char *get_msp_s;
	char *stack_limit;

	
	if(context_id == 0xFFFFFFFF)	return 0;
	
	__asm volatile (
			"mrs %0, msp;			\n\t"
			"mrs %1, msplim;		\n\t"
			: "=r" (get_msp_s), "=r" (stack_limit) :: "memory"
	);

	/* save secure stack pointer */
	stacks[context_id].sp = (int*)get_msp_s;
	//to keep compiler quiet
	(void) stack_limit;
	return 0;
}
