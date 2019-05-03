#ifndef QPERFDISPLAY_H
#define QPERFDISPLAY_H

#include <QFrame>

class PerfDisplay : public QFrame
{
public:
    PerfDisplay(QWidget* parent=nullptr);
	int displayStart();
	int displayFrames();
	void setDisplayFrames(int cFrames);
	void setDisplayStart(int iIndex);
    void paintEvent(QPaintEvent *)override;
	void wheelEvent(QWheelEvent *event)override;
	void mouseMoveEvent(QMouseEvent *event)override;
	void setLabel(QString label);
	QString label();
	int currentFrame();
	void setCurrentFrame(int iFrame);
	void drawFrame(QPainter& rPainter,  QPen& pen, QLine&rLine, int i, int iFrame, unsigned long long msec);
protected:
	int _curFrame;
	int _dispStart;
	int _dispFrames;
	int _scale;
	QString _label;
};

#endif // QPERFDISPLAY_H
