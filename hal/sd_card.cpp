#include "sd_card.h"
#include "pincfg.h"

#include <Arduino.h>
#include <SD_MMC.h>

static bool mounted = false;

bool sd_card_init(void)
{
    if (mounted) return true;

    SD_MMC.setPins(SD_MMC_CLK_PIN, SD_MMC_CMD_PIN, SD_MMC_D0_PIN,
                   SD_MMC_D1_PIN, SD_MMC_D2_PIN, SD_MMC_D3_PIN);

    mounted = SD_MMC.begin(SD_CARD_MOUNT_POINT, false, false, SDMMC_FREQ_HIGHSPEED, 4);
    if (!mounted) {
        Serial.println("[sd] mount failed");
        return false;
    }

    Serial.printf("[sd] mounted, size: %llu MB\n",
                  (unsigned long long)(SD_MMC.cardSize() / (1024ULL * 1024ULL)));
    return true;
}

bool sd_card_is_mounted(void)
{
    return mounted;
}

uint64_t sd_card_size_bytes(void)
{
    if (!mounted) return 0;
    return SD_MMC.cardSize();
}
