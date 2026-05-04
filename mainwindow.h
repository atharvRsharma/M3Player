#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGridLayout>
#include <vector>
#include <memory>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE


struct MediaSlot;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:

    bool eventFilter(QObject *obj, QEvent *event) override;


private:
    void addMedia(const QString &path);
    void openFiles();
    void rebuildGrid();
    void highlight();
    void play();
    void pause();
    void replay();
    void removeMedia(int index);
    void enterFullscreen(int index);
    void exitFullscreen();

    Ui::MainWindow *ui;
    QWidget        *container;
    QGridLayout    *grid;

    //selection related global vars(TODO get rid of ts </3)
    int hoveredIndex = -1;
    int fullscreenIndex = -1;
    bool justClicked = false;
    std::vector<int> selectedIndices{};


    std::vector<std::unique_ptr<MediaSlot>> mediaSlots;

};

#endif
