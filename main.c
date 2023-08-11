/***********************************************************************************
 * File Name:   main.c
 *
 * Description: This is the source code for the USB HID generic code example 
 *              application using emUSB device stack for ModusToolbox.
 *
 * Related Document: See README.md
 *
 ***********************************************************************************
* Copyright 2021-2023, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
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
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
 **********************************************************************************/

/* MTB Header file includes*/
#include "cybsp.h"
#include "cy_utils.h"
#include "cy_retarget_io.h"
#include <string.h>

/* emUSB-Device header file includes */
#include "USB.h"
#include "USB_HID.h"

/***********************************************************************************
 *       Defines configurable
 **********************************************************************************/
#define VENDOR_ID           0x058B  /* For Infineon Technologies */
#define PRODUCT_ID          0x0274  /* Procured PID for HID Generic device */

#define INPUT_REPORT_SIZE   64      /* Defines the input (device -> host) report size */
#define OUTPUT_REPORT_SIZE  64      /* Defines the output (Host -> device) report size */
#define VENDOR_PAGE_ID      0x12    /* Defines the vendor specific page that */
                                    /* shall be used, allowed values 0x00 - 0xff */
                                    /* This value must be identical to HOST application */

/***********************************************************************************
 *       Forward declarations
 **********************************************************************************/

#ifdef __cplusplus
extern "C" {     /* Make sure we have C-declarations in C++ programs */
#endif
    static void usbd_hid_init(void);
    static void usbd_hid_echo_task(void);
#ifdef __cplusplus
}
#endif


/***********************************************************************************
 *       Static const data
 **********************************************************************************/
 
/* Information that are used during enumeration  */
static const USB_DEVICE_INFO device_info = {
    VENDOR_ID,                              /* VendorId */
    PRODUCT_ID,                             /* ProductId */
    "Infineon Technologies",                /* VendorName */
    "HID 64-byte Generic emUSB device",     /* ProductName */
    "12345678"                              /* SerialNumber */
};


/***********************************************************************************
 *
 *       usb_hid_generic_report
 *
 * This report is generated according to the HID spec and HID Usage Tables
 * specifications given by the USB implementer's forum on their website. 
 *
 * [HID spec] -          Device Class Definition for Human Interface Devices (HID)
 *                       Firmware Specification—5/27/01, Version 1.11
 *                       https://www.usb.org/sites/default/files/hid1_11.pdf
 *
 * [HID Usage Tables] -  HID Usage Tables for Universal Serial Bus (USB)
 *                       Version 1.3
 *                       https://www.usb.org/sites/default/files/hut1_3_0.pdf
 *
 * [HID Descriptor] -    HID Descriptor Tool
 *                       https://www.usb.org/document-library/hid-descriptor-tool
 *
 *
 */
static const U8 usb_hid_generic_report[] = {
    0x06, VENDOR_PAGE_ID, 0xFF,    /* USAGE_PAGE (Vendor Defined Page 1) */
    0x09, 0x01,                    /* USAGE (Vendor Usage 1) */
    0xa1, 0x01,                    /* COLLECTION (Application) */
    0x05, 0x06,                    /* USAGE_PAGE (Generic Device) */
    0x09, 0x00,                    /* USAGE (Undefined) */
    0x15, 0x00,                    /* LOGICAL_MINIMUM (0) */
    0x26, 0xFF, 0x00,              /* LOGICAL_MAXIMUM (255) */
    0x95, INPUT_REPORT_SIZE,       /* REPORT_COUNT (64) -- Number of packets */
    0x75, 0x08,                    /* REPORT_SIZE (8) -- Size of each packet */
    0x81, 0x02,                    /* INPUT (Data,Var,Abs) */
    0x05, 0x06,                    /* USAGE_PAGE (Generic Device) */
    0x09, 0x00,                    /* USAGE (Undefined) */
    0x15, 0x00,                    /* LOGICAL_MINIMUM (0) */
    0x26, 0xFF, 0x00,              /* LOGICAL_MAXIMUM (255) */
    0x95, OUTPUT_REPORT_SIZE,      /* REPORT_COUNT (64) */
    0x75, 0x08,                    /* REPORT_SIZE (8) */
    0x91, 0x02,                    /* OUTPUT (Data,Var,Abs) */
    0xc0                           /* END_COLLECTION */
};

/***********************************************************************************
 * Global Variables
 **********************************************************************************/
static int8_t               hid_data_buffer[USB_HS_INT_MAX_PACKET_SIZE];
static USB_HID_HANDLE       hid_instance_handle;

/***********************************************************************************
 * Function Name: main
 ***********************************************************************************
 * Summary:
 * This is the main function for CM4 CPU. 
 *    1. It initializes the emUSB Device stack
 *    2. It enumerates a generic HID device. 
 *    3. It startes the HID echo functionality.
 * 
 * Parameters:
 *  void
 *
 * Return:
 *  int
 *
 **********************************************************************************/
int main(void)
{
    cy_rslt_t result;

    /* Initialize the device and board peripherals */
    result = cybsp_init() ;
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Enable global interrupts */
    __enable_irq();

    /* Initialize printf retarget */
    cy_retarget_io_init(CYBSP_DEBUG_UART_HW);

    /* \x1b[2J\x1b[;H - ANSI ESC sequence for clear screen */
    printf("\x1b[2J\x1b[;H");

    printf( "*********************************************************************\r\n"
            "\t\temUSB Device: HID generic application                              \r\n"
            "*********************************************************************\r\n\n");

    /* Initialize the emUSB device stack */
    USBD_Init();
    printf("APP_LOG: Initialization of USB Device successful\r\n\n");

    /* Initialize the endpoint for HID class */
    usbd_hid_init();
    printf("APP_LOG: End Point initialization done\r\n\n");

    /* Set the device info for enumeration*/
    USBD_SetDeviceInfo(&device_info);
    printf("APP_LOG: Configured the device information for enumeration \r\n\n");

    /* Start the USB stack*/
    USBD_Start();
    printf("\nAPP_LOG: USB device stack started\n\n");

    usbd_hid_echo_task();
}

/***********************************************************************************
 * Function Name: usbd_hid_init
 ***********************************************************************************
 * Summary:
 *  This function initializes the endpoints for HID class and captures the HID 
 *  description form the report descriptor defined in usb_hid_generic_report.   
 *
 * Parameters:
 *  void
 *
 * Return:
 *  void
 *
 **********************************************************************************/
static void usbd_hid_init(void) {
    static uint8_t              out_buffer[USB_HS_INT_MAX_PACKET_SIZE];
    USB_HID_INIT_DATA_EX        init_data;
    USB_ADD_EP_INFO             ep_int_in;
    USB_ADD_EP_INFO             ep_int_out;

    memset(&init_data, 0, sizeof(init_data));
    ep_int_in.Flags             = 0;                             /* Flags not used. */
    ep_int_in.InDir             = USB_DIR_IN;                    /* IN direction (Device to Host) */
    ep_int_in.Interval          = 1;                             /* Interval of 125 us (1 ms in full-speed) */
    ep_int_in.MaxPacketSize     = USB_HS_INT_MAX_PACKET_SIZE;    /* Maximum packet size (64 for Interrupt). */
    ep_int_in.TransferType      = USB_TRANSFER_TYPE_INT;         /* Endpoint type - Interrupt. */
    init_data.EPIn = USBD_AddEPEx(&ep_int_in, NULL, 0);

    ep_int_out.Flags            = 0;                             /* Flags not used. */
    ep_int_out.InDir            = USB_DIR_OUT;                   /* OUT direction (Host to Device) */
    ep_int_out.Interval         = 1;                             /* Interval of 125 us (1 ms in full-speed) */
    ep_int_out.MaxPacketSize    = USB_HS_INT_MAX_PACKET_SIZE;    /* Maximum packet size (64 for Interrupt). */
    ep_int_out.TransferType     = USB_TRANSFER_TYPE_INT;         /* Endpoint type - Interrupt. */
    init_data.EPOut = USBD_AddEPEx(&ep_int_out, out_buffer, sizeof(out_buffer));

    init_data.pReport           = usb_hid_generic_report;
    init_data.NumBytesReport    = sizeof(usb_hid_generic_report);
    init_data.pInterfaceName    = "HID raw";
    hid_instance_handle         = USBD_HID_AddEx(&init_data);
}

/*******************************************************************************
* Function Name: delay
********************************************************************************
* Summary:
* This is the delay generation function based on the MCU clock cycles
*
* Parameters:
*  uint32_t cycles
*
* Return:
*  void
*
*******************************************************************************/
 void delay(uint32_t cycles)
{
    while(--cycles)
    {
        __NOP();       /* No operation */
    }
}

/***********************************************************************************
 * Function Name: usbd_hid_echo_task
 ***********************************************************************************
 * Summary:
 *  This function implements the HID echo functionality. The HID device is
 *  configured and waits for status configuration. Once the device is connected
 *  to the host and receives a packet of HID generic input, it echoes the same 
 *  back to the source. 
 *
 * Parameters:
 *  void
 *
 * Return:
 *  void
 *
 **********************************************************************************/
static void usbd_hid_echo_task(void) {

    int8_t check_read = 0;
    int8_t check_write = 0;

    for(int32_t i =0; i < USB_HS_INT_MAX_PACKET_SIZE; i++) {
        /* Wait for configuration */
        while ((USBD_GetState() & USB_STAT_CONFIGURED ) != USB_STAT_CONFIGURED) {
            
            /* Ready to Read and Write HID messages. */
            /* Waiting for USB_STAT_CONFIGURED */

            delay(XMC_SCU_CLOCK_GetCpuClockFrequency());
            XMC_GPIO_SetOutputHigh(CYBSP_USER_LED_PORT, CYBSP_USER_LED_PIN);
            
            delay(XMC_SCU_CLOCK_GetCpuClockFrequency());
            XMC_GPIO_SetOutputLow(CYBSP_USER_LED_PORT, CYBSP_USER_LED_PIN);

            printf("Waiting for connection...\r\n");
        }

    check_read = USBD_HID_Read(hid_instance_handle, &hid_data_buffer, OUTPUT_REPORT_SIZE, 0);

    if(OUTPUT_REPORT_SIZE == check_read)
    {
        printf("\nRequested data was successfully read within the given timeout\n\n");
    }
    else if ((check_read >= 0) && (check_read < OUTPUT_REPORT_SIZE))
    {
        printf("Timeout has occurred. Number of bytes that have been read within the given timeout: %d", check_read);
    }
    else
    {
        printf("Failed to read\n\n");
    }
    check_write = USBD_HID_Write(hid_instance_handle, &hid_data_buffer, INPUT_REPORT_SIZE, 0);

    if(INPUT_REPORT_SIZE == check_write)
    {
        printf("Write transfer successfully completed\n\n");
    }
    else if(0 == check_write)
    {
        printf("Successful started an asynchronous write transfer or a timeout has occurred and no data was written.\n\n");
    }
    else if ((check_write > 0) && (check_write < INPUT_REPORT_SIZE))
    {
        printf("Timeout has occurred. Number of bytes that have been written within the given timeout: %d", check_write);
    }
    else
    {
        printf("Failed to write\n\n");
    }
  }
}


/* [] END OF FILE */
