#include "algoritmos.h"

#include <GL/glew.h>
#include <GL/glut.h>
#include <math.h>
#include <stdlib.h>

enum {
    DENTRO = 0,
    IZQUIERDA = 1,
    DERECHA = 2,
    ABAJO = 4,
    ARRIBA = 8
};

static void emitir_pixel(int x, int y, VentanaRecorte ventana)
{
    if (punto_dentro_ventana(x, y, ventana)) {
        glVertex2i(x, y);
    }
}

static int comparar_double(const void *izquierda, const void *derecha)
{
    const double valor_izquierda = *(const double *)izquierda;
    const double valor_derecha = *(const double *)derecha;

    if (valor_izquierda < valor_derecha) {
        return -1;
    }

    if (valor_izquierda > valor_derecha) {
        return 1;
    }

    return 0;
}

static int calcular_codigo_region(double x, double y, VentanaRecorte ventana)
{
    int codigo = DENTRO;

    if (x < ventana.xmin) {
        codigo |= IZQUIERDA;
    } else if (x > ventana.xmax) {
        codigo |= DERECHA;
    }

    if (y < ventana.ymin) {
        codigo |= ABAJO;
    } else if (y > ventana.ymax) {
        codigo |= ARRIBA;
    }

    return codigo;
}

static void trazar_octantes_circulo(Punto centro, int x, int y, VentanaRecorte ventana)
{
    emitir_pixel(centro.x + x, centro.y + y, ventana);
    emitir_pixel(centro.x - x, centro.y + y, ventana);
    emitir_pixel(centro.x + x, centro.y - y, ventana);
    emitir_pixel(centro.x - x, centro.y - y, ventana);
    emitir_pixel(centro.x + y, centro.y + x, ventana);
    emitir_pixel(centro.x - y, centro.y + x, ventana);
    emitir_pixel(centro.x + y, centro.y - x, ventana);
    emitir_pixel(centro.x - y, centro.y - x, ventana);
}

/* =============================================================================
   FUNCIONES PÚBLICAS DE UTILIDAD
   ============================================================================= */

void fijar_color(Color color)
{
    glColor3ub(color.r, color.g, color.b);
}

/* Verificar si un punto está dentro de la ventana de recorte */
bool punto_dentro_ventana(int x, int y, VentanaRecorte ventana)
{
    return x >= ventana.xmin && x <= ventana.xmax &&
           y >= ventana.ymin && y <= ventana.ymax;
}

/* Convertir un rectángulo a polígono */
Poligono rectangulo_a_poligono(Rectangulo rectangulo)
{
    Poligono poligono;

    poligono.cantidad = 4;
    poligono.puntos[0] = (Punto){rectangulo.x, rectangulo.y};
    poligono.puntos[1] = (Punto){rectangulo.x + rectangulo.ancho, rectangulo.y};
    poligono.puntos[2] = (Punto){rectangulo.x + rectangulo.ancho, rectangulo.y + rectangulo.alto};
    poligono.puntos[3] = (Punto){rectangulo.x, rectangulo.y + rectangulo.alto};
    return poligono;
}

/* =============================================================================
   ALGORITMO DE CLIPPING: COHEN-SUTHERLAND
   ============================================================================= */

bool recortar_linea(Punto *inicio, Punto *fin, VentanaRecorte ventana)
{
    double x0 = inicio->x;
    double y0 = inicio->y;
    double x1 = fin->x;
    double y1 = fin->y;
    int codigo0 = calcular_codigo_region(x0, y0, ventana);
    int codigo1 = calcular_codigo_region(x1, y1, ventana);

    while (true) {
        if ((codigo0 | codigo1) == 0) {
            inicio->x = (int)lround(x0);
            inicio->y = (int)lround(y0);
            fin->x = (int)lround(x1);
            fin->y = (int)lround(y1);
            return true;
        }

        if ((codigo0 & codigo1) != 0) {
            return false;
        }

        double x = 0.0;
        double y = 0.0;
        int codigo_salida = (codigo0 != 0) ? codigo0 : codigo1;

        if ((codigo_salida & ARRIBA) != 0) {
            if (y1 == y0) {
                return false;
            }

            x = x0 + (x1 - x0) * (ventana.ymax - y0) / (y1 - y0);
            y = ventana.ymax;
        } else if ((codigo_salida & ABAJO) != 0) {
            if (y1 == y0) {
                return false;
            }

            x = x0 + (x1 - x0) * (ventana.ymin - y0) / (y1 - y0);
            y = ventana.ymin;
        } else if ((codigo_salida & DERECHA) != 0) {
            if (x1 == x0) {
                return false;
            }

            y = y0 + (y1 - y0) * (ventana.xmax - x0) / (x1 - x0);
            x = ventana.xmax;
        } else if ((codigo_salida & IZQUIERDA) != 0) {
            if (x1 == x0) {
                return false;
            }

            y = y0 + (y1 - y0) * (ventana.xmin - x0) / (x1 - x0);
            x = ventana.xmin;
        }

        if (codigo_salida == codigo0) {
            x0 = x;
            y0 = y;
            codigo0 = calcular_codigo_region(x0, y0, ventana);
        } else {
            x1 = x;
            y1 = y;
            codigo1 = calcular_codigo_region(x1, y1, ventana);
        }
    }
}

/* =============================================================================
   ALGORITMO DE BRESENHAM PARA LÍNEAS
   ============================================================================= */

void dibujar_linea_bresenham(Punto inicio, Punto fin, Color color, VentanaRecorte ventana)
{
    Punto inicio_recortado = inicio;
    Punto fin_recortado = fin;

    if (!recortar_linea(&inicio_recortado, &fin_recortado, ventana)) {
        return;
    }

    int x0 = inicio_recortado.x;
    int y0 = inicio_recortado.y;
    const int x1 = fin_recortado.x;
    const int y1 = fin_recortado.y;
    const int dx = abs(x1 - x0);
    const int dy = abs(y1 - y0);
    const int paso_x = (x0 < x1) ? 1 : -1;
    const int paso_y = (y0 < y1) ? 1 : -1;
    int error = dx - dy;

    fijar_color(color);
    glBegin(GL_POINTS);

    while (true) {
        emitir_pixel(x0, y0, ventana);

        if (x0 == x1 && y0 == y1) {
            break;
        }

        const int error_doble = error * 2;

        if (error_doble > -dy) {
            error -= dy;
            x0 += paso_x;
        }

        if (error_doble < dx) {
            error += dx;
            y0 += paso_y;
        }
    }

    glEnd();
}

/* =============================================================================
   ALGORITMO DEL PUNTO MEDIO PARA CIRCUNFERENCIAS
   ============================================================================= */

void dibujar_circulo_punto_medio(Punto centro, int radio, Color color, VentanaRecorte ventana)
{
    int x = 0;
    int y = radio;
    int decision = 1 - radio;

    fijar_color(color);
    glBegin(GL_POINTS);

    while (x <= y) {
        trazar_octantes_circulo(centro, x, y, ventana);

        if (decision < 0) {
            decision += 2 * x + 3;
        } else {
            decision += 2 * (x - y) + 5;
            --y;
        }

        ++x;
    }

    glEnd();
}

/* =============================================================================
   ALGORITMO DE BARRIDO (SCANLINE) PARA RELLENO DE POLÍGONOS
   ============================================================================= */

void rellenar_poligono_scanline(const Poligono *poligono, Color color, VentanaRecorte ventana)
{
    int y_minimo = poligono->puntos[0].y;
    int y_maximo = poligono->puntos[0].y;

    for (int indice = 1; indice < poligono->cantidad; ++indice) {
        if (poligono->puntos[indice].y < y_minimo) {
            y_minimo = poligono->puntos[indice].y;
        }

        if (poligono->puntos[indice].y > y_maximo) {
            y_maximo = poligono->puntos[indice].y;
        }
    }

    if (y_minimo < ventana.ymin) {
        y_minimo = ventana.ymin;
    }

    if (y_maximo > ventana.ymax) {
        y_maximo = ventana.ymax;
    }

    fijar_color(color);
    glBegin(GL_POINTS);

    for (int y = y_minimo; y <= y_maximo; ++y) {
        double intersecciones[MAX_PUNTOS_POLIGONO];
        int cantidad_intersecciones = 0;

        for (int actual = 0; actual < poligono->cantidad; ++actual) {
            const int siguiente = (actual + 1) % poligono->cantidad;
            const Punto inicio = poligono->puntos[actual];
            const Punto fin = poligono->puntos[siguiente];

            if ((inicio.y <= y && fin.y > y) || (fin.y <= y && inicio.y > y)) {
                intersecciones[cantidad_intersecciones] = inicio.x +
                    (double)(y - inicio.y) * (double)(fin.x - inicio.x) / (double)(fin.y - inicio.y);
                ++cantidad_intersecciones;
            }
        }

        qsort(intersecciones, (size_t)cantidad_intersecciones, sizeof(double), comparar_double);

        for (int indice = 0; indice + 1 < cantidad_intersecciones; indice += 2) {
            int x_inicio = (int)ceil(intersecciones[indice]);
            int x_fin = (int)floor(intersecciones[indice + 1]);

            if (x_inicio < ventana.xmin) {
                x_inicio = ventana.xmin;
            }

            if (x_fin > ventana.xmax) {
                x_fin = ventana.xmax;
            }

            for (int x = x_inicio; x <= x_fin; ++x) {
                emitir_pixel(x, y, ventana);
            }
        }
    }

    glEnd();
}

/* Dibujar el contorno de un polígono */
void dibujar_contorno_poligono(const Poligono *poligono, Color color, VentanaRecorte ventana)
{
    for (int indice = 0; indice < poligono->cantidad; ++indice) {
        const int siguiente = (indice + 1) % poligono->cantidad;
        dibujar_linea_bresenham(poligono->puntos[indice], poligono->puntos[siguiente], color, ventana);
    }
}

/* Dibujar polígono relleno con contorno */
void dibujar_poligono_relleno(const Poligono *poligono, Color relleno, Color contorno, VentanaRecorte ventana)
{
    rellenar_poligono_scanline(poligono, relleno, ventana);
    dibujar_contorno_poligono(poligono, contorno, ventana);
}

/* Dibujar rectángulo relleno con contorno */
void dibujar_rectangulo(Rectangulo rectangulo, Color relleno, Color contorno, VentanaRecorte ventana)
{
    const Poligono poligono = rectangulo_a_poligono(rectangulo);

    dibujar_poligono_relleno(&poligono, relleno, contorno, ventana);
}

/* =============================================================================
   UTILIDADES DE RENDERIZADO
   ============================================================================= */

void dibujar_texto(float x, float y, const char *texto, Color color)
{
    const unsigned char *cursor = (const unsigned char *)texto;

    fijar_color(color);
    glRasterPos2f(x, y);
    while (*cursor != '\0') {
        glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *cursor);
        ++cursor;
    }
}