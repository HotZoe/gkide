#include <QtTest/QtTest>
#include "cell.h"

class Test: public QObject
{
	Q_OBJECT
private slots:
    void benchCell(void)
    {
        QBENCHMARK
        {
			Cell c('1', Qt::red, Qt::blue, QColor(), false, false, false, false);
		}
	}
};

QTEST_MAIN(Test)
#include "bench_cell.moc"
