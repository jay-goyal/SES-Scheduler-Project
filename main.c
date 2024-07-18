#include "stm32f407xx.h"
#include "stm32f4xx_hal.h"

#define STACK_SIZE 256
#define ARRIVE1 2000
#define ARRIVE2 4000

// Task states enumeration
typedef enum {NEW, READY, RUNNING, WAITING, TERMINATED} TaskState;

typedef struct {
	uint32_t *psp[7];
	uint32_t priorities[7];
	TaskState states[7];
} TCB;

// Maintaining State
TCB tcbs;
uint32_t taskStacks[7][STACK_SIZE];
uint32_t curr_task = 0;
uint32_t slotOne = 0;
uint32_t slotTwo = 0;
uint32_t topPriority = 2;
uint32_t timer = 0;
volatile uint32_t *psp_val;
uint32_t i1 = 0, i2 = 0, i3 = 0, i4 = 0, i5 = 0, i6 = 0;

// Tasks 1-6
void task1(void);
void task2(void);
void task3(void);
void task4(void);
void task5(void);
void task6(void);
void main(void);

// TCB and stack initialization functions
void initTCB(void);
uint32_t* initStack(uint32_t* psp, void (*task)(void));

// System Clock Configuration
void SystemClock_Config(void);

// Scheduler Utililty Functions
void saveContext(void);

void task1(void) {
	while(1) {
		i1++;
	}
}

void task2(void) {
	while(1) {
		i2++;
	}
}

void task3(void) {
	while(1) {
		i3++;
	}
}

void task4(void) {
	while(1) {
		i4++;
	}
}

void task5(void) {
	while(1) {
		i5++;
	}
}

void task6(void) {
	while(1) {
		i6++;
	}
}

// TCB Initilization Functions
void initTCB(void) {
	void (*taskFunctions[7])(void) = {main, task1, task2, task3, task4, task5, task6};
	for (uint32_t i = 0; i < 7; i++) {
		uint32_t* psp = taskStacks[i] + STACK_SIZE;
		tcbs.psp[i] = initStack(psp, taskFunctions[i]);
		tcbs.states[i] = (i>0) ? NEW : READY;
		tcbs.priorities[i] = (i>3) ? 0 : (i>0) ? 1 : 2;
	}
}

uint32_t* initStack(uint32_t* psp, void (*task)(void)) {
	*(--psp) = 0x01000000;
	*(--psp) = (uint32_t)task;
	*(--psp) = 0x00000000;
	for (int i = 0; i < 13; i++) {
        *(--psp) = 0;
  }
	return psp;
}

// Setup System Clock
void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;

  /* Enable Power Control clock */
  __HAL_RCC_PWR_CLK_ENABLE();

  /* The voltage scaling allows optimizing the power consumption when the
     device is clocked below the maximum system frequency (see datasheet). */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /*  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
     clocks dividers */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 |
                                RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);
	return;
}

// Implement Context Switching
__asm void PendSV_Handler(void) {
	LDR R3,=__cpp(&tcbs.psp)
	LDR R1,=__cpp(&curr_task)
	LDRB R2,[R1]
	LDR R0,[R3,R2,LSL#2]
	LDMFD R0!, {R4-R11}  // Load register values R4-R11 from the stack into the registers
  MSR PSP, R0
	SUBS R2,#0
	LDRNE LR, =0xFFFFFFFD
	BX LR
}


void SysTick_Handler(void) {
	saveContext();
	timer++;
	if (timer % 3 == 0 || timer == ARRIVE1 || timer == ARRIVE2) {
		tcbs.psp[curr_task] = (uint32_t*)psp_val;
		if (timer >= ARRIVE2 &&  topPriority > 0) {
			topPriority = 0;
			for (int i = 4; i < 7; i++) {
				tcbs.states[i] = READY;
			}
		} else if (timer >= ARRIVE1 && topPriority > 1) {
			topPriority = 1;
			for (int i = 1; i < 4; i++) {
				tcbs.states[i] = READY;
			}
		}
		
		do {
			curr_task = (curr_task + 1) % 7;
		} while (tcbs.priorities[curr_task] != topPriority || tcbs.states[curr_task] != READY);
	}
	
	SCB->ICSR|=SCB_ICSR_PENDSVSET_Msk;
	return;
}

__asm void HardFault_Handler(void) {
	LDR R0, =0xE000ED28
	LDR R1, [R0]
	B .
}

__asm void saveContext (void){
  MRS R0,PSP
  STMDB R0!, {R4-R11}
  LDR R1,=__cpp(&psp_val)
	STR R0,[R1]
	MOV R0,LR
	BX R0
}

void main(void) {
	SystemClock_Config();
	initTCB();
	HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
	HAL_NVIC_SetPriority(PendSV_IRQn, 15, 0);
	HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);
	HAL_SYSTICK_Config(0x000CD13F);
	while(1);
}
