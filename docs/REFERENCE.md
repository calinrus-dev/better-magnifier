# Referencia de Better Visual

[Inicio](../README.md) · [Descargas](https://github.com/calinrus-dev/better-magnifier/releases/tag/v1.2.0) · [Validación](VALIDATION.md)

## Instalación local y actualización

El paquete compilado utiliza la instalación estándar de Windhawk x64. Conserva juntas las carpetas `build`, `mods` y `scripts`. El instalador verifica la fuente y la DLL contra `build/build-info.json` y registra el resultado en `build/install-result.json`.

- **Actualización.** Windhawk descarga la DLL anterior antes de cambiar a la nueva. Si la DLL sigue cargada, se restaura la configuración previa. Si Windhawk se queda en **Cerrando…**, prueba con la Lupa cerrada mediante **Win + Esc**.
- **Instalación previa de Magnifier Headless Mode.** El instalador desactiva `magnifier-headless`, conservándolo instalado. Si compilas en Windhawk, desactiva ese mod antes de activar Better Visual para evitar dos controles de ocultación.
- **Desactivar.** Usa **Desactivar** en Windhawk o ejecuta el instalador con `-Disable`. Si la Lupa sigue abierta al descargar el mod, recupera sus controles.
- **Ajustes desde consola.** `-WheelStep 20` aplica el paso del 20 % en una instalación existente. `-Acceleration 35 -MaximumZoom 0` cambia la aceleración del pellizco y usa el límite nativo.

Puedes elegir los componentes desde PowerShell. Por ejemplo, para usar pellizco y teclado con un marco azul:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/install-local.ps1 -Core On -Touchpad On -Mouse Off -DarkMode Off -BorderColor Blue
```

`-Core`, `-Touchpad`, `-Mouse`, `-MouseChord`, `-DarkMode` y `-Border` aceptan `On` y `Off`. `-BorderColor` acepta `Red`, `Blue` y `Yellow`. Los ajustes omitidos conservan el valor anterior o toman el predeterminado en una instalación nueva. Para instalar solo el tema oscuro, usa `-Core Off -DarkMode On`.

Windhawk carga el mod en Explorer al iniciar la sesión. La Lupa se prepara en segundo plano al 100 % y puede volver a abrirse con un pellizco, **Win + rueda** o **Win + «+»** tras cerrarla. El reconocimiento de símbolos puede tardar en el primer arranque.

## Ajustes de Windhawk

Los nombres de los ajustes coinciden con la pestaña **Ajustes** del mod.

### Funciones opcionales

- **Núcleo de zoom del escritorio** gestiona la Lupa en pantalla completa y el zoom de teclado. Los controles del touchpad y el ratón usan este núcleo.
- **Desactivar controles del touchpad** deja a Windows sus gestos de tres dedos. El núcleo y la rueda siguen disponibles según sus propios ajustes.
- **Desactivar controles del ratón** deja el ratón con el comportamiento de Windows. El pellizco y el teclado pueden seguir ampliando.
- **Desactivar acorde de clic izquierdo y derecho** conserva la rueda y deja pasar los clics ordinarios directamente. Con el acorde activo, el primer botón puede esperar hasta 45 ms. Los mensajes de ratón identificados por Windows como lápiz o pantalla táctil se dejan pasar. [Identificación de eventos de lápiz y pantalla táctil](https://learn.microsoft.com/windows/win32/tablet/system-events-and-mouse-messages)
- **Modo oscuro para diálogos clásicos de Windows** funciona con un ajuste propio, además de los controles de zoom.
- **Color del marco del zoom** elige rojo, azul o amarillo. **Ocultar marco del zoom** oculta el indicador mientras la ampliación sigue disponible.

### Pellizco y teclado

- **Zoom sensitivity (%)** parte de 100 %. **Pinch acceleration (%)** parte de 35 % y admite 0 para dejar una respuesta proporcional. **Pinch activation threshold (%)** parte de 3 %. **Reverse pinch direction** intercambia acercar y alejar.
- **Keep the pointer stationary during a pinch** omite los reajustes sintéticos que hace la Lupa durante el gesto y sigue permitiendo el movimiento físico del ratón. La protección termina al levantar los dedos o al perderse el gesto.
- **Maximum zoom (%)** usa el rango nativo de 100 a 1600 % cuando vale 0.
- **Keyboard zoom increment (%)** conserva el incremento de Windows cuando vale 0. **Progressive keyboard hold speed** parte de 2. El valor 1 usa la repetición estándar y de 2 a 4 regula la velocidad máxima. La rampa comienza al mantener el atajo 200 ms. Al soltar se conserva la escala y la posición visibles.
- **Observe only (recognition without zoom)** permite comprobar la detección de gestos manteniendo la escala actual.

### Ratón y ventanas

- **Smooth mouse wheel zoom** controla el zoom continuo por rueda. **Wheel zoom per notch (%)** parte de 20 % y admite valores de 1 a 100 %. Una muesca aplica un factor de 1,2 y los deltas fraccionarios cuentan proporcionalmente. Se agrupan por cuadro y se aplica una transición de 32 ms. Si comienza un pellizco, se descarta la rueda pendiente y el gesto de ampliación conserva al menos el último nivel confirmado por la Lupa.
- **Use Win + mouse wheel for zoom** permite usar **Win + rueda** y **Win + clic central**. Este último vuelve al 100 % y apaga el modo. Desactivar el ajuste cambia el modificador de zoom a **Ctrl + Alt**.
- **Ctrl+Alt + mouse wheel switches windows** funciona en el perfil Win. Limita su recorrido pendiente a seis pasos y 200 ms. **Extra Ctrl+Alt mouse window actions** está desactivado por defecto. Al activarlo, **Ctrl + Alt + clic central** maximiza o restaura y **Ctrl + Alt + clic derecho** minimiza.
- El clic izquierdo y derecho simultáneo funciona sin modificadores. En ventanas elevadas, Windows recibe los clics directamente. Con el modo activo, la rueda lateral se convierte en desplazamiento vertical. Al apagarlo, conserva la entrada horizontal.
- **Switch windows directly with a horizontal three-finger swipe** cambia una ventana por gesto. Izquierda avanza y derecha retrocede. Desactívalo para usar el selector lateral de Windows. Los deslizamientos arriba y abajo mantienen la acción de Windows.
- **Show Task View at 100 percent** restaura el nivel y la posición anteriores al salir de la vista de tareas. Una entrada explícita de zoom también restaura la vista anterior. El escritorio sigue siendo ampliable, incluso después de **Win + D**.

### Lupa y tema

- **Hide Windows Magnifier window and touch controls** oculta la interfaz de la Lupa mientras su motor permanece disponible. **Suppress Magnifier startup feedback cursor** solicita que Windows omita el indicador azul al arrancarla.
- **Disable Magnifier click-to-read and its reading cursor** evita activar la lectura de la Lupa mediante **Ctrl + Alt + clic izquierdo** en binarios compatibles. Si la lectura ya estaba activa, sal de ella o vuelve a abrir la Lupa. Narrador y los demás atajos de lectura conservan su función.
- **Native Windows Magnifier integration** combina el pellizco, la rueda continua y los atajos. Puedes desactivar la integración para usar los controles estándar de la Lupa.
- **Dark mode for classic Windows dialogs** usa el tema de aplicaciones de Windows y respeta alto contraste. Se aplica a determinados diálogos y hojas de propiedades de Explorer y de varios applets de Windows. Los menús, la portada y las categorías del Panel de control usan los recursos nativos de Windows.

## Vista y captura

En **Configuración → Accesibilidad → Lupa → Vista** se ajusta el seguimiento del puntero, el foco del teclado, el cursor de texto y Narrador. En modo **Dentro de los bordes**, la vista se desplaza al acercar el puntero a un borde. En modo **Centrado**, se desplaza de forma continua cuando mueves el puntero.

El marco de DirectComposition compensa el zoom de cada monitor para conservar un grosor objetivo de cinco píxeles físicos en los cuatro lados. Solicita a Windows la exclusión de captura. En la prueba con OBS, **Captura de ventana** y **Captura de pantalla → Moderno (WGC)** compartieron el contenido sin la ampliación del escritorio. DXGI capturó la ampliación en ese equipo. La [validación](VALIDATION.md) identifica qué mediciones corresponden a versiones anteriores.

## Diagnóstico

Ejecuta este comando desde la carpeta del paquete x64.

```powershell
.\build\pinch-zoom-host.exe --status
```

Comprueba que figuran **Native gate: READY** y **Windows Magnifier: READY**. `actualZoom` muestra el nivel que comunica la Lupa. `--reset` solicita el 100 %. Los valores de resolución `1` y `2` indican el perfil local verificado y la resolución por símbolos, respectivamente.

Para observar los informes HID y contar gestos durante 30 segundos, usa `.\build\pinch-zoom-host.exe --diagnose --seconds 30` con el host de Windhawk desactivado. El diagnóstico cuenta informes sin almacenar coordenadas de los dedos.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/measure-performance.ps1
```

La medición predeterminada muestrea cinco segundos de CPU y memoria de los procesos completos de Explorer y Magnify. Con `-Exercise`, una prueba visible cambia temporalmente el zoom y restaura el nivel inicial. Ejecútala cuando puedas dejar quietos el touchpad y el ratón. La latencia medida es la entrega por IPC, sujeta al intervalo de sondeo.

## Compatibilidad tras actualizaciones de Windows

El lector HID y el filtro de gestos se ejecutan en Explorer. El complemento de Magnify se comunica con ese controlador para aplicar objetivos sobre la Lupa en pantalla completa. La integración nativa de zoom en Windows 11 x64 usa perfiles locales para las imágenes de las compilaciones 26200.9711 y 26200.9974 con Windhawk 1.7.3. Para el pellizco se requiere un Precision Touchpad con contactos HID. El tema oscuro tiene su propia comprobación de compatibilidad. Los modos lente y acoplado y el escritorio seguro de UAC quedan fuera del alcance. Consulta la [validación](VALIDATION.md) para combinaciones pendientes, como DPI mixtos y distintos monitores.

Cada perfil local verifica el GUID y la edad CodeView, el tamaño, el timestamp PE y las secciones de destino. La identidad del binario decide si puede usarse ese perfil. Cuando falta un perfil en la compilación 26200, se buscan firmas exactas por medio de la caché de Windhawk y de los PDB de Microsoft. La validación de direcciones se aplica antes de activar cada grupo de funciones.

En 26200.9974, la Lupa aplica los niveles del monitor activo en pasos de un punto porcentual. El mod acumula la entrada de la rueda y del pellizco antes de entregar esos niveles. Mantener Win + «+» o «−» usa la repetición nativa de Windows. Vista de tareas sigue el comportamiento de Windows en este perfil.

Si Windows cambia una función opcional, el motor conserva las funciones cuyos grupos completos siguen disponibles. Por ejemplo, sin la protección de puntero verificada se usa el seguimiento nativo. Si falta el filtro del shell, el touchpad conserva sus gestos normales y el teclado y la rueda pueden seguir ampliando si el motor está disponible. Si falta el núcleo, se usan la Lupa visible y sus atajos estándar.

El arranque en frío conserva una muesca de rueda por un máximo de 1,5 segundos o un primer aumento de pellizco de hasta 3 % si los tres dedos siguen apoyados. Bloqueo, suspensión y desconexión invalidan las entradas pendientes. El diagnóstico muestra las capacidades y el estado del motor.

La tecla **Win** por sí sola conserva Inicio. Durante un atajo de ratón consumido por el mod, el observador enmascara la activación de Inicio mediante una pulsación neutra. Si Windows rechaza la entrada sintética por UIPI, el evento físico se deja pasar. [SendInput y UIPI](https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-sendinput) · [Máscara del menú](https://github.com/AutoHotkey/AutoHotkeyDocs/blob/v2/docs/lib/A_MenuMaskKey.htm)
