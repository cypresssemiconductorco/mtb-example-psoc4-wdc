/******************************************************************************
* File Name:   main.c
*
* Description: This is the source code for the PSoC4 - Watchdog Counter
*              Interrupts Application for ModusToolbox.
*
* Related Document: See README.md
*
*******************************************************************************
* (c) (2020), Cypress Semiconductor Corporation. All rights reserved.
*******************************************************************************
* This software, including source code, documentation and related materials
* ("Software"), is owned by Cypress Semiconductor Corporation or one of its
* subsidiaries ("Cypress") and is protected by and subject to worldwide patent
* protection (United States and foreign), United States copyright laws and
* international treaty provisions. Therefore, you may use this Software only
* as provided in the license agreement accompanying the software package from
* which you obtained this Software ("EULA").
*
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software source
* code solely for use in connection with Cypress's integrated circuit products.
* Any reproduction, modification, translation, compilation, or representation
* of this Software except as specified above is prohibited without the express
* written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer of such
* system or application assumes all risk of such use and in doing so agrees to
* indemnify Cypress against all liability.
*******************************************************************************/

/*******************************************************************************
 * Include header files
 ******************************************************************************/
#include "cy_pdl.h"
#include "cybsp.h"

/*******************************************************************************
* Macros
********************************************************************************/
/* WDC Interrupt flag states */
#define WDC_INTERRUPT_FLAG_CLEAR    0
#define WDC_INTERRUPT_FLAG_SET      1

/* ILO Compensation flag states */
#define ILO_COMP_FLAG_CLEAR         0
#define ILO_COMP_FLAG_SET           1

/* Define COUNTER0 delay in microseconds */
#define COUNTER0_DELAY_US           (250 * 1000)

/*******************************************************************************
* Global Variables
********************************************************************************/
/* WDC Interrupt flag */
volatile uint32_t interruptFlag = 0;

/* ILO Compensation flag */
volatile uint32_t iloCompensationFlag = 0;

/*******************************************************************************
* Function Name: wdc_interrupt_handler
********************************************************************************
* Summary:
* Function to handle the WDC interrupt
*
*******************************************************************************/
void wdc_interrupt_handler(void)
{
    uint32_t status = Cy_WDC_GetInterruptStatus(WCO);
    if((status & CY_WDC_COUNTER0_Msk) != 0)
    {
        /*
         * Set the flag to perform ILO Compensation. ILO Compensation is
         * performed every time Counter0 interrupt occurs.
         */
        iloCompensationFlag = ILO_COMP_FLAG_SET;
    }
    if((status & CY_WDC_COUNTER1_Msk) != 0)
    {
        /* Does nothing */
    }
    if((status & CY_WDC_COUNTER2_Msk) != 0)
    {
        /* Set the WDC interrupt flag */
        interruptFlag = WDC_INTERRUPT_FLAG_SET;
    }
    Cy_WDC_ClearInterrupt(WCO, CY_WDC_COUNTERS_Msk);
}

/*******************************************************************************
* Function Name: main
********************************************************************************
* Summary:
* This is the main function for the application. This function enabled the WDC.
* It toggles the USER LED every time the Counter2 generates an interrupt.
*
* Parameters:
*  void
*
* Return:
*  int
*
*******************************************************************************/
int main(void)
{
    cy_rslt_t result;
    cy_en_wdc_status_t wdcStatus;

    uint32_t iloCompensatedCounts = 0U;
    uint32_t tempIloCounts = 0u;

    /* Initialize the device and board peripherals */
    result = cybsp_init() ;
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Enable global interrupts */
    __enable_irq();

    /* Starts the ILO accuracy/Trim measurement */
    Cy_SysClk_IloStartMeasurement();

    /* Setup the WDT interrupt */
    (void)Cy_SysInt_SetVector(CYBSP_WDC_IRQ, wdc_interrupt_handler);
    NVIC_EnableIRQ(CYBSP_WDC_IRQ);

    /* Initialize WDC */
    wdcStatus = Cy_WDC_Init(CYBSP_WDC_HW, &CYBSP_WDC_config);
    if(wdcStatus != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Enable all 3 WDC Counters */
    Cy_WDC_Enable(CYBSP_WDC_HW, CY_WDC_COUNTERS_Msk, CY_WDC_CLK_ILO_3CYCLES_US);

    for (;;)
    {
        /*
         * A time period of 1 second can be generated by using a single
         * counter. However, this CE uses all three counters to demonstrate
         * cascading of counters.
         */

        if(interruptFlag == WDC_INTERRUPT_FLAG_SET)
        {
            /* Toggle USER LED when WDC Counter2 interrupt occurs */
            Cy_GPIO_Inv(CYBSP_USER_LED_PORT, CYBSP_USER_LED_NUM);

            /* Clear the interrupt flag */
            interruptFlag = WDC_INTERRUPT_FLAG_CLEAR;
        }

        if(iloCompensationFlag == ILO_COMP_FLAG_SET)
        {
            /* Get the ILO compensated counts i.e. the actual counts for the desired delay */
            if(CY_SYSCLK_SUCCESS == Cy_SysClk_IloCompensate(COUNTER0_DELAY_US, &tempIloCounts))
            {
                iloCompensatedCounts = (uint32)tempIloCounts;
                /*
                 * Update the match value of Counter0. As the counters are cascaded, the
                 * counters Counter1 and Counter2 are dependent on Counter0 match value
                 * and need not be modified.
                 */
                Cy_WDC_SetMatch(CYBSP_WDC_HW, CY_WDC_COUNTER0, iloCompensatedCounts, 0);
            }
            /* Clear the ILO Compensation flag */
            iloCompensationFlag = ILO_COMP_FLAG_CLEAR;
        }
    }
}

/* [] END OF FILE */