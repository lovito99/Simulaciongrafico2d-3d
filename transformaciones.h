#ifndef TRANSFORMACIONES_H
#define TRANSFORMACIONES_H

#include "tipos.h"

/*
 * Matriz 3x3 homogenea en modo row-major:
 *   [ m[0] m[1] m[2] ]
 *   [ m[3] m[4] m[5] ]
 *   [ m[6] m[7] m[8] ]
 *
 * Aplicacion sobre punto (x, y):
 *   x' = m[0]*x + m[1]*y + m[2]
 *   y' = m[3]*x + m[4]*y + m[5]
 */
typedef struct { float m[9]; } Mat3;

/* Primitivas */
Mat3 mat3_identidad(void);
Mat3 mat3_traslacion(float tx, float ty);
Mat3 mat3_rotacion(float rad);
Mat3 mat3_escala(float sx, float sy);
Mat3 mat3_shear_x(float shx);

/* Transformaciones con pivote (composicion interna) */
Mat3 mat3_rotacion_pivote(float rad, float cx, float cy);
Mat3 mat3_escala_pivote(float sx, float sy, float cx, float cy);
Mat3 mat3_shear_x_pivote(float shx, float cx, float cy);

/* Reflexiones */
Mat3 mat3_reflexion_eje_x(float y0);   /* reflexion sobre linea horizontal y = y0 */
Mat3 mat3_reflexion_eje_y(float x0);   /* reflexion sobre linea vertical   x = x0 */

/* Algebra */
Mat3 mat3_mul(Mat3 a, Mat3 b);         /* producto a * b */

/* Aplicacion */
Punto    mat3_punto(Mat3 m, Punto p);
Poligono mat3_poligono(Mat3 m, const Poligono *poly);

#endif
