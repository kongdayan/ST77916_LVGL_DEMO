#ifndef SD_CARD_H
#define SD_CARD_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SD_CARD_MOUNT_POINT "/sdcard"

bool sd_card_init(void);
bool sd_card_is_mounted(void);
uint64_t sd_card_size_bytes(void);

#ifdef __cplusplus
}
#endif

#endif
