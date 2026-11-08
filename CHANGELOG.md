# Cambios

## 0.4.0 — 2026-11-08 (experimental)

- Cambio directo de ventana con los deslizamientos laterales, configurable, una ventana por gesto y sin perfiles de zoom por ventana.
- Zoom global al cambiar de ventana. Task View y Mostrar escritorio se muestran temporalmente al 100 %, restaurando nivel y posición cuando se cierra la última de esas vistas.
- Corrección del inicio tardío/repetido del gesto nativo, cancelación única de un pellizco confirmado tarde e inicios con información provisional de contactos.
- La reconstrucción temporal del renderizador ya no se trata como cierre de la Lupa. Los cambios nativos del factor también rebasan el pellizco aunque no atraviesen los controles de interfaz.
- La protección del puntero cubre ClipCursor, que puede recolocarlo sin emitir un movimiento sintético. Se verifica el caso del pellizco lento y se conserva el comportamiento del teclado.
- 135 comprobaciones automáticas, incluyendo callbacks reales con backend simulado. Instalada localmente con ambos componentes READY. El usuario confirmó pellizco lento, teclado, deslizamientos y restauración del zoom en este equipo.

## 0.3.2 — 2026-11-08 (experimental)

- Protección configurable del puntero, activada por defecto: evita los movimientos sintéticos de la Lupa durante el pellizco y mientras se mantienen tres dedos apoyados.
- Conserva el movimiento real del ratón, el comportamiento del teclado nativo y los deslizamientos normales.
- El diagnóstico informa del número de reajustes omitidos y de su origen, sin guardar coordenadas de los dedos.
- 27 pruebas del adaptador nativo, además de las 48 del núcleo y 21 de runtime/ABI: 96 en total.
- El usuario confirmó la fluidez de la ruta decimal y, después, que el puntero se mantenía estable y las ventanas dejaban de saltar al sostener el pellizco.

## 0.3.1 — 2026-11-08 (experimental)

Pendiente: fluidez en hardware y movimientos inesperados del puntero. Esta iteración no se considera estable.

- Factores decimales continuos durante el pellizco; primado único por interfaz y control de hilo/versión de la base para evitar solicitudes antiguas.
- Integración con el motor nativo de la Lupa, conservando los atajos y las animaciones de Windows, también con el teclado numérico.
- Sincronización del nivel entre teclado y pellizco. Los cambios nativos invalidan solicitudes antiguas y se adoptan antes del siguiente movimiento de dedos.
- Aceleración configurable por velocidad; temporización HID y respuesta inmediata al invertir el gesto en un límite.
- Eliminado el tope predeterminado de 400 %. El modo nativo usa el rango de Windows, hasta 1600 %.
- Seguimiento del ratón a través del controlador nativo de Windows, respetando sus ajustes de accesibilidad.
- Ocultación integrada de la ventana y controles táctiles de la Lupa, activada por defecto y reversible.
- Win+Esc deja la Lupa cerrada. Un nuevo pellizco puede iniciarla; se descarta el recorrido recibido mientras arranca para evitar saltos.
- Auxiliar nativo activado por avisos de entrada, con espera en reposo. Comprobación de compilación de Windows en ambos procesos.
- Instalador con conservación de ajustes, comprobación de correspondencia entre fuente y DLL y recuperación de la configuración previa ante errores.
- Eliminadas las actualizaciones duplicadas, los falsos cambios de teclado y el reinicio de trabajo nativo por porcentajes repetidos. Cierre seguro de la ventana interna de mensajes, incluido el caso reentrante.
- 48 pruebas del núcleo, 21 de runtime/ABI, 20 del adaptador nativo y una prueba optativa de integración con cinco comprobaciones.

## 0.2.0

- Filtro nativo de gestos en Explorer para distinguir pellizcos y deslizamientos de multitarea.

## 0.1.0

- Prototipo de reconocimiento HID y ampliación mediante la API independiente. La prueba física mostró conflictos y retraso.
