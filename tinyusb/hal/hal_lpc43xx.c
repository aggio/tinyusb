/*
 * hal_lpc43xx.c
 *
 *  Created on: Dec 4, 2012
 *      Author: hathach
 */

/*
 * Software License Agreement (BSD License)
 * Copyright (c) 2013, hathach (tinyusb.org)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the tiny usb stack.
 */

#include "common/common.h"
#include "hal.h"

#if MCU == MCU_LPC43XX

#include "lpc43xx_cgu.h"
#include "lpc43xx_scu.h"

enum {
  LPC43XX_USBMODE_DEVICE = 2,
  LPC43XX_USBMODE_HOST   = 3
};

enum {
  LPC43XX_USBMODE_VBUS_LOW  = 0,
  LPC43XX_USBMODE_VBUS_HIGH = 1
};

tusb_error_t hal_init(void)
{
  //------------- USB0 Clock -------------//
#if TUSB_CFG_CONTROLLER0_MODE
  CGU_EnableEntity(CGU_CLKSRC_PLL0, DISABLE); /* Disable PLL first */
  ASSERT_INT( CGU_ERROR_SUCCESS, CGU_SetPLL0(), TUSB_ERROR_FAILED); /* the usb core require output clock = 480MHz */
  CGU_EntityConnect(CGU_CLKSRC_XTAL_OSC, CGU_CLKSRC_PLL0);
  CGU_EnableEntity(CGU_CLKSRC_PLL0, ENABLE);   /* Enable PLL after all setting is done */
  LPC_CREG->CREG0 &= ~(1<<5); /* Turn on the phy */

  // reset controller & set role
  #if TUSB_CFG_CONTROLLER0_MODE & TUSB_MODE_HOST
  hcd_controller_reset(0); // TODO where to place prototype
  LPC_USB0->USBMODE_H = LPC43XX_USBMODE_HOST | (LPC43XX_USBMODE_VBUS_HIGH << 5);
  #else // TODO OTG
    #error device mode is not supported
  #endif

  hal_interrupt_enable(0);
#endif

  //------------- USB1 Clock, only use on-chip FS PHY -------------//
#if TUSB_CFG_CONTROLLER1_MODE
  /* connect CLK_USB1 to 60 MHz clock */
  CGU_EntityConnect(CGU_CLKSRC_PLL1, CGU_BASE_USB1); /* FIXME Run base BASE_USB1_CLK clock from PLL1 (assume PLL1 is 60 MHz, no division required) */
  LPC_SCU->SFSUSB = (TUSB_CFG_CONTROLLER1_MODE & TUSB_MODE_HOST) ? 0x16 : 0x12; // enable USB1 with on-chip FS PHY

  #if TUSB_CFG_CONTROLLER1_MODE & TUSB_MODE_HOST
  hcd_controller_reset(1); // TODO where to place prototype
  LPC_USB1->USBMODE_H = LPC43XX_USBMODE_HOST | (LPC43XX_USBMODE_VBUS_HIGH << 5);
  #else

  #endif

  LPC_USB1->PORTSC1_D |= (1<<24); // TODO abtract, force port to fullspeed

  hal_interrupt_enable(1);
#endif

  return TUSB_ERROR_NONE;
}

void USB0_IRQHandler(void)
{
#if TUSB_CFG_CONTROLLER0_MODE
  tusb_isr(0);
#endif
}

void USB1_IRQHandler(void)
{
#if TUSB_CFG_CONTROLLER1_MODE
  tusb_isr(1);
#endif
}

#endif
