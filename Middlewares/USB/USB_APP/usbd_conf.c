/**
 ****************************************************************************************************
 * @file        usbd_conf.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2024-05-21
 * @brief       USB设备配置文件
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 * 
 * 实验平台:正点原子 H7R3开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 * 
 ****************************************************************************************************
 */

#include "usbd_core.h"
#include "./MALLOC/malloc.h"
#include "./SYSTEM/delay/delay.h"

/* PCD句柄 */
PCD_HandleTypeDef g_pcd_handle = {0};

/* USB连接状态标志 */
uint8_t g_usb_conn_state = 0;

/**
 * @brief   OTG HS中断服务函数
 * @param   无
 * @retval  无
 */
void OTG_HS_IRQHandler(void)
{
    HAL_PCD_IRQHandler(&g_pcd_handle);
}

/**
 * @brief   HAL库PCD初始化MSP函数
 * @param   hpcd: PCD句柄指针
 * @retval  无
 */
void HAL_PCD_MspInit(PCD_HandleTypeDef *hpcd)
{
    RCC_PeriphCLKInitTypeDef rcc_periph_clk_init_struct = {0};
    GPIO_InitTypeDef gpio_init_struct = {0};
    
    if (hpcd->Instance == USB_OTG_HS)
    {
        /* 使能USB HS电压调节器 */
        HAL_PWREx_EnableUSBHSregulator();
        
        /* 配置时钟 */
        rcc_periph_clk_init_struct.PeriphClockSelection = RCC_PERIPHCLK_USBPHYC;
        rcc_periph_clk_init_struct.UsbPhycClockSelection = RCC_USBPHYCCLKSOURCE_HSE;
        HAL_RCCEx_PeriphCLKConfig(&rcc_periph_clk_init_struct);

        /* 使能USB电源监测器 */
        HAL_PWREx_EnableUSBVoltageDetector();
        
        /* 使能时钟 */
        __HAL_RCC_USB_OTG_HS_CLK_ENABLE();
        __HAL_RCC_USBPHYC_CLK_ENABLE();
        __HAL_RCC_GPIOM_CLK_ENABLE();
        
        /* 初始化DM、DP引脚 */
        gpio_init_struct.Pin = GPIO_PIN_5 | GPIO_PIN_6;
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;
        gpio_init_struct.Pull = GPIO_NOPULL;
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        gpio_init_struct.Alternate = GPIO_AF10_OTG_HS;
        HAL_GPIO_Init(GPIOM, &gpio_init_struct);
        
        /* 配置中断 */
        HAL_NVIC_SetPriority(OTG_HS_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(OTG_HS_IRQn);
    }
}

/**
 * @brief   HAL库PCD反初始化MSP函数
 * @param   hpcd: PCD句柄指针
 * @retval  无
 */
void HAL_PCD_MspDeInit(PCD_HandleTypeDef *hpcd)
{
    if(hpcd->Instance == USB_OTG_HS)
    {
        /* 失能时钟 */
        __HAL_RCC_USB_OTG_HS_CLK_DISABLE();
        __HAL_RCC_USBPHYC_CLK_DISABLE();
        
        /* 反初始化DM、DP引脚 */
        HAL_GPIO_DeInit(GPIOM, GPIO_PIN_5 | GPIO_PIN_6);
        
        /* 关闭中断 */
        HAL_NVIC_DisableIRQ(OTG_HS_IRQn);
    }
}

/**
 * @brief   HAL库PCD建立阶段回调函数
 * @param   hpcd: PCD句柄指针
 * @retval  无
 */
void HAL_PCD_SetupStageCallback(PCD_HandleTypeDef *hpcd)
{
    USBD_LL_SetupStage((USBD_HandleTypeDef *)hpcd->pData, (uint8_t *)hpcd->Setup);
}

/**
 * @brief   HAL库PCD数据输出回调函数
 * @param   hpcd: PCD句柄指针
 * @param   epnum: 端点编号
 * @retval  无
 */
void HAL_PCD_DataOutStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
    USBD_LL_DataOutStage((USBD_HandleTypeDef *)hpcd->pData, epnum, hpcd->OUT_ep[epnum].xfer_buff);
}

/**
 * @brief   HAL库PCD数据输入回调函数
 * @param   hpcd: PCD句柄指针
 * @param   epnum: 端点编号
 * @retval  无
 */
void HAL_PCD_DataInStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
    USBD_LL_DataInStage((USBD_HandleTypeDef *)hpcd->pData, epnum, hpcd->IN_ep[epnum].xfer_buff);
}

/**
 * @brief   HAL库PCD帧起始回调函数
 * @param   hpcd: PCD句柄指针
 * @retval  无
 */
void HAL_PCD_SOFCallback(PCD_HandleTypeDef *hpcd)
{
    USBD_LL_SOF((USBD_HandleTypeDef *)hpcd->pData);
}

/**
 * @brief   HAL库PCD复位回调函数
 * @param   hpcd: PCD句柄指针
 * @retval  无
 */
void HAL_PCD_ResetCallback(PCD_HandleTypeDef *hpcd)
{
    USBD_SpeedTypeDef speed = USBD_SPEED_FULL;
    
    if ( hpcd->Init.speed == PCD_SPEED_HIGH)
    {
        speed = USBD_SPEED_HIGH;
    }
    else if ( hpcd->Init.speed == PCD_SPEED_FULL)
    {
        speed = USBD_SPEED_FULL;
    }
    else
    {
        speed = USBD_SPEED_FULL;
    }
    
    USBD_LL_SetSpeed((USBD_HandleTypeDef *)hpcd->pData, speed);
    USBD_LL_Reset((USBD_HandleTypeDef *)hpcd->pData);
}

/**
 * @brief   HAL库PCD挂起回调函数
 * @param   hpcd: PCD句柄指针
 * @retval  无
 */
void HAL_PCD_SuspendCallback(PCD_HandleTypeDef *hpcd)
{
    g_usb_conn_state = 0;
    USBD_LL_Suspend((USBD_HandleTypeDef *)hpcd->pData);
    __HAL_PCD_GATE_PHYCLOCK(hpcd);
    if (hpcd->Init.low_power_enable)
    {
        HAL_SuspendTick();
        SCB->SCR |= (uint32_t)((uint32_t)(SCB_SCR_SLEEPDEEP_Msk | SCB_SCR_SLEEPONEXIT_Msk));
    }
}

/**
 * @brief   HAL库PCD恢复回调函数
 * @param   hpcd: PCD句柄指针
 * @retval  无
 */
void HAL_PCD_ResumeCallback(PCD_HandleTypeDef *hpcd)
{
    if (hpcd->Init.low_power_enable)
    {
        HAL_ResumeTick();
        SCB->SCR &= (uint32_t)~((uint32_t)(SCB_SCR_SLEEPDEEP_Msk | SCB_SCR_SLEEPONEXIT_Msk));
    }
    __HAL_PCD_UNGATE_PHYCLOCK(hpcd);
    USBD_LL_Resume((USBD_HandleTypeDef *)hpcd->pData);
}

/**
 * @brief   HAL库PCD ISO输出不完整回调函数
 * @param   hpcd: PCD句柄指针
 * @param   epnum: 端点编号
 * @retval  无
 */
void HAL_PCD_ISOOUTIncompleteCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
    USBD_LL_IsoOUTIncomplete((USBD_HandleTypeDef *)hpcd->pData, epnum);
}

/**
 * @brief   HAL库PCD ISO输入不完整回调函数
 * @param   hpcd: PCD句柄指针
 * @param   epnum: 端点编号
 * @retval  无
 */
void HAL_PCD_ISOINIncompleteCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
    USBD_LL_IsoINIncomplete((USBD_HandleTypeDef *)hpcd->pData, epnum);
}

/**
 * @brief   HAL库PCD连接回调函数
 * @param   hpcd: PCD句柄指针
 * @retval  无
 */
void HAL_PCD_ConnectCallback(PCD_HandleTypeDef *hpcd)
{
    g_usb_conn_state = 1;
    USBD_LL_DevConnected((USBD_HandleTypeDef *)hpcd->pData);
}

/**
 * @brief   HAL库PCD断开回调函数
 * @param   hpcd: PCD句柄指针
 * @retval  无
 */
void HAL_PCD_DisconnectCallback(PCD_HandleTypeDef *hpcd)
{
    g_usb_conn_state = 0;
    USBD_LL_DevDisconnected((USBD_HandleTypeDef *)hpcd->pData);
}

/**
  * @brief  Returns the USB status depending on the HAL status:
  * @param  hal_status: HAL status
  * @retval USB status
  */
USBD_StatusTypeDef USBD_Get_USB_Status(HAL_StatusTypeDef hal_status)
{
    USBD_StatusTypeDef usb_status = USBD_OK;
    
    switch (hal_status)
    {
        case HAL_OK:
        {
            usb_status = USBD_OK;
            break;
        }
        case HAL_ERROR:
        {
            usb_status = USBD_FAIL;
            break;
        }
        case HAL_BUSY:
        {
            usb_status = USBD_BUSY;
            break;
        }
        case HAL_TIMEOUT:
        {
            usb_status = USBD_FAIL;
            break;
        }
        default:
        {
            usb_status = USBD_FAIL;
            break;
        }
    }
    
    return usb_status;
}

/**
  * @brief  Initializes the Low Level portion of the Device driver.
  * @param  pdev: Device handle
  * @retval USBD Status
  */
USBD_StatusTypeDef USBD_LL_Init(USBD_HandleTypeDef *pdev)
{
    HAL_StatusTypeDef hal_status = HAL_OK;
    USBD_StatusTypeDef usb_status = USBD_OK;
    
    if (pdev->id == DEVICE_HS)
    {
        /* PCD初始化 */
        g_pcd_handle.pData = pdev;
        pdev->pData = &g_pcd_handle;
        g_pcd_handle.Instance = USB_OTG_HS;
        g_pcd_handle.Init.dev_endpoints = 9;
        g_pcd_handle.Init.speed = PCD_SPEED_HIGH;
        g_pcd_handle.Init.phy_itface = USB_OTG_HS_EMBEDDED_PHY;
        g_pcd_handle.Init.dma_enable = DISABLE;
        g_pcd_handle.Init.Sof_enable = DISABLE;
        g_pcd_handle.Init.low_power_enable = DISABLE;
        g_pcd_handle.Init.lpm_enable = DISABLE;
        g_pcd_handle.Init.use_dedicated_ep1 = DISABLE;
        g_pcd_handle.Init.vbus_sensing_enable = DISABLE;
        hal_status = HAL_PCD_Init(&g_pcd_handle);
        usb_status = USBD_Get_USB_Status(hal_status);
        if (usb_status != USBD_OK)
        {
            return usb_status;
        }
        
        /* 配置接收、发送FiFo */
        HAL_PCDEx_SetRxFiFo(&g_pcd_handle, 0x200);
        HAL_PCDEx_SetTxFiFo(&g_pcd_handle, 0, 0x40);
        HAL_PCDEx_SetTxFiFo(&g_pcd_handle, 1, 0x80);
    }
    
    return USBD_OK;
}

/**
  * @brief  De-Initializes the Low Level portion of the Device driver.
  * @param  pdev: Device handle
  * @retval USBD Status
  */
USBD_StatusTypeDef USBD_LL_DeInit(USBD_HandleTypeDef *pdev)
{
    HAL_StatusTypeDef hal_status = HAL_OK;
    USBD_StatusTypeDef usb_status = USBD_OK;
    
    hal_status = HAL_PCD_DeInit(pdev->pData);
    usb_status = USBD_Get_USB_Status(hal_status);
    
    return usb_status;
}

/**
  * @brief  Starts the Low Level portion of the Device driver.
  * @param  pdev: Device handle
  * @retval USBD Status
  */
USBD_StatusTypeDef USBD_LL_Start(USBD_HandleTypeDef *pdev)
{
    HAL_StatusTypeDef hal_status = HAL_OK;
    USBD_StatusTypeDef usb_status = USBD_OK;
    
    hal_status = HAL_PCD_Start(pdev->pData);
    usb_status = USBD_Get_USB_Status(hal_status);
    
    return usb_status;
}

/**
  * @brief  Stops the Low Level portion of the Device driver.
  * @param  pdev: Device handle
  * @retval USBD Status
  */
USBD_StatusTypeDef USBD_LL_Stop(USBD_HandleTypeDef *pdev)
{
    HAL_StatusTypeDef hal_status = HAL_OK;
    USBD_StatusTypeDef usb_status = USBD_OK;
    
    hal_status = HAL_PCD_Stop(pdev->pData);
    usb_status = USBD_Get_USB_Status(hal_status);
    
    return usb_status;
}

/**
  * @brief  Opens an endpoint of the Low Level Driver.
  * @param  pdev: Device handle
  * @param  ep_addr: Endpoint Number
  * @param  ep_type: Endpoint Type
  * @param  ep_mps: Endpoint Max Packet Size
  * @retval USBD Status
  */
USBD_StatusTypeDef USBD_LL_OpenEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr, uint8_t ep_type, uint16_t ep_mps)
{
    HAL_StatusTypeDef hal_status = HAL_OK;
    USBD_StatusTypeDef usb_status = USBD_OK;
    
    hal_status = HAL_PCD_EP_Open(pdev->pData, ep_addr, ep_mps, ep_type);
    usb_status = USBD_Get_USB_Status(hal_status);
    
    return usb_status;
}

/**
  * @brief  Closes an endpoint of the Low Level Driver.
  * @param  pdev: Device handle
  * @param  ep_addr: Endpoint Number
  * @retval USBD Status
  */
USBD_StatusTypeDef USBD_LL_CloseEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
    HAL_StatusTypeDef hal_status = HAL_OK;
    USBD_StatusTypeDef usb_status = USBD_OK;
    
    hal_status = HAL_PCD_EP_Close(pdev->pData, ep_addr);
    usb_status = USBD_Get_USB_Status(hal_status);
    
    return usb_status;
}

/**
  * @brief  Flushes an endpoint of the Low Level Driver.
  * @param  pdev: Device handle
  * @param  ep_addr: Endpoint Number
  * @retval USBD Status
  */
USBD_StatusTypeDef USBD_LL_FlushEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
    HAL_StatusTypeDef hal_status = HAL_OK;
    USBD_StatusTypeDef usb_status = USBD_OK;
    
    hal_status = HAL_PCD_EP_Flush(pdev->pData, ep_addr);
    usb_status = USBD_Get_USB_Status(hal_status);
    
    return usb_status;
}

/**
  * @brief  Sets a Stall condition on an endpoint of the Low Level Driver.
  * @param  pdev: Device handle
  * @param  ep_addr: Endpoint Number
  * @retval USBD Status
  */
USBD_StatusTypeDef USBD_LL_StallEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
    HAL_StatusTypeDef hal_status = HAL_OK;
    USBD_StatusTypeDef usb_status = USBD_OK;
    
    hal_status = HAL_PCD_EP_SetStall(pdev->pData, ep_addr);
    usb_status = USBD_Get_USB_Status(hal_status);
    
    return usb_status;
}

/**
  * @brief  Clears a Stall condition on an endpoint of the Low Level Driver.
  * @param  pdev: Device handle
  * @param  ep_addr: Endpoint Number
  * @retval USBD Status
  */
USBD_StatusTypeDef USBD_LL_ClearStallEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
    HAL_StatusTypeDef hal_status = HAL_OK;
    USBD_StatusTypeDef usb_status = USBD_OK;
    
    hal_status = HAL_PCD_EP_ClrStall(pdev->pData, ep_addr);
    usb_status = USBD_Get_USB_Status(hal_status);
    
    return usb_status;
}

/**
  * @brief  Returns Stall condition.
  * @param  pdev: Device handle
  * @param  ep_addr: Endpoint Number
  * @retval Stall (1: Yes, 0: No)
  */
uint8_t USBD_LL_IsStallEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
    PCD_HandleTypeDef *hpcd = (PCD_HandleTypeDef *)pdev->pData;
    
    if((ep_addr & 0x80) == 0x80)
    {
        return hpcd->IN_ep[ep_addr & 0x7F].is_stall;
    }
    else
    {
        return hpcd->OUT_ep[ep_addr & 0x7F].is_stall;
    }
}

/**
  * @brief  Assigns a USB address to the device.
  * @param  pdev: Device handle
  * @param  dev_addr: Endpoint Number
  * @retval USBD Status
  */
USBD_StatusTypeDef USBD_LL_SetUSBAddress(USBD_HandleTypeDef *pdev, uint8_t dev_addr)
{
    HAL_StatusTypeDef hal_status = HAL_OK;
    USBD_StatusTypeDef usb_status = USBD_OK;
    
    hal_status = HAL_PCD_SetAddress(pdev->pData, dev_addr);
    usb_status = USBD_Get_USB_Status(hal_status);
    
    return usb_status;
}

/**
  * @brief  Transmits data over an endpoint.
  * @param  pdev: Device handle
  * @param  ep_addr: Endpoint Number
  * @param  pbuf: Pointer to data to be sent
  * @param  size: Data size
  * @retval USBD Status
  */
USBD_StatusTypeDef USBD_LL_Transmit(USBD_HandleTypeDef *pdev, uint8_t ep_addr, uint8_t *pbuf, uint32_t size)
{
    HAL_StatusTypeDef hal_status = HAL_OK;
    USBD_StatusTypeDef usb_status = USBD_OK;
    
    hal_status = HAL_PCD_EP_Transmit(pdev->pData, ep_addr, pbuf, size);
    usb_status = USBD_Get_USB_Status(hal_status);
    
    return usb_status;
}

/**
  * @brief  Prepares an endpoint for reception.
  * @param  pdev: Device handle
  * @param  ep_addr: Endpoint Number
  * @param  pbuf: Pointer to data to be received
  * @param  size: Data size
  * @retval USBD Status
  */
USBD_StatusTypeDef USBD_LL_PrepareReceive(USBD_HandleTypeDef *pdev, uint8_t ep_addr, uint8_t *pbuf, uint32_t size)
{
    HAL_StatusTypeDef hal_status = HAL_OK;
    USBD_StatusTypeDef usb_status = USBD_OK;
    
    hal_status = HAL_PCD_EP_Receive(pdev->pData, ep_addr, pbuf, size);
    usb_status = USBD_Get_USB_Status(hal_status);
    
    return usb_status;
}

/**
  * @brief  Returns the last transferred packet size.
  * @param  pdev: Device handle
  * @param  ep_addr: Endpoint Number
  * @retval Received Data Size
  */
uint32_t USBD_LL_GetRxDataSize(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
    return HAL_PCD_EP_GetRxCount((PCD_HandleTypeDef *)pdev->pData, ep_addr);
}

/**
  * @brief  Static single allocation.
  * @param  size: Size of allocated memory
  * @retval None
  */
void *USBD_static_malloc(uint32_t size)
{
    return mymalloc(SRAMIN, size);

/* USB连接状态标志 */
uint8_t g_usb_conn_state = 0;
}

/**
  * @brief  Dummy memory free
  * @param  p: Pointer to allocated  memory address
  * @retval None
  */
void USBD_static_free(void *p)
{
    myfree(SRAMIN, p);
}

/**
  * @brief  Delays routine for the USB Device Library.
  * @param  Delay: Delay in ms
  * @retval None
  */
void USBD_LL_Delay(uint32_t Delay)
{
    delay_ms(Delay);
}
