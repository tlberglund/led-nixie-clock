#include "strip_config.h"
#include "hardware/flash.h"
#include "hardware/sync.h"

#define FLASH_TARGET_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)
#define FLASH_PAGE_SIZE 256
#define FLASH_SECTOR_SIZE 4096


// Standard CRC32 implementation
uint32_t calculate_crc32(const uint8_t *data, size_t length) {
    uint32_t crc = 0xFFFFFFFF;
    
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320;  // CRC32 polynomial
            else
                crc = crc >> 1;
        }
    }
    
    return ~crc;
}

bool save_config(const config_t *config) {
    // Create a working copy
    config_t temp_config = *config;
    
    // Calculate CRC over the config data excluding the CRC field itself
    temp_config.crc = calculate_crc32((const uint8_t*)&temp_config, 
                                    offsetof(config_t, crc));
    
    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(FLASH_TARGET_OFFSET, (const uint8_t*)&temp_config, sizeof(config_t));
    restore_interrupts(ints);
    
    // Verify the write was successful
    const config_t *flash_config = (const config_t*)(XIP_BASE + FLASH_TARGET_OFFSET);
    return memcmp(&temp_config, flash_config, sizeof(config_t)) == 0;
}

bool load_config(config_t *config) {
    const config_t *flash_config = (const config_t*)(XIP_BASE + FLASH_TARGET_OFFSET);
    
    // First check magic
    if (flash_config->magic != 0x4B415049) {  // 'IPAK'
        return false;
    }
    
    // Copy the config so we can verify CRC
    memcpy(config, flash_config, sizeof(config_t));
    
    // Calculate CRC of the loaded data
    uint32_t calculated_crc = calculate_crc32((const uint8_t*)config, 
                                            offsetof(config_t, crc));
    
    if (calculated_crc != config->crc) {
        return false;
    }
    
    return true;
}

void initialize_default_config(config_t *config) {
    memset(config, 0, sizeof(config_t));
    config->magic = 0x4B415049;
    strcpy(config->wifi_ssid, "default_ssid");
    strcpy(config->wifi_password, "default_password");
    config->use_dhcp = true;
    // Set other defaults as needed
}

// Example usage:
void handle_config() {
    config_t config;
    
    if (!load_config(&config)) {
        // Config invalid or corrupted, load defaults
        initialize_default_config(&config);
        if (!save_config(&config)) {
            // Handle save failure
            // Maybe retry or signal error
        }
    }
    
    // Use the config...
}