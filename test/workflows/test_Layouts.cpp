/**
 * @file test_Layouts.cpp
 * @brief Saved layouts come back after a restart with every dock where the user left it.
 */

#include "Fixtures.h"
#include "TestEnv.h"
#include "UiHelpers.h"

#include "common/CutterSeekable.h"
#include "core/MainWindow.h"
#include "widgets/CommentsWidget.h"
#include "widgets/Dashboard.h"
#include "widgets/FunctionsWidget.h"
#include "widgets/ImportsWidget.h"
#include "widgets/SearchWidget.h"
#include "widgets/SectionsWidget.h"
#include "widgets/StringsWidget.h"

#include <algorithm>
#include <QInputDialog>
#include <QMenu>
#include <QPointer>
#include <QTest>

namespace {

constexpr int settleMs = 200;
constexpr int rightAreaWidth = 360;
constexpr int bottomAreaHeight = 220;
constexpr int sizeTolerance = 4;

bool triggerAction(MainWindow *main, const char *objectName)
{
    auto *action = main->findChild<QAction *>(objectName);
    if (!action) {
        return false;
    }
    action->trigger();
    return true;
}

bool applyLayout(MainWindow *main, const QString &name)
{
    auto *menu = main->findChild<QMenu *>("menuLayouts");
    if (!menu) {
        return false;
    }
    const QList<QAction *> actions = menu->actions();
    for (QAction *action : actions) {
        if (action->text() == name) {
            action->trigger();
            return true;
        }
    }
    return false;
}

bool saveLayout(MainWindow *main, const QString &name)
{
    QObject context;
    CutterTest::onceVisible<QInputDialog>(&context, [name](QInputDialog *dialog) {
        dialog->setTextValue(name);
        dialog->accept();
    });
    return triggerAction(main, "actionSaveLayout");
}

QString areaName(Qt::DockWidgetArea area)
{
    switch (area) {
    case Qt::LeftDockWidgetArea:
        return "left";
    case Qt::RightDockWidgetArea:
        return "right";
    case Qt::TopDockWidgetArea:
        return "top";
    case Qt::BottomDockWidgetArea:
        return "bottom";
    default:
        return "none";
    }
}

/** One line per dock: area, floating, visible, tab group and sync, sorted by name. */
QStringList describeDocks(MainWindow *main)
{
    QStringList lines;
    const QList<QDockWidget *> docks = main->findChildren<QDockWidget *>();
    for (QDockWidget *dock : docks) {
        if (dock->parentWidget() != main && !dock->isFloating()) {
            continue;
        }
        QStringList tabs;
        const QList<QDockWidget *> tabbed = main->tabifiedDockWidgets(dock);
        for (QDockWidget *other : tabbed) {
            tabs << other->objectName();
        }
        tabs.sort();

        QString sync = "-";
        if (auto *addressable = qobject_cast<AddressableDockWidget *>(dock)) {
            sync = addressable->getSeekable()->isSynchronized() ? "synced" : "unsynced";
        }

        lines << QString("%1: %2 %3 %4 tabs=[%5] %6")
                         .arg(dock->objectName(), areaName(main->dockWidgetArea(dock)),
                              dock->isFloating() ? "floating" : "docked",
                              dock->isVisible() ? "visible" : "hidden", tabs.join(','), sync);
    }
    lines.sort();
    return lines;
}

} // namespace

class TestLayouts : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();

    /**
     * @brief A layout using every area, tabs, floating, hidden and extra docks comes back.
     */
    void complexLayoutIsRestored();

private:
    template<class Dock>
    Dock *dock();
    bool saveRestartAndApply(const QString &name, const QList<QPointer<QDockWidget>> &extras);

    MainWindow *main = nullptr;
};

void TestLayouts::initTestCase()
{
    main = CutterTest::mainWindow();
    QVERIFY(main);
    QTRY_VERIFY(main->isVisible());
}

void TestLayouts::cleanup()
{
    triggerAction(main, "actionDefaultLayout");
}

template<class Dock>
Dock *TestLayouts::dock()
{
    return main->findChild<Dock *>();
}

bool TestLayouts::saveRestartAndApply(const QString &name,
                                      const QList<QPointer<QDockWidget>> &extras)
{
    if (!saveLayout(main, name)) {
        return false;
    }
    main->readSettings();
    if (!triggerAction(main, "actionDefaultLayout")) {
        return false;
    }
    const bool extrasClosed = QTest::qWaitFor([&extras]() {
        return std::all_of(extras.begin(), extras.end(),
                           [](const QPointer<QDockWidget> &extra) { return extra.isNull(); });
    });
    return extrasClosed && applyLayout(main, name);
}

/**
 * @brief A layout using every dock area is restored exactly after a restart.
 *
 * The layout has tab groups, a floating dock, a hidden default dock and extra memory widgets. It is
 * saved, the settings are reloaded as a restart would, the default layout is applied and then the
 * saved one, and every dock is compared with what was saved.
 */
void TestLayouts::complexLayoutIsRestored()
{
    auto *functions = dock<FunctionsWidget>();
    auto *imports = dock<ImportsWidget>();
    auto *strings = dock<StringsWidget>();
    auto *comments = dock<CommentsWidget>();
    auto *search = dock<SearchWidget>();
    auto *sections = dock<SectionsWidget>();
    auto *dashboard = dock<Dashboard>();
    QVERIFY(functions && imports && strings && comments && search && sections && dashboard);

    const RVA offset = CutterTest::core()->getOffset();
    QPointer<QDockWidget> extraDisassembly =
            main->addNewMemoryWidget(MemoryWidgetType::Disassembly, offset, false);
    QPointer<QDockWidget> extraGraph =
            main->addNewMemoryWidget(MemoryWidgetType::Graph, offset, true);
    QPointer<QDockWidget> extraHexdump =
            main->addNewMemoryWidget(MemoryWidgetType::Hexdump, offset, false);

    main->addDockWidget(Qt::LeftDockWidgetArea, functions);
    main->tabifyDockWidget(functions, imports);

    main->addDockWidget(Qt::RightDockWidgetArea, strings);
    main->tabifyDockWidget(strings, extraDisassembly);

    main->addDockWidget(Qt::TopDockWidgetArea, sections);

    main->addDockWidget(Qt::BottomDockWidgetArea, search);
    main->tabifyDockWidget(search, comments);
    main->tabifyDockWidget(search, extraGraph);

    extraHexdump->setFloating(true);
    extraHexdump->resize(400, 300);

    dashboard->close();
    const QList<QDockWidget *> shownDocks = { functions, imports,  strings,
                                              sections,  search, comments };
    for (QDockWidget *shown : shownDocks) {
        shown->show();
    }

    main->resizeDocks({ strings }, { rightAreaWidth }, Qt::Horizontal);
    main->resizeDocks({ search }, { bottomAreaHeight }, Qt::Vertical);
    QTest::qWait(settleMs);

    const QStringList saved = describeDocks(main);
    const int savedWidth = strings->width();
    const int savedHeight = search->height();

    QVERIFY(saveRestartAndApply("cutter-test-complex",
                                { extraDisassembly, extraGraph, extraHexdump }));
    QTest::qWait(settleMs);

    QCOMPARE(describeDocks(main), saved);
    QCOMPARE(strings->width(), savedWidth);
    QCOMPARE(search->height(), savedHeight);
}

CUTTER_APP_TEST_MAIN_WITH_FILE(TestLayouts, CutterTest::Fixtures::helloX64())
#include "test_Layouts.moc"
