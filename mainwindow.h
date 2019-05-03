#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QLabel>
#include <qstring.h>
#include <QMainWindow>
#include <qtreewidget.h>
namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
	void load(QString fileName);
	void resizeEvent(QResizeEvent *event) override;
	void updateStatusBar(int iFrame,int cFrames);
	void updateTreeWidget(int iFrame, int cFrames);
	QTreeWidgetItem* onSetLabel(QTreeWidgetItem* item, QString strLabel);
	void reset();
private slots:
    void on_actionOpen_triggered();

    void on_actionExit_triggered();

    void on_baseSlider_sliderMoved(int position);

    void on_MainWindow_iconSizeChanged(const QSize &iconSize);

    void on_treeWidget_itemSelectionChanged();

    void on_slider_sliderMoved(int position);

    void on_treeWidget_clicked(const QModelIndex &index);

    void on_treeWidget_doubleClicked(const QModelIndex &index);
	void onSetLabelMenu(QAction*);
private:
	QLabel * _statusPane1;
	bool _bMoveSlider;
	bool _bSelectChange;
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
