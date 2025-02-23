#include <string.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

void main(void)
{
    char *board_name = hwinfo_get_board_name();
    LOG_INF("Board name: %s", board_name);
}