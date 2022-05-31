
#include <QtTest>

#include "AutoTest.h"
#include "CutterTest.h"

#include <Cutter.h>

class TestHexdumpWidget: public CutterTest
{
    Q_OBJECT
private slots:
    void initTestCase() override;
    void toUpper();
    void something();
};

void TestHexdumpWidget::initTestCase()
{
    qDebug("TestHexdumpWidget::initTestCase()");
}

void TestHexdumpWidget::toUpper()
{
    QString str = "Hello";
    QCOMPARE(str.toUpper(), QString("HELLO"));
}

void TestHexdumpWidget::something()
{
    QString a = Core()->cmd("fortune");
    qDebug() << "rz returned:" <<  a;
}

DECLARE_TEST(TestHexdumpWidget)
#include "TestHexdumpWidget.moc"

