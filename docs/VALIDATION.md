# Validación

Fecha: 7 de noviembre de 2026.

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
