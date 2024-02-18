# Sycon library (SyNAPSE controller)

## Dependencies

| Application | Version |
| ----------- | ------- |
| cmake       | >=3.16  |
| meson       | 1.3.1   |

## Building

Builds the `Debug` and `Release` versions (`libsycond.so` and `libsycon.so` respectively).

```bash
$ ./build.sh
```

## Install

Installs both the release and debug versions to `/usr/local/lib` and `/usr/local/include`.

```bash
$ ./install.sh
```

## Building an application

### Installed

To build an application `app.cpp` using the **release** version of `libsycon`
previously installed just link with `-lsycon` (or `-lsycond` for the debug version of the library).

For example:

```bash
$ c++ \
    -I$SDE_INSTALL/include \
    app.cpp \
    -o app \
    -Wl,-rpath,$SDE_INSTALL/lib \
    -lsycon
```

### Local build

Example on how to build the application `app.cpp` that uses a local build of `libsycon`:

```bash
$ c++ \
    -I$SDE_INSTALL/include \
    app.cpp \
    -o app \
    -Wl,-rpath,$SDE_INSTALL/lib \
    -Wl,-rpath,$PATH_TO_THIS_REPO/Debug/lib \
    -lsycon
```