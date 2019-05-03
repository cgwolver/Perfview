#include "perfdisplay.h"
#include <QPainter>
#include "perfdata.h"
#include <QWheelEvent>
PerfDisplay::PerfDisplay(QWidget* parent)
    :QFrame(parent)
{
	_dispStart = 0;
	_dispFrames = 0;
	_scale = 16;
	_curFrame = -1;
}
void PerfDisplay::setDisplayFrames(int cFrames)
{
	_dispFrames = cFrames;
}

void PerfDisplay::setDisplayStart(int iIndex)
{
	_dispStart = iIndex;
}

int PerfDisplay::displayFrames()
{
	return _dispFrames;
}

int PerfDisplay::displayStart()
{
	return _dispStart;
}

int PerfDisplay::currentFrame()
{
	return _curFrame;
}
void PerfDisplay::setCurrentFrame(int iFrame)
{
	_curFrame = iFrame;
}
QString PerfDisplay::label()
{
	return _label;
}
void PerfDisplay :: setLabel(QString label)
{
	_label = label;
}
void PerfDisplay::reset()
{
	QPainter painter(this);
	QPen& pen = const_cast<QPen&>(painter.pen());
	QRect rcClient = rect();
	QPoint pt = rcClient.bottomRight();
	pt = pt - QPoint(1, 1);
	rcClient.setBottomRight(pt);
	painter.drawRect(rcClient);
}
void PerfDisplay::drawFrame(QPainter& rPainter, QPen& pen, QLine&rLine, int i, int iFrame, unsigned long long msec)
{
	QLine line2;
	QSize sz = size();
	QPoint pt(QPoint(16 * i, sz.height() - msec*_scale));
	QPoint pt_base(QPoint(16 * i, sz.height()));

	rLine.setP2(pt);
	rPainter.drawLine(rLine);
	rLine.setP1(rLine.p2());

	bool bHeightZ = false;
	QPoint pt1 = pt, pt2 = pt;

	pt1 -= QPoint(2, 2);
	pt2 += QPoint(2, 2);
	if (pt1.y() >= sz.height() || pt2.y() >= sz.height())
	{
		bHeightZ = true;
	}

	if (_curFrame == iFrame)
	{
		if (bHeightZ)
		{
			pt.setY(0);
		}
		line2.setP1(pt);
		line2.setP2(pt_base);
		QColor penColor = pen.color();
		if (bHeightZ)
		{
			pen.setColor(QColor(192, 192, 192, 255));
		}
		else
		{
			pen.setColor(QColor(255, 0, 0, 255));
		}
		rPainter.drawLine(line2);
		pen.setColor(penColor);
	}

	QRect rc;
	QColor clr(0, 0, 255, 255);

	if (bHeightZ)
	{
		pt1.setY(sz.height() - 4);
		pt2.setY(sz.height());
		if (_curFrame == iFrame)
		{
			clr = QColor(255, 0, 0, 255);
		}
		else
		{
			clr = QColor(255, 0, 255, 255);
		}
	}

	rc.setBottomRight(pt1);
	rc.setTopLeft(pt2);
	rPainter.fillRect(rc, clr);
}
void PerfDisplay::paintEvent(QPaintEvent *evt)
{
	PerfData* perf = PerfData::instance();

    QPainter painter(this);
	QPen& pen = const_cast<QPen&>(painter.pen());
	QRect rcClient = rect();
	QPoint pt = rcClient.bottomRight();
	pt = pt - QPoint(1, 1);
	rcClient.setBottomRight(pt);
	painter.drawRect(rcClient);

	std::vector<TimeRange_t>& frameTimes = perf->frameTimes();
	int cFrames = frameTimes.size();
	QSize sz = size();
	QLine line,line2;

	if (cFrames > 0)
	{
		if (_label.compare("Frame") == 0)
		{
			line.setP1(QPoint(0, 0));

			for (int i = 0; i <= _dispFrames; i++)
			{
				int iFrame = _dispStart + i;
				if (iFrame >= cFrames)
					break;

				unsigned long long msec = perf->frameTime(iFrame);

				drawFrame(painter, pen, line, i, iFrame, msec);
			}
		}
		else
		{
			std::vector<PerfStampData_t>& samples = perf->samples();
			std::vector<SampleBracket_t>& brackets = perf->sampleBrackets();
			line.setP1(QPoint(0, 0));
			for (int i = 0; i <= _dispFrames; i++)
			{
				int iFrame = _dispStart + i;
				if (iFrame >= cFrames)
					break;
			
				int count = 0;
				int msec = perf->frameLabelTime(iFrame, _label, count);
			
				drawFrame(painter, pen, line, i, iFrame, msec);
			}
		}	
	}
}
void PerfDisplay::mouseMoveEvent(QMouseEvent *evt)
{
	QPoint pos = evt->pos();
}
void PerfDisplay::wheelEvent(QWheelEvent *evt)
{
	QPoint dpt = evt->angleDelta()/120;
	_scale += dpt.y();
	if (_scale < 1)
		_scale = 1;
	repaint();
}