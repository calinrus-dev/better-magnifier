# KDE Pinch Zoom para Windhawk

Zoom continuo de **todo el escritorio** con un pellizco de **tres dedos**, inspirado en la lupa de KDE Plasma. Abre los dedos para acercar y ciérralos para alejar. No requiere mantener una tecla pulsada.

**Versión 0.1.0 experimental.** Compila con Windhawk 1.7.3 en Windows x64. Las pruebas automáticas validan el reconocimiento con movimientos sintéticos; la convivencia del pellizco con los deslizamientos nativos necesita validación con el touchpad físico. El mod no modifica la configuración de gestos de Windows ni bloquea sus eventos.

## Qué incluye

- Reconocimiento del pellizco de tres dedos separado del desplazamiento de los tres dedos juntos.
- Ampliación proporcional y continua mediante la API de ampliación de Windows; no simula atajos de zoom.
- Sensibilidad, umbral, límite de ampliación y suavizado configurables.
- Los movimientos identificados como deslizamientos se dejan a Windows. Los gestos de dos y cuatro dedos se ignoran.
- Lectura HID en un proceso dedicado de Windhawk, sin inyectarse en todos los programas.
- Rechazo de contactos de palma, identificadores duplicados, datos inválidos, cambios de dedos y saltos de sensor.
- Ensamblado de informes HID paralelos e híbridos, independiente para cada dispositivo.
- Restauración de la vista al descargar el mod, bloquear la sesión o suspender el equipo, si la ampliación sigue bajo su control.
- Sin red, telemetría ni persistencia de coordenadas del touchpad.

## Instalar el mod

1. Descarga [`mods/kde-pinch-zoom.wh.cpp`](mods/kde-pinch-zoom.wh.cpp).
2. En Windhawk, pulsa **Crear un nuevo Mod**.
3. Sustituye el ejemplo completo por el contenido del archivo y guarda con `Ctrl+S`.
4. Pulsa **Compilar Mod** y activa el mod.
5. Para comprobar primero la clasificación sin ampliar la pantalla, activa **Observe only** en sus ajustes y **Log gesture decisions**. Revisa el registro de depuración de Windhawk mientras haces pellizcos y deslizamientos.

El mod es local; no es necesario publicarlo en el catálogo de Windhawk. La DLL del directorio de compilación sirve para comprobaciones y no debe copiarse a mano en el motor de Windhawk.

### Controles

- **Pellizco de tres dedos:** acercar o alejar.
- **Ctrl+Alt+Mayús+F10:** volver a la vista original, normalmente 100 %. Hay que levantar los dedos antes de volver a ampliar.
- **Ctrl+Alt+Mayús+F9:** pausar o reanudar; al pausar se restaura la vista original.
- **Desactivar el mod en Windhawk:** detenerlo y restaurar la vista si todavía la controla.

El arranque se cancela si otra aplicación ya usa el atajo de recuperación F10. La combinación F9 es opcional. La ampliación mínima es 100 % y la máxima predeterminada es 400 %.

## Compilar y verificar

Requisitos: Windows x64, Windhawk **1.7.3** con su compilador y PowerShell 5.1 o posterior. No se necesitan dependencias de C++ adicionales.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/check.ps1
```

Para una instalación portátil, añade `-WindhawkPath 'D:\Herramientas\Windhawk'`.

Resultados en `build/`:

- `core-tests.exe`: pruebas del reconocedor, ensamblado HID, zoom y suavizado.
- `pinch-zoom-host.exe`: host independiente para diagnosticar el mismo código del mod.
- `kde-pinch-zoom.dll`: compilación x64 para Windhawk.

Los ejecutables del host y las pruebas enlazan estáticamente el runtime de C++; no necesitan copiar DLL del compilador. El script no instala ni activa mods, y no inicia otra instancia de Windhawk.

### Comprobar el dispositivo sin cambiar el zoom

```powershell
.\build\pinch-zoom-host.exe --diagnose --seconds 30
```

Haz pellizcos y los cuatro deslizamientos mientras corre. La salida cuenta dispositivos, informes, fotogramas, pellizcos y deslizamientos. Un dispositivo detectado con cero informes **no** demuestra que los gestos funcionen: hace falta mover los dedos durante la captura y comprobar que esta sesión recibe sus eventos. No ejecutes el host y el mod al mismo tiempo.

### Probar la API de ampliación

```powershell
.\build\pinch-zoom-host.exe --smoke-zoom
```

Esta prueba aplica brevemente el 110 %, consulta el resultado real y restaura el 100 %. No se ejecuta en CI ni en la compilación predeterminada. También se puede invocar con `scripts/check.ps1 -SmokeZoom`.

### Probar el zoom fuera de Windhawk

```powershell
.\build\pinch-zoom-host.exe --run --seconds 60
```

Usa los mismos atajos de recuperación que el mod. Al acabar el plazo restaura la vista. Si se omite `--seconds`, se mantiene activo hasta recibir un cierre normal. Evita terminar el proceso por la fuerza mientras la pantalla esté ampliada: un cierre forzado impide ejecutar la restauración.

## Limitaciones que deben comprobarse

1. **Convivencia con los deslizamientos.** El reconocedor deja los desplazamientos a Windows, pero no tiene autoridad para cancelar una acción que Windows ya haya iniciado. Un pellizco asimétrico podría activar también un gesto nativo. Esta versión no incluye una interceptación selectiva del shell. Si sucede, aumenta el umbral y registra el caso; no se garantiza una solución solo ajustando sensibilidad.
2. **Otras lupas.** Cierra la Lupa de Windows y otros programas de ampliación antes de probarlo, incluido un proceso `magnify.exe` cuya ventana haya sido ocultada por otro mod. Si la pantalla ya está ampliada por otro programa, este mod no toma el control. Si detecta una transformación ajena, deja de intervenir.
3. **Ventanas elevadas y escritorios protegidos.** La entrega de informes HID en segundo plano puede variar con el nivel de integridad. No se implementan auxiliares en cada aplicación ni cambios de seguridad. Comprueba los programas que ejecutas como administrador. No funciona en el escritorio de UAC o de inicio de sesión.
4. **Hardware y monitores.** Requiere un Precision Touchpad que exponga número de contactos, identificadores, coordenadas y contacto de punta por HID. Los dispositivos antiguos que solo emulan un ratón no sirven. Se contemplan unidades físicas y coordenadas virtuales negativas; faltan pruebas físicas con monitores de distintas escalas y configuraciones.
5. **Cierres inesperados.** La descarga normal restaura la vista; un cierre forzado del proceso, fallo del controlador o apagado abrupto no permiten garantizar la limpieza. El mod no añade un servicio de vigilancia permanente.
6. **Compatibilidad.** Esta versión apunta a Windows x64 y Windhawk 1.7.3. ARM64, x86 y Windhawk 2 se consideran pendientes hasta verificarlos.

## Evidencia de validación

Consulta [`docs/VALIDATION.md`](docs/VALIDATION.md) para distinguir las comprobaciones automáticas, las realizadas en esta máquina y las pruebas manuales pendientes. GitHub Actions compila el mod y ejecuta las pruebas puras en Windows; no certifica gestos físicos.

## Cómo está construido

El archivo del mod es autocontenido. Su primera parte implementa el reconocedor y el ensamblador de fotogramas sin dependencias de Windows. La segunda lee informes HID con `RegisterRawInputDevices`, aplica ampliación con `MagSetFullscreenTransform` y gestiona la recuperación. La última adapta ese host al patrón de *tool mod* de Windhawk.

Se mide tanto el desplazamiento del centro de los tres contactos como el cambio de su separación. Un desplazamiento dominante se clasifica como deslizamiento y nunca se convierte en zoom durante ese gesto. Un cambio de separación dominante activa el pellizco y actualiza un factor continuo. La clasificación no reemplaza al reconocedor de Windows: ambos reciben el movimiento.

Licencia [GPL-3.0-or-later](LICENSE). Fuentes de referencia y atribuciones en [`THIRD_PARTY.md`](THIRD_PARTY.md).
