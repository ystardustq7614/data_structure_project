/** @file
 * @brief GUI 入口；QApplication 必须先于窗口和 QPixmap 缓存创建，事件循环结束后按栈顺序销毁。 */
#include "ui/mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
