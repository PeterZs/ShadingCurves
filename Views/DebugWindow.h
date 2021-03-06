#ifndef DEBUGWINDOW_H
#define DEBUGWINDOW_H

#include <QMainWindow>
#include <QImage>

namespace Ui {
class DebugWindow;
}

class DebugWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit DebugWindow(QWidget *parent = 0);
    ~DebugWindow();

    void setImage(QImage image);
    
private:
    Ui::DebugWindow *ui;
};

#endif // DEBUGWINDOW_H
