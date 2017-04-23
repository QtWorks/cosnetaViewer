#ifndef HARDWARECALIBRATION_H
#define HARDWARECALIBRATION_H

/* JDM Calibration (very rough data). */
/* Assuming acceleration and rotation rates are simple linear (y = ax +c) functions of data   */
/* returned by the board. These MACROs output data im MKS, that is rad/s and m/s2.            */

/*  Firstly, the analogue gyros, measuring rotation rate. Use 0.0 and 1.0 until I calculate data. */

#define COS_ANA_GYRO_ZERO_G1 518.7
#define COS_ANA_GYRO_ZERO_G2 511.0
#define COS_ANA_GYRO_ZERO_G3 509.0

#define COS_ANA_GYRO_SCALE_G1 0.06830
#define COS_ANA_GYRO_SCALE_G2 0.07076
#define COS_ANA_GYRO_SCALE_G3 0.07306

/* And, as much for exposition as anything else, we get: */
#define COSNETA_GYRO_RATE_G1(boardData) (((boardData)-COS_ANA_GYRO_ZERO_G1)*COS_ANA_GYRO_SCALE_G1)
#define COSNETA_GYRO_RATE_G2(boardData) (((boardData)-COS_ANA_GYRO_ZERO_G2)*COS_ANA_GYRO_SCALE_G2)
#define COSNETA_GYRO_RATE_G3(boardData) (((boardData)-COS_ANA_GYRO_ZERO_G3)*COS_ANA_GYRO_SCALE_G3)

/* And the accelerometers */

#define COS_ACC1_ZERO_X (  81.19 )
#define COS_ACC1_ZERO_Y ( -28.63 )
#define COS_ACC1_ZERO_Z ( 512.66 )
#define COS_ACC2_ZERO_X (-249.07 )
#define COS_ACC2_ZERO_Y (-453.17 )
#define COS_ACC2_ZERO_Z ( 742.95 )

#define COS_ACC1_SCALE_X (1.0/560.14)
#define COS_ACC1_SCALE_Y (1.0/563.36)
#define COS_ACC1_SCALE_Z (1.0/549.90)
#define COS_ACC2_SCALE_X (1.0/572.28)
#define COS_ACC2_SCALE_Y (1.0/565.59)
#define COS_ACC2_SCALE_Z (1.0/543.66)

/* And, as much for exposition as anything else, we get: */
#define COSNETA_ACC1_X(boardData) (((boardData)-COS_ACC1_ZERO_X)*COS_ACC1_SCALE_X)
#define COSNETA_ACC1_Y(boardData) (((boardData)-COS_ACC1_ZERO_Y)*COS_ACC1_SCALE_Y)
#define COSNETA_ACC1_Z(boardData) (((boardData)-COS_ACC1_ZERO_Z)*COS_ACC1_SCALE_Z)
#define COSNETA_ACC2_X(boardData) (((boardData)-COS_ACC2_ZERO_X)*COS_ACC2_SCALE_X)
#define COSNETA_ACC2_Y(boardData) (((boardData)-COS_ACC2_ZERO_Y)*COS_ACC2_SCALE_Y)
#define COSNETA_ACC2_Z(boardData) (((boardData)-COS_ACC2_ZERO_Z)*COS_ACC2_SCALE_Z)


#endif // HARDWARECALIBRATION_H
