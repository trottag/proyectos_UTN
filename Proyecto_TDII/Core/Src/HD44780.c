

#include "HD44780.h"

/*
 * Constants
 */

/** Address of the first position of the second line. */
static const uint8_t HD44780_SECOND_LINE_ADDRESS = 0x40;

/** Number of spaces that should be printed when a tab character is printed to the lcd. */
static const uint8_t HD44780_TAB_SIZE = 4;

/*
 * Commands
 */

static const uint8_t HD44780_CMD_CLEAR_DISPLAY = 0X01;

static const uint8_t HD44780_CMD_RETURN_HOME = 0X02;

static const uint8_t HD44780_CMD_ENTRY_MODE_SET = 0x04;
static const uint8_t HD44780_FLG_DIR_RTL = 0x00;
static const uint8_t HD44780_FLG_DIR_LTR = 0x02;
static const uint8_t HD44780_FLG_DISPLAY_NOSHIFT = 0x00;
static const uint8_t HD44780_FLG_DISPLAY_SHIFT = 0x01;

static const uint8_t HD44780_CMD_DISPLAY_CONTROL = 0x08;
static const uint8_t HD44780_FLG_DISPLAY_OFF = 0x00;
static const uint8_t HD44780_FLG_DISPLAY_ON = 0x04;
static const uint8_t HD44780_FLG_CURSOR_OFF = 0x00;
static const uint8_t HD44780_FLG_CURSOR_ON = 0x02;
static const uint8_t HD44780_FLG_BLINK_OFF = 0x00;
static const uint8_t HD44780_FLG_BLINK_ON = 0x01;

static const uint8_t HD44780_CMD_CURSOR_DISPLAY_SHIFT = 0x10;
// static const uint8_t HD44780_FLG_SHIFT_CURSOR = 0x00;
static const uint8_t HD44780_FLG_SHIFT_DISPLAY = 0x08;
static const uint8_t HD44780_FLG_SHIFT_LTR = 0x00;
static const uint8_t HD44780_FLG_SHIFT_RTL = 0x04;

static const uint8_t HD44780_CMD_FUNCTION_SET = 0x20;
static const uint8_t HD44780_FLG_DATA_LEN_4BIT = 0x00;
static const uint8_t HD44780_FLG_DATA_LEN_8BIT = 0x10;
static const uint8_t HD44780_FLG_1_LINE = 0x00;
static const uint8_t HD44780_FLG_2_LINE = 0x08;
static const uint8_t HD44780_FLG_FONT_5X8 = 0x00;
static const uint8_t HD44780_FLG_FONT_5X10 = 0x04;

static const uint8_t HD44780_CMD_SET_CGRAM_ADDRESS = 0x40;

static const uint8_t HD44780_CMD_SET_DDRAM_ADDRESS = 0x80;

static const uint8_t HD44780_CMD_READ_BUSYFLAG_AND_ADDRESS = 0X07;



/*
 * Internal function declarations
 */

/**
 * Initialize the GPIO peripheral to the desired mode of operation.
 */
static inline void GPIO_init(GPIO_TypeDef *gpio, uint16_t pin, uint32_t mode);

/**
 * Set the GPIO mode of the pins connected to the controller data lines.
 */
static void HD44780_set_data_mode(const HD44780 *lcd, uint32_t mode);

/**
 * Perform a read operation returning, depending on the chosen data length, the 4 or 8 bit value representing the state
 * of the mcu pins connected to the controller data lines.
 */
static uint8_t HD44780_pull_value(const HD44780 *lcd);

/**
 * Perform a write operation setting, depending on the chosen data length, a 4 bit or 8 bit value to the mcu pins
 * connected to the controller data lines.
 */
static void HD44780_push_value(const HD44780 *lcd, uint8_t byte);

/**
 * Read a byte from the lcd registers.
 */
static uint8_t HD44780_read_byte(const HD44780 *lcd);

/**
 * Write a byte to the lcd registers.
 */
static void HD44780_write_byte(const HD44780 *lcd, bool rs, uint8_t byte);

/**
 * Write a byte to the lcd registers in initialization mode,
 * where the data length is always 8 bit and the last 4 bits are discarded.
 */
static void HD44780_write_init(const HD44780 *lcd, uint8_t byte);

/**
 * Get the value of the address counter.
 * This address counter is used by both CG and DDRAM addresses,
 * and its value is determined by the previous instruction.
 * The address contents are the same as for instructions set CGRAM address and set DDRAM address.
 */
static inline uint8_t HD44780_get_address(const HD44780 *lcd);

/**
 * Read the busy flag (BF) indicating that the system is now internally operating on a previously received
 * instruction. If the return code is 1, the internal operation is in progress. The next instruction will not be
 * accepted until BF is reset to 0. Check the BF status before the next write operation.
 */
static inline uint8_t HD44780_get_busyflag(const HD44780 *lcd);

/**
 * Loop until the busy flag goes low.
 */
static inline void HD44780_await_busyflag(const HD44780 *lcd);

/**
 * Write a byte to the lcd instruction register.
 */
static inline void HD44780_write_instruction(const HD44780 *lcd, uint8_t byte);

/**
 * Write a byte to the lcd data register.
 */
static inline void HD44780_write_data(const HD44780 *lcd, uint8_t byte);

/**
 * Get the line on which the cursor is currently positioned.
 */
static inline uint8_t HD44780_get_current_line(const HD44780 *lcd);


/**
 * Initialization routine for use for Data Watchpoint Trace.
 *
 *
 */
void  DWT_Delay_Init(void);

/**
 * Simple Blocking Delay Routine utilizing ARM's DWT
 * (Data Watchpoint and Trace Unit)
 * @parameter time un useconds
 */
static inline void DWT_Delay_us(volatile uint32_t);




/*
 * Public function definitions
 */

void HD44780_init(const HD44780 *lcd)
{
	DWT_Delay_Init();
	HAL_Delay(50); //delay_init 25ms

    GPIO_init(lcd->rs_gpio, lcd->rs_pin, GPIO_MODE_OUTPUT_PP);
    GPIO_init(lcd->rw_gpio, lcd->rw_pin, GPIO_MODE_OUTPUT_PP);
    GPIO_init(lcd->en_gpio, lcd->en_pin, GPIO_MODE_OUTPUT_PP);
    HD44780_set_data_mode(lcd, GPIO_MODE_OUTPUT_PP);

    HAL_GPIO_WritePin(lcd->rs_gpio, lcd->rs_pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(lcd->rw_gpio, lcd->rw_pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(lcd->en_gpio, lcd->en_pin, GPIO_PIN_RESET);

    // Initialization by instruction.
    // See https://www.sparkfun.com/datasheets/LCD/HD44780.pdf pages 45-46.

   // Wait for more than 40 ms after VCC rises to 2.7V.
    HAL_Delay(50);
    HD44780_write_init(lcd, HD44780_CMD_FUNCTION_SET | HD44780_FLG_DATA_LEN_8BIT);
    HAL_Delay(5); // Wait for more than 4.1ms.
    HD44780_write_init(lcd, HD44780_CMD_FUNCTION_SET | HD44780_FLG_DATA_LEN_8BIT);
    HAL_Delay(1); // Wait for more than 100us.
    HD44780_write_init(lcd, HD44780_CMD_FUNCTION_SET | HD44780_FLG_DATA_LEN_8BIT);
    HAL_Delay(1); // BF cannot be checked before this instruction, wait more than 37us.

    if (!lcd->interface_8_bit)
    {
        HD44780_write_init(lcd, HD44780_CMD_FUNCTION_SET | HD44780_FLG_DATA_LEN_4BIT);
        HAL_Delay(1); // BF cannot be checked before this instruction, wait more than 37us.
    }

    uint8_t flg_data_len = lcd->interface_8_bit ? HD44780_FLG_DATA_LEN_8BIT : HD44780_FLG_DATA_LEN_4BIT;
    uint8_t flg_line_qty = lcd->single_line ? HD44780_FLG_1_LINE : HD44780_FLG_2_LINE;
    uint8_t flg_font_size = lcd->font_5x10 ? HD44780_FLG_FONT_5X10 : HD44780_FLG_FONT_5X8;

    HD44780_write_instruction(lcd, HD44780_CMD_FUNCTION_SET | flg_data_len | flg_line_qty | flg_font_size);
    HD44780_write_instruction(lcd, HD44780_CMD_DISPLAY_CONTROL);
    HD44780_write_instruction(lcd, HD44780_CMD_CLEAR_DISPLAY);
    HD44780_write_instruction(lcd, HD44780_CMD_ENTRY_MODE_SET | HD44780_FLG_DISPLAY_NOSHIFT | HD44780_FLG_DIR_LTR);
    HD44780_write_instruction(lcd, HD44780_CMD_DISPLAY_CONTROL | HD44780_FLG_DISPLAY_ON | HD44780_FLG_CURSOR_OFF |
                                       HD44780_FLG_BLINK_OFF);
}

void HD44780_configure(const HD44780 *lcd, const HD44780_Config *config)
{
    uint8_t flg_display_en = config->disable_display ? HD44780_FLG_DISPLAY_OFF : HD44780_FLG_DISPLAY_ON;
    uint8_t flg_cursor_en = config->enable_cursor ? HD44780_FLG_CURSOR_ON : HD44780_FLG_CURSOR_OFF;
    uint8_t flg_blink_en = config->enable_blink ? HD44780_FLG_BLINK_ON : HD44780_FLG_BLINK_OFF;
    uint8_t flg_shift_entity = config->shift_display ? HD44780_FLG_DISPLAY_SHIFT : HD44780_FLG_DISPLAY_NOSHIFT;
    uint8_t flg_shift_dir = config->shift_rtl ? HD44780_FLG_DIR_RTL : HD44780_FLG_DIR_LTR;

    HD44780_write_instruction(lcd, HD44780_CMD_ENTRY_MODE_SET | flg_shift_entity | flg_shift_dir);
    HD44780_write_instruction(lcd, HD44780_CMD_DISPLAY_CONTROL | flg_display_en | flg_cursor_en | flg_blink_en);
}

void HD44780_clear(const HD44780 *lcd)
{
    HD44780_write_instruction(lcd, HD44780_CMD_CLEAR_DISPLAY);
    HAL_Delay(5);
}

void HD44780_return_home(const HD44780 *lcd)
{
    HD44780_write_instruction(lcd, HD44780_CMD_RETURN_HOME);
    DWT_Delay_us(5);
}

void HD44780_cursor_to(const HD44780 *lcd, uint8_t column, uint8_t row)
{
    // When the display is configured for single line operation, the address range is 0x00 to 0x4F.
    // For two line operation the address range is 0x00 to 0x27 for the first line,
    // and 0x40 to 0x67 for the second line.
    uint8_t start = row % 2 && !lcd->single_line ? HD44780_SECOND_LINE_ADDRESS : 0;
    uint8_t addr = start + column;
    HD44780_write_instruction(lcd, HD44780_CMD_SET_DDRAM_ADDRESS | addr);
}

void HD44780_shift_display(const HD44780 *lcd, int8_t n)
{
    uint8_t flg_shift_dir = n < 0 ? HD44780_FLG_SHIFT_RTL : HD44780_FLG_SHIFT_LTR;

    for (uint8_t i = 0; i < abs(n); ++i)
    {
        HD44780_write_instruction(lcd, HD44780_CMD_CURSOR_DISPLAY_SHIFT | HD44780_FLG_SHIFT_DISPLAY | flg_shift_dir);
    }
}

void HD44780_create_symbol(const HD44780 *lcd, uint8_t address, bool font_5x10, const uint8_t symbol[])
{
    uint8_t ddram_address = HD44780_get_address(lcd);

    HD44780_write_instruction(lcd, HD44780_CMD_SET_CGRAM_ADDRESS | (address << 3));

    uint8_t height = font_5x10 ? 10 : 8;

    for (uint8_t i = 0; i < height; ++i)
    {
        HD44780_write_data(lcd, symbol[i]);
    }

    // Fill remaining pixels with whitespace.
    if (font_5x10)
    {
        for (uint8_t i = 0; i < 6; ++i)
        {
            HD44780_write_data(lcd, 0);
        }
    }

    HD44780_write_instruction(lcd, HD44780_CMD_SET_DDRAM_ADDRESS | ddram_address);
}

void HD44780_put_char(const HD44780 *lcd, uint8_t chr)
{
    switch (chr)
    {
    case '\n': {
        uint8_t line = HD44780_get_current_line(lcd);

        if (!lcd->single_line && !line)
        {
            HD44780_cursor_to(lcd, 0, 1);
        }
        else
        {
            HD44780_cursor_to(lcd, 0, 0);
        }

        break;
    }

    case '\t': {
        for (uint8_t i = 0; i < HD44780_TAB_SIZE; ++i)
        {
            HD44780_write_data(lcd, ' ');
        }

        break;
    }

    default: {
        HD44780_write_data(lcd, chr);
    }
    }
}

void HD44780_put_str(const HD44780 *lcd, const char *str)
{
    for (size_t i = 0; str[i] != '\0'; ++i)
    {
        HD44780_put_char(lcd, str[i]);
    }
}

/*
 * Internal function definitions
 */

static inline void GPIO_init(GPIO_TypeDef *gpio, uint16_t pin, uint32_t mode)
{
    GPIO_InitTypeDef GPIO_InitStruct = {.Pull = GPIO_NOPULL, .Speed = GPIO_SPEED_FREQ_HIGH, .Pin = pin, .Mode = mode};
    HAL_GPIO_Init(gpio, &GPIO_InitStruct);
}

static void HD44780_set_data_mode(const HD44780 *lcd, uint32_t mode)
{
    GPIO_init(lcd->d7_gpio, lcd->d7_pin, mode);
    GPIO_init(lcd->d6_gpio, lcd->d6_pin, mode);
    GPIO_init(lcd->d5_gpio, lcd->d5_pin, mode);
    GPIO_init(lcd->d4_gpio, lcd->d4_pin, mode);

    if (lcd->interface_8_bit)
    {
        GPIO_init(lcd->d3_gpio, lcd->d3_pin, mode);
        GPIO_init(lcd->d2_gpio, lcd->d2_pin, mode);
        GPIO_init(lcd->d1_gpio, lcd->d1_pin, mode);
        GPIO_init(lcd->d0_gpio, lcd->d0_pin, mode);
    }
}

static uint8_t HD44780_pull_value(const HD44780 *lcd)
{
    HAL_GPIO_WritePin(lcd->en_gpio, lcd->en_pin, GPIO_PIN_SET);

    // Data delay time = 360ns
    // Enable rise/fall time = 25ns
    // Total = 385ns

    HAL_Delay(1);
    uint8_t value = 0;

    if (lcd->interface_8_bit)
    {
        value |= HAL_GPIO_ReadPin(lcd->d7_gpio, lcd->d7_pin) << 7;
        value |= HAL_GPIO_ReadPin(lcd->d6_gpio, lcd->d6_pin) << 6;
        value |= HAL_GPIO_ReadPin(lcd->d5_gpio, lcd->d5_pin) << 5;
        value |= HAL_GPIO_ReadPin(lcd->d4_gpio, lcd->d4_pin) << 4;
        value |= HAL_GPIO_ReadPin(lcd->d3_gpio, lcd->d3_pin) << 3;
        value |= HAL_GPIO_ReadPin(lcd->d2_gpio, lcd->d2_pin) << 2;
        value |= HAL_GPIO_ReadPin(lcd->d1_gpio, lcd->d1_pin) << 1;
        value |= HAL_GPIO_ReadPin(lcd->d0_gpio, lcd->d0_pin) << 0;
    }
    else
    {
        value |= HAL_GPIO_ReadPin(lcd->d7_gpio, lcd->d7_pin) << 3;
        value |= HAL_GPIO_ReadPin(lcd->d6_gpio, lcd->d6_pin) << 2;
        value |= HAL_GPIO_ReadPin(lcd->d5_gpio, lcd->d5_pin) << 1;
        value |= HAL_GPIO_ReadPin(lcd->d4_gpio, lcd->d4_pin) << 0;
    }

    HAL_GPIO_WritePin(lcd->en_gpio, lcd->en_pin, GPIO_PIN_RESET);

    return value;
}

static void HD44780_push_value(const HD44780 *lcd, uint8_t byte)
{
    HAL_GPIO_WritePin(lcd->en_gpio, lcd->en_pin, GPIO_PIN_SET);

    if (lcd->interface_8_bit)
    {
        HAL_GPIO_WritePin(lcd->d7_gpio, lcd->d7_pin, byte & (1 << 7) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(lcd->d6_gpio, lcd->d6_pin, byte & (1 << 6) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(lcd->d5_gpio, lcd->d5_pin, byte & (1 << 5) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(lcd->d4_gpio, lcd->d4_pin, byte & (1 << 4) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(lcd->d3_gpio, lcd->d3_pin, byte & (1 << 3) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(lcd->d2_gpio, lcd->d2_pin, byte & (1 << 2) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(lcd->d1_gpio, lcd->d1_pin, byte & (1 << 1) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(lcd->d0_gpio, lcd->d0_pin, byte & (1 << 0) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
    else
    {
        HAL_GPIO_WritePin(lcd->d7_gpio, lcd->d7_pin, byte & (1 << 3) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(lcd->d6_gpio, lcd->d6_pin, byte & (1 << 2) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(lcd->d5_gpio, lcd->d5_pin, byte & (1 << 1) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(lcd->d4_gpio, lcd->d4_pin, byte & (1 << 0) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }

    // Data set-up time = 195ns
    // Enable rise/fall time = 25ns
    // Total = 220ns

    HAL_Delay(1);
    HAL_GPIO_WritePin(lcd->en_gpio, lcd->en_pin, GPIO_PIN_RESET);

    // Address hold time = 20ns
}

static uint8_t HD44780_read_byte(const HD44780 *lcd)
{
    HAL_GPIO_WritePin(lcd->rw_gpio, lcd->rw_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(lcd->rs_gpio, lcd->rs_pin, GPIO_PIN_RESET);

    // Address set-up time (RS, R/W to E) = 60ns

    HD44780_set_data_mode(lcd, GPIO_MODE_INPUT);

    uint8_t byte = 0;

    if (lcd->interface_8_bit)
    {
        byte = HD44780_pull_value(lcd);
    }
    else
    {
        byte |= HD44780_pull_value(lcd) << 4;
        byte |= HD44780_pull_value(lcd);
    }

    return byte;
}

static void HD44780_write_byte(const HD44780 *lcd, bool rs, uint8_t byte)
{
    HD44780_set_data_mode(lcd, GPIO_MODE_OUTPUT_PP);

    HAL_GPIO_WritePin(lcd->rw_gpio, lcd->rw_pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(lcd->rs_gpio, lcd->rs_pin, rs);

    // Address set-up time (RS, R/W to E) = 60ns

    if (lcd->interface_8_bit)
    {
        HD44780_push_value(lcd, byte);
    }
    else
    {
        HD44780_push_value(lcd, byte >> 4);
        HD44780_push_value(lcd, byte);
    }

   // HD44780_await_busyflag(lcd);
    HAL_Delay(1);
    // After execution of the CGRAM/DDRAM data write or read instruction,
    // the RAM address counter is incremented or decremented by 1.
    // The RAM address counter is updated after the busy flag turns off.
    // Address counter update time = 4us
    if (rs)
    {
    	HAL_Delay(1);
    }
}

static void HD44780_write_init(const HD44780 *lcd, uint8_t byte)
{
    if (lcd->interface_8_bit)
    {
        HD44780_push_value(lcd, byte);
    }
    else
    {
        HD44780_push_value(lcd, byte >> 4);
    }
}

static inline uint8_t HD44780_get_address(const HD44780 *lcd)
{
    return HD44780_read_byte(lcd) & ~(1 << HD44780_CMD_READ_BUSYFLAG_AND_ADDRESS);
}

static inline uint8_t HD44780_get_busyflag(const HD44780 *lcd)
{
    return HD44780_read_byte(lcd) >> HD44780_CMD_READ_BUSYFLAG_AND_ADDRESS & 1;
}

static inline void HD44780_write_instruction(const HD44780 *lcd, uint8_t byte)
{
    HD44780_write_byte(lcd, 0, byte);
}

static inline void HD44780_write_data(const HD44780 *lcd, uint8_t byte)
{
    HD44780_write_byte(lcd, 1, byte);
}

static inline uint8_t HD44780_get_current_line(const HD44780 *lcd)
{
    uint8_t address = HD44780_get_address(lcd);
    return !lcd->single_line && address >= HD44780_SECOND_LINE_ADDRESS;
}

static inline void HD44780_await_busyflag(const HD44780 *lcd)
{
    while (HD44780_get_busyflag(lcd))
        ;
}


// This Function Provides Delay In Microseconds Using DWT
static inline void DWT_Delay_us(volatile uint32_t au32_microseconds)
{

	 uint32_t startTick = DWT->CYCCNT,
	 delayTicks = au32_microseconds * (SystemCoreClock/1000000);

	    while (DWT->CYCCNT - startTick < delayTicks);
}




void DWT_Delay_Init(void)
{


if (!(CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk)) {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
	}
}

