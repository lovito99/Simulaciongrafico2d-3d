#include "render_escena.h"

#include <math.h>

#include "algoritmos.h"
#include "escena.h"
#include "transformaciones.h"

/* =============================================================================
   OBJETOS NUEVOS: NUBE ANIMADA
   Poligono base centrado en (0,0). Se traslada al dibujar.
   ============================================================================= */

static const Poligono NUBE_BASE = {
    .puntos = {
        {-55,-10}, {-45, 6}, {-20,16}, {0,20},
        {20,16},   {45, 6},  {55,-10}, {35,-20}, {-35,-20}
    },
    .cantidad = 9
};
static const Color COLOR_NUBE    = {228, 238, 255};
static const Color COLOR_NUBE_2  = {200, 215, 245};

/* =============================================================================
   DIBUJO DE OBJETOS ESTATICOS (identicos a Entrega 1)
   ============================================================================= */

static void dibujar_edificio(const Edificio *edificio)
{
    dibujar_rectangulo(edificio->cuerpo, edificio->relleno, edificio->contorno, VENTANA_RECORTE);

    const int mg_h = 16, mg_v = 22, sep_h = 14, sep_v = 18;
    const int aw   = edificio->cuerpo.ancho - 2*mg_h - (edificio->columnas_ventanas-1)*sep_h;
    const int ah   = edificio->cuerpo.alto  - 2*mg_v - (edificio->filas_ventanas-1)*sep_v;
    const int vw   = aw / edificio->columnas_ventanas;
    const int vh   = ah / edificio->filas_ventanas;

    for (int f = 0; f < edificio->filas_ventanas; ++f)
        for (int c = 0; c < edificio->columnas_ventanas; ++c) {
            Rectangulo ven = {
                edificio->cuerpo.x + mg_h + c*(vw+sep_h),
                edificio->cuerpo.y + mg_v + f*(vh+sep_v),
                vw, vh
            };
            dibujar_rectangulo(ven, edificio->ventanas, edificio->contorno, VENTANA_RECORTE);
        }
}

static void dibujar_casa(const Casa *casa)
{
    const Rectangulo puerta   = {casa->cuerpo.x + 54, casa->cuerpo.y, 25, 58};
    const Rectangulo vent_izq = {casa->cuerpo.x + 18, casa->cuerpo.y + 46, 28, 26};
    const Rectangulo vent_der = {casa->cuerpo.x + 88, casa->cuerpo.y + 46, 28, 26};
    const Color c_puerta = {129, 84, 51};
    const Color c_vent   = {186, 228, 247};

    dibujar_rectangulo(casa->cuerpo, casa->color_cuerpo, casa->contorno, VENTANA_RECORTE);
    dibujar_poligono_relleno(&casa->techo, casa->color_techo, casa->contorno, VENTANA_RECORTE);
    dibujar_rectangulo(puerta,   c_puerta, casa->contorno, VENTANA_RECORTE);
    dibujar_rectangulo(vent_izq, c_vent,   casa->contorno, VENTANA_RECORTE);
    dibujar_rectangulo(vent_der, c_vent,   casa->contorno, VENTANA_RECORTE);
}

/* Arbol con transform Mat3 para poder aplicar escala/traslacion */
static void dibujar_arbol_t(const Arbol *arbol, Mat3 M)
{
    Poligono tronco_p = rectangulo_a_poligono(arbol->tronco);
    Poligono tronco_t = mat3_poligono(M, &tronco_p);
    Poligono copa_t   = mat3_poligono(M, &arbol->copa);

    dibujar_poligono_relleno(&tronco_t, arbol->color_tronco, arbol->contorno, VENTANA_RECORTE);
    dibujar_poligono_relleno(&copa_t,   arbol->color_copa,   arbol->contorno, VENTANA_RECORTE);
}

static void dibujar_carretera(const Carretera *carretera)
{
    dibujar_rectangulo(carretera->cuerpo, carretera->relleno, carretera->contorno, VENTANA_RECORTE);
    for (int i = 0; i < carretera->cantidad_divisores; ++i)
        dibujar_linea_bresenham(carretera->divisores[i].inicio,
                                carretera->divisores[i].fin,
                                carretera->color_divisor,
                                VENTANA_RECORTE);
}

static void dibujar_semaforo(const Semaforo *semaforo)
{
    dibujar_rectangulo(semaforo->poste, semaforo->color_poste, semaforo->contorno, VENTANA_RECORTE);
    dibujar_rectangulo(semaforo->caja,  semaforo->color_caja,  semaforo->contorno, VENTANA_RECORTE);
    for (int i = 0; i < 3; ++i)
        dibujar_circulo_punto_medio(semaforo->luces[i], semaforo->radio,
                                    semaforo->colores_luces[i], VENTANA_RECORTE);
}

static void dibujar_sol(const Sol *sol)
{
    for (int i = 0; i < sol->cantidad_rayos; ++i)
        dibujar_linea_bresenham(sol->rayos[i].inicio, sol->rayos[i].fin,
                                sol->color, VENTANA_RECORTE);
    dibujar_circulo_punto_medio(sol->centro, sol->radio, sol->color, VENTANA_RECORTE);
}

static void dibujar_borde_ventana(void)
{
    const Color borde = {245, 245, 245};
    dibujar_linea_bresenham((Punto){VENTANA_RECORTE.xmin, VENTANA_RECORTE.ymin},
                            (Punto){VENTANA_RECORTE.xmax, VENTANA_RECORTE.ymin}, borde, VENTANA_RECORTE);
    dibujar_linea_bresenham((Punto){VENTANA_RECORTE.xmax, VENTANA_RECORTE.ymin},
                            (Punto){VENTANA_RECORTE.xmax, VENTANA_RECORTE.ymax}, borde, VENTANA_RECORTE);
    dibujar_linea_bresenham((Punto){VENTANA_RECORTE.xmax, VENTANA_RECORTE.ymax},
                            (Punto){VENTANA_RECORTE.xmin, VENTANA_RECORTE.ymax}, borde, VENTANA_RECORTE);
    dibujar_linea_bresenham((Punto){VENTANA_RECORTE.xmin, VENTANA_RECORTE.ymax},
                            (Punto){VENTANA_RECORTE.xmin, VENTANA_RECORTE.ymin}, borde, VENTANA_RECORTE);
}

/* =============================================================================
   TRANSFORMACIONES ANIMADAS
   ============================================================================= */

/*
 * TRASLACION + ROTACION + COMPOSICION
 * Dibuja el vehiculo con la matriz M aplicada a todos sus vertices.
 * Las ruedas tienen ademas un espolon que muestra el angulo de rotacion.
 *
 * Composicion demostrada en las ruedas:
 *   M_rueda = T(tx, 0)  *  [T(cx,cy) * R(ang) * T(-cx,-cy)]
 *           = T(vehiculo)  *  R_pivote(ang, centro_rueda)
 */
static void dibujar_vehiculo_t(const Vehiculo *v, Mat3 M, float ang_rueda)
{
    /* Cuerpo del vehiculo */
    Poligono cuerpo_p = rectangulo_a_poligono(v->cuerpo);
    Poligono cuerpo_t = mat3_poligono(M, &cuerpo_p);
    dibujar_poligono_relleno(&cuerpo_t, v->color_cuerpo, v->contorno, VENTANA_RECORTE);

    /* Cabina */
    Poligono cabina_t = mat3_poligono(M, &v->cabina);
    dibujar_poligono_relleno(&cabina_t, v->color_cabina, v->contorno, VENTANA_RECORTE);

    /* Linea separadora de cabina */
    Punto sep0 = mat3_punto(M, (Punto){v->cuerpo.x + 82, v->cuerpo.y});
    Punto sep1 = mat3_punto(M, (Punto){v->cuerpo.x + 82, v->cuerpo.y + v->cuerpo.alto});
    dibujar_linea_bresenham(sep0, sep1, v->contorno, VENTANA_RECORTE);

    /* Ruedas: circulo + 2 espolones cruzados para visualizar rotacion */
    for (int i = 0; i < 2; i++) {
        Punto centro = mat3_punto(M, v->ruedas[i]);
        dibujar_circulo_punto_medio(centro, v->radio_rueda, v->color_rueda, VENTANA_RECORTE);

        /* Aros interiores para efecto visual */
        dibujar_circulo_punto_medio(centro, v->radio_rueda / 2, (Color){60,60,60}, VENTANA_RECORTE);

        /* Espolones (2 perpendiculares) - demuestran la rotacion */
        const Color c_espolon = {190, 190, 190};
        for (int j = 0; j < 2; j++) {
            float a = ang_rueda + (float)j * 1.5708f; /* 90 grados entre espolones */
            Punto tip = {
                centro.x + (int)roundf((float)v->radio_rueda * cosf(a)),
                centro.y + (int)roundf((float)v->radio_rueda * sinf(a))
            };
            dibujar_linea_bresenham(centro, tip, c_espolon, VENTANA_RECORTE);
        }
    }
}

/*
 * REFLEXION
 * Dibuja el vehiculo reflejado respecto a la linea horizontal y = y_pivot,
 * con colores atenuados para simular reflexion en la carretera humeda.
 *
 * Matriz: M_refl = Ref_x(y_pivot) * T(tx, 0)
 * Orden: primero se traslada el vehiculo, luego se refleja.
 */
static void dibujar_vehiculo_reflexion(const Vehiculo *v, float tx, float y_pivot)
{
    Mat3 M_tras = mat3_traslacion(tx, 0.0f);
    Mat3 M_ref  = mat3_reflexion_eje_x(y_pivot);
    Mat3 M      = mat3_mul(M_ref, M_tras);

    /* Colores oscurecidos al 30% para efecto de sombra/reflexion */
    Vehiculo v_dim = *v;
    v_dim.color_cuerpo = (Color){
        (unsigned char)(v->color_cuerpo.r * 0.30f),
        (unsigned char)(v->color_cuerpo.g * 0.30f),
        (unsigned char)(v->color_cuerpo.b * 0.30f)
    };
    v_dim.color_cabina = (Color){
        (unsigned char)(v->color_cabina.r * 0.30f),
        (unsigned char)(v->color_cabina.g * 0.30f),
        (unsigned char)(v->color_cabina.b * 0.30f)
    };
    v_dim.color_rueda  = (Color){10, 10, 10};
    v_dim.contorno     = (Color){40, 10, 10};

    Poligono cuerpo_p = rectangulo_a_poligono(v->cuerpo);
    Poligono cuerpo_t = mat3_poligono(M, &cuerpo_p);
    dibujar_poligono_relleno(&cuerpo_t, v_dim.color_cuerpo, v_dim.contorno, VENTANA_RECORTE);

    Poligono cabina_t = mat3_poligono(M, &v->cabina);
    dibujar_poligono_relleno(&cabina_t, v_dim.color_cabina, v_dim.contorno, VENTANA_RECORTE);

    for (int i = 0; i < 2; i++) {
        Punto c = mat3_punto(M, v->ruedas[i]);
        dibujar_circulo_punto_medio(c, v->radio_rueda, v_dim.color_rueda, VENTANA_RECORTE);
    }
}

/*
 * DISTORSION (SHEAR)
 * Senal de transito con shear horizontal aplicado respecto a su centro.
 * Tambien muestra la senal original (en gris tenue) como referencia.
 *
 * Matriz: M_shear = T(cx,cy) * SH_x(shx) * T(-cx,-cy)
 */
static void dibujar_senal_t(const SenalTransito *senal, float shx)
{
    /* Centro aproximado del octogono */
    const float cx = 769.0f, cy = 239.0f;

    /* Senal original tenue (referencia de comparacion) */
    const Color c_ref = {80, 20, 20};
    dibujar_contorno_poligono(&senal->forma, c_ref, VENTANA_RECORTE);

    /* Senal con distorsion shear */
    Mat3 M = mat3_shear_x_pivote(shx, cx, cy);
    Poligono forma_t = mat3_poligono(M, &senal->forma);
    dibujar_poligono_relleno(&forma_t, senal->color_senal, senal->contorno, VENTANA_RECORTE);

    /* Poste (sin shear, estatico) */
    dibujar_linea_bresenham(senal->poste.inicio, senal->poste.fin,
                            senal->color_poste, VENTANA_RECORTE);
}

/*
 * REFLEXION DEL EDIFICIO
 * Dibuja una copia reflejada del edificio[0] respecto a su borde derecho (x=190).
 * La reflexion aparece como un edificio fantasma a la izquierda del original,
 * demostrando reflexion respecto al eje vertical x = x0.
 *
 * Matriz: M_ref = T(x0,0) * S(-1,1) * T(-x0,0)   [= mat3_reflexion_eje_y(x0)]
 */
static void dibujar_edificio_reflexion(const Edificio *ed, float x0)
{
    Mat3 M = mat3_reflexion_eje_y(x0);

    /* Cuerpo reflejado */
    Poligono cuerpo_p = rectangulo_a_poligono(ed->cuerpo);
    Poligono cuerpo_t = mat3_poligono(M, &cuerpo_p);
    const Color c_relleno = {
        (unsigned char)(ed->relleno.r * 0.35f),
        (unsigned char)(ed->relleno.g * 0.35f),
        (unsigned char)(ed->relleno.b * 0.35f)
    };
    const Color c_contorno = {30, 50, 80};
    dibujar_poligono_relleno(&cuerpo_t, c_relleno, c_contorno, VENTANA_RECORTE);

    /* Ventanas reflejadas */
    const int mg_h = 16, mg_v = 22, sep_h = 14, sep_v = 18;
    const int aw   = ed->cuerpo.ancho - 2*mg_h - (ed->columnas_ventanas-1)*sep_h;
    const int ah   = ed->cuerpo.alto  - 2*mg_v - (ed->filas_ventanas-1)*sep_v;
    const int vw   = aw / ed->columnas_ventanas;
    const int vh   = ah / ed->filas_ventanas;
    const Color c_vent = {(unsigned char)(ed->ventanas.r*0.35f),
                          (unsigned char)(ed->ventanas.g*0.35f),
                          (unsigned char)(ed->ventanas.b*0.35f)};

    for (int f = 0; f < ed->filas_ventanas; ++f)
        for (int c = 0; c < ed->columnas_ventanas; ++c) {
            Rectangulo v_rect = {
                ed->cuerpo.x + mg_h + c*(vw+sep_h),
                ed->cuerpo.y + mg_v + f*(vh+sep_v),
                vw, vh
            };
            Poligono vp = rectangulo_a_poligono(v_rect);
            Poligono vt = mat3_poligono(M, &vp);
            dibujar_poligono_relleno(&vt, c_vent, c_contorno, VENTANA_RECORTE);
        }
}

/*
 * ESCALA DEL ARBOL
 * Aplica escala uniforme al arbol respecto a la base de su tronco.
 * M = T(base_cx, base_y) * S(s,s) * T(-base_cx,-base_y)
 */
static void dibujar_arbol_escalado(const Arbol *arbol, float escala)
{
    const float base_cx = (float)(arbol->tronco.x + arbol->tronco.ancho / 2);
    const float base_y  = (float)(arbol->tronco.y);
    Mat3 M = mat3_escala_pivote(escala, escala, base_cx, base_y);
    dibujar_arbol_t(arbol, M);
}

/*
 * NUBE ANIMADA (traslacion pura)
 * La nube es un poligono base centrado en el origen que se traslada.
 * M = T(nube_tx, y_fijo)
 */
static void dibujar_nube(float tx, float ty, const Color *color, float escala)
{
    Mat3 M = mat3_mul(mat3_traslacion(tx, ty), mat3_escala(escala, escala));
    Poligono nube_t = mat3_poligono(M, &NUBE_BASE);
    dibujar_poligono_relleno(&nube_t, *color, *color, VENTANA_RECORTE);
}

/* Linea horizontal de referencia para la reflexion (eje de simetria) */
static void dibujar_eje_reflexion(int x0, int x1, int y, Color color)
{
    dibujar_linea_bresenham((Punto){x0, y}, (Punto){x1, y}, color, VENTANA_RECORTE);
}

/* =============================================================================
   LEYENDAS EN PANTALLA
   ============================================================================= */

static void dibujar_leyendas(const EstadoAnim *e)
{
    const Color blanco = {255, 255, 255};
    const Color amarillo = {250, 220, 60};
    const Color cian    = {80, 220, 210};
    const Color verde   = {100, 230, 120};
    const Color naranja = {240, 160, 50};
    const Color rosa    = {240, 100, 200};

    dibujar_texto(65.0f, 43.0f,
        "TRASLACION: vehiculo avanza | ROTACION: espolones de ruedas giran | COMPOSICION: M=T(tx)*R_piv(ang,cx,cy)",
        blanco);
    dibujar_texto(65.0f, 30.0f,
        "ESCALA: arbol derecho oscila | REFLEXION EJE-Y: edificio fantasma izq | DISTORSION(SHEAR): senal oscila",
        amarillo);
    dibujar_texto(65.0f, 17.0f,
        "TRASLACION nube | ESPACIO=pausa | E/S=escala | D/F=shear | R=gira rueda",
        cian);
    (void)verde; (void)naranja; (void)rosa;
    (void)e;
}

/* =============================================================================
   ETIQUETAS DE TRANSFORMACION EN LA ESCENA
   ============================================================================= */

static void dibujar_etiquetas(void)
{
    const Color amarillo = {250, 220, 60};
    const Color cian     = {80, 220, 210};
    const Color verde    = {100, 230, 120};
    const Color rosa     = {240, 100, 200};
    const Color naranja  = {240, 160,  50};

    dibujar_texto(280.0f, 165.0f, "TRASLACION + ROTACION + COMPOSICION", amarillo);
    dibujar_texto(72.0f,  500.0f, "REFLEXION EJE-Y", cian);
    dibujar_texto(620.0f, 175.0f, "ESCALA", verde);
    dibujar_texto(715.0f, 285.0f, "SHEAR", rosa);
    dibujar_texto(380.0f, 560.0f, "TRASLACION", naranja);
}

/* =============================================================================
   FUNCION PRINCIPAL: dibujar_escena_animada
   ============================================================================= */

void dibujar_escena_animada(const EstadoAnim *estado)
{
    const Color c_cielo  = {121, 193, 244};
    const Color c_cesped = {121, 183, 96};

    /* --- Fondo --- */
    dibujar_rectangulo(ESCENA_URBANA.cielo,   c_cielo,  c_cielo,  VENTANA_RECORTE);
    dibujar_rectangulo(ESCENA_URBANA.cesped,  c_cesped, c_cesped, VENTANA_RECORTE);
    dibujar_sol(&ESCENA_URBANA.sol);

    /* --- TRANSFORMACION: TRASLACION --- nube 1 */
    dibujar_nube(estado->nube_tx, 575.0f, &COLOR_NUBE, 1.0f);

    /* --- TRANSFORMACION: TRASLACION --- nube 2 (desfasada) */
    {
        float tx2 = estado->nube_tx - 320.0f;
        if (tx2 < -130.0f) tx2 += 1090.0f;
        dibujar_nube(tx2, 540.0f, &COLOR_NUBE_2, 0.75f);
    }

    /* --- TRANSFORMACION: REFLEXION eje-Y ---
     * Edificio[0] reflejado respecto a x = borde_derecho = 190.
     * La copia fantasma aparece a la izquierda del edificio original.
     * Matriz: M = T(190,0)*S(-1,1)*T(-190,0)                         */
    dibujar_edificio_reflexion(&ESCENA_URBANA.edificios[0], 190.0f);

    /* Eje de reflexion (linea vertical en x=190, tenue) */
    {
        const Color c_eje = {60, 100, 160};
        dibujar_linea_bresenham((Punto){190, VENTANA_RECORTE.ymin},
                                (Punto){190, VENTANA_RECORTE.ymax},
                                c_eje, VENTANA_RECORTE);
    }

    /* --- Edificios originales (estaticos) --- */
    for (int i = 0; i < ESCENA_URBANA.cantidad_edificios; ++i)
        dibujar_edificio(&ESCENA_URBANA.edificios[i]);

    dibujar_casa(&ESCENA_URBANA.casa);
    dibujar_semaforo(&ESCENA_URBANA.semaforo);

    /* --- Arbol izquierdo: sin transformacion --- */
    dibujar_arbol_t(&ESCENA_URBANA.arboles[0], mat3_identidad());

    /* --- TRANSFORMACION: ESCALA ---
     * Arbol derecho escalado uniformemente respecto a su base.
     * Matriz: M = T(649,140)*S(s,s)*T(-649,-140)                     */
    dibujar_arbol_escalado(&ESCENA_URBANA.arboles[1], estado->arbol_escala);

    /* --- TRANSFORMACION: DISTORSION (SHEAR) ---
     * Senal de transito con shear horizontal respecto a su centro.
     * Muestra el original tenue como referencia de comparacion.
     * Matriz: M = T(cx,cy)*SH_x(shx)*T(-cx,-cy)                     */
    dibujar_senal_t(&ESCENA_URBANA.senal, estado->shear_senal);

    /* --- Carretera --- */
    dibujar_carretera(&ESCENA_URBANA.carretera);

    /* --- TRANSFORMACION: REFLEXION eje-X ---
     * Reflexion del vehiculo respecto a y = 88 (piso del cuerpo).
     * Imagen atenuada aparece en la carretera simulando reflexion en piso humedo.
     * Orden: primero T(tx,0), luego Ref_x(88).
     * Matriz: M = Ref_x(88) * T(tx,0)                                */
    dibujar_vehiculo_reflexion(&ESCENA_URBANA.vehiculo,
                               estado->vehiculo_tx,
                               88.0f);

    /* Linea del eje de reflexion horizontal (piso del vehiculo) */
    {
        const Color c_eje = {180, 80, 80};
        dibujar_eje_reflexion(VENTANA_RECORTE.xmin, VENTANA_RECORTE.xmax, 88, c_eje);
    }

    /* --- TRANSFORMACION: TRASLACION + ROTACION + COMPOSICION ---
     * El vehiculo completo se traslada en X.
     * Las ruedas ademas rotan sobre su propio centro.
     *
     * M_cuerpo = T(tx, 0)
     * M_rueda  = T(tx, 0)  *  R_pivote(ang, cx_rueda, cy_rueda)
     *
     * Ambas composiciones se aplican en dibujar_vehiculo_t usando M_cuerpo
     * para el cuerpo/cabina, y ademas el angulo para los espolones.          */
    {
        Mat3 M_v = mat3_traslacion(estado->vehiculo_tx, 0.0f);
        dibujar_vehiculo_t(&ESCENA_URBANA.vehiculo, M_v, estado->angulo_rueda);
    }

    dibujar_borde_ventana();
    dibujar_etiquetas();
    dibujar_leyendas(estado);
}
