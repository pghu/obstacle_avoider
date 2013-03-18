#include <math.h>
#include "terasic_includes.h"
#include "LSM303.h"
#include "I2C.h"

// Defines ////////////////////////////////////////////////////////////////

// The Arduino two-wire interface uses a 7-bit number for the address, 
// and sets the last bit correctly based on reads and writes
#define MAG_ADDRESS            0x3C
#define ACC_ADDRESS_SA0_A_LOW  0x30
#define ACC_ADDRESS_SA0_A_HIGH 0x32

alt_u8 acc_address, _device;
vector a; // accelerometer readings
vector m; // magnetometer readings
vector m_max; // maximum magnetometer values, used for calibration
vector m_min; // minimum magnetometer values, used for calibration
unsigned int io_timeout;
bool did_timeout;

// Check timeout Methods //////////////////////////////////////////////////////////////
bool timeoutOccurred()
{
  return did_timeout;
}

void setTimeout(unsigned int timeout)
{
  io_timeout = timeout;
}

unsigned int getTimeout()
{
  return io_timeout;
}

// Initialize the LSM303
void LSM303_Init(void)
{  
	acc_address = ACC_ADDRESS_SA0_A_HIGH;
    _device = LSM303DLHC_DEVICE;

    m_max.x = +540; m_max.y = +500; m_max.z = 180;
     m_min.x = -520; m_min.y = -570; m_min.z = -770;

     io_timeout = 0;  // 0 = no timeout
     did_timeout = FALSE;
}

// Turns on the LSM303's accelerometer and magnetometers and places them in normal
// mode.
void enableDefault(void)
{
  // Enable Accelerometer
  // 0x27 = 0b00100111
  // Normal power mode, all axes enabled
 // writeAccReg(LSM303_CTRL_REG1_A, 0x27);
  
  // Enable Magnetometer
  // 0x00 = 0b00000000
  // Continuous conversion mode
 // writeMagReg(LSM303_MR_REG_M, 0x00);

	//For magnetic sensors the default (factory) 7-bit slave address is 0011110xb. R = 1, W = 0 so READ = 0x00111101 = 0x3D
  //I2C_Multiple_Write(COMPASS_I2C_SCL_BASE, COMPASS_I2C_SDA_BASE, MAG_ADDRESS, LSM303_MR_REG_M, 0x00, MAX_BUFFER_SIZE); //

  if (I2C_Write(COMPASS_I2C_SCL_BASE, COMPASS_I2C_SDA_BASE, MAG_ADDRESS, LSM303_MR_REG_M, 0x00)){
	  printf("Success!\n");
  }else{
        printf("Failed to enable magnetometer\r\n");
  }
}

// Writes a magnetometer register
void writeMagReg(alt_u8 reg, alt_u8 value)
{
	/*
  Wire.beginTransmission(MAG_ADDRESS);
  Wire.write(reg);
  Wire.write(value);
  last_status = Wire.endTransmission();
  */

  if (I2C_Write(COMPASS_I2C_SCL_BASE, COMPASS_I2C_SDA_BASE, MAG_ADDRESS, reg, value)){
	  printf("Success!\n");
  }else{
        printf("Failed to write magnetometer\r\n");
  }
}

// Reads a magnetometer register
alt_u8 readMagReg(int reg)
{
  alt_u8 *value;
   //return into value
  
  /*
  Wire.beginTransmission(MAG_ADDRESS);
  Wire.write(reg);
  last_status = Wire.endTransmission();
  Wire.requestFrom(MAG_ADDRESS, 1);
  value = Wire.read();
  Wire.endTransmission();
  */

  if (I2C_Read(COMPASS_I2C_SCL_BASE, COMPASS_I2C_SDA_BASE, MAG_ADDRESS, reg, value)){
	  printf("Success! %02xh \n", (int) value);
  }else{
        printf("Failed to read magnetometer\r\n");
        value = 0;
  }

  return *value;
}

void setMagGain(alt_u8 value)
{
	/*
  Wire.beginTransmission(MAG_ADDRESS);
  Wire.write(LSM303_CRB_REG_M);
  Wire.write((byte) value);
  Wire.endTransmission();
  */
	//check to see if value is in maggain

  if (I2C_Write(COMPASS_I2C_SCL_BASE, COMPASS_I2C_SDA_BASE, MAG_ADDRESS, LSM303_CRB_REG_M, value)){
	  printf("Success!\n");
  }else{
        printf("Failed to write magnetometer gain\r\n");
  }
}

// Reads the 3 magnetometer channels and stores them in vector m
void readMag(void)
{
	alt_u16 xhm, xlm, yhm, ylm, zhm, zlm;
	alt_u8 szBuf[5]; //need 6 values
	int Num, i;

	if (I2C_MultipleRead(COMPASS_I2C_SCL_BASE, COMPASS_I2C_SDA_BASE, MAG_ADDRESS, LSM303_OUT_X_H_M, szBuf, sizeof(szBuf)))
	{
		Num = sizeof(szBuf)/sizeof(szBuf[0]);
        for(i=0;i<Num;i++){
            printf("Addr[%d] = %02xh\r\n", i, szBuf[i]);
        }

        xhm = szBuf[0];
        xlm = szBuf[1];
        zhm = szBuf[2];
        zlm = szBuf[3];
        yhm = szBuf[4];
        ylm = szBuf[5];
    }
	else{
        printf("Failed to read magnetometer\r\n");
    }
	/*
  Wire.beginTransmission(MAG_ADDRESS);
  Wire.write(LSM303_OUT_X_H_M);
  last_status = Wire.endTransmission();
  Wire.requestFrom(MAG_ADDRESS, 6);

  unsigned int millis_start = millis();
  did_timeout = false;
  while (Wire.available() < 6) {
    if (io_timeout > 0 && ((unsigned int)millis() - millis_start) > io_timeout) {
      did_timeout = true;
      return;
    }
  }
  byte xhm = Wire.read();
  byte xlm = Wire.read();
  byte yhm, ylm, zhm, zlm;
  
	// DLM, DLHC: register address for Z comes before Y
	zhm = Wire.read();
	zlm = Wire.read();
	yhm = Wire.read();
	ylm = Wire.read();
*/

  // combine high and low bytes
  m.x = (alt_u16)(xhm << 8 | xlm);
  m.y = (alt_u16)(yhm << 8 | ylm);
  m.z = (alt_u16)(zhm << 8 | zlm);
}

// Reads all 6 channels of the LSM303 and stores them in the object variables
void LSM303_read()
{
  //readAcc();
  readMag();
}

// Returns the number of degrees from the -Y axis that it
// is pointing.
int heading_Y()
{
	vector new = {0, -1, 0};
	return heading(new);
}

// Returns the number of degrees from the From vector projected into
// the horizontal plane is away from north.
// 
// Description of heading algorithm: 
// Shift and scale the magnetic reading based on calibration data to
// to find the North vector. Use the acceleration readings to
// determine the Down vector. The cross product of North and Down
// vectors is East. The vectors East and North form a basis for the
// horizontal plane. The From vector is projected into the horizontal
// plane and the angle between the projected vector and north is
// returned.
int heading(vector from)
{
    // shift and scale
    m.x = (m.x - m_min.x) / (m_max.x - m_min.x) * 2 - 1.0;
    m.y = (m.y - m_min.y) / (m_max.y - m_min.y) * 2 - 1.0;
    m.z = (m.z - m_min.z) / (m_max.z - m_min.z) * 2 - 1.0;

    vector temp_a = a;
    // normalize
    vector_normalize(&temp_a);
    //vector_normalize(&m);

    // compute E and N
    vector E;
    vector N;
    vector_cross(&m, &temp_a, &E);
    vector_normalize(&E);
    vector_cross(&temp_a, &E, &N);
  
    // compute heading
    int heading = round(atan2(vector_dot(&E, &from), vector_dot(&N, &from)) * 180 / M_PI);
    if (heading < 0) heading += 360;
  return heading;
}

void vector_cross(const vector *a,const vector *b, vector *out)
{
  out->x = a->y*b->z - a->z*b->y;
  out->y = a->z*b->x - a->x*b->z;
  out->z = a->x*b->y - a->y*b->x;
}

float vector_dot(const vector *a,const vector *b)
{
  return a->x*b->x+a->y*b->y+a->z*b->z;
}

void vector_normalize(vector *a)
{
  float mag = sqrt(vector_dot(a,a));
  a->x /= mag;
  a->y /= mag;
  a->z /= mag;
}

// Private Methods //////////////////////////////////////////////////////////////

/*
byte detectSA0_A(void)
{
  Wire.beginTransmission(ACC_ADDRESS_SA0_A_LOW);
  Wire.write(LSM303_CTRL_REG1_A);
  last_status = Wire.endTransmission();
  Wire.requestFrom(ACC_ADDRESS_SA0_A_LOW, 1);
  if (Wire.available())
  {
    Wire.read();
    return LSM303_SA0_A_LOW;
  }
  else
    return LSM303_SA0_A_HIGH;
}
*/
