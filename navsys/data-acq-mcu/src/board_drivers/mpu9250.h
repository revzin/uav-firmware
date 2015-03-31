#ifndef __MPU9250__H__
#define __MPU9250__H__

typedef enum {
	RA_WHO_AM_I = 0x75,
	RA_CONFIG = 0x1A,
	RA_GYRO_CONFIG = 0x1B,
	RA_ACCEL_CONFIG = 0x1C,
	RA_ACCEL_CONFIG2 = 0x1D,
	RA_SMPLRT_DIV = 0x19,

	/* Выходы гироскопа */
	RA_ACCEL_XOUT_H = 0x3B,
	RA_ACCEL_XOUT_L = 0x3C,
	RA_ACCEL_YOUT_H = 0x3D,
	RA_ACCEL_YOUT_L = 0x3E,
	RA_ACCEL_ZOUT_H = 0x3F,
	RA_ACCEL_ZOUT_L = 0x40,
	
	/* Выходы температуры */
	RA_TEMP_OUT_H = 0x41,
	RA_TEMP_OUT_L = 0x42,
	
	/* Выходы гироскопа */
	RA_GYRO_XOUT_H = 0x43,
	RA_GYRO_XOUT_L = 0x44,
	RA_GYRO_YOUT_H = 0x45,
	RA_GYRO_YOUT_L = 0x46,
	RA_GYRO_ZOUT_H = 0x47,
	RA_GYRO_ZOUT_L = 0x48,
} MPU9250_REG_ADDRn;





void IMU_2_Setup(int div);

void IMU_2_ReadCommand(MPU9250_REG_ADDRn reg);

/* Добавляет в стек адрес регистра для чтения */
void IMU2_ReadReg(MPU9250_REG_ADDRn reg);

/* Добавляет в стек адрес регистра для записи и его новое значение */
void IMU2_WriteReg(MPU9250_REG_ADDRn reg, char data);

/* Обработчик прерывания */
void IMU2_SPI4InterruptHandler(void);

/* Запуск связи */

#endif 
