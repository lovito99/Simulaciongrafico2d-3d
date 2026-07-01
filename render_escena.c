#include "render_escena.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "algoritmos.h"
#include "escena.h"
#include "transformaciones.h"

/* =============================================================================
   NUBE ANIMADA (poligono base centrado en el origen)
   ============================================================================= */

static const Poligono NUBE_BASE = {
    .puntos = {
        {-55,-10}, {-45, 6}, {-20,16}, {0,20},
        {20,16},   {45, 6},  {55,-10}, {35,-20}, {-35,-20}
    },
    .cantidad = 9
};
static const Color COLOR_NUBE   = {228, 238, 255};
static const Color COLOR_NUBE_2 = {200, 215, 245};

/* =============================================================================
   HELPERS DE INTERFAZ (dibujan fuera de VENTANA_RECORTE con GL directo)
   ============================================================================= */

static void panel_rect(float x, float y, float w, float h, Color c)
{
    glColor3ub(c.r, c.g, c.b);
    glBegin(GL_QUADS);
    glVertex2f(x,   y);
    glVertex2f(x+w, y);
    glVertex2f(x+w, y+h);
    glVertex2f(x,   y+h);
    glEnd();
}

static void panel_borde(float x, float y, float w, float h, Color c)
{
    glColor3ub(c.r, c.g, c.b);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x,   y);
    glVertex2f(x+w, y);
    glVertex2f(x+w, y+h);
    glVertex2f(x,   y+h);
    glEnd();
}

static Color mezclar(Color a, Color b, float t)
{
    Color r;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    r.r = (unsigned char)((1.0f-t)*a.r + t*b.r);
    r.g = (unsigned char)((1.0f-t)*a.g + t*b.g);
    r.b = (unsigned char)((1.0f-t)*a.b + t*b.b);
    return r;
}

static Color aclarar(Color c, float t) { return mezclar(c, (Color){255,255,255}, t); }
static Color oscurecer(Color c, float t) { return mezclar(c, (Color){0,0,0}, t); }

/* Primitivas decorativas. Los contornos principales siguen usando los
 * algoritmos academicos de Bresenham, punto medio y scan-line. */
static void circulo_relleno(Punto c, int radio, Color color)
{
    glColor3ub(color.r, color.g, color.b);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2i(c.x, c.y);
    for (int i = 0; i <= 40; ++i) {
        float a = 6.2831853f * (float)i / 40.0f;
        glVertex2f((float)c.x + radio*cosf(a), (float)c.y + radio*sinf(a));
    }
    glEnd();
}

static void fondo_degradado(Rectangulo r, Color inferior, Color superior, int bandas)
{
    for (int i = 0; i < bandas; ++i) {
        int y0 = r.y + (r.alto*i)/bandas;
        int y1 = r.y + (r.alto*(i+1))/bandas;
        Color c = mezclar(inferior, superior, (float)i/(float)(bandas-1));
        /* El fondo no requiere scan-line ni clipping por pixel: un quad por
         * banda reduce considerablemente el trabajo de cada fotograma. */
        float x0 = (float)((r.x < VENTANA_RECORTE.xmin) ? VENTANA_RECORTE.xmin : r.x);
        float x1 = (float)(((r.x+r.ancho) > VENTANA_RECORTE.xmax)
                           ? VENTANA_RECORTE.xmax : (r.x+r.ancho));
        panel_rect(x0, (float)y0, x1-x0, (float)(y1-y0+1), c);
    }
}

static void dibujar_silueta_lejana(void)
{
    const int anchos[] = {65,48,80,55,72,44,70,62,75,50,64,55};
    const int altos[]  = {105,72,128,90,116,68,98,122,82,108,74,118};
    int x = 60;
    Color base = {91,137,166};
    for (int i = 0; i < 12; ++i) {
        Color c = (i%2) ? base : aclarar(base, .07f);
        dibujar_rectangulo((Rectangulo){x,140,anchos[i],altos[i]}, c, c, VENTANA_RECORTE);
        /* Antenas y unas pocas ventanas dan escala sin competir con el frente. */
        if (i%3 == 0)
            dibujar_linea_bresenham((Punto){x+anchos[i]/2,140+altos[i]},
                                    (Punto){x+anchos[i]/2,153+altos[i]},
                                    oscurecer(c,.15f), VENTANA_RECORTE);
        x += anchos[i] + 10;
    }
}

/* =============================================================================
   MATRIZ COMPUESTA PARA EDIFICIOS
   Orden de aplicacion al punto (derecha a izquierda):
     T(-cx,-cy)  -> SH  -> S  -> R  -> [Ref]  -> T(cx,cy)  -> T(tx,ty)
   ============================================================================= */

static Mat3 mat_edificio(
    float tx, float ty,
    float ang, float esc, float sh,
    int ref,
    float cx, float cy)
{
    Mat3 M = mat3_traslacion(-cx, -cy);
    M = mat3_mul(mat3_shear_x(sh),        M);
    M = mat3_mul(mat3_escala(esc, esc),   M);
    M = mat3_mul(mat3_rotacion(ang),      M);
    if (ref) M = mat3_mul(mat3_escala(-1.0f, 1.0f), M);
    M = mat3_mul(mat3_traslacion(cx, cy), M);
    M = mat3_mul(mat3_traslacion(tx, ty), M);
    return M;
}

/* =============================================================================
   EDIFICIO CON TRANSFORMACION MATRICIAL
   ============================================================================= */

static void dibujar_edificio_t(const Edificio *ed, Mat3 M)
{
    /* Sombra proyectada y franja lateral: convierten el bloque plano en volumen. */
    Poligono sombra_p = rectangulo_a_poligono((Rectangulo){
        ed->cuerpo.x + 8, ed->cuerpo.y - 7, ed->cuerpo.ancho, ed->cuerpo.alto});
    Poligono sombra_t = mat3_poligono(M, &sombra_p);
    dibujar_poligono_relleno(&sombra_t, (Color){45,65,78}, (Color){45,65,78}, VENTANA_RECORTE);

    /* Cuerpo */
    Poligono cuerpo_p = rectangulo_a_poligono(ed->cuerpo);
    Poligono cuerpo_t = mat3_poligono(M, &cuerpo_p);
    dibujar_poligono_relleno(&cuerpo_t, ed->relleno, ed->contorno, VENTANA_RECORTE);

    Rectangulo brillo = {ed->cuerpo.x+5, ed->cuerpo.y+8, 7, ed->cuerpo.alto-16};
    Poligono brillo_p = rectangulo_a_poligono(brillo);
    Poligono brillo_t = mat3_poligono(M, &brillo_p);
    dibujar_poligono_relleno(&brillo_t, aclarar(ed->relleno,.24f),
                             aclarar(ed->relleno,.24f), VENTANA_RECORTE);

    Rectangulo azotea = {ed->cuerpo.x-3, ed->cuerpo.y+ed->cuerpo.alto-7,
                         ed->cuerpo.ancho+6, 10};
    Poligono azotea_p = rectangulo_a_poligono(azotea);
    Poligono azotea_t = mat3_poligono(M, &azotea_p);
    dibujar_poligono_relleno(&azotea_t, aclarar(ed->relleno,.15f),
                             ed->contorno, VENTANA_RECORTE);

    /* Ventanas */
    const int mg_h = 16, mg_v = 22, sep_h = 14, sep_v = 18;
    const int aw = ed->cuerpo.ancho - 2*mg_h - (ed->columnas_ventanas-1)*sep_h;
    const int ah = ed->cuerpo.alto  - 2*mg_v - (ed->filas_ventanas-1)*sep_v;
    const int vw = aw / ed->columnas_ventanas;
    const int vh = ah / ed->filas_ventanas;

    for (int f = 0; f < ed->filas_ventanas; ++f)
        for (int cv = 0; cv < ed->columnas_ventanas; ++cv) {
            Rectangulo ven = {
                ed->cuerpo.x + mg_h + cv*(vw+sep_h),
                ed->cuerpo.y + mg_v + f*(vh+sep_v),
                vw, vh
            };
            Poligono vp = rectangulo_a_poligono(ven);
            Poligono vt = mat3_poligono(M, &vp);
            dibujar_poligono_relleno(&vt, ed->ventanas, oscurecer(ed->relleno,.45f), VENTANA_RECORTE);
            Punto a = mat3_punto(M, (Punto){ven.x+ven.ancho/2, ven.y+2});
            Punto b = mat3_punto(M, (Punto){ven.x+ven.ancho/2, ven.y+ven.alto-2});
            dibujar_linea_bresenham(a, b, aclarar(ed->relleno,.45f), VENTANA_RECORTE);
        }
}

/* =============================================================================
   EDIFICIO ESTATICO (sin transformacion - edificio morado)
   ============================================================================= */

static void dibujar_edificio(const Edificio *edificio)
{
    dibujar_edificio_t(edificio, mat3_identidad());
}

/* =============================================================================
   BUS AZUL CON TRANSFORMACION MATRICIAL
   ============================================================================= */

static void dibujar_bus_t(const Vehiculo *v, Mat3 M, float ang)
{
    /* Cuerpo principal */
    Poligono cuerpo_p = rectangulo_a_poligono(v->cuerpo);
    Poligono cuerpo_t = mat3_poligono(M, &cuerpo_p);
    dibujar_poligono_relleno(&cuerpo_t, v->color_cuerpo, v->contorno, VENTANA_RECORTE);

    Rectangulo franja = {v->cuerpo.x+8, v->cuerpo.y+21, v->cuerpo.ancho-16, 4};
    Poligono franja_t = mat3_poligono(M, &(Poligono){
        .puntos={{franja.x,franja.y},{franja.x+franja.ancho,franja.y},
                 {franja.x+franja.ancho,franja.y+franja.alto},{franja.x,franja.y+franja.alto}},
        .cantidad=4});
    dibujar_poligono_relleno(&franja_t, aclarar(v->color_cuerpo,.42f),
                             aclarar(v->color_cuerpo,.42f), VENTANA_RECORTE);

    /* Parabrisas izquierdo: frente coherente con el avance hacia la izquierda. */
    Poligono cabina_t = mat3_poligono(M, &v->cabina);
    dibujar_poligono_relleno(&cabina_t, v->color_cabina, v->contorno, VENTANA_RECORTE);

    /* Seis ventanas aprovechan la carroceria ampliada. */
    {
        const Color c_vent     = {185, 220, 255};
        const Color c_cont_ven = {18,  55,  115};
        const int bx0 = v->cuerpo.x + 55;
        const int by  = v->cuerpo.y + 5;
        const int bw  = 27, bh = 14, bgap = 7;
        for (int i = 0; i < 6; i++) {
            Rectangulo ven = {bx0 + i*(bw+bgap), by, bw, bh};
            Poligono vp = rectangulo_a_poligono(ven);
            Poligono vt = mat3_poligono(M, &vp);
            dibujar_poligono_relleno(&vt, c_vent, c_cont_ven, VENTANA_RECORTE);
        }
    }

    /* Ruedas con espolones (muestran rotacion) */
    for (int i = 0; i < 2; i++) {
        Punto centro = mat3_punto(M, v->ruedas[i]);
        dibujar_circulo_punto_medio(centro, v->radio_rueda, v->color_rueda, VENTANA_RECORTE);
        circulo_relleno(centro, v->radio_rueda-2, v->color_rueda);
        dibujar_circulo_punto_medio(centro, v->radio_rueda, (Color){8,18,30}, VENTANA_RECORTE);
        circulo_relleno(centro, v->radio_rueda/2, (Color){74,91,108});
        dibujar_circulo_punto_medio(centro, v->radio_rueda / 2, (Color){55,55,55}, VENTANA_RECORTE);

        const Color c_espolon = {180, 180, 180};
        for (int j = 0; j < 2; j++) {
            float a = ang + (float)j * 1.5708f;
            Punto tip = {
                centro.x + (int)roundf((float)v->radio_rueda * cosf(a)),
                centro.y + (int)roundf((float)v->radio_rueda * sinf(a))
            };
            dibujar_linea_bresenham(centro, tip, c_espolon, VENTANA_RECORTE);
        }
    }
}

/* =============================================================================
   VEHICULO ROJO CON TRANSFORMACION MATRICIAL
   ============================================================================= */

static void dibujar_vehiculo_t(const Vehiculo *v, Mat3 M, float ang_rueda)
{
    Poligono cuerpo_p = rectangulo_a_poligono(v->cuerpo);
    Poligono cuerpo_t = mat3_poligono(M, &cuerpo_p);
    dibujar_poligono_relleno(&cuerpo_t, v->color_cuerpo, v->contorno, VENTANA_RECORTE);

    Rectangulo brillo = {v->cuerpo.x+12,v->cuerpo.y+v->cuerpo.alto-10,
                         v->cuerpo.ancho-24,5};
    Poligono bp = rectangulo_a_poligono(brillo);
    Poligono bt = mat3_poligono(M,&bp);
    dibujar_poligono_relleno(&bt, aclarar(v->color_cuerpo,.38f),
                             aclarar(v->color_cuerpo,.38f), VENTANA_RECORTE);

    Poligono cabina_t = mat3_poligono(M, &v->cabina);
    dibujar_poligono_relleno(&cabina_t, v->color_cabina, v->contorno, VENTANA_RECORTE);
    /* Cristales separados con reflejo diagonal. */
    Punto techo0 = mat3_punto(M,(Punto){395,102});
    Punto techo1 = mat3_punto(M,(Punto){468,102});
    dibujar_linea_bresenham(techo0,techo1,aclarar(v->color_cabina,.55f),VENTANA_RECORTE);

    Punto sep0 = mat3_punto(M, (Punto){v->cuerpo.x + 82, v->cuerpo.y});
    Punto sep1 = mat3_punto(M, (Punto){v->cuerpo.x + 82, v->cuerpo.y + v->cuerpo.alto});
    dibujar_linea_bresenham(sep0, sep1, v->contorno, VENTANA_RECORTE);

    for (int i = 0; i < 2; i++) {
        Punto centro = mat3_punto(M, v->ruedas[i]);
        dibujar_circulo_punto_medio(centro, v->radio_rueda, v->color_rueda, VENTANA_RECORTE);
        circulo_relleno(centro, v->radio_rueda-2, v->color_rueda);
        dibujar_circulo_punto_medio(centro, v->radio_rueda, (Color){12,12,17}, VENTANA_RECORTE);
        circulo_relleno(centro, v->radio_rueda/2, (Color){86,88,94});
        dibujar_circulo_punto_medio(centro, v->radio_rueda / 2, (Color){60,60,60}, VENTANA_RECORTE);

        const Color c_espolon = {190, 190, 190};
        for (int j = 0; j < 2; j++) {
            float a = ang_rueda + (float)j * 1.5708f;
            Punto tip = {
                centro.x + (int)roundf((float)v->radio_rueda * cosf(a)),
                centro.y + (int)roundf((float)v->radio_rueda * sinf(a))
            };
            dibujar_linea_bresenham(centro, tip, c_espolon, VENTANA_RECORTE);
        }
    }
}

/* REFLEXION del vehiculo rojo en el piso humedo */
static void dibujar_vehiculo_reflexion(const Vehiculo *v, float tx, float y_pivot)
{
    Mat3 M_tras = mat3_traslacion(tx, 0.0f);
    Mat3 M_ref  = mat3_reflexion_eje_x(y_pivot);
    Mat3 M      = mat3_mul(M_ref, M_tras);

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
    v_dim.contorno = (Color){40, 10, 10};

    Poligono cuerpo_p = rectangulo_a_poligono(v->cuerpo);
    Poligono cuerpo_t = mat3_poligono(M, &cuerpo_p);
    dibujar_poligono_relleno(&cuerpo_t, v_dim.color_cuerpo, v_dim.contorno, VENTANA_RECORTE);

    Poligono cabina_t = mat3_poligono(M, &v->cabina);
    dibujar_poligono_relleno(&cabina_t, v_dim.color_cabina, v_dim.contorno, VENTANA_RECORTE);

    for (int i = 0; i < 2; i++) {
        Punto c = mat3_punto(M, v->ruedas[i]);
        dibujar_circulo_punto_medio(c, v->radio_rueda, (Color){10,10,10}, VENTANA_RECORTE);
    }
}

/* =============================================================================
   SENAL DE TRANSITO CON DISTORSION (SHEAR)
   ============================================================================= */

static void dibujar_senal_t(const SenalTransito *senal, float shx)
{
    const float cx = 769.0f, cy = 239.0f;

    const Color c_ref = {80, 20, 20};
    dibujar_contorno_poligono(&senal->forma, c_ref, VENTANA_RECORTE);

    Mat3 M = mat3_shear_x_pivote(shx, cx, cy);
    Poligono forma_t = mat3_poligono(M, &senal->forma);
    dibujar_poligono_relleno(&forma_t, senal->color_senal, senal->contorno, VENTANA_RECORTE);

    dibujar_linea_bresenham(senal->poste.inicio, senal->poste.fin,
                            senal->color_poste, VENTANA_RECORTE);
}

/* =============================================================================
   ARBOLES (estaticos)
   ============================================================================= */

static void dibujar_arbol_t(const Arbol *arbol, Mat3 M)
{
    Poligono tronco_p = rectangulo_a_poligono(arbol->tronco);
    Poligono tronco_t = mat3_poligono(M, &tronco_p);
    Poligono copa_t   = mat3_poligono(M, &arbol->copa);

    dibujar_poligono_relleno(&tronco_t, arbol->color_tronco, arbol->contorno, VENTANA_RECORTE);
    dibujar_poligono_relleno(&copa_t, oscurecer(arbol->color_copa,.12f),
                             arbol->contorno, VENTANA_RECORTE);

    /* Capas de follaje sobre la silueta triangular original. */
    Punto centro = mat3_punto(M, (Punto){
        arbol->tronco.x + arbol->tronco.ancho/2,
        arbol->tronco.y + arbol->tronco.alto + 33});
    Color hoja = arbol->color_copa;
    circulo_relleno((Punto){centro.x-24,centro.y-4},25,oscurecer(hoja,.08f));
    circulo_relleno((Punto){centro.x+23,centro.y-3},27,hoja);
    circulo_relleno((Punto){centro.x,centro.y+20},31,aclarar(hoja,.08f));
    circulo_relleno((Punto){centro.x-8,centro.y+31},12,aclarar(hoja,.22f));
}

/* =============================================================================
   ELEMENTOS DE ESCENA ESTATICOS
   ============================================================================= */

static void dibujar_casa(const Casa *casa)
{
    const Rectangulo puerta   = {casa->cuerpo.x + 54, casa->cuerpo.y, 25, 58};
    const Rectangulo vent_izq = {casa->cuerpo.x + 18, casa->cuerpo.y + 46, 28, 26};
    const Rectangulo vent_der = {casa->cuerpo.x + 88, casa->cuerpo.y + 46, 28, 26};
    const Color c_puerta = {129, 84, 51};
    const Color c_vent   = {186, 228, 247};

    dibujar_rectangulo((Rectangulo){casa->cuerpo.x+8,casa->cuerpo.y-5,
                                    casa->cuerpo.ancho,casa->cuerpo.alto},
                       (Color){70,76,78}, (Color){70,76,78}, VENTANA_RECORTE);
    dibujar_rectangulo(casa->cuerpo, casa->color_cuerpo, casa->contorno, VENTANA_RECORTE);
    dibujar_rectangulo((Rectangulo){casa->cuerpo.x+5,casa->cuerpo.y+8,6,casa->cuerpo.alto-16},
                       aclarar(casa->color_cuerpo,.35f), aclarar(casa->color_cuerpo,.35f),
                       VENTANA_RECORTE);
    dibujar_poligono_relleno(&casa->techo, casa->color_techo, casa->contorno, VENTANA_RECORTE);
    dibujar_rectangulo(puerta,   c_puerta, casa->contorno, VENTANA_RECORTE);
    dibujar_rectangulo(vent_izq, c_vent,   casa->contorno, VENTANA_RECORTE);
    dibujar_rectangulo(vent_der, c_vent,   casa->contorno, VENTANA_RECORTE);
    dibujar_linea_bresenham((Punto){vent_izq.x+14,vent_izq.y},
                            (Punto){vent_izq.x+14,vent_izq.y+26}, casa->contorno, VENTANA_RECORTE);
    dibujar_linea_bresenham((Punto){vent_der.x+14,vent_der.y},
                            (Punto){vent_der.x+14,vent_der.y+26}, casa->contorno, VENTANA_RECORTE);
    circulo_relleno((Punto){puerta.x+19,puerta.y+29},2,(Color){242,198,73});
}

static void dibujar_carretera(const Carretera *carretera)
{
    dibujar_rectangulo((Rectangulo){0,132,900,14}, (Color){210,202,188},
                       (Color){210,202,188}, VENTANA_RECORTE);
    dibujar_rectangulo(carretera->cuerpo, carretera->relleno, carretera->contorno, VENTANA_RECORTE);
    dibujar_rectangulo((Rectangulo){0,62,900,9}, oscurecer(carretera->relleno,.17f),
                       oscurecer(carretera->relleno,.17f), VENTANA_RECORTE);
    for (int i = 0; i < carretera->cantidad_divisores; ++i)
        for (int grosor=-2; grosor<=2; ++grosor)
            dibujar_linea_bresenham(
                (Punto){carretera->divisores[i].inicio.x,
                        carretera->divisores[i].inicio.y+grosor},
                (Punto){carretera->divisores[i].fin.x,
                        carretera->divisores[i].fin.y+grosor},
                carretera->color_divisor, VENTANA_RECORTE);
    /* Marcas finas del asfalto para romper la superficie uniforme. */
    for (int x=80; x<840; x+=125)
        dibujar_linea_bresenham((Punto){x,77+(x%3)*5},(Punto){x+28,76+(x%3)*5},
                                (Color){89,92,102},VENTANA_RECORTE);
}

static void dibujar_semaforo(const Semaforo *semaforo, int fase)
{
    /* Colores "apagados" (oscuros) para luces inactivas */
    static const Color DIM[3] = {
        {50, 14, 14},   /* rojo apagado    */
        {52, 44, 14},   /* amarillo apagado */
        {14, 42, 20}    /* verde apagado    */
    };

    dibujar_rectangulo((Rectangulo){semaforo->poste.x+5,semaforo->poste.y-2,
                                    semaforo->poste.ancho,semaforo->poste.alto},
                       (Color){49,55,61},(Color){49,55,61},VENTANA_RECORTE);
    dibujar_rectangulo(semaforo->poste, semaforo->color_poste, semaforo->contorno, VENTANA_RECORTE);
    dibujar_rectangulo(semaforo->caja,  semaforo->color_caja,  semaforo->contorno, VENTANA_RECORTE);

    for (int i = 0; i < 3; ++i) {
        Color c = (i == fase) ? semaforo->colores_luces[i] : DIM[i];
        /* Pequena visera sobre cada lente. */
        dibujar_linea_bresenham((Punto){semaforo->luces[i].x-12,semaforo->luces[i].y+11},
                                (Punto){semaforo->luces[i].x+12,semaforo->luces[i].y+11},
                                (Color){18,21,24},VENTANA_RECORTE);
        circulo_relleno(semaforo->luces[i], semaforo->radio-1, c);
        dibujar_circulo_punto_medio(semaforo->luces[i], semaforo->radio, c, VENTANA_RECORTE);
    }

    /* Halo de brillo extra alrededor de la luz activa */
    dibujar_circulo_punto_medio(semaforo->luces[fase],
                                semaforo->radio + 4,
                                semaforo->colores_luces[fase],
                                VENTANA_RECORTE);
}

static void dibujar_sol(const Sol *sol)
{
    circulo_relleno(sol->centro, sol->radio+12, (Color){255,224,139});
    for (int i = 0; i < sol->cantidad_rayos; ++i)
        dibujar_linea_bresenham(sol->rayos[i].inicio, sol->rayos[i].fin,
                                sol->color, VENTANA_RECORTE);
    dibujar_circulo_punto_medio(sol->centro, sol->radio, sol->color, VENTANA_RECORTE);
    circulo_relleno(sol->centro, sol->radio-2, sol->color);
    circulo_relleno((Punto){sol->centro.x-12,sol->centro.y+13},11,
                    aclarar(sol->color,.32f));
}

static void dibujar_nube(float tx, float ty, const Color *color, float escala)
{
    Mat3 M = mat3_mul(mat3_traslacion(tx, ty), mat3_escala(escala, escala));
    Poligono nube_t = mat3_poligono(M, &NUBE_BASE);
    dibujar_poligono_relleno(&nube_t, *color, *color, VENTANA_RECORTE);
    Punto c1 = mat3_punto(M,(Punto){-27,0});
    Punto c2 = mat3_punto(M,(Punto){0,8});
    Punto c3 = mat3_punto(M,(Punto){29,0});
    circulo_relleno(c1,(int)(22*escala),*color);
    circulo_relleno(c2,(int)(28*escala),aclarar(*color,.09f));
    circulo_relleno(c3,(int)(21*escala),*color);
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
   ETIQUETAS DE TRANSFORMACION EN LA ESCENA (espanol)
   ============================================================================= */

static void dibujar_etiquetas(const EstadoAnim *e)
{
    const Color amarillo = {250, 220,  60};
    const Color cian     = { 80, 220, 210};
    const Color rosa     = {240, 100, 200};
    const Color naranja  = {240, 160,  50};
    const Color verde    = {100, 230, 120};
    const Color lila     = {200, 140, 255};

    /* Vehiculo rojo: traslacion + rotacion + composicion */
    dibujar_texto(250.0f, 148.0f, "CARRO: CARRIL INFERIOR + ROTACION", amarillo);

    /* Bus azul */
    dibujar_texto(300.0f, 192.0f, "BUS: CARRIL SUPERIOR (sentido contrario)", cian);

    /* Senal: distorsion shear automatica */
    dibujar_texto(690.0f, 285.0f, "SHEAR", rosa);

    /* Nube: traslacion */
    dibujar_texto(340.0f, 560.0f, "TRASLACION (nube)", naranja);

    /* Reflexion del vehiculo en el piso */
    dibujar_texto(65.0f, 72.0f, "REFLEXION EJE-X (piso)", (Color){200,80,80});

    /* Edificio naranja: mostrar transformaciones activas */
    {
        char buf[64] = "ESCALA";
        int activos = 0;
        if (e->ed_naranja_angulo != 0.0f)  { if (activos++) snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), " + ROT"); else snprintf(buf, sizeof(buf), "ROT"); activos=1; }
        if (e->ed_naranja_shear  != 0.0f)  { if (activos) snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), " + SHEAR"); else { snprintf(buf, sizeof(buf), "SHEAR"); activos=1; } }
        if (e->ed_naranja_reflejo)          { if (activos) snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), " + REFLEJO"); else { snprintf(buf, sizeof(buf), "REFLEJO"); activos=1; } }
        if (e->ed_naranja_tx != 0.0f || e->ed_naranja_ty != 0.0f) {
            if (activos) snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), " + TRASL"); else { snprintf(buf, sizeof(buf), "TRASLACION"); activos=1; }
        }
        if (activos > 1) snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), " [COMPOSICION]");
        (void)activos;
        dibujar_texto(675.0f, 510.0f, buf, verde);
    }

    /* Edificio azul: mostrar transformaciones activas */
    {
        char buf[64] = "";
        int activos = 0;
        if (e->ed_azul_escala != 1.0f) { snprintf(buf, sizeof(buf), "ESCALA"); activos++; }
        if (e->ed_azul_angulo != 0.0f) { if (activos++) snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), " + ROT"); else { snprintf(buf, sizeof(buf), "ROT"); activos=1; } }
        if (e->ed_azul_shear  != 0.0f) { if (activos) snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), " + SHEAR"); else { snprintf(buf, sizeof(buf), "SHEAR"); activos=1; } }
        if (e->ed_azul_reflejo)         { if (activos) snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), " + REFLEJO"); else { snprintf(buf, sizeof(buf), "REFLEJO"); activos=1; } }
        if (e->ed_azul_tx != 0.0f || e->ed_azul_ty != 0.0f) {
            if (activos) snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), " + TRASL"); else { snprintf(buf, sizeof(buf), "TRASLACION"); activos=1; }
        }
        if (activos > 1) snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), " [COMPOSICION]");
        if (activos > 0)
            dibujar_texto(65.0f, 510.0f, buf, lila);
    }
}

/* =============================================================================
   PANEL HUD: SELECTOR DE OBJETO + CONTROLES (espanol)
   ============================================================================= */

static void dibujar_panel_hud(const EstadoAnim *e)
{
    /* --- Colores del panel --- */
    const Color fondo_panel  = {13,  18,  31};
    const Color blanco       = {255, 255, 255};
    const Color gris_claro   = {180, 185, 200};
    const Color amarillo_hud = {245, 215,  60};
    const Color verde_hud    = { 80, 210, 120};
    const Color rojo_hud     = {220,  80,  80};

    /* Colores de cada objeto (sel = activo, unsel = inactivo) */
    const Color tab_col_sel[4]   = {{110,25,25},  {20,60,130},  {120,65,18},  {28,48,115}};
    const Color tab_col_unsel[4] = {{ 50,12,12},  {12,32, 70},  { 58,30, 8},  {14,24, 58}};
    const Color tab_texto_sel[4] = {{255,120,120},{120,190,255},{255,180,80}, {130,160,255}};
    const char *tab_nombre[4]    = {
        "1. VEHICULO ROJO",
        "2. BUS AZUL",
        "3. ED. NARANJA",
        "4. ED. AZUL"
    };

    /* ===== PANEL SUPERIOR (y=621..700) ===== */
    panel_rect(0.0f, 621.0f, 900.0f, 79.0f, fondo_panel);
    panel_rect(0.0f, 621.0f, 7.0f, 79.0f, (Color){55,185,220});
    panel_rect(0.0f, 621.0f, 900.0f, 2.0f, (Color){45,70,95});

    /* Titulo */
    dibujar_texto(65.0f, 682.0f, "ESCENA URBANA 2D  -  ENTREGA 2: TRANSFORMACIONES GEOMETRICAS", blanco);

    /* Estado pausa / ESC */
    if (e->pausa)
        dibujar_texto(700.0f, 682.0f, "[PAUSADO]", rojo_hud);
    else
        dibujar_texto(700.0f, 682.0f, "[ANIMANDO]", verde_hud);
    dibujar_texto(790.0f, 682.0f, "ESC=Salir", gris_claro);

    /* ---- Tabs de seleccion (y=650..672) ---- */
    const float tab_x0  = 65.0f;
    const float tab_w   = 190.0f;
    const float tab_gap = 5.0f;
    const float tab_y   = 649.0f;
    const float tab_h   = 23.0f;

    for (int i = 0; i < 4; i++) {
        float tx = tab_x0 + (float)i * (tab_w + tab_gap);
        Color bg  = (e->objeto_sel == i) ? tab_col_sel[i]   : tab_col_unsel[i];
        Color txt = (e->objeto_sel == i) ? tab_texto_sel[i] : gris_claro;
        panel_rect(tx, tab_y, tab_w, tab_h, bg);
        if (e->objeto_sel == i)
            panel_rect(tx, tab_y, tab_w, 3.0f, tab_texto_sel[i]);
        if (e->objeto_sel == i)
            panel_borde(tx, tab_y, tab_w, tab_h, tab_texto_sel[i]);
        dibujar_texto(tx + 6.0f, tab_y + 7.0f, tab_nombre[i], txt);
    }

    /* Instruccion de seleccion */
    dibujar_texto(65.0f, 636.0f, "TAB o teclas 1-4 para seleccionar objeto activo", gris_claro);

    /* ===== PANEL INFERIOR (y=0..59) ===== */
    panel_rect(0.0f, 0.0f, 900.0f, 59.0f, fondo_panel);
    panel_rect(0.0f, 0.0f, 7.0f, 59.0f, (Color){55,185,220});

    /* Linea separadora superior del panel inferior */
    glColor3ub(40, 50, 80);
    glBegin(GL_LINES);
    glVertex2f(0.0f, 59.0f);
    glVertex2f(900.0f, 59.0f);
    glEnd();

    /* Controles especificos segun objeto seleccionado */
    switch (e->objeto_sel) {

    case OBJ_VEHICULO_ROJO:
        dibujar_texto(65.0f, 44.0f, "VEHICULO ROJO  |  CONTROLES:", amarillo_hud);
        dibujar_texto(65.0f, 28.0f, "Flecha Izq/Der = Mover manualmente    ESPACIO = Pausar / Reanudar animacion", blanco);
        dibujar_texto(65.0f, 12.0f, "0 = Reiniciar posicion", gris_claro);
        break;

    case OBJ_BUS_AZUL:
        dibujar_texto(65.0f, 44.0f, "BUS AZUL  |  CONTROLES:", (Color){120,190,255});
        dibujar_texto(65.0f, 28.0f, "Flecha Izq/Der = Mover manualmente    ESPACIO = Pausar / Reanudar animacion", blanco);
        dibujar_texto(65.0f, 12.0f, "0 = Reiniciar posicion", gris_claro);
        break;

    case OBJ_ED_NARANJA: {
        dibujar_texto(65.0f, 44.0f, "EDIFICIO NARANJA  |  CONTROLES:", (Color){255,180,80});
        dibujar_texto(65.0f, 28.0f,
            "Flechas = Trasladar  |  E/S = Escala +/-  |  Q/W = Rotar  |  D/F = Shear  |  R = Reflexion  |  0 = Reset",
            blanco);
        char vals[120];
        snprintf(vals, sizeof(vals),
            "Escala=%.2f  Angulo=%.2f  Shear=%.2f  Tx=%.0f  Ty=%.0f  Reflexion=%s",
            e->ed_naranja_escala, e->ed_naranja_angulo, e->ed_naranja_shear,
            e->ed_naranja_tx, e->ed_naranja_ty,
            e->ed_naranja_reflejo ? "SI" : "NO");
        dibujar_texto(65.0f, 12.0f, vals, gris_claro);
        break;
    }

    case OBJ_ED_AZUL: {
        dibujar_texto(65.0f, 44.0f, "EDIFICIO AZUL  |  CONTROLES:", (Color){130,160,255});
        dibujar_texto(65.0f, 28.0f,
            "Flechas = Trasladar  |  E/S = Escala +/-  |  Q/W = Rotar  |  D/F = Shear  |  R = Reflexion  |  0 = Reset",
            blanco);
        char vals[120];
        snprintf(vals, sizeof(vals),
            "Escala=%.2f  Angulo=%.2f  Shear=%.2f  Tx=%.0f  Ty=%.0f  Reflexion=%s",
            e->ed_azul_escala, e->ed_azul_angulo, e->ed_azul_shear,
            e->ed_azul_tx, e->ed_azul_ty,
            e->ed_azul_reflejo ? "SI" : "NO");
        dibujar_texto(65.0f, 12.0f, vals, gris_claro);
        break;
    }
    }
}

/* =============================================================================
   FUNCION PRINCIPAL: dibujar_escena_animada
   ============================================================================= */

void dibujar_escena_animada(const EstadoAnim *estado)
{
    const Color c_cielo_inf = {151, 211, 235};
    const Color c_cielo_sup = {57, 133, 196};
    const Color c_cesped_inf = {72, 132, 72};
    const Color c_cesped_sup = {128, 181, 92};

    /* --- Fondo --- */
    fondo_degradado(ESCENA_URBANA.cielo, c_cielo_inf, c_cielo_sup, 18);
    fondo_degradado(ESCENA_URBANA.cesped, c_cesped_inf, c_cesped_sup, 8);
    dibujar_sol(&ESCENA_URBANA.sol);

    /* --- TRASLACION: nubes --- */
    dibujar_nube(estado->nube_tx, 575.0f, &COLOR_NUBE, 1.0f);
    {
        float tx2 = estado->nube_tx - 320.0f;
        if (tx2 < -130.0f) tx2 += 1090.0f;
        dibujar_nube(tx2, 540.0f, &COLOR_NUBE_2, 0.75f);
    }

    dibujar_silueta_lejana();

    /* --- Edificio morado: estatico (no se transforma) --- */
    dibujar_edificio(&ESCENA_URBANA.edificios[1]);

    /* --- Edificio azul: TODAS las transformaciones manuales ---
     *   COMPOSICION: M = T(tx,ty) * T(cx,cy) * R * S * SH * [Ref] * T(-cx,-cy)  */
    {
        const float cx = 20.0f  + 170.0f / 2.0f;   /* 105 */
        const float cy = 140.0f + 350.0f / 2.0f;   /* 315 */
        Mat3 M = mat_edificio(
            estado->ed_azul_tx, estado->ed_azul_ty,
            estado->ed_azul_angulo, estado->ed_azul_escala,
            estado->ed_azul_shear, estado->ed_azul_reflejo,
            cx, cy);
        dibujar_edificio_t(&ESCENA_URBANA.edificios[0], M);
    }

    /* --- Edificio naranja: TODAS las transformaciones manuales ---
     *   COMPOSICION: M = T(tx,ty) * T(cx,cy) * R * S * SH * [Ref] * T(-cx,-cy) */
    {
        const float cx = 700.0f + 190.0f / 2.0f;   /* 795 */
        const float cy = 140.0f + 360.0f / 2.0f;   /* 320 */
        Mat3 M = mat_edificio(
            estado->ed_naranja_tx, estado->ed_naranja_ty,
            estado->ed_naranja_angulo, estado->ed_naranja_escala,
            estado->ed_naranja_shear, estado->ed_naranja_reflejo,
            cx, cy);
        dibujar_edificio_t(&ESCENA_URBANA.edificios[2], M);
    }

    /* --- Casa y elementos fijos --- */
    dibujar_casa(&ESCENA_URBANA.casa);
    dibujar_semaforo(&ESCENA_URBANA.semaforo, estado->semaforo_fase);

    /* --- Arboles estaticos --- */
    dibujar_arbol_t(&ESCENA_URBANA.arboles[0], mat3_identidad());
    dibujar_arbol_t(&ESCENA_URBANA.arboles[1], mat3_identidad());

    /* --- DISTORSION (SHEAR): senal de transito, automatica ---
     *   M = T(cx,cy) * SH_x(shx) * T(-cx,-cy)                     */
    dibujar_senal_t(&ESCENA_URBANA.senal, estado->shear_senal);

    /* --- Carretera --- */
    dibujar_carretera(&ESCENA_URBANA.carretera);

    /* --- REFLEXION EJE-X: vehiculo rojo reflejado en el piso ---
     *   M = Ref_x(88) * T(tx,0)                                     */
    dibujar_vehiculo_reflexion(&ESCENA_URBANA.vehiculo,
                               estado->vehiculo_tx, 70.0f);
    {
        const Color c_eje = {160, 60, 60};
        dibujar_linea_bresenham(
            (Punto){VENTANA_RECORTE.xmin, 70},
            (Punto){VENTANA_RECORTE.xmax, 70},
            c_eje, VENTANA_RECORTE);
    }

    /* --- TRASLACION + ROTACION + COMPOSICION: vehiculo rojo ---
     *   M_cuerpo = T(tx, 0)
     *   M_rueda  = T(tx, 0) * R_pivote(ang, cx_rueda, cy_rueda)     */
    {
        Mat3 M_v = mat3_traslacion(estado->vehiculo_tx, 0.0f);
        dibujar_vehiculo_t(&ESCENA_URBANA.vehiculo, M_v, estado->angulo_rueda);
    }

    /* --- TRASLACION: bus azul (carril contrario, derecha -> izquierda) ---
     *   M_bus = T(bus_tx, 0)                                         */
    {
        Mat3 M_b = mat3_traslacion(estado->bus_tx, 0.0f);
        dibujar_bus_t(&BUS_AZUL, M_b, estado->bus_angulo);
    }

    /* --- Borde de la ventana de recorte --- */
    dibujar_borde_ventana();

    /* --- Etiquetas de transformacion en la escena --- */
    dibujar_etiquetas(estado);

    /* --- Panel HUD (selector + controles) --- */
    dibujar_panel_hud(estado);
}
