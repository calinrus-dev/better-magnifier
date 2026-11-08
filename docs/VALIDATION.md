# Validación

Fecha de 0.3.1: 8 de noviembre de 2026.

## Estado de 0.3.1

La ruta por interfaz con porcentajes enteros mostró tirones y desplazamiento del puntero; se desactivó y se restauraron las preferencias de Windows. La revisión posterior conserva el factor decimal en los fotogramas intermedios y usa la interfaz solo para entrar/salir de ampliación. Falta confirmar físicamente esta última ruta. Pasar los tests no acredita por sí solo la fluidez percibida.

- Compilación x64 con Clang 20.1.3 de Windhawk 1.7.3 y `-Wall -Wextra -Werror`.
- 48 pruebas del reconocedor: aceleración por velocidad, temporización HID, reversión en los límites, cambios de zoom por teclado, clasificación nativa y descarte del recorrido recibido durante el arranque.
- 21 comprobaciones de runtime/ABI: incluyen adopción de zoom externo, recuperación al 100 %, anclas tras cambios externos y movimiento del puntero durante un pellizco en el modo API.
- 20 comprobaciones del adaptador nativo con backend simulado: primado único, factores decimales distintos sin cuantización, una única escritura por solicitud, descarte de bases anteriores al primado/teclado, separación de cambios internos y comandos reales, hilo correcto, recuperación y cierre reentrante.
- 5 pruebas de integración ejecutadas contra una iteración anterior de la Lupa real instalada: objetivo desde el controlador de pellizco, cambios rápidos, incremento por IPC desde el nivel del pellizco, vuelta al pellizco y recuperación al 100 %. Se comprueba el factor realmente renderizado con `MagGetFullscreenTransform`. El incremento por IPC no es una pulsación física de teclado.
- Se ha instalado el mod en Explorer y Magnify; ambos auxiliares informan `READY`, con ventana oculta y sin error informado. El mod anterior `magnifier-headless` está desactivado y sigue instalado.
- Se identificaron los nombres de las opciones en el binario de Magnify: FollowMouse (16), FollowFocus (17), FollowCaret (18), FollowNarrator (19) y FullScreenTrackingMode (29). Se confirmó por desensamblado que el modo 1 centra el ratón. Se retiró el experimento que invocaba directamente el actualizador de opciones tras una regresión. El mod final conserva las preferencias nativas de seguimiento.
- La revisión independiente detectó y permitió corregir la doble ruta de escritura, cambios internos confundidos con teclado, reinicio por porcentajes repetidos y una carrera de descarga. La ventana de mensajes se destruye en su hilo; un cierre reentrante conserva el código de la DLL hasta terminar el proceso.
- Los hooks de la Lupa no bloquean `MagSetFullscreenTransform` ni reemplazan el comportamiento de los atajos. La entrada/salida de ampliación usa `MagnifierUI::HandleSetZoomLevel(long)`; los factores decimales intermedios usan `FullscreenMagnifier::ChangeMagnificationFactor(float,CPoint*)`. Se exige que el hilo capturado de render coincida con el del callback antes de llamar a este método. Los hooks de comandos de interfaz observan cambios nativos para rebasar el reconocedor.
- El usuario confirmó «Funciona bien» al alternar pellizco, Win+menos numérico y pellizco en la integración nativa anterior a los últimos ajustes de cierre/arranque.
- Tras informar de reapertura con Win+Esc, se eliminó el reinicio automático. Tras informar de un salto al arrancar desde cerrada, se descartó el movimiento acumulado durante la espera y las solicitudes de una instancia anterior. El usuario confirmó después que Win+Esc permanece cerrado al dejar el touchpad y las teclas sin tocar durante cinco segundos. La latencia física de ese arranque sigue pendiente de confirmación; no se afirma que abrir Magnify sea instantáneo.

## Pendiente en 0.3.1

- Confirmación física del primer pellizco después de cerrar y de la ruta final de seguimiento.
- Estabilidad del seguimiento y continuidad del factor de zoom: la ruta decimal revisada requiere confirmación física tras retirar la variante entera.
- Cambios de ajustes desde la interfaz de Windhawk, incluida la ocultación reversible.
- Los cuatro deslizamientos nativos, gestos de dos/cuatro dedos, bloqueo y suspensión en la versión final.
- Monitores con DPI distintos y aplicaciones elevadas.

Las pruebas de integración cambian brevemente el zoom y son optativas. No se ejecutan en CI ni por compilar. Las pruebas sintéticas no demuestran por sí solas la calidad de un touchpad físico.

## Historial

## Estado de 0.2.0

- 37 pruebas del reconocedor y del estado de filtrado, incluidos inicio sin salto, punto de zoom fijo y supresión de un gesto de multitarea cuando se clasifica como pellizco.
- 12 comprobaciones del runtime y de las exportaciones ABI del mod.
- Windows confirma que `TouchpadGesturesController.IsSupported` es verdadero en esta máquina.
- Los símbolos públicos de Microsoft y el desensamblado de la versión instalada confirman las firmas usadas por los cuatro hooks. El contexto de `StartSwipe` se pasa como un valor de 8 bytes en x64.
- El filtro nativo se ha instalado y cargado en Explorer; la memoria de estado devuelve `Native gate: READY` y el PID correcto del shell.
- Se ha recibido una llamada al hook nativo con tres contactos y se ha reenviado por el camino original de Windows.
- Durante una captura de 0.2, el lector recibió 585 informes, ensambló 196 fotogramas y reconoció un pellizco sin errores de decodificación.
- Se ha verificado la ejecución permanente del hilo integrado en Explorer: `Native gate: READY`, `hostEnabled=1` y señal de vida actualizada. No necesita un host de prueba externo.
- La compilación de 0.2 y sus comprobaciones también pasan en GitHub Actions, ejecución `42421864166`.
- La sensación de uso y la supresión del conflicto en los gestos físicos de la versión final siguen pendientes de confirmación del usuario.

Una prueba adicional de la API de ampliación se detuvo al detectar otra ampliación activa. El instalador cierra la Lupa anterior de esta sesión y restaura el 100 % antes de activar el mod.

## Historial de 0.1

La prueba física reconoció 6 pellizcos y 2 deslizamientos en 1.505 fotogramas. El usuario informó de conflictos con la multitarea, retraso y mala sensación de seguimiento. Esta versión no cumple el objetivo; las pruebas sintéticas que pasaba no cubrían la convivencia real con Windows.

## Verificaciones anteriores del prototipo 0.1

- Compilación x64 con Clang 20.1.3 incluido con Windhawk 1.7.3.
- Compilación con `-Wall -Wextra -Werror`.
- 31 pruebas del reconocedor, límites del zoom, rechazo de deslizamientos, cambios de contactos, datos inválidos, ruido, temporización y ensamblado de informes HID.
- Compilación del host independiente y de la DLL del mod.
- 12 comprobaciones del runtime: unidades y coordenadas HID, carga del PE sin ejecutar código y exportaciones ABI que utiliza Windhawk.
- Compilación adicional con el script de compatibilidad del repositorio oficial de mods, usando los metadatos y opciones del archivo distribuible.

## Máquina de desarrollo y comprobaciones iniciales

- Windows 11 25H2, compilación 26200.9711.
- La enumeración Raw Input identifica un descriptor HID de Precision Touchpad compatible.
- Una captura de 60 segundos detectó el dispositivo, pero recibió cero informes: no acredita reconocimiento físico de los gestos.
- La API de ampliación acepta 110 %, devuelve la ampliación aplicada y permite restaurar el 100 %.
- El script de compilación no modifica el registro de gestos. Los valores observados antes de las pruebas: deslizamientos de tres dedos habilitados y deslizamientos de cuatro dedos configurados.

## Pendiente de validación manual

- Calidad del seguimiento, latencia percibida y ampliación ligada al movimiento de los dedos en 0.2.
- Cambios de ajustes desde la interfaz de Windhawk.
- Que un pellizco no active también un deslizamiento nativo de Windows.
- Cambio de aplicaciones a izquierda/derecha, mostrar escritorio abajo y multitarea arriba con tres dedos, antes y después de usar zoom.
- Que el zoom de aplicaciones con dos dedos y los gestos de cuatro dedos conserven su comportamiento.
- Atajos de recuperación, pausar/reanudar y desactivación del mod mientras el escritorio está ampliado.
- Suspensión, bloqueo de sesión y retirada del dispositivo mientras hay ampliación.
- Aplicaciones elevadas, múltiples pantallas y escalas de DPI diferentes.

No se debe interpretar una compilación correcta ni una prueba sintética como demostración de estas condiciones.

La interfaz de Windhawk se abrió durante la comprobación, pero los controles no respondieron a la automatización. La versión 0.2 se instaló posteriormente mediante el script local elevado; el registro de este hecho no implica que la calidad de uso ya esté validada.
