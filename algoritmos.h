#ifndef ALGORITMOS_H
#define ALGORITMOS_H

#include <stdbool.h>
#include "tipos.h"

void fijar_color(Color color);
bool punto_dentro_ventana(int x, int y, VentanaRecorte ventana);
Poligono rectangulo_a_poligono(Rectangulo rectangulo);
bool recortar_linea(Punto *inicio, Punto *fin, VentanaRecorte ventana);
void dibujar_linea_bresenham(Punto inicio, Punto fin, Color color, VentanaRecorte ventana);
void dibujar_circulo_punto_medio(Punto centro, int radio, Color color, VentanaRecorte ventana);
void rellenar_poligono_scanline(const Poligono *poligono, Color color, VentanaRecorte ventana);
void dibujar_contorno_poligono(const Poligono *poligono, Color color, VentanaRecorte ventana);
void dibujar_poligono_relleno(const Poligono *poligono, Color relleno, Color contorno, VentanaRecorte ventana);
void dibujar_rectangulo(Rectangulo rectangulo, Color relleno, Color contorno, VentanaRecorte ventana);
void dibujar_texto(float x, float y, const char *texto, Color color);

#endif