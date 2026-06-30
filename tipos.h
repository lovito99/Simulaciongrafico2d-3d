#ifndef TIPOS_H
#define TIPOS_H

#define ANCHO_VENTANA 900
#define ALTO_VENTANA 700
#define MAX_PUNTOS_POLIGONO 16

typedef struct {
    unsigned char r;
    unsigned char g;
    unsigned char b;
} Color;

typedef struct {
    int x;
    int y;
} Punto;

typedef struct {
    Punto inicio;
    Punto fin;
} SegmentoLinea;

typedef struct {
    int x;
    int y;
    int ancho;
    int alto;
} Rectangulo;

typedef struct {
    Punto puntos[MAX_PUNTOS_POLIGONO];
    int cantidad;
} Poligono;

typedef struct {
    int xmin;
    int ymin;
    int xmax;
    int ymax;
} VentanaRecorte;

typedef struct {
    Rectangulo cuerpo;
    Color relleno;
    Color contorno;
    Color ventanas;
    int filas_ventanas;
    int columnas_ventanas;
} Edificio;

typedef struct {
    Rectangulo cuerpo;
    Poligono techo;
    Color color_cuerpo;
    Color color_techo;
    Color contorno;
} Casa;

typedef struct {
    Rectangulo tronco;
    Poligono copa;
    Color color_tronco;
    Color color_copa;
    Color contorno;
} Arbol;

typedef struct {
    Rectangulo cuerpo;
    Poligono cabina;
    Punto ruedas[2];
    int radio_rueda;
    Color color_cuerpo;
    Color color_cabina;
    Color color_rueda;
    Color contorno;
} Vehiculo;

typedef struct {
    Rectangulo cuerpo;
    SegmentoLinea divisores[4];
    int cantidad_divisores;
    Color relleno;
    Color contorno;
    Color color_divisor;
} Carretera;

typedef struct {
    Rectangulo poste;
    Rectangulo caja;
    Punto luces[3];
    int radio;
    Color color_poste;
    Color color_caja;
    Color colores_luces[3];
    Color contorno;
} Semaforo;

typedef struct {
    Poligono forma;
    SegmentoLinea poste;
    Color color_senal;
    Color color_poste;
    Color contorno;
} SenalTransito;

typedef struct {
    Punto centro;
    int radio;
    SegmentoLinea rayos[8];
    int cantidad_rayos;
    Color color;
} Sol;

typedef struct {
    Rectangulo cielo;
    Rectangulo cesped;
    Carretera carretera;
    Edificio edificios[3];
    int cantidad_edificios;
    Casa casa;
    Arbol arboles[2];
    int cantidad_arboles;
    Vehiculo vehiculo;
    Semaforo semaforo;
    SenalTransito senal;
    Sol sol;
} Escena;

/* Estado de animacion para Entrega 2 */
typedef struct {
    float vehiculo_tx;    /* traslacion del vehiculo en X (pixeles) */
    float angulo_rueda;   /* angulo de rotacion de las ruedas (radianes) */
    float nube_tx;        /* posicion X de la nube animada */
    float arbol_escala;   /* factor de escala uniforme del arbol derecho */
    float shear_senal;    /* coeficiente shx de distorsion de la senal */
    int   pausa;          /* 1 = animacion pausada */
} EstadoAnim;

#endif