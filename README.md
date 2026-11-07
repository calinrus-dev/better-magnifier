# KDE Pinch Zoom para Windhawk

Zoom continuo de **todo el escritorio** con un pellizco de **tres dedos**, inspirado en la lupa de KDE Plasma. Abre los dedos para acercar y ciérralos para alejar. No requiere mantener una tecla pulsada.

**Versión 0.2.0 experimental.** Requiere Windhawk 1.7.3, Windows x64 y **compilación 26200 de Windows 11**. Añade una interceptación en Explorer que mantiene en espera el gesto de multitarea hasta distinguir pellizco y deslizamiento. Un pellizco suprime esa acción; un deslizamiento continúa mediante el controlador original de Windows. La interceptación aún necesita validación física. El mod no modifica la configuración de gestos de Windows.

La versión 0.1 reconocía pellizcos, pero su prueba física mostró retraso y conflictos con la multitarea. No se considera una solución funcional para el objetivo de este proyecto.

## Qué incluye

- Reconocimiento del pellizco de tres dedos separado del desplazamiento de los tres dedos juntos.
- Ampliación proporcional y continua mediante la API de ampliación de Windows; no simula atajos de zoom.
- Sensibilidad, umbral, límite de ampliación y suavizado configurables.
- Los movimientos identificados como deslizamientos se dejan a Windows. Los gestos de dos y cuatro dedos se ignoran.
- Lectura HID en un hilo propio y filtrado de gestos en Explorer. No se inyecta en todos los programas. El arranque se cancela si otro componente de Explorer ya ha registrado la misma entrada HID.
- Punto de ampliación fijo durante cada pellizco, sin salto al superar el umbral y seguimiento directo de cada fotograma de entrada. Sin suavizado artificial predeterminado.
- Rechazo de contactos de palma, identificadores duplicados, datos inválidos, cambios de dedos y saltos de sensor.
- Ensamblado de informes HID paralelos e híbridos, independiente para cada dispositivo.
- Restauración de la vista al descargar el mod, bloquear la sesión o suspender el equipo, si la ampliación sigue bajo su control.
- Sin red, telemetría ni persistencia de coordenadas del touchpad.

## Instalar el mod

1. Descarga [`mods/kde-pinch-zoom.wh.cpp`](mods/kde-pinch-zoom.wh.cpp).
2. En Windhawk, pulsa **Crear un nuevo Mod**.
3. Sustituye el ejemplo completo por el contenido del archivo y guarda con `Ctrl+S`.
4. Pulsa **Compilar Mod** y activa el mod.
   La versión 0.2 se carga en `explorer.exe`. La resolución inicial de símbolos del shell puede tardar. El zoom permanece desactivado mientras no esté listo el filtro nativo.
5. Para comprobar primero la clasificación sin ampliar la pantalla, activa **Observe only** en sus ajustes y **Log gesture decisions**. Revisa el registro de depuración de Windhawk mientras haces pellizcos y deslizamientos.

El mod es local; no es necesario publicarlo en el catálogo de Windhawk. La DLL del directorio de compilación sirve para comprobaciones y no debe copiarse a mano en el motor de Windhawk. Al actualizar una versión anterior, sustituye el archivo completo: también han cambiado los procesos de destino.

### Instalación local por script

Después de compilar, se puede ejecutar `scripts/install-local.ps1` desde PowerShell **como administrador**. Está limitado a la instalación estándar de Windhawk en `%ProgramData%\Windhawk` y a la clave `local@kde-pinch-zoom`. Copia el código y la DLL, establece los ajustes iniciales, cierra `Magnify.exe` de la sesión actual y restaura el 100 % antes de activar el mod. No modifica las claves de gestos del touchpad ni otros mods. `scripts/install-local.ps1 -Disable` desactiva solo este mod. Escribe el resultado en `build/install-result.json`.

Puedes consultar su activación sin abrir otra captura con `build/pinch-zoom-host.exe --status`. Deben aparecer `Native gate: READY` y `hostEnabled=1`. `--reset` restaura explícitamente la ampliación del escritorio al 100 %.

### Controles

- **Pellizco de tres dedos:** acercar o alejar.
- **Ctrl+Alt+Mayús+F10:** volver a la vista original, normalmente 100 %. Hay que levantar los dedos antes de volver a ampliar.
- **Ctrl+Alt+Mayús+F9:** pausar o reanudar; al pausar se restaura la vista original.
- **Desactivar el mod en Windhawk:** detenerlo y restaurar la vista si todavía la controla.

El arranque se cancela si otra aplicación ya usa el atajo de recuperación F10. La combinación F9 es opcional. La ampliación mínima es 100 % y la máxima predeterminada es 400 %.

## Compilar y verificar

Requisitos de compilación: Windows x64, Windhawk **1.7.3** con su compilador y PowerShell 5.1 o posterior. La activación del auxiliar nativo exige Windows 11 compilación 26200. No se necesitan dependencias de C++ adicionales.

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

Usa los mismos atajos de recuperación que el mod. **El auxiliar nativo de la versión 0.2 debe estar cargado en Explorer; el host solo no amplía la pantalla.** No ejecutes dos hosts a la vez. Al acabar el plazo restaura la vista. Si se omite `--seconds`, se mantiene activo hasta recibir un cierre normal. Evita terminar el proceso por la fuerza mientras la pantalla esté ampliada: un cierre forzado impide ejecutar la restauración.

## Limitaciones que deben comprobarse

1. **Interceptación experimental.** La versión 0.2 intercepta `StartSwipe`, `ContinueSwipe`, `FinishSwipe` y `CancelSwipe` de `MultitaskingViewGestureHandler` en `twinui.pcshell.dll`, usando símbolos públicos. Se ha comprobado su firma, el contexto de 8 bytes, la carga en Explorer y la llegada de un gesto nativo de tres contactos. Los gestos pendientes se retienen; los deslizamientos reconocidos se reenvían al controlador original. No se sustituyen por atajos. Falta confirmar la supresión del conflicto durante pellizcos físicos y su calidad de seguimiento. No se activa en otras compilaciones de Windows.
2. **Otras lupas.** Cierra la Lupa de Windows y otros programas de ampliación antes de probarlo, incluido un proceso `magnify.exe` cuya ventana haya sido ocultada por otro mod. Si la pantalla ya está ampliada por otro programa, este mod no toma el control. Si detecta una transformación ajena, deja de intervenir.
3. **Ventanas elevadas y escritorios protegidos.** La entrega de informes HID en segundo plano puede variar con el nivel de integridad. No se implementan auxiliares en cada aplicación ni cambios de seguridad. Comprueba los programas que ejecutas como administrador. No funciona en el escritorio de UAC o de inicio de sesión.
4. **Hardware y monitores.** Requiere un Precision Touchpad que exponga número de contactos, identificadores, coordenadas y contacto de punta por HID. Los dispositivos antiguos que solo emulan un ratón no sirven. Se contemplan unidades físicas y coordenadas virtuales negativas; faltan pruebas físicas con monitores de distintas escalas y configuraciones.
5. **Cierres inesperados.** La descarga normal restaura la vista; un cierre forzado del proceso, fallo del controlador o apagado abrupto no permiten garantizar la limpieza. El mod no añade un servicio de vigilancia permanente.
6. **Compatibilidad.** Esta versión apunta a Windows 11 x64 compilación 26200 y Windhawk 1.7.3. Otras compilaciones, ARM64, x86 y Windhawk 2 se consideran pendientes hasta verificarlos.

## Evidencia de validación

Consulta [`docs/VALIDATION.md`](docs/VALIDATION.md) para distinguir las comprobaciones automáticas, las realizadas en esta máquina y las pruebas manuales pendientes. GitHub Actions compila el mod y ejecuta las pruebas puras en Windows; no certifica gestos físicos.

## Cómo está construido

El archivo del mod es autocontenido. Su primera parte implementa el reconocedor, el ensamblador de fotogramas y el estado de clasificación sin dependencias de Windows. La segunda lee informes HID con `RegisterRawInputDevices`, aplica ampliación con `MagSetFullscreenTransform` y gestiona la recuperación. El filtro de Explorer consulta la clasificación en una memoria compartida sin coordenadas y conserva los callbacks originales para los deslizamientos. El controlador inicia y detiene el hilo de entrada con la carga y descarga del mod.

Se mide tanto el desplazamiento del centro de los tres contactos como el cambio de su separación. Un desplazamiento dominante se clasifica como deslizamiento y nunca se convierte en zoom durante ese gesto. Un cambio de separación dominante activa el pellizco y actualiza un factor continuo. Una vez decidido, el gesto no cambia de categoría. Si el host deja de enviar su señal de vida, los gestos nuevos vuelven al controlador nativo.

Licencia [GPL-3.0-or-later](LICENSE). Fuentes de referencia y atribuciones en [`THIRD_PARTY.md`](THIRD_PARTY.md).
