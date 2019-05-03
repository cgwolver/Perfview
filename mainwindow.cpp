#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <map>
#include <string>
#include <stdlib.h>
#include <assert.h>
#include <qpainter.h>
#include "perfdata.h"
#include <QLabel>
#include <QMenu>
#include <set>
#include <qmessagebox.h>
#include "errordialog.h"
enum
{
	COL_LABEL,
	COL_NUM,
	COL_TIME_SEC,
	COL_TIME_START_MSEC,
	COL_TIME_MSEC,
	COL_PERCENT,
	COL_DATA1,
	COL_DATA2,

	NUM_COLS
};

enum
{
    MENU_FILE,
    MENU_LABEL
};

void _getTreeItemsVec(QTreeWidgetItem* root, std::vector<QTreeWidgetItem*>&itemsVec)
{
	itemsVec.push_back(root);
	int cChilds = root->childCount();
	for (int i = 0; i < cChilds; i++)
	{
		QTreeWidgetItem* ch = root->child(i);
		_getTreeItemsVec(ch, itemsVec);
	}
}

QTreeWidgetItem* _findTreeItems(QTreeWidgetItem* root, QString& str)
{
	QString strLabel = root->text(COL_LABEL);
	if (strLabel.compare(str) == 0)
		return root;
	int cChilds = root->childCount();
	for (int i = 0; i < cChilds; i++)
	{
		QTreeWidgetItem* ch = root->child(i);
		QTreeWidgetItem* ret = _findTreeItems(ch, str);
		if (ret)
			return ret;
	}

	return nullptr;
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
	ui->treeWidget->setColumnWidth(COL_NUM, 32);
	ui->treeWidget->setColumnWidth(COL_LABEL, 256);
	
	_statusPane1 = new QLabel();
	ui->statusBar->addWidget(_statusPane1);
	_bMoveSlider = false;
	_bSelectChange = false;

}

MainWindow::~MainWindow()
{
    delete ui;
}




void MainWindow::on_actionOpen_triggered()
{
    QFileDialog fd;
    QString fileName = fd.getOpenFileName();
    if(fileName.length()>0)
    {
        load(fileName);
    }
}

void MainWindow::on_actionExit_triggered()
{
    qApp->exit();
}
void MainWindow::reset()
{

	PerfData* perf = PerfData::instance();

	perf->reset();

	ui->treeWidget->clear();

}

void MainWindow::load(QString fileName)
{
	reset();
	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly))
		return;

	bool bDisplayError = true;

	ui->view->setLabel("Frame");
	PerfData* perf = PerfData::instance();
	
	std::map<unsigned int, std::string>& strMaps = perf->strMaps();
	std::vector<PerfStampData_t>& samples = perf->samples();
	std::vector<TimeRange_t>& frameTimes = perf->frameTimes();
	std::vector<SampleBracket_t> & brackets = perf->sampleBrackets();
    std::set<std::string> labels;
	
	std::map<void*, int> labIndex;

	PerfHeader_t hdr;
	file.read((char*)&hdr, sizeof(PerfHeader_t));
	int widSlider = ui->baseSlider->size().width();
	if (hdr.magic == PERF_STAMP_FILE_ID)
	{
		int sampleCount = hdr.count;
		int sliderCount = widSlider / 16;
		
		int files = 0;

		//PerfStamp_t Stamp;
		int size = sizeof(PerfStamp_t);
		int dataSize = size * sampleCount;
		for (int i = 0; i<sampleCount; i++)
		{
			if (file.atEnd())
			{
				if (i < sampleCount - 1)
				{
					QMessageBox msbx(QMessageBox::NoIcon,QString("Error"), QString("UnExpected File End"));
					msbx.exec();
					
				}
				sampleCount = i;
				break;
			}
			PerfStampData_t Stamp;
			file.read((char*)&Stamp.stamp, size);
			samples.push_back(Stamp);
		}
		qint64 pos = file.pos();
		int Strings = 0;
		int sz = sizeof(int);
		int c = file.read((char*)&Strings, sizeof(int));
		int bufsize = 1024;
		char* str = (char*)malloc(bufsize);
		for (int i = 0; i<Strings; i++)
		{
			if (file.atEnd())
				break;
			unsigned int addr = 0;
			unsigned int lens = 0;

			file.read((char*)&addr, sizeof(addr));
			file.read((char*)&lens, sizeof(lens));
			if (lens > bufsize)
			{
				bufsize = lens;
				str = (char*)realloc(str, lens);
			}
			file.read(str, lens);
			//assert(strlen(str)>0);
			strMaps[addr] = str;
		}
		int iFrameIndex = 0;
		QTreeWidgetItem* item = nullptr;
		bool bFrameStart = false;

		//////////////////////////////////////////////////////////////////
		bFrameStart = false;
		iFrameIndex = 0;
		brackets.resize(sampleCount);

		for (int i = 0; i < sampleCount; i++)
		{
			brackets[i].type = 0;
			brackets[i].sampleIndex = -1;
			PerfStamp_t & sample = samples[i].stamp;
			const char* str = perf->cstr(sample.Label_);
			samples[i].name = str;
			unsigned short Type = 0x00ff & sample.Type;
			if (strcmp(str, "Frame") == 0)
			{
				if (Type == 1)
				{
					brackets[i].type = 1;
					if (bFrameStart == false)
						bFrameStart = true;
					if (bFrameStart)
					{
						TimeRange_t tm;
						tm.t1 = 0;
						tm.t2 = 0;
						tm.frameIndex = iFrameIndex;
						tm.sampleIndex1 = i;
						tm.sampleIndex2 = i;
						tm.t1 = sample.Time;
						frameTimes.push_back(tm);
						
						iFrameIndex++;
					}
				}
				else if (Type == 2)
				{
					brackets[i].type = 2;

					if (bFrameStart)
					{
						assert(iFrameIndex > 0);
						assert(iFrameIndex == frameTimes.size());
						TimeRange_t& tm = frameTimes[iFrameIndex - 1];
						
						tm.t2 = sample.Time;
						tm.sampleIndex2 = i;
						brackets[i].sampleIndex = tm.sampleIndex1;
						brackets[tm.sampleIndex1].sampleIndex = i;
					}
					
				}
			}
		}
		////////////////////////////////////////////////////////////////////
		bFrameStart = false;
		iFrameIndex = 0;

		bool bSkipFrame = false;
		QTreeWidgetItem* itemCurrFrame = nullptr;
		for (int iSamp = 0; iSamp<sampleCount; iSamp++)
		{
			PerfStampData_t& sampleData = samples[iSamp];
			PerfStamp_t & sample = sampleData.stamp;
			unsigned int addr = (unsigned int)sample.File;
			auto kItor = strMaps.find(addr);
			if (kItor != strMaps.end())
			{
				//samples[i].File = kItor->second.c_str();
			}
			else
			{
				//samples[i].File = "Error(File)";
			}

			addr = (unsigned int)sample.Func;
			kItor = strMaps.find(addr);
			if (kItor != strMaps.end())
			{
				//samples[i].Func = kItor->second.c_str();
			}
			else
			{
				//samples[i].Func = "Error(Func)";
			}

			addr = (unsigned int)sample.Label_;
			kItor = strMaps.find(addr);
			if (kItor != strMaps.end())
			{
				//samples[i].Label_ = kItor->second.c_str();
			}
			else
			{
				//samples[i].Label_ = "Error(Label)";
			}
			const char* str = perf->cstr(sample.Label_);

			unsigned short Type = 0x00ff & sample.Type;
			unsigned short Tag = 0xff00 & sample.Type;

		

			if (strcmp(str, "Frame") == 0)
			{
				if (Type == 1)
				{
					if (bFrameStart == false)
						bFrameStart = true;

					if (bFrameStart)
					{
						brackets[iSamp].type = 1;
						iFrameIndex++;
						char buf[256];

						item = new QTreeWidgetItem();
						sprintf(buf,"%d", iFrameIndex);
						item->setText(COL_NUM, buf);

						sprintf(buf, "Frame", iFrameIndex);
						
                        labels.insert("Frame");
						item->setText(COL_LABEL, buf);
						itemCurrFrame = item;
						ui->treeWidget->addTopLevelItem(item);
						bSkipFrame = false;
					}
					
				}
				else if (Type == 2)
				{
					
					if (bFrameStart)
					{
						assert(iFrameIndex > 0);
						//assert(iFrameIndex == frameTimes.size());
						TimeRange_t& tm = frameTimes[iFrameIndex - 1];
						TimeRange_t& tm0 = frameTimes[0];

						//QString text = item->text(0);
						//QString text1 = str;
						//assert(text == text1);
						unsigned long long dTime = tm.t2 - tm.t1;
						unsigned long long dTime0 = tm.t1 - tm0.t1;
						char tmBuf[256];
						sprintf(tmBuf, "%ld", dTime0 );
						if (dTime0 < 0)
						{
							printf("");
						}
						item->setText(COL_TIME_START_MSEC, tmBuf);
						float time = (dTime) / 1000.f;
						sprintf(tmBuf, "%f", time); 
						item->setText(COL_TIME_SEC, tmBuf);
						sprintf(tmBuf, "%ld", dTime);
						item->setText(COL_TIME_MSEC, tmBuf);
						
						item->setText(COL_PERCENT, "100");
						item = nullptr;
						bSkipFrame = false;
					}				
				}
			}
			else if (bFrameStart && iFrameIndex>0)
			{
				if (bSkipFrame)
				{
					bFrameStart = false;
					item = nullptr;
					continue;
				}
				if (!item)
					continue;
				TimeRange_t& tm = frameTimes[iFrameIndex - 1];

				if (Type == 3)
				{
					QTreeWidgetItem* childItem = new QTreeWidgetItem();
					//brackets[i].type = 3;
					char buf[256];
					sprintf(buf, "%s", str);
					childItem->setText(COL_LABEL, buf);
					labels.insert(str);


					item->addChild(childItem);

					char tmBuf[256];
					unsigned long long frameTime = tm.t2 - tm.t1;
					unsigned long long dTimeFrame = sample.Time-tm.t1;
					sprintf(tmBuf, "%ld", dTimeFrame);
					childItem->setText(COL_TIME_START_MSEC, "N/A");
					unsigned long long dTime = sample.Time;
					float time = dTime / 1000.f;
					sprintf(tmBuf, "%f", time);

					childItem->setText(COL_TIME_SEC, tmBuf);


					sprintf(tmBuf, "%ld", dTime);
					childItem->setText(COL_TIME_MSEC, tmBuf);

					TimeRange_t& tm = frameTimes[iFrameIndex - 1];

					double percent = frameTime == 0 ? 0 : (double)dTime / (double)(frameTime);

					sprintf(tmBuf, "%10.2lf", percent * 100);

					childItem->setText(COL_PERCENT, tmBuf);

					const char* str = perf->cstr(sample.File);
					if (str)
						sprintf(tmBuf, "%s", str);
					else
						sprintf(tmBuf, "%d", "(null)");

					childItem->setText(COL_DATA1, tmBuf);		

					str = perf->cstr(sample.Func);
					if (str)
						sprintf(tmBuf, "%s", str);
					else
						sprintf(tmBuf, "%d", "(null)");

					childItem->setText(COL_DATA1, tmBuf);
				
				}
				else if (Type == 1)
				{
					QTreeWidgetItem* childItem = new QTreeWidgetItem();
					brackets[iSamp].type = 1;
					char buf[256];
					sprintf(buf, "%s", str);
					childItem->setText(COL_LABEL, buf);
                    labels.insert(str);
					
					item->addChild(childItem);
					labIndex[childItem] = iSamp;
					item = childItem;
				}
				else if (Type == 2)
				{

					QString text = item->text(COL_LABEL);
					QString text1 = str;

					if (text.compare(text1)!=0)
					{
						//QObject* obj = ui->treeWidget->parent();
						if (bDisplayError)
						{
							ErrorDialog ed;


                           QString str = QString("labels aren't match: the current is <%1>,the before is <%2>")
                                     .arg(text)
                                     .arg(text1);


                            ed.setErrorText(str);
							ed.copyTreeWidget(ui->treeWidget);
							int ret = ed.exec();

							if (ret == QDialog::Accepted)
							{
								
							}
							else
							{
								bDisplayError = false;
							}
							

						}
					
						int cc = itemCurrFrame->columnCount();
						for (int j = 0; j < cc; j++)
						{
							itemCurrFrame->setTextColor(j, Qt::red);
						}
						QTreeWidgetItem* it = item;
						while (it)
						{
							it->setTextColor(0, Qt::red);
							it = it->parent();
						}
						//msbx.exec();
						bSkipFrame = true;
					}
					auto kItor = labIndex.find(item);
					assert(kItor != labIndex.end());
					int idx = kItor->second;
					PerfStamp_t& st1 = samples[idx].stamp;
					PerfStamp_t& st2 = sample;

					brackets[iSamp].type = 2;
					brackets[iSamp].sampleIndex = idx;
					brackets[idx].sampleIndex = iSamp;

					std::string file = strMaps[st2.File];
					std::string func = strMaps[st2.Func];

					
					char tmBuf[256];
					unsigned long long frameTime = tm.t2 - tm.t1;
					unsigned long long dTimeFrame = st1.Time - tm.t1;
					sprintf(tmBuf, "%ld", dTimeFrame);
					item->setText(COL_TIME_START_MSEC, tmBuf);
					unsigned long long dTime = st2.Time - st1.Time;
					float time = dTime / 1000.f;
					sprintf(tmBuf, "%f", time);
					
					item->setText(COL_TIME_SEC, tmBuf);


					sprintf(tmBuf, "%ld", dTime);
					item->setText(COL_TIME_MSEC, tmBuf);

					TimeRange_t& tm = frameTimes[iFrameIndex - 1];

					double percent = frameTime == 0?0:(double)dTime / (double)(frameTime);
					
					sprintf(tmBuf, "%10.2lf", percent * 100);

					item->setText(COL_PERCENT, tmBuf);
					
					const char* str = nullptr;
					
					if (Tag == 0)
					{
						perf->cstr(sample.File);

						if (str)
							sprintf(tmBuf, "%s", str);
						else
							sprintf(tmBuf, "%d", "(null)");

						item->setText(COL_DATA1, tmBuf);

						str = perf->cstr(sample.Func);
						if (str)
							sprintf(tmBuf, "%s", str);
						else if (sample.Func)
							sprintf(tmBuf, "%d", "(null)");

						item->setText(COL_DATA2, tmBuf);
					}
					else
					{
						sprintf(tmBuf, "%d", sample.File);
						item->setText(COL_DATA1, tmBuf);
						sprintf(tmBuf, "%d", sample.Func);
						item->setText(COL_DATA2, tmBuf);
					}


					item = item->parent();

				}
				else
				{
					if (bDisplayError)
					{
						ErrorDialog ed;

						QString text = item->text(COL_LABEL);
						QString text1 = str;
						QString str = QString("invaild label type:%d: the current is <%1>,the before is <%2>")
							.arg(Type)
							.arg(text)
							.arg(text1);


						ed.setErrorText(str);
						ed.copyTreeWidget(ui->treeWidget);
						int ret = ed.exec();

						if (ret == QDialog::Accepted)
						{

						}
						else
						{
							bDisplayError = false;
						}
						int cc = itemCurrFrame->columnCount();
						for (int j = 0; j < cc; j++)
						{
							itemCurrFrame->setTextColor(j, Qt::red);
						}
						QTreeWidgetItem* it = item;
						while (it)
						{
							it->setTextColor(0, Qt::red);
							it = it->parent();
						}
					}

					bSkipFrame = true;
				}
			}
			
		}
		int frameCount = frameTimes.size();
		ui->view->setDisplayFrames(sliderCount);
		ui->view->setDisplayStart(0);
		ui->baseSlider->setSliderPosition(0);
		ui->slider->setSliderPosition(0);
		ui->slider->setRange(0, sliderCount);
		ui->baseSlider->setRange(0, frameCount - sliderCount);

        auto kItor = labels.begin();
        while(kItor!=labels.end())
        {
            QAction* act = ui->labelMenu->addAction(kItor->c_str());
			
			connect(ui->labelMenu, SIGNAL(triggered(QAction*)), this, SLOT(onSetLabelMenu(QAction*)));
            kItor++;
        }

		delete str;
		
	}
	file.close();
	
}
void MainWindow::onSetLabelMenu(QAction*act)
{
	QString strLabel = act->text();
	
	ui->view->setLabel(strLabel);

	QTreeWidgetItem* item = ui->treeWidget->currentItem();
	while (item->parent())
		item = item->parent();
	if (item)
	{
		ui->treeWidget->clearSelection();
		ui->treeWidget->collapseAll();
		item = onSetLabel(item, strLabel);
		item->setSelected(true);
		ui->treeWidget->scrollToItem(item);
		ui->treeWidget->repaint();
	}

	

}
void MainWindow::on_MainWindow_iconSizeChanged(const QSize &iconSize)
{

}

void MainWindow::resizeEvent(QResizeEvent *event)
{
	int widSlider = ui->baseSlider->size().width();
	int sliderCount = widSlider / 16;
	int cFrames = PerfData::instance()->frameTimes().size();
	ui->baseSlider->setRange(0, cFrames - sliderCount);
	ui->slider->setRange(0, sliderCount);
	ui->view->setDisplayFrames(sliderCount);

}

void MainWindow::updateStatusBar(int iFrame, int cFrames)
{
	char buf[256];
	PerfData* perf = PerfData::instance();
	int tm = 0;
	QString labStr = ui->view->label();
	int count = 0;

	if (labStr.compare("Frame") == 0)
	{
		tm = perf->frameTime(iFrame);
		sprintf(buf, "  label(%s) time(%d) frame(%d) total(%d) ", labStr.toLocal8Bit().toStdString().c_str(), tm, iFrame, cFrames);
	}
	else
	{
		tm = (int)perf->frameLabelTime(iFrame, labStr, count);
		if (count > 1)
		{
			float avg = (float)tm / (float)count;
			sprintf(buf, "  label(%s) time(%d) count(%d) avg(%10.2f) frame(%d) total(%d) ", labStr.toLocal8Bit().toStdString().c_str(), tm, count,avg, iFrame, cFrames);
		}
		else
		{
			sprintf(buf, "  label(%s) time(%d) frame(%d) total(%d) ", labStr.toLocal8Bit().toStdString().c_str(), tm, iFrame, cFrames);
		}
	}
	
	_statusPane1->setText(buf);
}

void MainWindow::on_treeWidget_itemSelectionChanged()
{
	_bSelectChange = true;
	if (_bMoveSlider == false)
	{
		QTreeWidgetItem* item = ui->treeWidget->currentItem();
		if (item->parent() == nullptr)
		{
			ui->view->setLabel("Frame");
		}
		else
		{
			QString label = item->text(0);
			ui->view->setLabel(label);
		}
		QTreeWidgetItem*itemTopLevel = item;
		while (itemTopLevel->parent()){
			itemTopLevel = itemTopLevel->parent();
		}

		if (itemTopLevel)
		{
			int index = ui->treeWidget->indexOfTopLevelItem(itemTopLevel);
			int maxSlider = ui->baseSlider->maximum();
			int dispFrames = ui->view->displayFrames();
			int dispStart = ui->view->displayStart();

			if (index >= dispStart && index - dispStart < dispFrames)
			{
				ui->slider->setSliderPosition(index - dispStart);
			}
			else
			{	
				ui->view->setDisplayStart(index);	    
				ui->baseSlider->setSliderPosition(index);
				ui->slider->setSliderPosition(0);
			}	
			int cFrames = PerfData::instance()->frameTimes().size();
			
			updateStatusBar(index, cFrames);

			ui->view->setCurrentFrame(index);
		}
		ui->view->repaint();
	}
	_bSelectChange = false;
}

void MainWindow::on_treeWidget_clicked(const QModelIndex &index)
{
	
	
	ui->view->repaint();
}

void MainWindow::on_treeWidget_doubleClicked(const QModelIndex &index)
{	
	ui->view->repaint();
}
QTreeWidgetItem* MainWindow::onSetLabel(QTreeWidgetItem* item,QString strLabel)
{
	QTreeWidgetItem* resItem = _findTreeItems(item, strLabel);
	QTreeWidgetItem* tarItem = resItem;
	while (tarItem)
	{

		QTreeWidgetItem*parItem = tarItem->parent();
		if (parItem)
			parItem->setExpanded(true);
		tarItem = parItem;
	}
	if (resItem)
		item = resItem;
	return item;
}
void MainWindow::updateTreeWidget(int iFrame, int cFrames)
{
	PerfData* perf = PerfData::instance();
	ui->treeWidget->clearSelection();
	ui->treeWidget->collapseAll();
	QTreeWidgetItem* item = ui->treeWidget->topLevelItem(iFrame);
	if (item)
	{
		
		QString strLabel = ui->view->label();
		
		if (strLabel.compare("Frame") != 0)
		{
			item = onSetLabel(item, strLabel);
		}

		item->setSelected(true);
		ui->treeWidget->scrollToItem(item);
	}
}
void MainWindow::on_baseSlider_sliderMoved(int position)
{
	//int widSlider = ui->baseSlider->size().width();
	//int sliderCount = widSlider / 16;
	_bMoveSlider = true;
	if (_bSelectChange == false)
	{
		ui->view->setDisplayStart(position);
		
		int cFrames = PerfData::instance()->frameTimes().size();
		char buf[256];	
		int pos = ui->slider->sliderPosition() + position;
			
		updateStatusBar(pos, cFrames);

		ui->view->setCurrentFrame(pos);
		ui->view->repaint();
		
		updateTreeWidget(pos, cFrames);
	}
	_bMoveSlider = false;
}

void MainWindow::on_slider_sliderMoved(int position)
{
	_bMoveSlider = true;
	if (_bSelectChange == false)
	{
		int cFrames = PerfData::instance()->frameTimes().size();
		char buf[256];
		int pos = ui->baseSlider->sliderPosition() + position;
		
		updateStatusBar(pos, cFrames);

		ui->view->setCurrentFrame(pos);
		ui->view->repaint();

		updateTreeWidget(pos, cFrames);
	}
	_bMoveSlider = false;
}


