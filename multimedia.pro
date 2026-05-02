QT       += core gui multimedia multimediawidgets widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

LIBS += -LC:\msys64\mingw64\lib -lpoppler-qt6 -lpoppler
INCLUDEPATH += C:\msys64\mingw64\include\poppler\qt6
INCLUDEPATH += C:\msys64\mingw64\include\poppler
INCLUDEPATH += C:\msys64\mingw64\include

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
