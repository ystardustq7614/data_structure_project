#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>   // QMainWindow 是 Qt 提供的主窗口类
// 这里是 Qt Designer 自动生成的 UI 命名空间
// mainwindow.ui 里的界面会生成一个 Ui::MainWindow 类
QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE
// MainWindow 是我们程序的主窗口
// 它继承 QMainWindow，所以可以作为一个标准 Qt 主窗口使用
class MainWindow : public QMainWindow
{
    Q_OBJECT             // Qt 的信号槽机制需要这个宏
public:
    // 构造函数
    // 程序创建 MainWindow 时会调用
    explicit MainWindow(QWidget *parent = nullptr);
    // 析构函数
    // 窗口关闭、对象销毁时会调用
    ~MainWindow();
private:
    // ui 指针指向 mainwindow.ui 生成的界面对象
    // 这个是 Qt Creator 自动生成的
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
