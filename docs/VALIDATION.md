# Validación de 0.1.0

Fecha: 7 de noviembre de 2026.

## Verificado automáticamente

- Compilación x64 con Clang 20.1.3 incluido con Windhawk 1.7.3.
- Compilación con `-Wall -Wextra -Werror`.
- 31 pruebas del reconocedor, límites del zoom, rechazo de deslizamientos, cambios de contactos, datos inválidos, ruido, temporización y ensamblado de informes HID.
- Compilación del host independiente y de la DLL del mod.
- 12 comprobaciones del runtime: unidades y coordenadas HID, carga del PE sin ejecutar código y exportaciones ABI que utiliza Windhawk.
- Compilación adicional con el script de compatibilidad del repositorio oficial de mods, usando los metadatos y opciones del archivo distribuible.

## Verificado en la máquina de desarrollo

- Windows 11 25H2, compilación 26200.9711.
- La enumeración Raw Input identifica un descriptor HID de Precision Touchpad compatible.
- Una captura de 60 segundos detectó el dispositivo, pero recibió cero informes: no acredita reconocimiento físico de los gestos.
- La API de ampliación acepta 110 %, devuelve la ampliación aplicada y permite restaurar el 100 %.
- El script de compilación no modifica el registro de gestos. Los valores observados antes de las pruebas: deslizamientos de tres dedos habilitados y deslizamientos de cuatro dedos configurados.

## Pendiente de validación manual

- Reconocimiento con movimientos físicos del usuario, con recepción de informes HID durante la captura.
- Carga del adaptador de tool mod dentro de Windhawk y cambios de ajustes desde su interfaz.
- Que un pellizco no active también un deslizamiento nativo de Windows.
- Cambio de aplicaciones a izquierda/derecha, mostrar escritorio abajo y multitarea arriba con tres dedos, antes y después de usar zoom.
- Que el zoom de aplicaciones con dos dedos y los gestos de cuatro dedos conserven su comportamiento.
- Atajos de recuperación, pausar/reanudar y desactivación del mod mientras el escritorio está ampliado.
- Suspensión, bloqueo de sesión y retirada del dispositivo mientras hay ampliación.
- Aplicaciones elevadas, múltiples pantallas y escalas de DPI diferentes.

No se debe interpretar una compilación correcta ni una prueba sintética como demostración de estas condiciones.

La interfaz de Windhawk se abrió durante la comprobación, pero los controles no respondieron a la automatización. No se considera instalado ni activado el mod; hay instrucciones para importarlo manualmente.
