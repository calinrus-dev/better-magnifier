# KWin-inspired Accessible Pinch Zoom

*An experimental accessibility mod for Windhawk: KWin-inspired three-finger pinch zoom powered by Windows Magnifier.*

Un proyecto de **accesibilidad para Windows**, inspirada en el efecto de zoom de **KWin / KDE Plasma**. Permite ampliar todo el escritorio con un **pellizco de tres dedos**, usando el motor de la Lupa de Windows. Abre los dedos para acercar y ciérralos para alejar. Puedes alternarlo con **Win + «+» / «−»**, también en el teclado numérico.

**0.4.0 experimental, comprobada en el touchpad del equipo de desarrollo.** Incorpora cambio lateral directo, zoom global y suspensión al 100 % en multitarea/escritorio. Corrige rutas que podían provocar parpadeos y recolocar el puntero en pellizcos lentos. El usuario confirmó el pellizco lento, la alternancia con teclado y los gestos de ventanas. La compatibilidad con otros equipos sigue pendiente.

Requiere Windows 11 x64, compilación **26200**, y Windhawk **1.7.3**. Los símbolos privados de Windows pueden cambiar con una actualización. El mod conserva la configuración de gestos del touchpad.

Pensada para leer texto pequeño, inspeccionar detalles y moverse por la pantalla con menos cambios entre touchpad y teclado. Proyecto independiente, sin afiliación con KDE o Microsoft.

## Comportamiento

- Distingue el pellizco del movimiento de los tres dedos juntos. Los gestos de dos y cuatro dedos conservan su comportamiento.
- Un deslizamiento horizontal de tres dedos cambia directamente de ventana, una por gesto, sin navegar por un selector: izquierda siguiente, derecha anterior. Es configurable. Filtra ventanas ocultas, auxiliares y cloaked, y conserva una secuencia para que los gestos sucesivos recorran más de dos ventanas.
- Arriba/abajo conservan la acción de Windows. El inicio nativo mantiene su momento original cuando la información de contactos está disponible; un pellizco confirmado después cancela la acción provisional una sola vez.
- El zoom es global: cambiar de ventana mantiene el nivel. **No hay perfiles de zoom por ventana.**
- Task View y Mostrar escritorio se ven al 100 %. Al salir se restaura el nivel y la posición global anteriores. Las dos vistas pueden solaparse sin sobrescribir la instantánea. La opción se puede desactivar.
- La interfaz nativa activa y cierra la ampliación; durante el pellizco, el controlador de pantalla completa recibe factores decimales continuos. Cada solicitud realiza una sola actualización y se descartan las basadas en un nivel anterior al teclado. Windows conserva las animaciones, los atajos y el seguimiento del ratón y del foco. El mod no bloquea las actualizaciones del motor nativo ni sustituye los atajos por una transformación propia.
- El seguimiento del puntero y del foco lo controla Windows. Para mantener la vista en el ratón, usa **Configuración > Accesibilidad > Lupa > Vista**: activa el seguimiento del puntero y elige mantenerlo centrado. Si no quieres que el teclado, el texto o Narrador desplacen la vista, ajusta sus casillas de seguimiento. El mod conserva esos ajustes nativos.
- La aceleración depende de la velocidad de separación de los dedos. Los movimientos lentos mantienen precisión; los rápidos reciben una ganancia adicional moderada. Los tiempos HID evitan confundir paquetes acumulados con un gesto rápido.
- La ventana, la entrada en la barra de tareas y los controles táctiles de la Lupa se ocultan por defecto. Se pueden mostrar desactivando **Hide Windows Magnifier window and touch controls**. La Lupa sigue ejecutándose como motor del zoom.
- **Win+Esc** conserva el cierre de Windows. El mod no vuelve a abrir la Lupa por su temporizador; un nuevo pellizco puede iniciarla otra vez. El arranque en frío necesita abrir `Magnify.exe`: se descarta el movimiento recibido durante esa espera para evitar un salto al activarse.
- No hay un límite propio de 400 %. Con `Maximum zoom = 0` se usa el rango nativo de esta Lupa, **100–1600 %**. Su motor tiene ese límite; el máximo teórico de la API independiente no amplía el rango del motor nativo.

## Ajustes

En la pestaña **Ajustes** del mod:

- **Zoom sensitivity:** 100 % por defecto, seguimiento proporcional a la separación.
- **Pinch acceleration:** 35 % por defecto; 0 desactiva la aceleración.
- **Reverse pinch direction:** invierte acercar y alejar.
- **Pinch activation threshold:** 3 % por defecto.
- **Maximum zoom:** 0 por defecto, sin un tope adicional al de Windows.
- **Keyboard zoom increment:** 0 mantiene el paso configurado en Windows. Un valor positivo lo personaliza.
- **Switch windows directly with a horizontal three-finger swipe:** activado por defecto; desactívalo para conservar el selector lateral de Windows.
- **Show Task View and desktop at 100 percent:** activado por defecto; conserva una única instantánea global para restaurarla al salir.
- **Keep the pointer stationary during a pinch:** activado por defecto. Omite los reajustes sintéticos del cursor que hace la Lupa durante el pellizco, también al mantener los dedos. No bloquea el movimiento real del ratón ni los atajos nativos.
- **Hide Windows Magnifier window and touch controls:** activado por defecto.
- **Observe only:** reconoce sin aplicar zoom.

El seguimiento del ratón, el teclado, el texto y Narrador se configura en la Lupa de Windows. Durante Task View o Mostrar escritorio el zoom permanece temporalmente suspendido; los comandos de ampliación se reanudan al salir. `Smoothing` solo afecta al modo independiente de la API; el modo nativo conserva sus animaciones. Desactivar `Share zoom with Windows Magnifier shortcuts` habilita el modo experimental de API independiente y pierde la integración nativa; no es la opción recomendada.

## Instalación

1. Descarga [`mods/kde-pinch-zoom.wh.cpp`](mods/kde-pinch-zoom.wh.cpp).
2. En Windhawk, usa **Crear un nuevo Mod**, sustituye el ejemplo por el archivo completo y compila.
3. Actívalo. Se carga en `explorer.exe` y `magnify.exe`. La primera resolución de símbolos puede tardar.
4. Si usabas **Magnifier Headless Mode**, desactívalo para evitar dos mods ocultando las mismas ventanas. La ocultación ya está integrada aquí.

El mod es local; no necesita publicarse en el catálogo de Windhawk. Al actualizar, sustituye el archivo completo para incluir los dos procesos de destino.

Después de compilar también puedes ejecutar `scripts/install-local.ps1` en PowerShell **como administrador**. Está limitado a la instalación estándar de Windhawk y a `local@kde-pinch-zoom`. Conserva los ajustes existentes y añade los nuevos valores predeterminados. Por la integración solicitada, desactiva únicamente el antiguo `magnifier-headless`, conservándolo instalado. Cierra la Lupa de la sesión actual, restaura el 100 % e instala una DLL con nombre basado en su hash. Ante un fallo recupera la configuración previa. No modifica las claves de gestos del touchpad ni otros mods.

`-Disable` desactiva este mod. `-Acceleration 35 -MaximumZoom 0` permite cambiar esos dos ajustes explícitamente. El resultado queda en `build/install-result.json`. El instalador comprueba `build/build-info.json` para evitar instalar una DLL que no corresponda al código.

## Recuperación y diagnóstico

- **Ctrl+Alt+Mayús+F10:** solicitar el 100 %.
- **Ctrl+Alt+Mayús+F9:** pausar/reanudar el pellizco, volviendo al 100 % al pulsarlo.
- **Win+Esc:** cerrar la Lupa.
- Al descargar el mod, la Lupa nativa puede continuar abierta; sus controles se vuelven visibles y los atajos siguen siendo de Windows.

`build/pinch-zoom-host.exe --status` debe mostrar `Native gate: READY` y `Windows Magnifier: READY`. `actualZoom` consulta el nivel renderizado, no el objetivo pendiente de la animación. `--reset` solicita el 100 %.

## Compilar y comprobar

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/check.ps1
```

Para Windhawk portátil añade `-WindhawkPath 'D:\Herramientas\Windhawk'`. Se generan la DLL, el host de diagnóstico y las pruebas en `build/`. El script no instala ni activa mods, no inicia Windhawk y no cambia el zoom.

La compilación ejecuta 52 pruebas del reconocedor, 21 del runtime/ABI, 44 del adaptador nativo y 18 del filtro de gestos del shell con backend simulado, sin modificar el escritorio. También compila una prueba de integración que **solo se ejecuta explícitamente**, con el mod instalado:

```powershell
.\build\native-integration.exe --live
```

Esta prueba cambia brevemente el zoom y restaura el nivel inicial. Comprueba el renderizado nativo, las inversiones rápidas, el incremento por IPC y la vuelta al pellizco. No simula teclas ni certifica por sí sola el comportamiento de un touchpad físico. No muevas el touchpad ni cambies el zoom durante ella.

El diagnóstico de solo lectura es `build/pinch-zoom-host.exe --diagnose --seconds 30`. Cuenta informes y gestos sin guardar coordenadas. No ejecutes otro lector/host mientras esté activo el mod.

### Rendimiento sin cambiar el zoom

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/measure-performance.ps1
```

Muestrea cinco segundos de CPU y memoria de Explorer y Magnify. Son cifras del proceso completo, con Windows, otros mods y la actividad del escritorio; no aíslan el coste del mod.

La opción `-Exercise` ejecuta una prueba **visible** que acerca y aleja repetidamente durante unos segundos y restaura el nivel inicial. No se ejecuta al compilar ni en CI. Puede interferir si usas el touchpad o los atajos al mismo tiempo; no la ejecutes durante tu trabajo. Su latencia mide entrega por IPC y está limitada por la temporización del sondeo; no mide movimiento físico a píxel.

## Compatibilidad y validación

Se requiere un Precision Touchpad que exponga sus contactos mediante HID y la Lupa en pantalla completa. Los modos lente y acoplado no están soportados. Faltan pruebas físicas con varios monitores, DPI mixtos, suspensión y aplicaciones elevadas. No funciona en el escritorio protegido de UAC.

La versión 0.1 presentaba conflictos con la multitarea. El usuario confirmó mejoría en 0.2 y, durante la corrección de 0.3, confirmó que podía alternar pellizco y teclado numérico. Eso no certifica todas las combinaciones de hardware o gestos. Consulta [`docs/VALIDATION.md`](docs/VALIDATION.md) para separar pruebas automáticas, comprobaciones locales y aspectos pendientes.

El código distribuible es un único archivo autocontenido. El lector HID y el filtro de gestos viven en Explorer; el auxiliar de Magnify actualiza el objetivo de zoom sobre el controlador de pantalla completa de Windows. Si falta el filtro nativo, no se aplica zoom. El protocolo compartido no almacena coordenadas de los dedos.

Cambios en [CHANGELOG.md](CHANGELOG.md). Puedes abrir issues o enviar mejoras siguiendo [CONTRIBUTING.md](CONTRIBUTING.md).

Licencia [GPL-3.0-or-later](LICENSE). Referencias en [`THIRD_PARTY.md`](THIRD_PARTY.md).
