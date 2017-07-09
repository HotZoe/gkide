#include <QtTest/QtTest>
#include "shellwidget.h"

class Test: public QObject
{
	Q_OBJECT
private slots:
    void clearRegion(void);
};


void Test::clearRegion(void)
{
	ShellWidget *w = new ShellWidget();
	w->show();
	w->resizeShell(2, 2);
}

QTEST_MAIN(Test)
#include "test_shellwidget.moc"
