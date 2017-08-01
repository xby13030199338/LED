#ifndef _AIPWM_H_
#define _AIPWM_H_



#define DI_PIN_MUX		PERIPHS_IO_MUX_MTCK_U
#define DI_PIN_FUNC		FUNC_GPIO13
#define DI_PIN			13
#define DI_H			GPIO_OUTPUT_SET(DI_PIN,1)
#define DI_L			GPIO_OUTPUT_SET(DI_PIN,0)

#define DCK_PIN_MUX		PERIPHS_IO_MUX_MTDO_U
#define DCK_PIN_FUNC	FUNC_GPIO15
#define DCK_PIN			15
#define DCK_H			GPIO_OUTPUT_SET(DCK_PIN,1)
#define DCK_L			GPIO_OUTPUT_SET(DCK_PIN,0)

enum
{
	RED_CHANNEL=0,
	GREEN_CHANNEL,
	BLUE_CHANNEL,
	WHITE_CHANNEL,
	ALL_CHANNEL,
	MAX_CHANNEL,
};

void aipwm_init(void);
void aipwm_set(u8 channel, u16 value);

#endif
