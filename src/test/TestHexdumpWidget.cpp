
#include <QtTest>

#include "AutoTest.h"
#include "CutterTest.h"

class TestHexdumpWidget: public CutterTest
{
    Q_OBJECT
private slots:
    void initTestCase() override;
    void toUpper();
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

DECLARE_TEST(TestHexdumpWidget)
#include "TestHexdumpWidget.moc"

