#include "transformaciones.h"

#include <math.h>

/* =============================================================================
   PRIMITIVAS
   ============================================================================= */

Mat3 mat3_identidad(void)
{
    Mat3 r = {{1,0,0, 0,1,0, 0,0,1}};
    return r;
}

/* T(tx, ty) */
Mat3 mat3_traslacion(float tx, float ty)
{
    Mat3 r = {{1,0,tx, 0,1,ty, 0,0,1}};
    return r;
}

/* R(rad) - rotacion antihoraria */
Mat3 mat3_rotacion(float rad)
{
    float c = cosf(rad), s = sinf(rad);
    Mat3 r = {{c,-s,0, s,c,0, 0,0,1}};
    return r;
}

/* S(sx, sy) */
Mat3 mat3_escala(float sx, float sy)
{
    Mat3 r = {{sx,0,0, 0,sy,0, 0,0,1}};
    return r;
}

/* SH_x(shx): x' = x + shx*y,  y' = y */
Mat3 mat3_shear_x(float shx)
{
    Mat3 r = {{1,shx,0, 0,1,0, 0,0,1}};
    return r;
}

/* =============================================================================
   TRANSFORMACIONES CON PIVOTE
   Patron: T(pivot) * Op * T(-pivot)
   ============================================================================= */

/* Rotacion de angulo rad alrededor del punto (cx, cy) */
Mat3 mat3_rotacion_pivote(float rad, float cx, float cy)
{
    Mat3 t1 = mat3_traslacion( cx,  cy);
    Mat3 ro = mat3_rotacion(rad);
    Mat3 t2 = mat3_traslacion(-cx, -cy);
    return mat3_mul(t1, mat3_mul(ro, t2));
}

/* Escala (sx, sy) respecto al punto (cx, cy) */
Mat3 mat3_escala_pivote(float sx, float sy, float cx, float cy)
{
    Mat3 t1 = mat3_traslacion( cx,  cy);
    Mat3 sc = mat3_escala(sx, sy);
    Mat3 t2 = mat3_traslacion(-cx, -cy);
    return mat3_mul(t1, mat3_mul(sc, t2));
}

/* Shear horizontal respecto al punto (cx, cy) */
Mat3 mat3_shear_x_pivote(float shx, float cx, float cy)
{
    Mat3 t1 = mat3_traslacion( cx,  cy);
    Mat3 sh = mat3_shear_x(shx);
    Mat3 t2 = mat3_traslacion(-cx, -cy);
    return mat3_mul(t1, mat3_mul(sh, t2));
}

/* =============================================================================
   REFLEXIONES
   ============================================================================= */

/* Reflexion respecto a la linea horizontal y = y0
 * Matriz: T(0,y0) * S(1,-1) * T(0,-y0)         */
Mat3 mat3_reflexion_eje_x(float y0)
{
    Mat3 t1 = mat3_traslacion(0.0f,  y0);
    Mat3 sc = mat3_escala(1.0f, -1.0f);
    Mat3 t2 = mat3_traslacion(0.0f, -y0);
    return mat3_mul(t1, mat3_mul(sc, t2));
}

/* Reflexion respecto a la linea vertical x = x0
 * Matriz: T(x0,0) * S(-1,1) * T(-x0,0)          */
Mat3 mat3_reflexion_eje_y(float x0)
{
    Mat3 t1 = mat3_traslacion( x0, 0.0f);
    Mat3 sc = mat3_escala(-1.0f, 1.0f);
    Mat3 t2 = mat3_traslacion(-x0, 0.0f);
    return mat3_mul(t1, mat3_mul(sc, t2));
}

/* =============================================================================
   ALGEBRA DE MATRICES
   ============================================================================= */

/* Producto de matrices row-major: C = A * B */
Mat3 mat3_mul(Mat3 a, Mat3 b)
{
    Mat3 r;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) {
            r.m[i*3+j] = 0.0f;
            for (int k = 0; k < 3; k++)
                r.m[i*3+j] += a.m[i*3+k] * b.m[k*3+j];
        }
    return r;
}

/* =============================================================================
   APLICACION SOBRE GEOMETRIA
   ============================================================================= */

Punto mat3_punto(Mat3 m, Punto p)
{
    float x = m.m[0]*(float)p.x + m.m[1]*(float)p.y + m.m[2];
    float y = m.m[3]*(float)p.x + m.m[4]*(float)p.y + m.m[5];
    return (Punto){(int)roundf(x), (int)roundf(y)};
}

Poligono mat3_poligono(Mat3 m, const Poligono *poly)
{
    Poligono r;
    r.cantidad = poly->cantidad;
    for (int i = 0; i < poly->cantidad; i++)
        r.puntos[i] = mat3_punto(m, poly->puntos[i]);
    return r;
}
