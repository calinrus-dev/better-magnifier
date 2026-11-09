# Better Magnifier

*An experimental accessibility mod for Windhawk: smooth pinch, keyboard and Win+mouse-wheel zoom, inspired by KWin and powered by Windows Magnifier.*

Un proyecto de **accesibilidad para Windows**, inspirado en el efecto de zoom de **KWin / KDE Plasma**. Permite ampliar todo el escritorio con un **pellizco de tres dedos**, usando el motor de la Lupa de Windows. Abre los dedos para acercar y ciérralos para alejar. Puedes alternarlo con **Win + «+» / «−»**, también en el teclado numérico.

**0.7.5 experimental.** Perfil predeterminado: **Win+rueda** para ampliar, **Win+clic central** para volver al 100 % y **Ctrl+Alt+rueda** para recorrer ventanas. El teclado mantenido acelera de forma continua y se detiene en el nivel visible al soltar. Se revisan el arranque al 100 %, el escritorio y la alternancia de entradas. La 0.7.5 corrige la pérdida del controlador al abrir una instancia temporal de la Lupa y el menú Inicio al soltar Win tras usar el ratón. La validación física se registra por separado en `docs/VALIDATION.md`.

Las funciones avanzadas están validadas en Windows 11 x64, compilación **26200**, con Windhawk **1.7.3**. Una versión o interfaz no verificada conserva los controles estándar de la Lupa; no implica soporte del zoom continuo. El mod conserva la configuración de gestos del touchpad.

Pensada para leer texto pequeño, inspeccionar detalles y moverse por la pantalla con menos cambios entre touchpad y teclado. Proyecto independiente, sin afiliación con KDE o Microsoft.

## Comportamiento

- **Win+rueda** amplía o reduce de forma continua. Cada muesca aplica un cambio proporcional (50 % por defecto); una rueda de alta resolución conserva los deltas pequeños. Los giros rápidos se detectan por volumen reciente, conservan como máximo 40 ms de recorrido pendiente y usan un límite de velocidad separado. El exceso se descarta para evitar una cola larga tras frenar; no se extrapola sin nuevos eventos. Cambiar de sentido cancela el impulso anterior. Una trayectoria nueva aprovecha hasta 16 ms del tiempo real de captura, evitando un frame inicial vacío; no simula Win+«+» ni añade un aumento mínimo artificial. La rueda sin esa combinación conserva su función normal.

- **Win+clic central** vuelve rápidamente al 100 %. **Ctrl+Alt+rueda** selecciona la ventana anterior al girar hacia arriba y la siguiente al girar hacia abajo, sin desplegar un selector. Conserva cada muesca de una ráfaga, confirma la ventana activada antes de avanzar y descarta el recorrido pendiente al invertir. La cola está limitada a seis pasos y 200 ms para una rueda libre.
- **Ctrl+Alt+clic central** maximiza/restaura y **Ctrl+Alt+clic derecho** minimiza la ventana actual únicamente si activas **Extra Ctrl+Alt mouse window actions**. Esas acciones adicionales están desactivadas por defecto.
- **Win+«+»/«−»**, también en el numérico: el primer toque mantiene el paso nativo; tras 200 ms, la pulsación mantenida pasa a una trayectoria continua cuya velocidad aumenta gradualmente. Al soltar fija el nivel y la posición visibles, sin completar un objetivo lejano. Invertir el sentido parte del nivel visible.
- Distingue el pellizco del movimiento de los tres dedos juntos. Los gestos de dos y cuatro dedos conservan su comportamiento.
- Un deslizamiento horizontal de tres dedos cambia directamente de ventana, una por gesto, sin navegar por un selector: izquierda siguiente, derecha anterior. Es configurable. Filtra ventanas ocultas, auxiliares y cloaked, y conserva una secuencia para que los gestos sucesivos recorran más de dos ventanas.
- Arriba/abajo conservan la acción de Windows. El inicio nativo mantiene su momento original cuando la información de contactos está disponible; un pellizco confirmado después cancela la acción provisional una sola vez.
- El zoom es global: cambiar de ventana mantiene el nivel. **No hay perfiles de zoom por ventana.**
- Task View se ve al 100 %. Al salir recupera el nivel y la posición global anteriores. Una entrada nueva y explícita de zoom recupera ese nivel y continúa desde él. **El escritorio sigue siendo ampliable**, incluso después de Win+D. La opción se puede desactivar.
- La interfaz nativa activa y cierra la ampliación; durante el pellizco, el controlador de pantalla completa recibe factores decimales continuos. Cada solicitud realiza una sola actualización y se descartan las basadas en un nivel anterior al teclado. Windows conserva las animaciones, los atajos y el seguimiento del ratón y del foco. Mientras hay un objetivo gestionado vigente, se filtran los setters de zoom atrasados que intentarían deshacerlo. Los comandos nativos explícitos recuperan el control; el renderizado y el seguimiento siguen siendo de Windows.
- El seguimiento del puntero y del foco lo controla Windows. Para mantener la vista en el ratón, usa **Configuración > Accesibilidad > Lupa > Vista**: activa el seguimiento del puntero y elige mantenerlo centrado. Si no quieres que el teclado, el texto o Narrador desplacen la vista, ajusta sus casillas de seguimiento. El mod conserva esos ajustes nativos.
- La aceleración depende de la velocidad de separación de los dedos. Los movimientos lentos mantienen precisión; los rápidos reciben una ganancia adicional moderada. Los tiempos HID evitan confundir paquetes acumulados con un gesto rápido.
- La ventana, la entrada en la barra de tareas y los controles táctiles de la Lupa se ocultan por defecto. Se pueden mostrar desactivando **Hide Windows Magnifier window and touch controls**. La Lupa sigue ejecutándose como motor del zoom.
- **Win+Esc** conserva el cierre de Windows. El mod no vuelve a abrir la Lupa por su temporizador; un nuevo pellizco puede iniciarla otra vez. El arranque en frío necesita abrir `Magnify.exe`. La rueda conserva como máximo una muesca durante 1,5 segundos; un reset, otro gesto o un cambio de sesión la descartan. No se reproduce una cola de giro libre al terminar de cargar. El pellizco conserva como máximo un primer aumento del 3 % durante 1,5 segundos, sólo mientras sigan los tres dedos en ese gesto; el resto del recorrido de espera se descarta.
- No hay un límite propio de 400 %. Con `Maximum zoom = 0` se usa el rango nativo de esta Lupa, **100–1600 %**. Su motor tiene ese límite; el máximo teórico de la API independiente no amplía el rango del motor nativo.

## Ajustes

En la pestaña **Ajustes** del mod:

- **Use Win + mouse wheel for zoom:** activado por defecto. Desactívalo para recuperar el perfil **Ctrl+Alt+rueda** y **Ctrl+Alt+clic central**; el cambio de ventanas con la rueda deja de interceptarse en ese perfil.
- **Smooth mouse wheel zoom:** activado por defecto. Desactiva el zoom continuo por rueda; los atajos de teclado nativos permanecen disponibles.
- **Ctrl+Alt + mouse wheel switches windows:** activado por defecto en el perfil Win.
- **Extra Ctrl+Alt mouse window actions:** desactivado por defecto.
- **Disable Magnifier click-to-read and its reading cursor:** activado por defecto en el binario verificado. Evita nuevas activaciones de Ctrl+Alt+clic izquierdo y de su cursor de lectura. No desactiva Narrador ni otros atajos de lectura de la Lupa. Si el modo de lectura ya estaba activo al cambiar el ajuste, sal de él o vuelve a abrir la Lupa.
- **Wheel zoom per notch:** 50 % por defecto, ajustable entre 1 y 100 %.
- **Wheel transition time:** 100 ms por defecto, ajustable entre 40 y 250 ms.
- **Maximum normal wheel zoom growth per second:** 400 % de crecimiento por segundo por defecto (hasta multiplicar por cinco en un segundo), para movimientos normales y de precisión.
- **Maximum free-spinning wheel zoom growth per second:** 1000 % por defecto (hasta multiplicar por once en un segundo) para ráfagas rápidas, con transición y recorrido pendiente de 40 ms. Usa el mayor de este límite y el normal; `0` desactiva la adaptación. Se detecta por unidades de rueda, no por número de eventos: subdividir una muesca en deltas pequeños no aumenta su ganancia. Este tiempo corresponde a la trayectoria del mod, no a una garantía de latencia física de Windows o de la pantalla.
- **Suppress Magnifier startup feedback cursor:** activado por defecto. Pide a Windows que omita el indicador azul al abrir la Lupa desde el mod; es una petición opcional que no impide abrirla si Windows la rechaza.

- **Zoom sensitivity:** 100 % por defecto, seguimiento proporcional a la separación.
- **Pinch acceleration:** 35 % por defecto; 0 desactiva la aceleración.
- **Reverse pinch direction:** invierte acercar y alejar.
- **Pinch activation threshold:** 3 % por defecto.
- **Maximum zoom:** 0 por defecto, sin un tope adicional al de Windows.
- **Progressive keyboard hold speed:** 2 por defecto, entre 1 y 4. El valor 1 mantiene las repeticiones estándar; 2–4 regula la velocidad máxima de la rampa continua. La suelta se detecta en el siguiente turno UI, normalmente un fotograma.
- **Keyboard zoom increment:** 0 mantiene el paso configurado en Windows. Un valor positivo lo personaliza.
- **Switch windows directly with a horizontal three-finger swipe:** activado por defecto; desactívalo para conservar el selector lateral de Windows.
- **Show Task View at 100 percent:** activado por defecto; conserva una única instantánea global para restaurarla al salir.
- **Keep the pointer stationary during a pinch:** activado por defecto. Omite los reajustes sintéticos del cursor que hace la Lupa durante el pellizco, también al mantener los dedos. No bloquea el movimiento real del ratón ni los atajos nativos.
- **Hide Windows Magnifier window and touch controls:** activado por defecto.
- **Observe only:** reconoce sin aplicar zoom.

El seguimiento del ratón, el teclado, el texto y Narrador se configura en la Lupa de Windows. Durante Task View el zoom se suspende temporalmente. Una entrada explícita nueva de teclado, rueda o pellizco recupera la vista anterior y tiene prioridad sobre esa suspensión. El escritorio no se suspende. **Native Windows Magnifier integration** permite combinar pellizco, rueda y atajos. Al desactivarla se conserva el comportamiento estándar de Windows.

## Instalación

Descargas de esta versión: [Better Magnifier 0.7.5](https://github.com/calinrus-dev/better-magnifier/releases/tag/v0.7.5). Incluye el archivo de Windhawk, el código fuente, un paquete x64 compilado y `SHA256SUMS.txt` para comprobar las descargas.

1. Descarga [`mods/kde-pinch-zoom.wh.cpp`](mods/kde-pinch-zoom.wh.cpp).
2. En Windhawk, usa **Crear un nuevo Mod**, sustituye el ejemplo por el archivo completo y compila.
3. Actívalo. Se carga en `explorer.exe` y `magnify.exe`. La primera resolución de símbolos puede tardar.
4. Si usabas **Magnifier Headless Mode**, desactívalo para evitar dos mods ocultando las mismas ventanas. La ocultación ya está integrada aquí.

El mod es local; no necesita publicarse en el catálogo de Windhawk. Al actualizar, sustituye el archivo completo para incluir los dos procesos de destino.

Después de compilar también puedes ejecutar `scripts/install-local.ps1` en PowerShell **como administrador**. Está limitado a la instalación estándar de Windhawk y a `local@kde-pinch-zoom`. Conserva los ajustes existentes y añade los nuevos valores predeterminados. Por la integración solicitada, desactiva únicamente el antiguo `magnifier-headless`, conservándolo instalado. Cierra la Lupa de la sesión actual, restaura el 100 % e instala una DLL con nombre basado en su hash. Ante un fallo recupera la configuración previa. No modifica las claves de gestos del touchpad ni otros mods.

Con el paquete `better-magnifier-0.7.5-windows-x64.zip`, extráelo completo y ejecuta ese script desde su carpeta en una consola de administrador; los binarios ya están compilados. Conserva las carpetas `build`, `mods` y `scripts` juntas. Para actualizar desde una versión anterior, usa el mismo mod local: el identificador sigue siendo `local@kde-pinch-zoom`, aunque el nombre visible sea Better Magnifier. Las revisiones intermedias 0.7.1–0.7.4 no se publicaron como releases.

`-Disable` desactiva este mod. `-WheelStep 50 -WheelFreeSpinMaxRate 1000` actualiza explícitamente la ganancia y el límite rápido de una instalación existente; sus otros ajustes se conservan. `-Acceleration 35 -MaximumZoom 0` permite cambiar esos dos ajustes explícitamente. El resultado queda en `build/install-result.json`. El instalador comprueba `build/build-info.json` para evitar instalar una DLL que no corresponda al código.

### Inicio de Windows

El mod se carga en Explorer mediante Windhawk. Mantén Windhawk habilitado al inicio; en la instalación estándar su servicio debe estar en inicio automático. No hace falta abrir la interfaz del mod en cada sesión. Al arrancar se prepara la Lupa oculta al 100 %. Una carga temprana de Explorer solicita el componente del shell desde System32 si todavía no estaba cargado.

El controlador nativo sólo publica su estado tras comprobar que posee la interfaz real de la Lupa. Una instancia de paso no puede cambiar su PID, vaciar solicitudes ni desconectarlo al salir. Win+Esc cierra la Lupa y no se reabre por un temporizador. Un pellizco nuevo, Win+rueda o Win+«+» puede abrirla de nuevo. La rueda conserva una intención pequeña y acotada durante el arranque; el primer aumento desde una UI lista al 100 % no espera a que exista un renderer ampliado. La primera resolución de símbolos puede tardar.

Bloqueo, suspensión y desconexión invalidan la entrada pendiente. Al volver, el observador resincroniza las teclas físicas para evitar estados antiguos de Win+L. Al reanudar se comprueba que la sesión esté activa y desbloqueada. Los intentos de apertura tienen un intervalo mínimo y un límite; una entrada física nueva permite recuperarse de un fallo temporal. El instalador conserva el identificador `local@kde-pinch-zoom` y el archivo de fuente para actualizar las instalaciones anteriores de Better Magnifier.

## Recuperación y diagnóstico

- **Ctrl+Alt+Mayús+F10:** solicitar el 100 %.
- **Ctrl+Alt+Mayús+F9:** pausar/reanudar el pellizco y la rueda, volviendo al 100 % al pulsarlo.
- **Win+Esc:** cerrar la Lupa.
- Al descargar el mod, la Lupa nativa puede continuar abierta; sus controles se vuelven visibles y los atajos siguen siendo de Windows.

`build/pinch-zoom-host.exe --status` debe mostrar `Native gate: READY` y `Windows Magnifier: READY`. `actualZoom` consulta el nivel renderizado, no el objetivo pendiente de la animación. `--reset` solicita el 100 %.

## Compatibilidad tras actualizaciones

Para los binarios verificados incluidos, el mod resuelve localmente las direcciones mediante su GUID/edad CodeView, tamaño de imagen y timestamp PE, y comprueba las secciones de destino. No necesita descargar sus símbolos. Un cambio de identidad invalida el perfil completo: nunca reutiliza direcciones de otra actualización. Los perfiles actuales corresponden a los binarios comprobados de Windows 11 26200.9711.

En una compilación admitida sin perfil, intenta resolver símbolos Microsoft con Windhawk y valida cada dirección. Las capacidades se habilitan por grupos completos: núcleo, rueda, gestos del shell, señales de vista y restauración espacial. Un grupo opcional ausente desactiva su función. Si falta el gate del shell, la rueda puede continuar aunque se desactive el pellizco. Si falta el núcleo, conserva la Lupa estándar visible y sus atajos; no sustituye el motor por una transformación propia ni reintenta abrirla en bucle.

Esto reduce dependencia de descargas y evita usar direcciones antiguas. Las interfaces privadas siguen pudiendo cambiar; no se promete compatibilidad universal con una actualización futura. `--status` muestra capacidades, motivos y resolutores (`1=perfil offline verificado`, `2=símbolos`).

## Compilar y comprobar

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/check.ps1
```

Para Windhawk portátil añade `-WindhawkPath 'D:\Herramientas\Windhawk'`. Se generan la DLL, el host de diagnóstico y las pruebas en `build/`. El script no instala ni activa mods, no inicia Windhawk y no cambia el zoom.

La compilación ejecuta **705 comprobaciones** del reconocedor, perfiles de ratón, runtime/ABI, adaptador nativo, filtro del shell, lanzamiento y teclado progresivo con backend simulado, sin modificar el escritorio. También compila una prueba de integración que **solo se ejecuta explícitamente**, con el mod instalado:

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

Se requiere un Precision Touchpad que exponga sus contactos mediante HID y la Lupa en pantalla completa. Los modos lente y acoplado no están soportados. Faltan pruebas físicas con varios monitores, DPI mixtos, suspensión y aplicaciones elevadas. No funciona en el escritorio protegido de UAC. El perfil Win usa un observador de teclado que siempre deja pasar los eventos. Tras una acción de ratón consumida envía una pareja neutra VK_E8 y conserva una marca hasta soltar Win; la liberación se enmascara desde el hook para cubrir la repetición de la tecla. Win por sí solo conserva Inicio; no sintetiza Win, Ctrl ni Alt. Windows puede rechazar esa máscara ante una aplicación elevada por UIPI: entonces el evento se deja pasar. El perfil Ctrl+Alt es la alternativa configurable. [SendInput/UIPI](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-sendinput), [patrón de máscara de menú](https://github.com/AutoHotkey/AutoHotkeyDocs/blob/v2/docs/lib/A_MenuMaskKey.htm).

La versión 0.1 presentaba conflictos con la multitarea. El usuario confirmó mejoría en 0.2 y, durante la corrección de 0.3, confirmó que podía alternar pellizco y teclado numérico. Eso no certifica todas las combinaciones de hardware o gestos. Consulta [`docs/VALIDATION.md`](docs/VALIDATION.md) para separar pruebas automáticas, comprobaciones locales y aspectos pendientes.

El código distribuible es un único archivo autocontenido. El lector HID y el filtro de gestos viven en Explorer; el auxiliar de Magnify actualiza el objetivo de zoom sobre el controlador de pantalla completa de Windows. Si falta el filtro nativo, se desactiva el pellizco; la rueda conserva su capacidad independiente cuando el núcleo está disponible. El protocolo compartido no almacena coordenadas de los dedos.

Cambios en [CHANGELOG.md](CHANGELOG.md). Puedes abrir issues o enviar mejoras siguiendo [CONTRIBUTING.md](CONTRIBUTING.md).

Licencia [GPL-3.0-or-later](LICENSE). Referencias en [`THIRD_PARTY.md`](THIRD_PARTY.md).
