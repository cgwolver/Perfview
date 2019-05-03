#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
	//-platformpluginpath plugins\platforms - platform windows
	char** new_argv = new char*[argc + 2];
	for (int i = 0; i < argc; i++)
		new_argv[i] = argv[i];
	new_argv[argc] = new char[64];
	strcpy(new_argv[argc],"-platformpluginpath");
	new_argv[argc + 1] = new char[64];
	strcpy(new_argv[argc + 1], "plugins\\platforms");
	int new_argc = argc + 2;
	QApplication a(new_argc, new_argv);
    MainWindow w;
    w.show();

    return a.exec();
}
