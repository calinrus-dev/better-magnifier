# Better Visual

**Zoom continuo del escritorio para Windows 11 x64.** Mod de accesibilidad para Windhawk, inspirado en KWin. Amplía texto y detalles con un pellizco de tres dedos, la rueda del ratón o el teclado. La Lupa de Windows se encarga de renderizar la vista y seguir el puntero.

[**Descargar 1.2.0**](https://github.com/calinrus-dev/better-magnifier/releases/tag/v1.2.0) · [Ajustes y diagnóstico](docs/REFERENCE.md) · [Validación](docs/VALIDATION.md)

[![Compilación y pruebas](https://github.com/calinrus-dev/better-magnifier/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/calinrus-dev/better-magnifier/actions/workflows/ci.yml)

> **Zoom nativo** · Perfiles incluidos para Windows 11 25H2, compilaciones 26200.9711 y 26200.9974, con Windhawk 1.7.3 y Lupa en pantalla completa. El pellizco requiere un Precision Touchpad con contactos HID. [Compatibilidad tras actualizar Windows](docs/REFERENCE.md#compatibilidad-tras-actualizaciones-de-windows)

## Instalar

**Desde Windhawk.** Descarga [better-visual.wh.cpp](https://github.com/calinrus-dev/better-magnifier/releases/download/v1.2.0/better-visual.wh.cpp). Elige **Crear un nuevo Mod**, sustituye el ejemplo por el archivo completo y compila. Para actualizar Better Magnifier, edita el mod local existente. Better Visual conserva su identificador, `local@kde-pinch-zoom`.

**Paquete compilado x64.** Descarga el [ZIP para Windows](https://github.com/calinrus-dev/better-magnifier/releases/download/v1.2.0/better-visual-1.2.0-windows-x64.zip), extráelo completo y ejecuta este comando desde su carpeta en PowerShell como administrador.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/install-local.ps1
```

La publicación incluye [SHA256SUMS.txt](https://github.com/calinrus-dev/better-magnifier/releases/download/v1.2.0/SHA256SUMS.txt). El instalador conserva tus ajustes y revierte el cambio si Windhawk aún está descargando la DLL anterior. Consulta la [guía de instalación y actualización](docs/REFERENCE.md#instalación-local-y-actualización) si vienes de una versión anterior.

## Elige las funciones

En **Ajustes** de Windhawk puedes activar el **núcleo de zoom del escritorio** y elegir los **controles del touchpad**, los **controles del ratón** y el **tema oscuro**. El núcleo usa la Lupa y añade los controles de teclado. El tema oscuro puede funcionar por sí solo.

El **acorde de clic izquierdo y derecho** tiene su propio ajuste. El marco puede ser **rojo, azul o amarillo**, y se puede ocultar desde **Ocultar marco del zoom**. El instalador x64 también acepta estas opciones al instalar o actualizar. [Guía de instalación y ajustes](docs/REFERENCE.md#instalación-local-y-actualización)

## Controles

- **Pellizco de tres dedos.** Abre los dedos para ampliar y ciérralos para reducir. Si el gesto llega al 100 %, se apaga el modo zoom.
- **Clic izquierdo y derecho a la vez.** Alterna el modo zoom del ratón y conserva el nivel de ampliación.
- **Rueda principal.** Amplía o reduce mientras el modo está activo. Al llegar al 100 %, el modo queda listo para ampliar de nuevo. **Win + rueda** también permite usar el zoom.
- **Clic de rueda.** Restablece el 100 % y mantiene preparado el modo. **Rueda lateral.** Desplaza en vertical con el modo activo.
- **Win + «+» / «−».** Funciona también en el teclado numérico. Una pulsación aplica el paso nativo. La repetición gradual se aplica a los perfiles compatibles.
- **Ctrl + Alt + Mayús + F9** pausa o reanuda el zoom. **Ctrl + Alt + Mayús + F10** restablece el 100 %. **Win + Esc** cierra la Lupa.

El marco de cinco píxeles señala que el modo de zoom del ratón está activo y deja pasar los clics. El nivel de ampliación es global y se conserva al cambiar de ventana. Los gestos de dos y cuatro dedos conservan su comportamiento. El deslizamiento lateral de tres dedos puede cambiar de ventana directamente.

## Vista y tema

- **Seguimiento.** En **Configuración → Accesibilidad → Lupa → Vista** puedes elegir si la vista sigue al puntero en los bordes o lo mantiene centrado. También puedes ajustar el seguimiento del foco y el cursor de texto.
- **Captura.** En OBS, **Captura de ventana** permite compartir una aplicación a su escala normal. Para compartir un monitor, consulta los [resultados por método de captura](docs/REFERENCE.md#vista-y-captura).
- **Tema oscuro.** El ajuste de Windhawk sigue el modo de aplicaciones de Windows y respeta alto contraste. Cubre diálogos y hojas de propiedades clásicos compatibles. Windows dibuja los menús y la página principal del Panel de control.

## Desarrollo

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/check.ps1
```

La compilación ejecuta las pruebas automáticas en un entorno simulado. La [validación](docs/VALIDATION.md) recoge por separado las pruebas físicas y la compatibilidad observada.

[Cambios](CHANGELOG.md) · [Contribuir](CONTRIBUTING.md) · [Licencia GPL-3.0-or-later](LICENSE) · [Atribuciones](THIRD_PARTY.md)
