/***************************************************************************
  This is a library for the LSM303 Accelerometer and magnentometer/compass

  Designed specifically to work with the Adafruit LSM303DLHC Breakout

  These displays use I2C to communicate, 2 pins are required to interface.

  Adafruit invests time and resources providing this open source code,
  please support Adafruit andopen-source hardware by purchasing products
  from Adafruit!

  Written by Bryan Siepert for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ***************************************************************************/
#if ARDUINO >= 100
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

#include <Wire.h>

#include <limits.h>

#include "Adafruit_LSM303_Accel.h"

/* enabling this #define will enable the debug print blocks
#define LSM303_DEBUG
*/
// keepting for reference later; seems to disagree with the stm32 driver
//static float _lsm303Accel_MG_LSB     = 0.001F;   // 1, 2, 4 or 12 mg per lsb

/***************************************************************************
 ACCELEROMETER
 ***************************************************************************/



/***************************************************************************
 CONSTRUCTOR
 ***************************************************************************/

/**************************************************************************/
/*!
    @brief  Instantiates a new Adafruit_LSM303 class
*/
/**************************************************************************/
Adafruit_LSM303_Accel_Unified::Adafruit_LSM303_Accel_Unified(int32_t sensorID) {
  _sensorID = sensorID;

  // Clear the raw accel data
  raw.x = 0;
  raw.y = 0;
  raw.z = 0;
}

/***************************************************************************
 PUBLIC FUNCTIONS
 ***************************************************************************/

/**************************************************************************/
/*!
    @brief  Sets up the HW
*/
/**************************************************************************/
bool Adafruit_LSM303_Accel_Unified::begin(uint8_t i2c_address, TwoWire *wire)
{
  // Enable I2C
  i2c_dev = new Adafruit_I2CDevice(i2c_address, wire);

  if (!i2c_dev->begin()) {
    Serial.println("Failed to init i2c address");
    return false;
  }
  Adafruit_BusIO_Register ctrl1 = Adafruit_BusIO_Register(i2c_dev, LSM303_REGISTER_ACCEL_CTRL_REG1_A, 1);
  ctrl1.write(0x57);
  // // Enable the accelerometer (100Hz)
  // write8(LSM303_ADDRESS_ACCEL, LSM303_REGISTER_ACCEL_CTRL_REG1_A, 0x57);

  // // LSM303DLHC has no WHOAMI register so read CTRL_REG1_A back to check
  // // if we are connected or not
  // uint8_t reg1_a = read8(LSM303_ADDRESS_ACCEL, LSM303_REGISTER_ACCEL_CTRL_REG1_A);
  // if (reg1_a != 0x57)
  // {
  //   return false;
  // }

  return true;
}

/**************************************************************************/
/*!
    @brief  Gets the most recent sensor event
*/
/**************************************************************************/
bool Adafruit_LSM303_Accel_Unified::getEvent(sensors_event_t *event) {
  /* Clear the event */
  memset(event, 0, sizeof(sensors_event_t));

  /* Read new data */
  read();

  float lsb = getLSB();
  event->version   = sizeof(sensors_event_t);
  event->sensor_id = _sensorID;
  event->type      = SENSOR_TYPE_ACCELEROMETER;
  event->timestamp = millis();
  event->acceleration.x = (float)raw.x * lsb * SENSORS_GRAVITY_STANDARD;
  event->acceleration.y = (float)raw.y * lsb * SENSORS_GRAVITY_STANDARD;
  event->acceleration.z = (float)raw.z * lsb * SENSORS_GRAVITY_STANDARD;

  return true;
}

/**************************************************************************/
/*!
    @brief  Gets the sensor_t data
*/
/**************************************************************************/
void Adafruit_LSM303_Accel_Unified::getSensor(sensor_t *sensor) {
  /* Clear the sensor_t object */
  memset(sensor, 0, sizeof(sensor_t));

  /* Insert the sensor name in the fixed length char array */
  strncpy (sensor->name, "LSM303", sizeof(sensor->name) - 1);
  sensor->name[sizeof(sensor->name)- 1] = 0;
  sensor->version     = 1;
  sensor->sensor_id   = _sensorID;
  sensor->type        = SENSOR_TYPE_ACCELEROMETER;
  sensor->min_delay   = 0;
  sensor->max_value   = 0.0F; // TBD
  sensor->min_value   = 0.0F; // TBD
  sensor->resolution  = 0.0F; // TBD
}


/**************************************************************************/
/*!
    @brief Sets the accelerometer's range
    @param new_range an `lsm303_accel_range_t` representing the range of
    measurements in +/-G. The smaller the range, the more accurate.
*/
/**************************************************************************/
void Adafruit_LSM303_Accel_Unified::setRange(lsm303_accel_range_t new_range){
  Adafruit_BusIO_Register ctrl_4 =
    Adafruit_BusIO_Register(i2c_dev, LSM303_REGISTER_ACCEL_CTRL_REG4_A, 1);
  Adafruit_BusIO_RegisterBits range =
    Adafruit_BusIO_RegisterBits(&ctrl_4, 2, 4);
  range.write(new_range);
}

/**************************************************************************/
/*!
    @brief Gets the accelerometer's range
    @returns The `lsm303_accel_range_t` representing the range of
    measurements in +/-G.
*/
/**************************************************************************/
lsm303_accel_range_t Adafruit_LSM303_Accel_Unified::getRange(void){
  Adafruit_BusIO_Register ctrl_4 =
    Adafruit_BusIO_Register(i2c_dev, LSM303_REGISTER_ACCEL_CTRL_REG4_A, 1);
  Adafruit_BusIO_RegisterBits range =
    Adafruit_BusIO_RegisterBits(&ctrl_4, 2, 4);
  return (lsm303_accel_range_t)range.read();
}

/**************************************************************************/
/*!
    @brief Sets the accelerometer's power mode
    @param new_mode an `lsm303_accel_mode_t` representing the power mode.
    The mode effects the precision of the sensor's readings
    High resolution is 12-bit
    Normal mode is 10-bit
    Low power is 8-bit
*/
/**************************************************************************/

void Adafruit_LSM303_Accel_Unified::setMode(lsm303_accel_mode_t new_mode){

  Adafruit_BusIO_Register ctrl_1 =
    Adafruit_BusIO_Register(i2c_dev, LSM303_REGISTER_ACCEL_CTRL_REG1_A, 1);
  Adafruit_BusIO_Register ctrl_4 =
    Adafruit_BusIO_Register(i2c_dev, LSM303_REGISTER_ACCEL_CTRL_REG4_A, 1);

  Adafruit_BusIO_RegisterBits low_power = Adafruit_BusIO_RegisterBits(&ctrl_1, 1, 3);
  Adafruit_BusIO_RegisterBits hi_res = Adafruit_BusIO_RegisterBits(&ctrl_4, 1, 3);

  hi_res.write(new_mode & 0b01);
  low_power.write((new_mode & 0b10) >>1);

}
/**************************************************************************/
/*!
    @brief Get the accelerometer's power mode
    @returns The `lsm303_accel_mode_t` representing the power mode.
*/
/**************************************************************************/
lsm303_accel_mode_t Adafruit_LSM303_Accel_Unified::getMode(void){

  Adafruit_BusIO_Register ctrl_1 =
    Adafruit_BusIO_Register(i2c_dev, LSM303_REGISTER_ACCEL_CTRL_REG1_A, 1);
  Adafruit_BusIO_Register ctrl_4 =
    Adafruit_BusIO_Register(i2c_dev, LSM303_REGISTER_ACCEL_CTRL_REG4_A, 1);

  Adafruit_BusIO_RegisterBits low_power = Adafruit_BusIO_RegisterBits(&ctrl_1, 1, 3);
  Adafruit_BusIO_RegisterBits hi_res = Adafruit_BusIO_RegisterBits(&ctrl_4, 1, 3);

  uint8_t low_power_bit = low_power.read();
  uint8_t hi_res_bit = hi_res.read();

  return (lsm303_accel_mode_t)(low_power_bit<<1 | hi_res_bit);
}
// /***************************************************************************
//  PRIVATE FUNCTIONS
//  ***************************************************************************/
  // Adafruit_BusIO_RegisterBits(Adafruit_BusIO_Register *reg, uint8_t bits, uint8_t shift);

/**************************************************************************/
/*!
    @brief  Reads the raw data from the sensor
*/
/**************************************************************************/
void Adafruit_LSM303_Accel_Unified::read()
{
  // this sucks but using one register with a 6 byte read to buffer doesn't work.
  Adafruit_BusIO_Register data_reg0 =
      Adafruit_BusIO_Register(i2c_dev, LSM303_REGISTER_ACCEL_OUT_X_L_A, 1);
  Adafruit_BusIO_Register data_reg1 =
      Adafruit_BusIO_Register(i2c_dev, LSM303_REGISTER_ACCEL_OUT_X_H_A, 1);
  Adafruit_BusIO_Register data_reg2 =
      Adafruit_BusIO_Register(i2c_dev, LSM303_REGISTER_ACCEL_OUT_Y_L_A, 1);
  Adafruit_BusIO_Register data_reg3 =
      Adafruit_BusIO_Register(i2c_dev, LSM303_REGISTER_ACCEL_OUT_Y_H_A, 1);
  Adafruit_BusIO_Register data_reg4 =
      Adafruit_BusIO_Register(i2c_dev, LSM303_REGISTER_ACCEL_OUT_Z_L_A, 1);
  Adafruit_BusIO_Register data_reg5 =
      Adafruit_BusIO_Register(i2c_dev, LSM303_REGISTER_ACCEL_OUT_Z_H_A, 1);

  uint8_t xlo = data_reg0.read();
  uint8_t xhi = data_reg1.read();
  uint8_t ylo = data_reg2.read();
  uint8_t yhi = data_reg3.read();
  uint8_t zlo = data_reg4.read();
  uint8_t zhi = data_reg5.read();

  raw.x = (int16_t)(xlo | (xhi << 8)) >> 4;
  raw.y = (int16_t)(ylo | (yhi << 8)) >> 4;
  raw.z = (int16_t)(zlo | (zhi << 8)) >> 4;
}

float Adafruit_LSM303_Accel_Unified::getLSB(void){
  float lsb;
  switch (getRange()) {
    case LSM303_RANGE_2G: lsb = 0.00098; break;
    case LSM303_RANGE_4G: lsb = 0.00195; break;
    case LSM303_RANGE_8G: lsb = 0.0039; break;
    case LSM303_RANGE_16G: lsb = 0.01172; break;
  }
  return lsb; //what about the shift?
}