# Referencias y atribuciones

Este proyecto se distribuye bajo GPL-3.0-or-later.

- **KWin, efecto Zoom**, KDE y sus colaboradores, GPL-2.0-or-later. Se estudió su registro de pellizcos de tres dedos y la actualización continua del zoom. No se incorporan bibliotecas de KDE. [Código de referencia](https://github.com/KDE/kwin/blob/master/src/plugins/zoom/zoom.cpp).
- **Three Finger Drag**, Rodrigo Thomazi, GPL-3.0. Referencia para descubrir campos HID de touchpads de precisión, interpretar informes híbridos y usar un proceso dedicado. El reconocedor de pellizcos y su implementación son específicos de este proyecto. Referencia estudiada: commit `e4a1aeceacc4da99b4b2f1848ef84b6655ee5250` del repositorio `ramensoftware/windhawk-mods`. [Archivo](https://github.com/ramensoftware/windhawk-mods/blob/e4a1aeceacc4da99b4b2f1848ef84b6655ee5250/mods/three-finger-drag.wh.cpp).
- **Plantilla de tool mods de Windhawk**, Michael Maltsev / Ramen Software. Se estudió y usó durante el prototipo 0.1. La versión 0.2 usa un hilo en Explorer y ya no incorpora ese arranque. [Documentación de referencia](https://github.com/ramensoftware/windhawk/wiki/Mods-as-tools:-Running-mods-in-a-dedicated-process).
- **Magnifier Headless Mode**, BCRTVKCS. Se estudió el mod instalado para identificar las ventanas de la Lupa y sus controles táctiles. La ocultación integrada conserva ventanas reales y permite restaurar su visibilidad. No incorpora su terminación forzada del proceso al descargar. [Autor](https://github.com/bcrtvkcs).
- **Windhawk 1.7.3**, Ramen Software. Se usa su API y compilador instalados; no se redistribuye Windhawk ni su runtime. Commit de la etiqueta: `c165997c54e71dad897460232d595db7cf2c06ab`. [Proyecto](https://github.com/ramensoftware/windhawk).
- **Microsoft**, documentación de [Raw Input](https://learn.microsoft.com/en-us/windows/win32/inputdev/raw-input), [HID](https://learn.microsoft.com/en-us/windows-hardware/drivers/hid/interpreting-hid-reports) y [Magnification API](https://learn.microsoft.com/en-us/windows/win32/winauto/magapi/magapi-intro).

La compilación usa las cabeceras y bibliotecas del compilador LLVM/MinGW incluido con Windhawk. Los binarios enlazan su runtime estáticamente. Los avisos correspondientes se incluyen en `licenses/` y deben acompañar los binarios redistribuidos. El código fuente de este proyecto está disponible junto a sus entregables.
