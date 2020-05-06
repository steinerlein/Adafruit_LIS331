/*!
 * @file Adafruit_LIS3X.cpp
 */

#include "Arduino.h"

#include <Adafruit_LIS3X.h>
#include <Wire.h>

/*!
 *  @brief  Instantiates a new LIS3X class in I2C
 *  @param  Wi
 *          optional wire object
 */
Adafruit_LIS3X::Adafruit_LIS3X(TwoWire *Wi) {}

/*!
 *   @brief  Instantiates a new LIS3X class using hardware SPI
 *   @param  cspin
 *           number of CSPIN (Chip Select)
 *   @param  *theSPI
 *           optional parameter contains spi object
 */
Adafruit_LIS3X::Adafruit_LIS3X(int8_t cspin, SPIClass *theSPI) {}

/*!
 *   @brief  Instantiates a new LIS3X class using software SPI
 *   @param  cspin
 *           number of CSPIN (Chip Select)
 *   @param  mosipin
 *           number of pin used for MOSI (Master Out Slave In))
 *   @param  misopin
 *           number of pin used for MISO (Master In Slave Out)
 *   @param  sckpin
 *           number of pin used for CLK (clock pin)
 */
Adafruit_LIS3X::Adafruit_LIS3X(int8_t cspin, int8_t mosipin, int8_t misopin,
                               int8_t sckpin) {}

/*
bool Adafruit_LIS3X::_init(void){

  // Adafruit_BusIO_Register _ctrl4 = Adafruit_BusIO_Register(
  //     i2c_dev, spi_dev, ADDRBIT8_HIGH_TOREAD, LIS3X_REG_CTRL4, 1);
  // _ctrl4.write(0x88); // High res & BDU enabled

  // Adafruit_BusIO_Register _ctrl3 = Adafruit_BusIO_Register(
  //     i2c_dev, spi_dev, ADDRBIT8_HIGH_TOREAD, LIS3X_REG_CTRL3, 1);
  // _ctrl3.write(0x10); // DRDY on INT1
}
*/

/*!
 *  @brief  Get Device ID from LIS3X_REG_WHOAMI
 *  @return WHO AM I value
 */
uint8_t Adafruit_LIS3X::getDeviceID(void) {
  Adafruit_BusIO_Register _chip_id = Adafruit_BusIO_Register(
      i2c_dev, spi_dev, ADDRBIT8_HIGH_TOREAD, LIS3X_REG_WHOAMI, 1);

  return _chip_id.read();
}

/*!
 *  @brief  Reads x y z values at once
 */
void Adafruit_LIS3X::read(void) {

  uint8_t register_address = LIS3X_REG_OUT_X_L;
  if (i2c_dev) {
    register_address |= 0x80; // set [7] for auto-increment
  } else {
    register_address |= 0x40; // set [6] for auto-increment
    register_address |= 0x80; // set [7] for read
  }

  Adafruit_BusIO_Register xl_data = Adafruit_BusIO_Register(
      i2c_dev, spi_dev, ADDRBIT8_HIGH_TOREAD, register_address, 6);

  uint8_t buffer[6];
  xl_data.read(buffer, 6);

  x = buffer[0];
  x |= ((uint16_t)buffer[1]) << 8;
  y = buffer[2];
  y |= ((uint16_t)buffer[3]) << 8;
  z = buffer[4];
  z |= ((uint16_t)buffer[5]) << 8;

  _scaleValues();
}

/**
 * @brief Setup  the INT1 or INT2 pin to trigger when new data is ready
 *
 * @param irqnum The interrupt number/pin to configure
 * @param activelow The polarity of the pin. true: active low false: active high
 * @param opendrain The pinmode for the given interrupt pin. true: open drain.
 * Connects to GND when activated false: push-pull: connects to VCC when
 * activated
 * @return true
 * @return false
 */
bool Adafruit_LIS3X::configIntDataReady(uint8_t irqnum, bool activelow,
                                        bool opendrain) {
  Adafruit_BusIO_Register ctrl3_reg = Adafruit_BusIO_Register(
      i2c_dev, spi_dev, ADDRBIT8_HIGH_TOREAD, LIS3X_REG_CTRL3, 1);

  Adafruit_BusIO_RegisterBits opendrain_and_polarity_bits =
      Adafruit_BusIO_RegisterBits(&ctrl3_reg, 2, 6);
  Adafruit_BusIO_RegisterBits int1_bits =
      Adafruit_BusIO_RegisterBits(&ctrl3_reg, 2, 0);
  Adafruit_BusIO_RegisterBits int2_bits =
      Adafruit_BusIO_RegisterBits(&ctrl3_reg, 2, 3);
  opendrain_and_polarity_bits.write((activelow << 1) | (opendrain));

  if (irqnum == 1) {
    int1_bits.write(0b10);
    int2_bits.write(0);
  } else {
    int2_bits.write(0b10);
    int1_bits.write(0);
  }
}

/**************************************************************************/
/*!
    @brief Enables the high pass filter and/or slope filter
    @param filter_enabled Whether to enable the slope filter (see datasheet)
    @param cutoff The frequency below which signals will be filtered out
*/
/**************************************************************************/

void Adafruit_LIS3X::enableHighPassFilter(bool filter_enabled,
                                          lis3x_hpf_cutoff_t cutoff, bool use_reference) {
  Adafruit_BusIO_Register ctrl2_reg = Adafruit_BusIO_Register(
      i2c_dev, spi_dev, ADDRBIT8_HIGH_TOREAD, LIS3X_REG_CTRL2);

  Adafruit_BusIO_RegisterBits HPF_mode =
      Adafruit_BusIO_RegisterBits(&ctrl2_reg, 1, 5);

  Adafruit_BusIO_RegisterBits HPF_internal_filter_en =
      Adafruit_BusIO_RegisterBits(&ctrl2_reg, 1, 4);

  Adafruit_BusIO_RegisterBits HPF_cuttoff =
      Adafruit_BusIO_RegisterBits(&ctrl2_reg, 2, 0);

  HPF_mode.write(use_reference); // "Reference signal for filtering"

  // HPF_mode.write(filter_enabled);
  HPF_internal_filter_en.write(filter_enabled);
  HPF_cuttoff.write(cutoff);

}
/**
 * @brief
 *
 *
 *
Noise Density (µg/√ Hz) (typ)	218	650	1500
 LIS331DLH

 *
 * @param reference
 */
void Adafruit_LIS3X::setHPFReference(int8_t reference){
  Adafruit_BusIO_Register reference_reg = Adafruit_BusIO_Register(
      i2c_dev, spi_dev, ADDRBIT8_HIGH_TOREAD, LIS3X_REG_REFERENCE);
  reference_reg.write(reference);
}
/**
 * @brief Gets the current high-pass filter reference/offset
 *
 * Full scale Reference mode LSB value (mg)
    2g                    ~16
    4g                    ~31
    8g                    ~63

 * @return int8_t The current reference value.
 */
int8_t Adafruit_LIS3X::getHPFReference(void){
  Adafruit_BusIO_Register reference_reg = Adafruit_BusIO_Register(
      i2c_dev, spi_dev, ADDRBIT8_HIGH_TOREAD, LIS3X_REG_REFERENCE);
  return reference_reg.read();
}

/**
 * @brief Zero the measurement offsets while the high-pass filter is enabled.
 *
 *
 *
 */
void Adafruit_LIS3X::HPFReset(void){
  Adafruit_BusIO_Register reference_reset_reg =
    Adafruit_BusIO_Register(i2c_dev, spi_dev, ADDRBIT8_HIGH_TOREAD, LIS3X_REG_HP_FILTER_RESET);
  reference_reset_reg.read();
}
/**
 * @brief Scales the raw variables based on the current measurement range
 *
 */
void Adafruit_LIS3X::_scaleValues(void) {}

/**
 * @brief Set the measurement range for the sensor
 *
 * @param range The measurement range to set
 */
void Adafruit_LIS3X::writeRange(uint8_t range) {

  Adafruit_BusIO_Register _ctrl4 = Adafruit_BusIO_Register(
      i2c_dev, spi_dev, ADDRBIT8_HIGH_TOREAD, LIS3X_REG_CTRL4, 1);

  Adafruit_BusIO_RegisterBits range_bits =
      Adafruit_BusIO_RegisterBits(&_ctrl4, 2, 4);
  range_bits.write(range);
  delay(15); // delay to let new setting settle
}

/**
 * @brief Get the measurement range
 *
 * @return uint8_t The measurement range
 */
uint8_t Adafruit_LIS3X::readRange(void) {
  Adafruit_BusIO_Register _ctrl4 = Adafruit_BusIO_Register(
      i2c_dev, spi_dev, ADDRBIT8_HIGH_TOREAD, LIS3X_REG_CTRL4, 1);

  Adafruit_BusIO_RegisterBits range_bits =
      Adafruit_BusIO_RegisterBits(&_ctrl4, 2, 4);

  return (uint8_t)range_bits.read();
}
//////// START POWER MODE/DATA RATE/LPF ////////////

/*!
 *  @brief  Sets the data rate for the LIS3X (controls power consumption)
 *  @param  data_rate The new data rate to set.
 */
void Adafruit_LIS3X::setDataRate(lis331_data_rate_t data_rate) {
  int8_t dr_value = 0;
  int8_t pm_value = 0;
  lis331_mode_t new_mode = getMode(data_rate);
  Adafruit_BusIO_Register _ctrl1 = Adafruit_BusIO_Register(
      i2c_dev, spi_dev, ADDRBIT8_HIGH_TOREAD, LIS3X_REG_CTRL1, 1);
  switch (new_mode) {
  case LIS3X_MODE_SHUTDOWN:
    break;

  case LIS3X_MODE_LOW_POWER:
    // ODR bits are in CTRL1[7:5] (PM)
    pm_value = ((data_rate & 0x1C)) >> 2;
    break;

  case LIS3X_MODE_NORMAL:
    pm_value = ((data_rate & 0x1C)) >> 2;

    // ODR bits are in CTRL1[4:3] (DR)
    dr_value = (data_rate & 0x7);

    // only Normal mode uses DR to set ODR, so we can set it here
    Adafruit_BusIO_RegisterBits dr_bits =
        Adafruit_BusIO_RegisterBits(&_ctrl1, 2, 3);
    dr_bits.write(dr_value);
    break;
  }
  Adafruit_BusIO_RegisterBits pm_bits =
      Adafruit_BusIO_RegisterBits(&_ctrl1, 3, 5);
  pm_bits.write(pm_value);
}

/*!
 *   @brief  Gets the data rate for the LIS3X (controls power consumption)
 *   @return Returns Data Rate value
 */
lis331_data_rate_t Adafruit_LIS3X::getDataRate(void) {
  Adafruit_BusIO_Register _ctrl1 = Adafruit_BusIO_Register(
      i2c_dev, spi_dev, ADDRBIT8_HIGH_TOREAD, LIS3X_REG_CTRL1, 1);
  Adafruit_BusIO_RegisterBits pm_dr_bits =
      Adafruit_BusIO_RegisterBits(&_ctrl1, 5, 3);
  return (lis331_data_rate_t)pm_dr_bits.read();
}

/**
 * @brief  Return the current power mode from the current data rate
 *
 * @return lis331_mode_t The currently set power mode
 */
lis331_mode_t Adafruit_LIS3X::getMode(void) {
  lis331_data_rate_t current_rate = getDataRate();
  return getMode(current_rate);
}

/**
 * @brief Return the current power mode from a given data rate value
 *
 * @param data_rate The `lis331_data_rate_t` to return the `lis331_mode_t` for
 * @return lis331_mode_t
 */
lis331_mode_t Adafruit_LIS3X::getMode(lis331_data_rate_t data_rate) {
  uint8_t pm_value = (data_rate & 0x1C) >> 2;
  if (pm_value >= LIS3X_MODE_LOW_POWER) {
    return LIS3X_MODE_LOW_POWER;
  }
  return (lis331_mode_t)pm_value;
}
/**
 * @brief Set the Low Pass Filter cutoff frequency. Useful for removing high
 * frequency noise while sensing orientation using the acceleration from
 * gravity.
 *
 * **Will not work** when sensor is **in Normal mode** because the LPF cutoff
 * bits are used to set the ODR in Normal mode.
 *
 * @param cutoff The frequency above which signals will be ignored.
 * @returns true: success false: cuttoff frequency was not set because the
 */
bool Adafruit_LIS3X::setLPFCutoff(lis3x_lpf_cutoff_t cutoff) {

  lis331_mode_t current_mode = getMode();
  if (current_mode == LIS3X_MODE_NORMAL) {
    return false;
  }
  Adafruit_BusIO_Register _ctrl1 = Adafruit_BusIO_Register(
      i2c_dev, spi_dev, ADDRBIT8_HIGH_TOREAD, LIS3X_REG_CTRL1, 1);
  Adafruit_BusIO_RegisterBits data_rate_bits =
      // Adafruit_BusIO_RegisterBits(&_ctrl1, 4, 4);
      Adafruit_BusIO_RegisterBits(&_ctrl1, 2, 3); // including LPen bit

  data_rate_bits.write(cutoff);
  return true;
}
////////  END  POWER MODE/DATA RATE/LPF ////////////
/*!
 *  @brief  Gets the most recent sensor event
 *  @param  *event
 *          sensor event that we want to read
 *  @return true if successful
 */
bool Adafruit_LIS3X::getEvent(sensors_event_t *event) {
  /* Clear the event */
  memset(event, 0, sizeof(sensors_event_t));

  event->version = sizeof(sensors_event_t);
  event->sensor_id = _sensorID;
  event->type = SENSOR_TYPE_ACCELEROMETER;
  event->timestamp = 0;

  read();

  event->acceleration.x = x_g * SENSORS_GRAVITY_STANDARD;
  event->acceleration.y = y_g * SENSORS_GRAVITY_STANDARD;
  event->acceleration.z = z_g * SENSORS_GRAVITY_STANDARD;

  return true;
}

/*!
 *   @brief  Gets the sensor_t data
 *   @param  *sensor
 *           sensor that we want to write data into
 */
void Adafruit_LIS3X::getSensor(sensor_t *sensor) {
  /* Clear the sensor_t object */
  memset(sensor, 0, sizeof(sensor_t));

  /* Insert the sensor name in the fixed length char array */
  strncpy(sensor->name, "LIS3X", sizeof(sensor->name) - 1);
  sensor->name[sizeof(sensor->name) - 1] = 0;
  sensor->version = 1;
  sensor->sensor_id = _sensorID;
  sensor->type = SENSOR_TYPE_ACCELEROMETER;
  sensor->min_delay = 0;
  sensor->max_value = 0;
  sensor->min_value = 0;
  sensor->resolution = 0;
}