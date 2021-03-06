#include "SPI.h"
#include <hardware/spi.h>
#include <hardware/gpio.h>

SPIClassRP2040::SPIClassRP2040(spi_inst_t *spi) {
    _spi = spi;
    _initted = false;
    _spis = SPISettings();
}

inline spi_cpol_t SPIClassRP2040::cpol() {
    switch(_spis.getDataMode()) {
        case SPI_MODE0: return SPI_CPOL_0; 
        case SPI_MODE1: return SPI_CPOL_0;
        case SPI_MODE2: return SPI_CPOL_1;
        case SPI_MODE3: return SPI_CPOL_1;
    }
    // Error
    return SPI_CPOL_0;
}

inline spi_cpha_t SPIClassRP2040::cpha() {
    switch(_spis.getDataMode()) {
        case SPI_MODE0: return SPI_CPHA_0;
        case SPI_MODE1: return SPI_CPHA_1;
        case SPI_MODE2: return SPI_CPHA_0;
        case SPI_MODE3: return SPI_CPHA_1;
    }
    // Error
    return SPI_CPHA_0;
}    

inline uint8_t SPIClassRP2040::reverseByte(uint8_t b) {
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return b;
}

inline uint16_t SPIClassRP2040::reverse16Bit(uint16_t w) {
    return ( reverseByte(w & 0xff) << 8 ) | ( reverseByte(w >> 8) );
}

// The HW can't do LSB first, only MSB first, so need to bitreverse
void SPIClassRP2040::adjustBuffer(const void *s, void *d, size_t cnt, bool by16) {
    if (_spis.getBitOrder() == MSBFIRST) {
        memcpy(d, s, cnt * (by16? 2 : 1));
    } else if (!by16) {
        const uint8_t *src = (const uint8_t *)s;
        uint8_t *dst = (uint8_t *)d;
        for (auto i = 0; i < cnt; i++) {
          *(dst++) = reverseByte( *(src++) );
        }
    } else { /* by16 */
        const uint16_t *src = (const uint16_t *)s;
        uint16_t *dst = (uint16_t *)d;
        for (auto i = 0; i < cnt; i++) {
          *(dst++) = reverse16Bit( *(src++) );
        }
    }
}

byte SPIClassRP2040::transfer(uint8_t data) {
    uint8_t ret;
    if (!_initted) return 0;
    data = (_spis.getBitOrder() == MSBFIRST) ? data : reverseByte(data);
    spi_set_format(_spi, 8, cpol(), cpha(), SPI_MSB_FIRST);
    spi_write_read_blocking(_spi, &data, &ret, 1);
    ret = (_spis.getBitOrder() == MSBFIRST) ? ret : reverseByte(ret);
    return ret;
}

uint16_t SPIClassRP2040::transfer16(uint16_t data) {
    uint16_t ret;
    if (!_initted) {
        return 0;
    }
    data = (_spis.getBitOrder() == MSBFIRST) ? data : reverse16Bit(data);
    spi_set_format(_spi, 16, cpol(), cpha(), SPI_MSB_FIRST);
    spi_write16_read16_blocking(_spi, &data, &ret, 1);
    ret = (_spis.getBitOrder() == MSBFIRST) ? ret : reverseByte(ret);
    return ret;
}

void SPIClassRP2040::transfer(void *buf, size_t count) {
    uint8_t *buff = reinterpret_cast<uint8_t *>(buf);
    for (auto i = 0; i < count; i++) {
        *buff = transfer(*buff);
        *buff = (_spis.getBitOrder() == MSBFIRST) ? *buff : reverseByte(*buff);
        buff++;
    }
}

void SPIClassRP2040::beginTransaction(SPISettings settings) {
    _spis = settings;
    if (_initted) {
        spi_deinit(_spi);
    }
    spi_init(_spi, _spis.getClockFreq());
    _initted = true;
}

void SPIClassRP2040::endTransaction(void) {
    spi_deinit(_spi);
    _initted = false;
}

void SPIClassRP2040::begin(bool hwCS, pin_size_t spiRX) {
    switch (spi_get_index(_spi)) {
        case 0:
            switch (spiRX) {
                case 0:
                case 4:
                case 16:
                case 20:
                    _pin = spiRX;
                    break;
                default:
                    // error
                    return;
            }
            break;
        case 1:
            switch (spiRX) {
                case 8:
                case 12:
                case 24:
                case 28:
                    _pin = spiRX;
                    break;
                default:
                    // error
                    return;
            }
            break;
        default:
            // error
            return;
    }

    gpio_set_function(_pin + 0, GPIO_FUNC_SPI);
    _hwCS = hwCS;
    if (hwCS) {
        gpio_set_function(_pin + 1, GPIO_FUNC_SPI);
    }
    gpio_set_function(_pin + 2, GPIO_FUNC_SPI);
    gpio_set_function(_pin + 3, GPIO_FUNC_SPI);
    // Give a default config in case user doesn't use beginTransaction
    beginTransaction(_spis);
}

void SPIClassRP2040::end() {
    gpio_set_function(_pin + 0, GPIO_FUNC_SIO);
    if (_hwCS) {
        gpio_set_function(_pin + 1, GPIO_FUNC_SIO);
    }
    gpio_set_function(_pin + 2, GPIO_FUNC_SIO);
    gpio_set_function(_pin + 3, GPIO_FUNC_SIO);
}

void SPIClassRP2040::setBitOrder(BitOrder order) {
    _spis = SPISettings( _spis.getClockFreq(), order, _spis.getDataMode() );
    beginTransaction(_spis);
}

void SPIClassRP2040::setDataMode(uint8_t uc_mode) {
    _spis = SPISettings( _spis.getClockFreq(), _spis.getBitOrder(), uc_mode );
    beginTransaction(_spis);
}

void SPIClassRP2040::setClockDivider(uint8_t uc_div) {
  (void) uc_div; // no-op
}


SPIClassRP2040 SPI(spi0);
SPIClassRP2040 SPI1(spi1);
