#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>
namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();
	static bool compare(const QPair<QString,QString> &first, const QPair<QString,QString> &second);
private:
	Ui::MainWindow *ui;

};

#endif // MAINWINDOW_H
