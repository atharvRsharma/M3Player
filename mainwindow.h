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

private slots:
    void on_actionOpen_triggered();
    void on_actionPause_triggered();
    void on_actionPlay_triggered();
    void on_actionReplay_triggered();

private:
    void addMedia(const QString &path);
    void rebuildGrid();
    void highlight();
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
