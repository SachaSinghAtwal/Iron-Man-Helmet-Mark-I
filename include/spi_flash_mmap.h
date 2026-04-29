#pragma once

// Arduino-ESP32 exposes spi_flash_mmap APIs via esp_spi_flash.h.
// This shim satisfies libraries that include <spi_flash_mmap.h>.
#include <esp_spi_flash.h>
