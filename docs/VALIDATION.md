# Validación

Fecha de revisión: 9 de noviembre de 2026.

## Estado de 0.7.0

- **609 comprobaciones automáticas correctas**: 75 núcleo, 61 helpers de ratón, 83 runtime/ABI, 49 callbacks reales del broker con APIs falsas, 110 adaptador nativo, 25 shell, 14 lanzamiento y 192 teclado progresivo. Compilación completa x64 con `-Wall -Wextra -Werror`; prueba Live compilada, sin ejecutar.
- Instalada localmente y comparada por SHA-256 con el manifiesto: fuente y DLL coinciden; versión 0.7.0 habilitada. Gate/Native/UI de pantalla completa y hooks de ratón/lectura verificados READY, nativeCaps=0x7f, gateCaps=0x7, resolutores offline y lastError=0. El diagnóstico registró eventos de rueda físicos tras instalar; esto no sustituye la confirmación del usuario.
- Muestra pasiva de 5,04 segundos, sin estímulos sintéticos: ambos Explorer 0,00 % de un núcleo; Magnify 0,31 %. Conjuntos de trabajo: 620,1/155,3 MiB en Explorer y 194,1 MiB en Magnify. Memoria privada estable en Explorer y −0,06 MiB en Magnify; handles 0/0/+2. Son procesos completos: no aíslan el coste del mod, no miden GPU/latencia física ni demuestran ausencia de fugas.

- Refactor del contrato de inicio: una UI de pantalla completa validada al 100 % puede recibir el primer objetivo; el renderer se exige después del setter inicial. Objetivos finos comunes a pinch, wheel y teclado IPC; un fallo de primado no recibe ACK ni genera reintentos infinitos.
- Escritorio independiente de Task View; instantánea preservada hasta publicar la restauración, incluso cuando la señal del shell se retira antes. Una trayectoria nueva de rueda no invalida su propia revisión al recuperar la vista.
- Teclado progresivo probado con reloj y renderizador falsos: primer toque nativo, retraso inicial, velocidad logarítmica creciente, límites, frame detenido, suelta visible, objetivos adelantados e inversión directa. El número de aserciones incluye comprobaciones por frame, no escenarios de hardware independientes.
- Perfil Win de ratón: estado físico L/R, Win+S/espacio/zoom pasantes, una máscara por pulsación consumida, fallo reintentable, deltas fraccionarios, cambio de ventanas limitado, deuda caducada e inversión, y propiedad DOWN/UP conservada al soltar modificadores.
- Primer pellizco frío limitado a un 3 %, entrega desde timer sin exigir nuevo movimiento HID y cancelación al soltar, cerrar, cambiar de entrada/dispositivo/sesión o caducar.
- Rueda de arranque con PID falso del proceso de test (sin abrir Magnify); caducidad, cancelación y ausencia de replay antiguo. Backend no compatible conserva los controles nativos; no se usa una transformación propia como reemplazo.
- Supresión del modo leer-desde-aquí contrastada con símbolos y desensamblado del Magnify exacto. Sólo tres entradas privadas, sin reset persistente de bindings ni escritura global de cursores. Identidad o grupo incompleto: se mantienen los originales.
- Callbacks reales del broker ejercitados con endpoints Win32 falsos: sin hooks instalados ni SendInput/PostMessage reales, incluyendo falla parcial de máscara, DOWN duplicado, teclas/capacidades que cambian y recuperación tras perder UP durante bloqueo.
- La integración visible se compiló, pero no se ejecutó. Las pruebas automáticas no generan teclas, clics ni oscilaciones sobre el escritorio.

Pendiente: comprobación física del nuevo perfil Win, mantener/soltar OEM y numérico, primer arranque, UAC/aplicaciones elevadas, DPI mixtos, otros equipos y reinicio completo de Windows. No se confunden los resultados de la base 0.6.0 con validación de 0.7.

## Estado de 0.6.1

- 277 comprobaciones sin teclas, clics ni zoom sintéticos sobre el escritorio: las 235 de la base y 42 específicas del teclado mantenido.
- Primer toque nativo, aceleración de repeticiones ×2, botones de UI, ambos Win, OEM y numérico, VK de layout transportado por WM_HOTKEY y timestamps de 32 bits con vigencia acotada mediante reloj de 64 bits.
- Suelta antes de una repetición en cola, factor/posición visibles, reactivación del tracker si el objetivo llegó al 100 % mientras la vista aún está ampliada, y cancelación por entradas nuevas o cambios de sesión/vista.
- Fallo de timer, capacidades ausentes, cierre/reentrancia, lector que cambia la revisión o cancela, estado de UI/renderizador y drenaje de activeUpdates. La congelación no escribe una transformación propia de API.
- Se conserva la cadencia nativa: el binario actual limita hotkeys repetidos del mismo identificador a unos 125 ms; el cambio aumenta el paso, no la velocidad global del teclado. La detección de suelta ocurre en el siguiente turno UI, aproximadamente un frame; no se promete latencia física cero.
- Instalada y comprobada por hash. La captura pasiva de 45 segundos no registró pulsaciones de las teclas observadas; no demuestra el resultado físico de la función nueva.

Pendiente: confirmación física de mantener/soltar, seguimiento y alternancia con rueda/pellizco. Las limitaciones de hardware, reinicio y versiones de Windows de la base siguen vigentes.

## Estado de 0.6.0

- 75 pruebas del núcleo, 53 del runtime/ABI, 68 del adaptador nativo, 25 del shell y 14 del lanzamiento público: 235 comprobaciones sin zoom ni teclas/clics sintéticos.
- Ráfagas de 10.000 entradas, deltas fraccionarios, extremos signed, saturación, reversión, caducidad y paquetes publicados durante un frame. Primer delta pequeño con primado único y entrega decimal fina en el mismo turno de UI.
- Protocolo del shell probado ante cuarto contacto después de cancelar, rechazo de compatibilidad, Start inicial/diferido fallido y Continue fallido. Un gesto terminal no se reactiva.
- Teclado explícito probado con overview ya suspendido y aún pendiente: restaura y prevalece, sin volver inmediatamente al 100 %.
- Perfiles probados frente a GUID/edad/timestamp distintos, secciones no ejecutables, destinos fuera de imagen y CodeView truncado. Los tres perfiles del equipo se verificaron mapeando los archivos como recursos, sin ejecutar sus entradas.
- Fallos de símbolos opcionales conservan el núcleo; wheel sin gate funciona. Un backend no verificado o perdido no recibe transformaciones públicas alternativas, ni desde pausa/reset.
- API pública de feedback COM contrastada con SDK 26100 y comprobada con lanzador falso, sin abrir procesos reales.

Regresión física de 0.5.0: tras una validación inicial positiva, el usuario notificó Inicio/Win+S/Win+«−» y clic izquierdo intermitentes; confirmó recuperación de las teclas al desactivar el mod. La revisión encontró una secuencia inválida Start→Cancel→Continue/Finish al cambiar contactos, y consumo de comandos de teclado en overview. Las correcciones se verifican automáticamente; aún no se atribuye toda la regresión a una sola causa.

Registro pasivo del cursor de 45 segundos: no observó WAIT/APPSTARTING; no confirma el origen del indicador azul referido. La nueva opción solo pide suprimir feedback de arranque de Magnify; conserva cursores de las demás aplicaciones.

La 0.6.0 final está instalada con DLL verificada por SHA-256. Diagnóstico: nativeCaps=0x7f, gateCaps=0x7, los tres resolutores en perfil offline verificado, sin error. El usuario confirmó Inicio, Win+S, Win+«+»/«−» y clics normales antes/después de zoom; también primer paso pequeño desde el 100 %, giro libre rápido e inversión, alternando con pellizco.

Pendiente: reinicio completo de Windows, suspensión, nueva activación lateral con SetForegroundWindow y otros equipos/pantallas. El mod anterior se dejó desactivado durante la corrección y la nueva revisión se activó para esa comprobación. No se han ejecutado oscilaciones visibles de zoom.

## Estado de 0.5.0

- 65 pruebas del núcleo, 33 del runtime/ABI, 56 del adaptador nativo y 18 del filtro del shell: 172 en total. Backend simulado, sin cambiar el escritorio.
- La rueda se prueba con deltas fraccionarios, acumulación de ráfagas, trayectoria intermedia decimal, finalización, inversión, límites y cancelación.
- El host real se prueba con backend falso: rechaza eventos anteriores al pellizco/reset o a un cambio de teclado, conserva varias muescas nuevas de la misma revisión y no convierte un evento de despertar en zoom retrasado.
- El callback nativo de rueda se prueba con entrega correcta/fallida, opt-out, host ausente y multitarea. Se comprueba que no aplique un segundo zoom nativo.
- Las pruebas de sesión separan conexión, bloqueo y suspensión. Las de arranque cubren límites de reintentos, recuperación por nueva entrada, Win+Esc y reconstrucción temporal del renderizador.
- La función MagnifierUI::HandleMouseWheelZoom(int) y el uso de delta/120 se verificaron contra símbolos públicos y desensamblado del Magnify de este equipo.
- Servicio Windhawk observado en ejecución e inicio automático. No se ha reiniciado Windows, cerrado la sesión ni suspendido físicamente el equipo durante esta revisión.

Instalada con ambos componentes READY, DLL verificada por SHA-256 y controlador perteneciente al Explorer principal. Se detectó un segundo Explorer de carpetas y se excluyó para que no compita por el estado compartido ni los atajos de recuperación. La recuperación de UI/renderizadores preexistentes se prueba con callbacks falsos sin escribir el zoom.

El usuario confirmó Ctrl+Alt+rueda fluido, inversión y alternancia con pellizco/teclado sin los fallos consultados. También confirmó Win+Esc cerrado durante cinco segundos y respuesta al nuevo gesto. Es confirmación física de este equipo; no certifica estabilidad prolongada.

Muestra pasiva de 5,03 segundos: Explorer principal consumió 0,31 % de un núcleo y Magnify 0,00 %. Sus conjuntos de trabajo fueron 549,9 MiB y 195,1 MiB; los handles variaron −33 y 0. Son procesos completos, no el coste aislado del mod, y esta muestra no mide latencia durante rueda ni demuestra ausencia de fugas.

Pendiente: sesión nueva tras reiniciar Windows, suspensión física y otros equipos/pantallas. No se han ejecutado oscilaciones sintéticas visibles.

## Estado de 0.4.0

La revisión está compilada e instalada en el equipo de desarrollo. El diagnóstico informa READY para el filtro del shell y la Lupa nativa, sin error de inicialización. El hash SHA-256 de la DLL instalada coincide con el artefacto compilado. El usuario confirmó en este equipo el pellizco lento y sostenido sin los fallos observados, la alternancia con Win + «+»/«−», el cambio lateral directo, los gestos arriba/abajo y la restauración del nivel y la posición al salir de multitarea.

- 52 pruebas del núcleo, 21 de runtime/ABI, 44 del adaptador nativo y 18 del filtro del shell: 135 en total.
- El filtro del shell se prueba con sus funciones reales y callbacks falsos: inicio puntual, ausencia de replay, cancelación única, contactos desconocidos y cuatro dedos, cambio lateral único y razones de visibilidad independientes.
- El backend simulado cubre Task View al 100 %, razones de suspensión solapadas, gestos provisionales dentro de una vista ya suspendida, preservación de la instantánea y restauración de nivel/posición.
- Se comprueba la diferencia entre proceso nativo vivo y renderizador temporalmente indisponible. Se prueba que un objetivo del renderizador actualizado por teclado no se sobrescriba con una caché de opciones antigua.
- La protección de ClipCursor se prueba durante la actualización del pellizco y durante el hold; el teclado y la liberación posterior conservan su comportamiento nativo.
- Señales verificadas con los símbolos públicos y el binario actual: Task View es el bit 0x2 de MultitaskingViewVisibilityService::ViewShown/ViewDismissed. Mostrar escritorio usa el dato de Explorer g_fDesktopRaised. No depende de títulos de ventana ni de adivinar el foreground del shell.
- MoveTo recibe this completo, RECT alineado a 16 y bool. Se solicita actualizar el destino de animación para que una transición pendiente no vuelva a una posición antigua.

Pendiente: otros modelos de touchpad, otras versiones de Windows y configuraciones con varios monitores. La comprobación física anterior procede de la respuesta del usuario; no certifica estabilidad prolongada. No se han repetido pruebas automáticas visibles de oscilación del zoom.

## Historial de 0.3.2


La ruta por interfaz con porcentajes enteros mostró tirones y desplazamiento del puntero; se desactivó y se restauraron las preferencias de Windows. La revisión posterior conserva el factor decimal en los fotogramas intermedios y usa la interfaz solo para entrar/salir de ampliación. El usuario confirmó después que la ruta decimal era fluida y que el seguimiento y la alternancia con teclado funcionaban. Tras la protección añadida en 0.3.2, confirmó que el puntero se mantenía y las ventanas dejaban de saltar al sostener el pellizco. Pasar los tests no acredita por sí solo la fluidez percibida.

- Compilación x64 con Clang 20.1.3 de Windhawk 1.7.3 y `-Wall -Wextra -Werror`.
- 48 pruebas del reconocedor: aceleración por velocidad, temporización HID, reversión en los límites, cambios de zoom por teclado, clasificación nativa y descarte del recorrido recibido durante el arranque.
- 21 comprobaciones de runtime/ABI: incluyen adopción de zoom externo, recuperación al 100 %, anclas tras cambios externos y movimiento del puntero durante un pellizco en el modo API.
- 27 comprobaciones del adaptador nativo con backend simulado: primado único, factores decimales distintos sin cuantización, una única escritura por solicitud, descarte de bases anteriores al primado/teclado, separación de cambios internos y comandos reales, hilo correcto, recuperación, cierre reentrante y protección del cursor tanto síncrona como diferida durante el hold. Se verifica que el teclado y los deslizamientos quedan fuera de esa protección.
- 5 pruebas de integración ejecutadas contra una iteración anterior de la Lupa real instalada: objetivo desde el controlador de pellizco, cambios rápidos, incremento por IPC desde el nivel del pellizco, vuelta al pellizco y recuperación al 100 %. Se comprueba el factor realmente renderizado con `MagGetFullscreenTransform`. El incremento por IPC no es una pulsación física de teclado.
- Se ha instalado el mod en Explorer y Magnify; ambos auxiliares informan `READY`, con ventana oculta y sin error informado. El mod anterior `magnifier-headless` está desactivado y sigue instalado.
- Se identificaron los nombres de las opciones en el binario de Magnify: FollowMouse (16), FollowFocus (17), FollowCaret (18), FollowNarrator (19) y FullScreenTrackingMode (29). Se confirmó por desensamblado que el modo 1 centra el ratón. Se retiró el experimento que invocaba directamente el actualizador de opciones tras una regresión. El mod final conserva las preferencias nativas de seguimiento.
- La revisión independiente detectó y permitió corregir la doble ruta de escritura, cambios internos confundidos con teclado, reinicio por porcentajes repetidos y una carrera de descarga. La ventana de mensajes se destruye en su hilo; un cierre reentrante conserva el código de la DLL hasta terminar el proceso.
- Los hooks de la Lupa no bloquean `MagSetFullscreenTransform` ni reemplazan el comportamiento de los atajos. La entrada/salida de ampliación usa `MagnifierUI::HandleSetZoomLevel(long)`; los factores decimales intermedios usan `FullscreenMagnifier::ChangeMagnificationFactor(float,CPoint*)`. Se exige que el hilo capturado de render coincida con el del callback antes de llamar a este método. Los hooks de comandos de interfaz observan cambios nativos para rebasar el reconocedor.
- El usuario confirmó «Funciona bien» al alternar pellizco, Win+menos numérico y pellizco en la integración nativa anterior a los últimos ajustes de cierre/arranque.
- Tras informar de reapertura con Win+Esc, se eliminó el reinicio automático. Tras informar de un salto al arrancar desde cerrada, se descartó el movimiento acumulado durante la espera y las solicitudes de una instancia anterior. El usuario confirmó después que Win+Esc permanece cerrado al dejar el touchpad y las teclas sin tocar durante cinco segundos. La latencia física de ese arranque sigue pendiente de confirmación; no se afirma que abrir Magnify sea instantáneo.

## Comprobación física de 0.3.2

El segundo revisor identificó que `Magnifier::MoveMouse(CPoint)` genera entrada de ratón sintética y que existe una restauración diferida. Se omite esta función únicamente durante una actualización de pellizco o mientras el host está vivo, identifica un pellizco y siguen tres contactos apoyados. Una llamada de teclado nativo conserva su comportamiento, incluso con los dedos apoyados. La protección no inyecta entradas para compensar ni modifica las preferencias de seguimiento de Windows.

Fuente y DLL instaladas coinciden por SHA-256 con la compilación local. Los dos auxiliares están `READY`. La confirmación del usuario cubre esta máquina, no todas las variantes de hardware.

## Consumo observado

Muestra pasiva de 5,05 segundos con la ruta decimal cargada: Explorer consumió 0,31 % de un núcleo y Magnify 0,62 %. El conjunto de trabajo fue 444,6 MiB y 194,8 MiB respectivamente; los handles no crecieron en esa muestra. Son procesos completos, no el coste aislado del mod, y cinco segundos no demuestran ausencia de fugas o estabilidad prolongada.

Una prueba visible anterior envió 300 objetivos con cero timeouts, pero pertenecía a una iteración retirada. Su sondeo estaba limitado por la granularidad de Sleep, por lo que no se presenta como latencia física ni como rendimiento de la versión final. No se repitieron oscilaciones automáticas después de la queja del usuario.

Muestra pasiva de 0.3.2 durante 5,05 segundos: Explorer 1,55 % de un núcleo y Magnify 0 % en la resolución de la muestra; ambos sin crecimiento de memoria privada. Conjuntos de trabajo: 487,0 y 194,8 MiB. La actividad del escritorio explica que estas muestras no sean comparaciones aisladas del coste del mod.

## Pendiente en 0.3.2

- Pruebas prolongadas y repetidas de arranque en frío, mantener los dedos y seguimiento en otros equipos.
- Generalizar las comprobaciones físicas a otros dispositivos y condiciones; las confirmaciones actuales corresponden a esta máquina.
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
