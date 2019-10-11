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

static float _lsm303Accel_MG_LSB     = 0.001F;   // 1, 2, 4 or 12 mg per lsb

/***************************************************************************
 ACCELEROMETER
 ***************************************************************************/
// /***************************************************************************
//  PRIVATE FUNCTIONS
//  ***************************************************************************/

// /**************************************************************************/
// /*!
//     @brief  Abstract away platform differences in Arduino wire library
// */
// /**************************************************************************/
// void Adafruit_LSM303_Accel_Unified::write8(byte address, byte reg, byte value)
// {
//   Wire.beginTransmission(address);
//   #if ARDUINO >= 100
//     Wire.write((uint8_t)reg);
//     Wire.write((uint8_t)value);
//   #else
//     Wire.send(reg);
//     Wire.send(value);
//   #endif
//   Wire.endTransmission();
// }

// /**************************************************************************/
// /*!
//     @brief  Abstract away platform differences in Arduino wire library
// */
// /**************************************************************************/
// byte Adafruit_LSM303_Accel_Unified::read8(byte address, byte reg)
// {
//   byte value;

//   Wire.beginTransmission(address);
//   #if ARDUINO >= 100
//     Wire.write((uint8_t)reg);
//   #else
//     Wire.send(reg);
//   #endif
//   Wire.endTransmission();
//   Wire.requestFrom(address, (byte)1);
//   #if ARDUINO >= 100
//     value = Wire.read();
//   #else
//     value = Wire.receive();
//   #endif
//   Wire.endTransmission();

//   return value;
// }


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

  event->version   = sizeof(sensors_event_t);
  event->sensor_id = _sensorID;
  event->type      = SENSOR_TYPE_ACCELEROMETER;
  event->timestamp = millis();
  event->acceleration.x = (float)raw.x * _lsm303Accel_MG_LSB * SENSORS_GRAVITY_STANDARD;
  event->acceleration.y = (float)raw.y * _lsm303Accel_MG_LSB * SENSORS_GRAVITY_STANDARD;
  event->acceleration.z = (float)raw.z * _lsm303Accel_MG_LSB * SENSORS_GRAVITY_STANDARD;

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
    @brief  Reads the raw data from the sensor
*/
/**************************************************************************/
void Adafruit_LSM303_Accel_Unified::read()
{
  

  // uint8_t shift = 4;

  // Adafruit_BusIO_Register data_reg =
  //     Adafruit_BusIO_Register(i2c_dev, LSM303_REGISTER_ACCEL_OUT_X_L_A, 1);
  // uint16_t buffer[3];
  // data_reg.read((uint8_t *)buffer, 6);


  // raw.x = buffer[0] >> shift;
  // raw.y = buffer[1] >> shift;
  // raw.z = buffer[2] >> shift;

  // this sucks but the above doesn't work.
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