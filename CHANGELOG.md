# Cambios

## 0.7.5 — 2026-11-09 (experimental)

Publicación que reúne las correcciones posteriores a 0.7.0. Las versiones intermedias no se publicaron como releases.

- Corrección de la desconexión al abrir instancias temporales de Magnify: metadatos nativos aislados hasta autenticar la UI real, lease por proceso y limpieza condicionada al dueño. Los relays no sustituyen el PID del renderer ni vacían el estado compartido al terminar.
- La máscara de Inicio se aplica también antes de reenviar Win-UP cuando esa pulsación se usó con el ratón. Resiste la repetición de Win y pares izquierdo/derecho. La tecla Win sola y los atajos de teclado siguen pasando sin sintetizar modificadores.
- Una trayectoria de rueda nueva usa el tiempo real de captura, acotado a 16 ms, y evita el primer envío vacío del 100 %. Conserva deltas finos y las reglas de inversión; no inyecta Win+«+» para activar el motor.
- Filtrado de ecos asíncronos del setter nativo durante un objetivo gestionado. Evita retrocesos de la imagen y cancelaciones de rueda/teclado mantenido; el primer toque y los comandos nativos explícitos conservan prioridad.
- Seguimiento adaptativo para ruedas de giro libre: límite rápido independiente de 700 %/s, transición y recorrido pendiente de 40 ms, precisión normal intacta y cancelación del impulso al invertir. Ajustable y desactivable con `wheelFreeSpinMaxRate=0`.
- Diagnóstico de máscaras de Inicio y resultados del cambio de ventanas, con protocolo v14 para separar datos de revisiones anteriores.

## 0.7.0 — 2026-11-09 (experimental)

- Ganancia predeterminada de rueda al 35 %, conservando deltas fraccionarios y límite de velocidad.
- Ciclado completo de ventanas por cada muesca, incluidas ráfagas, con confirmación asíncrona del foco; los fallos no avanzan el orden ni reinician bucles de dos ventanas.
- Perfil Win+rueda para zoom, Win+clic central al 100 % y Ctrl+Alt+rueda para recorrer ventanas. Perfil anterior Ctrl+Alt configurable. Acciones extra de maximizar/restaurar/minimizar opcionales, desactivadas por defecto.
- Hilo dedicado para hooks breves, resincronización tras bloqueo y ruta sin sondeos de teclas para movimientos normales del ratón; teclado observado siempre pasante. Máscara neutral una vez por pulsación Win consumida; no se sintetizan modificadores. DOWN/UP emparejados aunque se suelte el modificador primero.
- Teclado mantenido progresivo tras 200 ms, sin multiplicar saltos nativos. Factor continuo, reloj acotado y detención en el nivel/posición visibles; inversión sin arrastrar objetivos adelantados.
- UI al 100 % lista para el primer zoom sin exigir un renderer preexistente. Primado fino común a pellizco, rueda y solicitudes de teclado; modos lente/acoplado conservan controles nativos.
- El escritorio ya no se confunde con Task View. La multitarea suspende/restaura el zoom global; una entrada nueva explícita prevalece conservando la trayectoria y descartando revisiones antiguas.
- Primer pellizco en frío limitado a un 3 % y a que el mismo gesto siga activo; intención de rueda de arranque limitada a una muesca y 1,5 s; cancelación por reset, nuevo gesto, sesión y comandos nativos. Giro libre sin colas diferidas.
- Supresión reversible y opcional de «leer desde aquí» y su cursor especial, ligada a la identidad exacta del Magnify verificado. Los clics ordinarios y Narrador no se interceptan.
- Tests renovados y revisión independiente de tres agentes. Las comprobaciones de hardware y las limitaciones se detallan en `docs/VALIDATION.md`.

## 0.6.1 — 2026-11-08 (experimental)

- Mantener Win+«+»/«−» aplica ×2 a las repeticiones, configurable de 1 a 4, incluidos OEM y teclado numérico. El primer toque, los botones de la Lupa y la repetición global del teclado conservan su comportamiento.
- Al soltar fija el factor decimal y la posición que Windows estaba renderizando, descartando repeticiones antiguas. Una entrada nueva de rueda/pellizco o reset cancela el seguimiento sin sobrescribir su resultado.
- El cambio exige capacidades verificadas y ofrece comportamiento nativo si faltan. La pausa del mod desactiva también la aceleración.
- 277 comprobaciones automáticas, incluidas 42 específicas nuevas. Instalada localmente; validación física de la nueva función pendiente.

## 0.6.0 — 2026-11-08 (experimental)

- Sensibilidad predeterminada de rueda al 20 %, primer paso decimal fino y control configurable de velocidad para ruedas libres. Acumulador acotado por revisión; caducidad de 200 ms sin cola de mensajes por delta.
- Cancelación terminal de gestos: ningún Continue/Finish después de Cancel o de un Start fallido. Limpieza ante Continue fallido. Activación lateral mediante SetForegroundWindow sin Alt/Ctrl+Tab implícito.
- Los comandos nativos de teclado prevalecen sobre la suspensión de multitarea, incluida una suspensión todavía pendiente.
- Perfiles offline ligados a la identidad exacta de los binarios, validación de secciones y capacidades independientes. La Lupa nativa permanece operativa si una interfaz es incompatible.
- Supresión opcional del indicador de arranque mediante una interfaz pública del Shell; un fallo de esa opción no impide abrir la Lupa.
- 235 comprobaciones automáticas. El usuario confirmó recuperación de teclas/clics, primer paso y rueda libre controlada con pellizco y teclado.

## 0.5.0 — 2026-11-08 (experimental)

- Nuevo nombre: Better Magnifier. Conserva el identificador del mod para actualizar instalaciones previas.
- Ctrl+Alt+rueda con recorrido proporcional, deltas de alta resolución y transición decimal finita. Sensibilidad y duración configurables.
- Teclado, pellizco, multitarea, pausa y cambios de sesión invalidan la trayectoria y los eventos antiguos de rueda. Protección del puntero durante los intervalos entre fotogramas.
- La Lupa nativa procesa cada evento una sola vez; el hook global solo despierta una Lupa cerrada y no reproduce recorrido antiguo.
- Carga temprana del módulo del shell, reintentos acotados y separación entre bloqueo y suspensión. Win+Esc mantiene la Lupa cerrada hasta una nueva entrada física.
- 172 comprobaciones automáticas locales. El usuario confirmó rueda fluida, alternancia de entradas y cierre/reapertura. La comprobación tras reiniciar Windows queda pendiente.

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
