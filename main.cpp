#include "LoyaltyPointAccountant.h"
#include <QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	LoyaltyPointAccountant w;
	w.show();

	return a.exec();
}
