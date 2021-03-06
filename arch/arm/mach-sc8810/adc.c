/*
 * Copyright (C) 2012 Spreadtrum Communications Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/irqflags.h>
#include <linux/delay.h>

#include <mach/hardware.h>
#include <mach/adi.h>
#include <mach/adc.h>

#ifdef ADC_CHIP_PHY_R3P0
#include <linux/err.h>
#include <linux/hwspinlock.h>
#endif

#ifdef ADC_CHIP_PHY_R0P0
#define ANA_CTL_ADC_BASE		(SPRD_MISC_BASE + 0x0300)

/* registers definitions for controller ANA_CTL_ADC */
#define ANA_REG_ADC_CTRL                (ANA_CTL_ADC_BASE + 0x0000)
#define ANA_REG_ADC_CS                  (ANA_CTL_ADC_BASE + 0x0004)
#define ANA_REG_ADC_TPC_CH_CTRL         (ANA_CTL_ADC_BASE + 0x0008)
#define ANA_REG_ADC_DAT                 (ANA_CTL_ADC_BASE + 0x000c)
#define ANA_REG_ADC_IE                  (ANA_CTL_ADC_BASE + 0x0010)
#define ANA_REG_ADC_IC                  (ANA_CTL_ADC_BASE + 0x0014)
#define ANA_REG_ADC_ISTS                (ANA_CTL_ADC_BASE + 0x0018)
#define ANA_REG_ADC_ISRC                (ANA_CTL_ADC_BASE + 0x001c)

/* bits definitions for register REG_ADC_CTRL */
#define BIT_ADC_STATUS                  ( BIT(4) )
#define BIT_HW_INT_EN                   ( BIT(3) )
#define BIT_TPC_CH_ON                   ( BIT(2) )
#define BIT_SW_CH_ON                    ( BIT(1) )
#define BIT_ADC_EN                      ( BIT(0) )

/* bits definitions for register REG_ADC_CS */
#define BIT_ADC_SLOW                    ( BIT(5) )
#define BIT_ADC_SCALE                   ( BIT(4) )
#define BITS_ADC_CS(_x_)                ( (_x_) & 0x0F )

#define SHIFT_ADC_CS                    ( 0 )
#define MASK_ADC_CS                     ( 0x0F )

/* bits definitions for register REG_ADC_TPC_CH_CTRL */
#define BITS_TPC_CH_DELAY(_x_)          ( (_x_) << 8 & 0xFF00 )
#define BITS_TPC_Y_CH(_x_)              ( (_x_) << 4 & 0xF0 )
#define BITS_TPC_X_CH(_x_)              ( (_x_) << 0 & 0x0F )

/* bits definitions for register REG_ADC_DAT */
#define BITS_ADC_DAT(_x_)               ( (_x_) << 0 & 0x3FF )

#define SHIFT_ADC_DAT                   ( 0 )
#define MASK_ADC_DAT                    ( 0x3FF )

/* bits definitions for register REG_ADC_IE */
#define BIT_ADC_IE                      ( BIT(0) )

/* bits definitions for register REG_ADC_IC */
#define BIT_ADC_IC                      ( BIT(0) )

/* bits definitions for register REG_ADC_ISTS */
#define BIT_ADC_MIS                     ( BIT(0) )

/* bits definitions for register REG_ADC_ISRC */
#define BIT_ADC_RIS                     ( BIT(0) )

/* adc global regs */
#ifdef CONFIG_ARCH_SC7710
#define ANA_REG_GLB_APB_CLK_EN		(SPRD_MISC_BASE + 0x0800)
#else
#define ANA_REG_GLB_APB_CLK_EN		(SPRD_MISC_BASE + 0x0600)
#endif
#define BIT_CLK_AUXAD_EN                ( BIT(14) )
#define BIT_CLK_AUXADC_EN               ( BIT(13) )
#define BIT_ADC_EB                      ( BIT(5) )

void sci_adc_enable(void)
{
	/* enable adc */
	sci_adi_set(ANA_REG_GLB_APB_CLK_EN,
		    BIT_ADC_EB | BIT_CLK_AUXADC_EN | BIT_CLK_AUXAD_EN);
	sci_adi_set(ANA_CTL_ADC_BASE, BIT_ADC_EN);
}

#ifdef CONFIG_NKERNEL
#define sci_adc_lock()				\
		flags = hw_local_irq_save()
#define sci_adc_unlock()			\
		hw_local_irq_restore(flags)

#else
#define sci_adc_lock()
#define sci_adc_unlock()

#endif

int sci_adc_get_value(unsigned int channel, int scale)
{
	unsigned long flags;
	int ret, cnt = 12;

	BUG_ON(channel >= ADC_MAX);

	if (scale)
		scale = BIT_ADC_SCALE;

	sci_adc_lock();

	/* clear int */
	sci_adi_set(ANA_REG_ADC_IC, BIT_ADC_IC);

	sci_adi_raw_write(ANA_REG_ADC_CS, channel | scale);

	/* turn on sw channel */
	sci_adi_set(ANA_REG_ADC_CTRL, BIT_SW_CH_ON);

	/* wait adc complete */
	while (!(sci_adi_read(ANA_REG_ADC_ISRC) & BIT_ADC_RIS) && cnt--) {
		udelay(50);
	}

	WARN_ON(!cnt);

	ret = sci_adi_read(ANA_REG_ADC_DAT) & MASK_ADC_DAT;

	/* turn off sw channel */
	sci_adi_clr(ANA_REG_ADC_CTRL, BIT_SW_CH_ON);

	/* clear int */
	sci_adi_set(ANA_REG_ADC_IC, BIT_ADC_IC);

	sci_adc_unlock();

	return ret;
}
#elif defined(ADC_CHIP_PHY_R3P0)

static u32 io_base;		/* Mapped base address */

#define adc_write(val,reg) \
	do { \
		sci_adi_raw_write((u32)reg,val); \
} while (0)
static unsigned adc_read(unsigned addr)
{
	return sci_adi_read(addr);
}

#ifdef CONFIG_NKERNEL
static DEFINE_SPINLOCK(adc_lock);
#define sci_adc_lock()				\
		hw_flags = hw_local_irq_save(); \
		spin_lock_irqsave(&adc_lock, flags); \
		WARN_ON(IS_ERR_VALUE(hwspin_lock_timeout(arch_get_hwlock(HWLOCK_ADC), -1)))
#define sci_adc_unlock()			\
		hwspin_unlock(arch_get_hwlock(HWLOCK_ADC)); \
		spin_unlock_irqrestore(&adc_lock, flags); \
		hw_local_irq_restore(hw_flags)
#else
/*FIXME:If we have not hwspinlock , we need use spinlock to do it*/
static DEFINE_SPINLOCK(adc_lock);
#define sci_adc_lock() 		do { \
				spin_lock_irqsave(&adc_lock, flags);} while(0)
#define sci_adc_unlock() 	do { \
				spin_unlock_irqrestore(&adc_lock, flags);} while(0)
#endif

#define ADC_CTL		(0x00)
#define ADC_SW_CH_CFG		(0x04)
#define ADC_FAST_HW_CHX_CFG(_X_)		((_X_) * 0x4 + 0x8)
#define ADC_SLOW_HW_CHX_CFG(_X_)		((_X_) * 0x4 + 0x28)
#define ADC_HW_CH_DELAY		(0x48)

#define ADC_DAT		(0x4c)
#define adc_get_data(_SAMPLE_BITS_)		(adc_read(io_base + ADC_DAT) & (_SAMPLE_BITS_))

#define ADC_IRQ_EN		(0x50)
#define adc_enable_irq(_X_)	do {adc_write(((_X_) & 0x1),io_base + ADC_IRQ_EN);} while(0)

#define ADC_IRQ_CLR		(0x54)
#define adc_clear_irq()		do {adc_write(0x1, io_base + ADC_IRQ_CLR);} while (0)

#define ADC_IRQ_STS		(0x58)
#define adc_mask_irqstatus()     adc_read(io_base + ADC_IRQ_STS)

#define ADC_IRQ_RAW		(0x5c)
#define adc_raw_irqstatus()     adc_read(io_base + ADC_IRQ_RAW)

#define ADC_DEBUG		(0x60)

/* adc global regs */
#define ANA_REG_GLB_ANA_APB_CLK_EN		(SPRD_MISC_BASE + 0x0800)
#define BIT_ANA_CLK_AUXAD_EN                ( BIT(14) )
#define BIT_ANA_CLK_AUXADC_EN               ( BIT(13) )
#define BIT_ANA_ADC_EB                      ( BIT(5) )

/*ADC_CTL */
#define ADC_MAX_SAMPLE_NUM			(0x10)
#define ADC_SW_RUN_NUM_MSK          (0xf << 4)
#define BIT_SW_CH_RUN_NUM(_X_)		((((_X_) - 1) & 0xf ) << 4)
#define BIT_ADC_BIT_MODE(_X_)		(((_X_) & 0x1) << 2)	/*0: adc in 10bits mode, 1: adc in 12bits mode */
#define BIT_ADC_BIT_MODE_MASK		BIT_ADC_BIT_MODE(1)
#define BIT_SW_CH_ON                    ( BIT(1) ) /*WO*/
#define BIT_ADC_EN                      ( BIT(0) )
/*ADC_SW_CH_CFG && ADC_FAST(SLOW)_HW_CHX_CFG*/
#define BIT_CH_IN_MODE(_X_)		(((_X_) & 0x1) << 8)	/*0: resistance path, 1: capacitance path */
#define BIT_CH_SLOW(_X_)		(((_X_) & 0x1) << 6)	/*0: quick mode, 1: slow mode */
#define BIT_CH_SCALE(_X_)		(((_X_) & 0x1) << 5)	/*0: little scale, 1: big scale */
#define BIT_CH_ID(_X_)			((_X_) & 0x1f)
/*ADC_FAST(SLOW)_HW_CHX_CFG*/
#define BIT_CH_DLY_EN(_X_)		(((_X_) & 0x1) << 7)	/*0:disable, 1:enable */
/*ADC_HW_CH_DELAY*/
#define BIT_HW_CH_DELAY(_X_)		((_X_) & 0xff)	/*its unit is ADC clock */
#define BIT_ADC_EB                  ( BIT(5) )
#define BIT_CLK_AUXADC_EN                      ( BIT(13) )
#define BIT_CLK_AUXAD_EN						( BIT(14) )
#define ADC_HW_CH_ID_MSK            (0x1F)
#define CP_PA_TEMPRATURE_HW_CFG        (4)
#define CP_PA_TEMPRATURE_CHANNEL        (0)

void sci_adc_hw_slow_init(void)
{
    u32 adc_hw_addr;
    u32 adc_reg_val;

    adc_hw_addr = io_base + ADC_SLOW_HW_CHX_CFG(CP_PA_TEMPRATURE_HW_CFG);

    /* init cp PA temprature channel*/
    adc_write(0xe0, (io_base + ADC_HW_CH_DELAY));
    adc_write(adc_read(adc_hw_addr) | BIT(6), adc_hw_addr);

    adc_reg_val = adc_read(adc_hw_addr) & ~ADC_HW_CH_ID_MSK;
    adc_reg_val = adc_reg_val | BIT_CH_ID(CP_PA_TEMPRATURE_CHANNEL);
    adc_write(adc_reg_val, adc_hw_addr);
}

void sci_adc_enable(void)
{
	/* enable adc */
	sci_adi_set(ANA_REG_GLB_ANA_APB_CLK_EN,
		    BIT_ANA_ADC_EB | BIT_ANA_CLK_AUXADC_EN |
		    BIT_ANA_CLK_AUXAD_EN);

	sci_adi_set(io_base,  BIT(0));

    /* init HW slow channel */
    sci_adc_hw_slow_init();
}

void sci_adc_dump_register()
{
	unsigned _base = (unsigned)(io_base);
	unsigned _end = _base + 0x64;

	printk("sci_adc_dump_register begin\n");
	for (; _base < _end; _base += 4) {
		printk("_base = 0x%x, value = 0x%x\n", _base, adc_read(_base));
	}
	printk("sci_adc_dump_register end\n");
}

EXPORT_SYMBOL(sci_adc_dump_register);

void sci_adc_init(void __iomem * adc_base)
{
	io_base = (u32) adc_base;
	adc_enable_irq(0);
	adc_clear_irq();
	sci_adc_enable();
}

EXPORT_SYMBOL(sci_adc_init);

/*
*	Notes: for hw channel,  config its hardware configuration register and using sw channel to read it.
*/
static int sci_adc_config(struct adc_sample_data *adc)
{
	unsigned addr = 0;
	unsigned val = 0;
	int ret = 0;

	BUG_ON(!adc);
	BUG_ON(adc->channel_id > ADC_MAX);

	val = BIT_CH_IN_MODE(adc->signal_mode);
	val |= BIT_CH_SLOW(adc->sample_speed);
	val |= BIT_CH_SCALE(adc->scale);
	val |= BIT_CH_ID(adc->channel_id);
	val |= BIT_CH_DLY_EN(adc->hw_channel_delay ? 1 : 0);

	adc_write(val, io_base + ADC_SW_CH_CFG);

	if (adc->channel_type > 0) {	/*hardware */
		adc_write(BIT_HW_CH_DELAY(adc->hw_channel_delay),
			  io_base + ADC_HW_CH_DELAY);

		if (adc->channel_type == 1) {	/*slow */
			addr = io_base + ADC_SLOW_HW_CHX_CFG(adc->channel_id);
		} else {
			addr = io_base + ADC_FAST_HW_CHX_CFG(adc->channel_id);
		}
		adc_write(val, addr);
	}

	return ret;
}

void sci_adc_get_vol_ratio(unsigned int channel_id, int scale,
			   unsigned int *div_numerators,
			   unsigned int *div_denominators)
{
	unsigned int chip_id = 0;

	switch (channel_id) {

	case ADC_CHANNEL_0:
	case ADC_CHANNEL_1:
	case ADC_CHANNEL_2:
	case ADC_CHANNEL_3:
		if (scale) {
			*div_numerators = 16;
			*div_denominators = 41;
		} else {
			*div_numerators = 1;
			*div_denominators = 1;
		}
		return;
	case ADC_CHANNEL_PROG:	//channel 4
	case ADC_CHANNEL_VCHGBG:	//channel 7
	case ADC_CHANNEL_HEADMIC:	//18
		*div_numerators = 1;
		*div_denominators = 1;
		return;
	case ADC_CHANNEL_VBAT:	//channel 5
	case ADC_CHANNEL_ISENSE:	//channel 8
		*div_numerators = 7;
		*div_denominators = 29;
		return;
	case ADC_CHANNEL_VCHGSEN:	//channel 6
		*div_numerators = 77;
		*div_denominators = 1024;
		return;
	case ADC_CHANNEL_TPYD:	//channel 9
	case ADC_CHANNEL_TPYU:	//channel 10
	case ADC_CHANNEL_TPXR:	//channel 11
	case ADC_CHANNEL_TPXL:	//channel 12
		if (scale) {	//larger
			*div_numerators = 2;
			*div_denominators = 5;
		} else {
			*div_numerators = 3;
			*div_denominators = 5;
		}
		return;
	case ADC_CHANNEL_DCDCCORE:	//channel 13
	case ADC_CHANNEL_DCDCARM:	//channel 14
		if (scale) {	//lager
			*div_numerators = 4;
			*div_denominators = 5;
		} else {
			*div_numerators = 1;
			*div_denominators = 1;
		}
		return;
	case ADC_CHANNEL_DCDCMEM:	//channel 15
		if (scale) {	//lager
			*div_numerators = 3;
			*div_denominators = 5;
		} else {
			*div_numerators = 4;
			*div_denominators = 5;
		}
		return;
	case ADC_CHANNEL_DCDCLDO:	//16
		*div_numerators = 4;
		*div_denominators = 9;
		return;
	case ADC_CHANNEL_VBATBK:	//channel 17
	case ADC_CHANNEL_LDO0:	//channel 19,20
	case ADC_CHANNEL_LDO1:
		*div_numerators = 11;
		*div_denominators = 35;
		return;
	case ADC_CHANNEL_LDO2:	//channel 21
		*div_numerators = 1;
		*div_denominators = 3;
		return;
	case ADC_CHANNEL_LP_LDO0://lp ldo ref
	case ADC_CHANNEL_LP_LDO1:
	case ADC_CHANNEL_LP_LDO2:
		*div_numerators = 1;
		*div_denominators = 2;
		return;
	default:
		*div_numerators = 1;
		*div_denominators = 1;
		break;
	}
}

int sci_adc_get_values(struct adc_sample_data *adc)
{
	unsigned long flags, hw_flags;
	int cnt = 12;
	unsigned addr = 0;
	unsigned val = 0;
	int ret = 0;
	int num = 0;
	int sample_bits_msk = 0;
	int *pbuf = 0;

	if (!adc || adc->channel_id > ADC_MAX)
		return -EINVAL;

	pbuf = adc->pbuf;
	if (!pbuf)
		return -EINVAL;

	num = adc->sample_num;
	if (num > ADC_MAX_SAMPLE_NUM)
		return -EINVAL;

	sci_adc_lock();

	sci_adc_config(adc);	//configs adc sample.

	addr = io_base + ADC_CTL;
	val = adc_read(addr);
	val &= ~(BIT_SW_CH_ON | BIT_ADC_BIT_MODE_MASK);
	adc_write(val, addr);

	adc_clear_irq();

	val = BIT_SW_CH_RUN_NUM(num);
	val |= BIT_ADC_BIT_MODE(adc->sample_bits);
	val |= BIT_SW_CH_ON;

	adc_write(((adc_read(addr) & (~ADC_SW_RUN_NUM_MSK)) | val), addr);

	while ((!adc_raw_irqstatus()) && cnt--) {
		udelay(50);
	}

	if (cnt < 0) {
		ret = -1;
		WARN_ON(1);
		goto Exit;
	}

	adc_clear_irq();

	if (adc->sample_bits)
		sample_bits_msk = ((1 << 12) - 1);	//12
	else
		sample_bits_msk = ((1 << 10) - 1);	//10
	while (num--)
		*pbuf++ = adc_get_data(sample_bits_msk);

	sci_adc_unlock();

	return ret;

Exit:
	val = adc_read(addr);
	val &= ~BIT_SW_CH_ON;
	adc_write(val, addr);

	sci_adc_unlock();

	return ret;
}

EXPORT_SYMBOL_GPL(sci_adc_get_values);
#endif
