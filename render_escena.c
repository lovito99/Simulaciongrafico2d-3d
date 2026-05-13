#include "render_escena.h"

#include "algoritmos.h"
#include "escena.h"

static void dibujar_edificio(const Edificio *edificio)
{
    dibujar_rectangulo(edificio->cuerpo, edificio->relleno, edificio->contorno, VENTANA_RECORTE);

    const int margen_horizontal = 16;
    const int margen_vertical = 22;
    const int separacion_horizontal = 14;
    const int separacion_vertical = 18;
    const int ancho_disponible = edificio->cuerpo.ancho - 2 * margen_horizontal -
        (edificio->columnas_ventanas - 1) * separacion_horizontal;
    const int alto_disponible = edificio->cuerpo.alto - 2 * margen_vertical -
        (edificio->filas_ventanas - 1) * separacion_vertical;
    const int ancho_ventana = ancho_disponible / edificio->columnas_ventanas;
    const int alto_ventana = alto_disponible / edificio->filas_ventanas;

    for (int fila = 0; fila < edificio->filas_ventanas; ++fila) {
        for (int columna = 0; columna < edificio->columnas_ventanas; ++columna) {
            Rectangulo ventana = {
                edificio->cuerpo.x + margen_horizontal + columna * (ancho_ventana + separacion_horizontal),
                edificio->cuerpo.y + margen_vertical + fila * (alto_ventana + separacion_vertical),
                ancho_ventana,
                alto_ventana
            };

            dibujar_rectangulo(ventana, edificio->ventanas, edificio->contorno, VENTANA_RECORTE);
        }
    }
}

static void dibujar_casa(const Casa *casa)
{
    const Rectangulo puerta = {casa->cuerpo.x + 54, casa->cuerpo.y, 25, 58};
    const Rectangulo ventana_izquierda = {casa->cuerpo.x + 18, casa->cuerpo.y + 46, 28, 26};
    const Rectangulo ventana_derecha = {casa->cuerpo.x + 88, casa->cuerpo.y + 46, 28, 26};
    const Color color_puerta = {129, 84, 51};
    const Color color_ventana = {186, 228, 247};

    dibujar_rectangulo(casa->cuerpo, casa->color_cuerpo, casa->contorno, VENTANA_RECORTE);
    dibujar_poligono_relleno(&casa->techo, casa->color_techo, casa->contorno, VENTANA_RECORTE);
    dibujar_rectangulo(puerta, color_puerta, casa->contorno, VENTANA_RECORTE);
    dibujar_rectangulo(ventana_izquierda, color_ventana, casa->contorno, VENTANA_RECORTE);
    dibujar_rectangulo(ventana_derecha, color_ventana, casa->contorno, VENTANA_RECORTE);
}

static void dibujar_arbol(const Arbol *arbol)
{
    dibujar_rectangulo(arbol->tronco, arbol->color_tronco, arbol->contorno, VENTANA_RECORTE);
    dibujar_poligono_relleno(&arbol->copa, arbol->color_copa, arbol->contorno, VENTANA_RECORTE);
}

static void dibujar_carretera(const Carretera *carretera)
{
    dibujar_rectangulo(carretera->cuerpo, carretera->relleno, carretera->contorno, VENTANA_RECORTE);

    for (int indice = 0; indice < carretera->cantidad_divisores; ++indice) {
        dibujar_linea_bresenham(carretera->divisores[indice].inicio,
                                carretera->divisores[indice].fin,
                                carretera->color_divisor,
                                VENTANA_RECORTE);
    }
}

static void dibujar_vehiculo(const Vehiculo *vehiculo)
{
    dibujar_rectangulo(vehiculo->cuerpo, vehiculo->color_cuerpo, vehiculo->contorno, VENTANA_RECORTE);
    dibujar_poligono_relleno(&vehiculo->cabina, vehiculo->color_cabina, vehiculo->contorno, VENTANA_RECORTE);
    dibujar_circulo_punto_medio(vehiculo->ruedas[0], vehiculo->radio_rueda, vehiculo->color_rueda, VENTANA_RECORTE);
    dibujar_circulo_punto_medio(vehiculo->ruedas[1], vehiculo->radio_rueda, vehiculo->color_rueda, VENTANA_RECORTE);
    dibujar_linea_bresenham((Punto){vehiculo->cuerpo.x + 82, vehiculo->cuerpo.y},
                            (Punto){vehiculo->cuerpo.x + 82, vehiculo->cuerpo.y + vehiculo->cuerpo.alto},
                            vehiculo->contorno,
                            VENTANA_RECORTE);
}

static void dibujar_semaforo(const Semaforo *semaforo)
{
    dibujar_rectangulo(semaforo->poste, semaforo->color_poste, semaforo->contorno, VENTANA_RECORTE);
    dibujar_rectangulo(semaforo->caja, semaforo->color_caja, semaforo->contorno, VENTANA_RECORTE);

    for (int indice = 0; indice < 3; ++indice) {
        dibujar_circulo_punto_medio(semaforo->luces[indice], semaforo->radio, semaforo->colores_luces[indice], VENTANA_RECORTE);
    }
}

static void dibujar_senal(const SenalTransito *senal)
{
    dibujar_linea_bresenham(senal->poste.inicio, senal->poste.fin, senal->color_poste, VENTANA_RECORTE);
    dibujar_poligono_relleno(&senal->forma, senal->color_senal, senal->contorno, VENTANA_RECORTE);
}

static void dibujar_sol(const Sol *sol)
{
    for (int indice = 0; indice < sol->cantidad_rayos; ++indice) {
        dibujar_linea_bresenham(sol->rayos[indice].inicio, sol->rayos[indice].fin, sol->color, VENTANA_RECORTE);
    }

    dibujar_circulo_punto_medio(sol->centro, sol->radio, sol->color, VENTANA_RECORTE);
}

static void dibujar_borde_ventana(void)
{
    const Color borde = {245, 245, 245};

    dibujar_linea_bresenham((Punto){VENTANA_RECORTE.xmin, VENTANA_RECORTE.ymin}, (Punto){VENTANA_RECORTE.xmax, VENTANA_RECORTE.ymin}, borde, VENTANA_RECORTE);
    dibujar_linea_bresenham((Punto){VENTANA_RECORTE.xmax, VENTANA_RECORTE.ymin}, (Punto){VENTANA_RECORTE.xmax, VENTANA_RECORTE.ymax}, borde, VENTANA_RECORTE);
    dibujar_linea_bresenham((Punto){VENTANA_RECORTE.xmax, VENTANA_RECORTE.ymax}, (Punto){VENTANA_RECORTE.xmin, VENTANA_RECORTE.ymax}, borde, VENTANA_RECORTE);
    dibujar_linea_bresenham((Punto){VENTANA_RECORTE.xmin, VENTANA_RECORTE.ymax}, (Punto){VENTANA_RECORTE.xmin, VENTANA_RECORTE.ymin}, borde, VENTANA_RECORTE);
}

void dibujar_escena_urbana(void)
{
    const Color color_cielo = {121, 193, 244};
    const Color color_cesped = {121, 183, 96};
    const Color color_texto = {255, 255, 255};

    dibujar_rectangulo(ESCENA_URBANA.cielo, color_cielo, color_cielo, VENTANA_RECORTE);
    dibujar_sol(&ESCENA_URBANA.sol);

    for (int indice = 0; indice < ESCENA_URBANA.cantidad_edificios; ++indice) {
        dibujar_edificio(&ESCENA_URBANA.edificios[indice]);
    }

    dibujar_rectangulo(ESCENA_URBANA.cesped, color_cesped, color_cesped, VENTANA_RECORTE);
    dibujar_casa(&ESCENA_URBANA.casa);

    for (int indice = 0; indice < ESCENA_URBANA.cantidad_arboles; ++indice) {
        dibujar_arbol(&ESCENA_URBANA.arboles[indice]);
    }

    dibujar_semaforo(&ESCENA_URBANA.semaforo);
    dibujar_senal(&ESCENA_URBANA.senal);
    dibujar_carretera(&ESCENA_URBANA.carretera);
    dibujar_vehiculo(&ESCENA_URBANA.vehiculo);
    dibujar_borde_ventana();

    dibujar_texto(20.0f, 670.0f, "Entrega 1: escena urbana 2D con Bresenham, punto medio, scan-line y clipping", color_texto);
    dibujar_texto(20.0f, 650.0f, "Arquitectura separada en escena, algoritmos y renderizado.", color_texto);
}