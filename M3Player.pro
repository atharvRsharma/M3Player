QT       += core gui multimedia multimediawidgets widgets pdf pdfwidgets concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

VCPKG_ROOT = C:/vcpkg


win32 {
    INCLUDEPATH += $$VCPKG_ROOT/installed/x64-windows/include

    LIBS += -L$$VCPKG_ROOT/installed/x64-windows/lib
    LIBS += -ltag
    LIBS += -lpoppler-qt6

    VCPKG_BIN = $$VCPKG_ROOT/installed/x64-windows/bin
    DESTDIR_WIN = $$shell_path($$OUT_PWD)

    copytagdll.commands = \
        copy /Y $$shell_path($$VCPKG_BIN/tag.dll) $$DESTDIR_WIN & \
        copy /Y $$shell_path($$VCPKG_BIN/poppler-qt6.dll) $$DESTDIR_WIN & \
        copy /Y $$shell_path($$VCPKG_BIN/poppler.dll) $$DESTDIR_WIN & \
        copy /Y $$shell_path($$VCPKG_BIN/jpeg62.dll) $$DESTDIR_WIN

    first.depends = $(first) copytagdll

    export(first.depends)
    export(copytagdll.commands)

    QMAKE_EXTRA_TARGETS += first copytagdll
}

unix {
    macx {
        CONFIG += link_pkgconfig
        PKGCONFIG += taglib poppler-qt6
    } else {
        CONFIG += link_pkgconfig
        PKGCONFIG += taglib poppler-qt6
    }
}

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

QMAKE_PROJECT_DEPTH = 0

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    mediatypes.cpp

HEADERS += \
    mainwindow.h \
    mediatypes.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc
