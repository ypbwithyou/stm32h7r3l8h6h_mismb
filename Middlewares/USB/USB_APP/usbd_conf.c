/**
 ****************************************************************************************************
 * @file        usbd_conf.c
 * @author      ����ԭ���Ŷ�(ALIENTEK)
 * @version     V1.0
 * @date        2024-05-21
 * @brief       USB�豸�����ļ�
 * @license     Copyright (c) 2020-2032, �������������ӿƼ����޹�˾
 ****************************************************************************************************
 * @attention
 *
 * ʵ��ƽ̨:����ԭ�� H7R3������
 * ������Ƶ:www.yuanzige.com
 * ������̳:www.openedv.com
 * ��˾��ַ:www.alientek.com
 * �����ַ:openedv.taobao.com
 *
 ****************************************************************************************************
 */

#include "usbd_core.h"
#include "./MALLOC/malloc.h"
#include "./SYSTEM/delay/delay.h"

/* PCD��� */
PCD_HandleTypeDef g_pcd_handle = {0};

/* USB����״̬��־ */
uint8_t g_usb_conn_state = 0;

/**
 * @brief   OTG HS�жϷ�����
 * @param   ��
 * @retval  ��
 */
void OTG_HS_IRQHandler(void)
{
    HAL_PCD_IRQHandler(&g_pcd_handle);
}

/**
 * @brief   HAL��PCD��ʼ��MSP����
 * @param   hpcd: PCD���ָ��
 * @retval  ��
 */
void HAL_PCD_MspInit(PCD_HandleTypeDef *hpcd)
{
    RCC_PeriphCLKInitTypeDef rcc_periph_clk_init_struct = {0};
    GPIO_InitTypeDef gpio_init_struct = {0};

    if (hpcd->Instance == USB_OTG_HS)
    {
        /* ʹ��USB HS��ѹ������ */
        HAL_PWREx_EnableUSBHSregulator();

        /* ����ʱ�� */
        rcc_periph_clk_init_struct.PeriphClockSelection = RCC_PERIPHCLK_USBPHYC;
        rcc_periph_clk_init_struct.UsbPhycClockSelection = RCC_USBPHYCCLKSOURCE_HSE;
        HAL_RCCEx_PeriphCLKConfig(&rcc_periph_clk_init_struct);

        /* ʹ��USB��Դ����� */
        HAL_PWREx_EnableUSBVoltageDetector();

        /* ʹ��ʱ�� */
        __HAL_RCC_USB_OTG_HS_CLK_ENABLE();
        __HAL_RCC_USBPHYC_CLK_ENABLE();
        __HAL_RCC_GPIOM_CLK_ENABLE();

        /* ��ʼ��DM��DP���� */
        gpio_init_struct.Pin = GPIO_PIN_5 | GPIO_PIN_6;
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;
        gpio_init_struct.Pull = GPIO_NOPULL;
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        gpio_init_struct.Alternate = GPIO_AF10_OTG_HS;
        HAL_GPIO_Init(GPIOM, &gpio_init_struct);

        /* �����ж� */
        HAL_NVIC_SetPriority(OTG_HS_IRQn, 10, 0);
        HAL_NVIC_EnableIRQ(OTG_HS_IRQn);
    }
}
 
/**
 * @brief   HAL��PCD����ʼ��MSP����
 * @param   hpcd: PCD���ָ��
 * @retval  ��
 */
void HAL_PCD_MspDeInit(PCD_HandleTypeDef *hpcd)
{
    if (hpcd->Instance == USB_OTG_HS)
    {
        /* ʧ��ʱ�� */
        __HAL_RCC_USB_OTG_HS_CLK_DISABLE();
        __HAL_RCC_USBPHYC_CLK_DISABLE();

        /* ����ʼ��DM��DP���� */
        HAL_GPIO_DeInit(GPIOM, GPIO_PIN_5 | GPIO_PIN_6);

        /* �ر��ж� */
        HAL_NVIC_DisableIRQ(OTG_HS_IRQn);
    }
}

/**
 * @brief   HAL��PCD�����׶λص�����
 * @param   hpcd: PCD���ָ��
 * @retval  ��
 */
void HAL_PCD_SetupStageCallback(PCD_HandleTypeDef *hpcd)
{
    USBD_LL_SetupStage((USBD_HandleTypeDef *)hpcd->pData, (uint8_t *)hpcd->Setup);
}

/**
 * @brief   HAL��PCD��������ص�����
 * @param   hpcd: PCD���ָ��
 * @param   epnum: �˵���
 * @retval  ��
 */
void HAL_PCD_DataOutStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
    USBD_LL_DataOutStage((USBD_HandleTypeDef *)hpcd->pData, epnum, hpcd->OUT_ep[epnum].xfer_buff);
}

/**
 * @brief   HAL��PCD��������ص�����
 * @param   hpcd: PCD���ָ��
 * @param   epnum: �˵���
 * @retval  ��
 */
void HAL_PCD_DataInStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
    USBD_LL_DataInStage((USBD_HandleTypeDef *)hpcd->pData, epnum, hpcd->IN_ep[epnum].xfer_buff);
}

/**
 * @brief   HAL��PCD֡��ʼ�ص�����
 * @param   hpcd: PCD���ָ��
 * @retval  ��
 */
void HAL_PCD_SOFCallback(PCD_HandleTypeDef *hpcd)
{
    USBD_LL_SOF((USBD_HandleTypeDef *)hpcd->pData);
}

/**
 * @brief   HAL��PCD��λ�ص�����
 * @param   hpcd: PCD���ָ��
 * @retval  ��
 */
void HAL_PCD_ResetCallback(PCD_HandleTypeDef *hpcd)
{
    USBD_SpeedTypeDef speed = USBD_SPEED_FULL;

    if (hpcd->Init.speed == PCD_SPEED_HIGH)
    {
        speed = USBD_SPEED_HIGH;
    }
    else if (hpcd->Init.speed == PCD_SPEED_FULL)
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
 * @brief   HAL��PCD����ص�����
 * @param   hpcd: PCD���ָ��
 * @retval  ��
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
 * @brief   HAL��PCD�ָ��ص�����
 * @param   hpcd: PCD���ָ��
 * @retval  ��
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
 * @brief   HAL��PCD ISO����������ص�����
 * @param   hpcd: PCD���ָ��
 * @param   epnum: �˵���
 * @retval  ��
 */
void HAL_PCD_ISOOUTIncompleteCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
    USBD_LL_IsoOUTIncomplete((USBD_HandleTypeDef *)hpcd->pData, epnum);
}

/**
 * @brief   HAL��PCD ISO���벻�����ص�����
 * @param   hpcd: PCD���ָ��
 * @param   epnum: �˵���
 * @retval  ��
 */
void HAL_PCD_ISOINIncompleteCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
    USBD_LL_IsoINIncomplete((USBD_HandleTypeDef *)hpcd->pData, epnum);
}

/**
 * @brief   HAL��PCD���ӻص�����
 * @param   hpcd: PCD���ָ��
 * @retval  ��
 */
void HAL_PCD_ConnectCallback(PCD_HandleTypeDef *hpcd)
{
    g_usb_conn_state = 1;
    USBD_LL_DevConnected((USBD_HandleTypeDef *)hpcd->pData);
}

/**
 * @brief   HAL��PCD�Ͽ��ص�����
 * @param   hpcd: PCD���ָ��
 * @retval  ��
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
        /* PCD��ʼ�� */
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

        /* ���ý��ա�����FiFo */
        HAL_PCDEx_SetRxFiFo(&g_pcd_handle, 0x200);
        HAL_PCDEx_SetTxFiFo(&g_pcd_handle, 0, 0x40);  /* EP0 control   64B */
        HAL_PCDEx_SetTxFiFo(&g_pcd_handle, 1, 0x80);  /* EP1 CDC bulk 512B */
        HAL_PCDEx_SetTxFiFo(&g_pcd_handle, 2, 0x10);  /* EP2 CDC notify 64B */
        HAL_PCDEx_SetTxFiFo(&g_pcd_handle, 3, 0x80);  /* EP3 MSC bulk 512B */
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

    if ((ep_addr & 0x80) == 0x80)
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

    /* USB����״̬��־ */
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

