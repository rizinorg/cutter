/**
 * @file test_Docking.cpp
 * @brief Cutter's own dock behavior: the docking modifier and closing extra docks.
 */

#include "Fixtures.h"
#include "TestEnv.h"

#include "core/MainWindow.h"
#include "shortcuts/ShortcutManager.h"
#include "widgets/CutterDockWidget.h"
#include "widgets/MemoryDockWidget.h"

#include <QAction>
#include <QPointer>
#include <QScopeGuard>
#include <QTest>
#include <QWindow>

namespace {

constexpr int moveStep = 40;

Qt::Key modifierKey(Qt::KeyboardModifier modifier)
{
    switch (modifier) {
    case Qt::ShiftModifier:
        return Qt::Key_Shift;
    case Qt::ControlModifier:
        return Qt::Key_Control;
    case Qt::AltModifier:
        return Qt::Key_Alt;
    case Qt::MetaModifier:
        return Qt::Key_Meta;
    default:
        return Qt::Key_unknown;
    }
}

} // namespace

class TestDocking : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    /**
     * @brief Moving a floating dock with the modifier held blocks docking; releasing allows it.
     */
    void modifierHeldBlocksDockingWhileMoving();
    /**
     * @brief A closed extra dock is deleted and gone from layout save and layout switch.
     */
    void closedExtraDockLeavesNoDanglingDock();

private:
    MainWindow *main = nullptr;
};

void TestDocking::initTestCase()
{
    main = CutterTest::mainWindow();
    QVERIFY(main);
    QTRY_VERIFY(main->isVisible());
}

/**
 * @brief Holding the Docking.toggle modifier keeps a moved floating dock from docking.
 */
void TestDocking::modifierHeldBlocksDockingWhileMoving()
{
    const Qt::KeyboardModifier modifier =
            Shortcuts()->convertKeyToModifer(Shortcuts()->getKeySequence("Docking.toggle"));
    const Qt::Key key = modifierKey(modifier);
    if (key == Qt::Key_unknown) {
        QSKIP("Docking.toggle is not bound to a modifier key");
    }

    QPointer<QDockWidget> dock = main->addNewMemoryWidget(
            MemoryWidgetType::Hexdump, CutterTest::core()->getOffset(), false);
    const auto closeDock = qScopeGuard([&dock]() {
        if (dock) {
            dock->close();
        }
    });
    dock->setFloating(true);
    QTRY_VERIFY(dock->isFloating() && dock->windowHandle());
    QWindow *window = dock->windowHandle();

    QTest::keyPress(window, key, modifier);
    dock->move(dock->pos() + QPoint(moveStep, moveStep));
    QTRY_COMPARE(dock->allowedAreas(), Qt::NoDockWidgetArea);
    QVERIFY(dock->isFloating());

    QTest::keyRelease(window, key, Qt::NoModifier);
    dock->move(dock->pos() + QPoint(moveStep, moveStep));
    QTRY_COMPARE(dock->allowedAreas(), Qt::AllDockWidgetAreas);
}

/**
 * @brief A closed extra dock is deleted and leaves the main window's dock list.
 *
 * Otherwise the next layout save or switch walks a deleted widget.
 */
void TestDocking::closedExtraDockLeavesNoDanglingDock()
{
    QPointer<MemoryDockWidget> extra = main->addNewMemoryWidget(
            MemoryWidgetType::Disassembly, CutterTest::core()->getOffset(), false);
    QVERIFY(extra);
    const QString name = extra->objectName();

    extra->close();
    QTRY_VERIFY2(extra.isNull(), "the closed extra dock was not deleted");

    main->saveSettings();
    auto *defaultLayout = main->findChild<QAction *>("actionDefaultLayout");
    QVERIFY(defaultLayout);
    defaultLayout->trigger();

    QVERIFY(!main->findChild<QDockWidget *>(name));
}

CUTTER_APP_TEST_MAIN_WITH_FILE(TestDocking, CutterTest::Fixtures::helloX64())
#include "test_Docking.moc"
