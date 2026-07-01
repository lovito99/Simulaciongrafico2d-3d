#include "escena.h"

/*
 * BUS AZUL
 * Cuerpo en coordenadas locales: x=0..240, y=109..140 (carril superior y=100..140).
 * Viaja de derecha a izquierda (carril superior).
 * Parabrisas en el lado DERECHO (x=196..240): en 2D el bus entra por la
 * derecha mostrando su frente primero, dando aspecto de avance hacia adelante.
 * Ruedas: radio=13, centros en y=109 para quedar dentro del carril superior.
 * Rueda trasera x=44, rueda delantera x=168 (deja libre x=182..196 antes del parabrisas).
 * Offset de posicion en pantalla: mat3_traslacion(bus_tx, 0).
 */
const Vehiculo BUS_AZUL = {
    .cuerpo = {0, 109, 240, 31},
    .cabina = {
        .puntos  = {{240, 140}, {240, 109}, {212, 109}, {196, 140}},
        .cantidad = 4
    },
    .ruedas      = {{44, 109}, {168, 109}},
    .radio_rueda = 13,
    .color_cuerpo = {55, 125, 210},
    .color_cabina = {110, 175, 235},
    .color_rueda  = {22, 22, 22},
    .contorno     = {18, 55, 115}
};

const VentanaRecorte VENTANA_RECORTE = {60, 60, 840, 620};

const Escena ESCENA_URBANA = {
    .cielo = {0, 200, 900, 500},
    .cesped = {0, 140, 900, 60},
    .carretera = {
        .cuerpo = {0, 60, 900, 80},
        .divisores = {
            {{90, 100}, {180, 100}},
            {{250, 100}, {340, 100}},
            {{410, 100}, {500, 100}},
            {{570, 100}, {660, 100}}
        },
        .cantidad_divisores = 4,
        .relleno = {70, 74, 84},
        .contorno = {235, 235, 235},
        .color_divisor = {245, 211, 71}
    },
    .edificios = {
        {
            .cuerpo = {20, 140, 170, 350},
            .relleno = {86, 122, 168},
            .contorno = {26, 42, 68},
            .ventanas = {249, 235, 182},
            .filas_ventanas = 5,
            .columnas_ventanas = 3
        },
        {
            .cuerpo = {250, 140, 150, 280},
            .relleno = {157, 115, 184},
            .contorno = {54, 33, 70},
            .ventanas = {247, 240, 191},
            .filas_ventanas = 4,
            .columnas_ventanas = 3
        },
        {
            .cuerpo = {700, 140, 190, 360},
            .relleno = {210, 136, 96},
            .contorno = {84, 48, 29},
            .ventanas = {255, 243, 200},
            .filas_ventanas = 6,
            .columnas_ventanas = 3
        }
    },
    .cantidad_edificios = 3,
    .casa = {
        .cuerpo = {455, 140, 135, 110},
        .techo = {
            .puntos = {{435, 250}, {522, 330}, {610, 250}},
            .cantidad = 3
        },
        .color_cuerpo = {244, 228, 197},
        .color_techo = {170, 62, 61},
        .contorno = {72, 37, 24}
    },
    .arboles = {
        {
            .tronco = {170, 140, 28, 72},
            .copa = {
                .puntos = {{138, 212}, {184, 300}, {230, 212}},
                .cantidad = 3
            },
            .color_tronco = {121, 78, 44},
            .color_copa = {73, 156, 92},
            .contorno = {33, 79, 41}
        },
        {
            .tronco = {635, 140, 28, 72},
            .copa = {
                .puntos = {{603, 212}, {649, 305}, {695, 212}},
                .cantidad = 3
            },
            .color_tronco = {121, 78, 44},
            .color_copa = {68, 144, 84},
            .contorno = {32, 72, 39}
        }
    },
    .cantidad_arboles = 2,
    .vehiculo = {
        .cuerpo = {300, 88, 185, 48},
        .cabina = {
            .puntos = {{338, 136}, {388, 178}, {445, 178}, {470, 136}},
            .cantidad = 4
        },
        .ruedas = {{342, 88}, {442, 88}},
        .radio_rueda = 18,
        .color_cuerpo = {218, 62, 72},
        .color_cabina = {240, 125, 131},
        .color_rueda = {24, 24, 24},
        .contorno = {80, 15, 20}
    },
    .semaforo = {
        .poste = {615, 140, 12, 165},
        .caja = {596, 305, 50, 115},
        .luces = {{621, 390}, {621, 362}, {621, 334}},
        .radio = 10,
        .color_poste = {82, 88, 96},
        .color_caja = {48, 54, 60},
        .colores_luces = {{228, 73, 73}, {247, 207, 78}, {72, 196, 103}},
        .contorno = {15, 16, 18}
    },
    .senal = {
        .forma = {
            .puntos = {{738, 252}, {756, 270}, {782, 270}, {800, 252}, {800, 226}, {782, 208}, {756, 208}, {738, 226}},
            .cantidad = 8
        },
        .poste = {{769, 140}, {769, 208}},
        .color_senal = {201, 47, 49},
        .color_poste = {140, 140, 146},
        .contorno = {85, 20, 21}
    },
    .sol = {
        .centro = {775, 610},
        .radio = 42,
        .rayos = {
            {{775, 660}, {775, 690}},
            {{810, 646}, {830, 668}},
            {{825, 610}, {858, 610}},
            {{810, 574}, {833, 552}},
            {{775, 560}, {775, 530}},
            {{740, 574}, {718, 552}},
            {{725, 610}, {692, 610}},
            {{740, 646}, {718, 668}}
        },
        .cantidad_rayos = 8,
        .color = {252, 204, 84}
    }
};