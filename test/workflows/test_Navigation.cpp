/**
 * @file test_Navigation.cpp
 * @brief Jumping through the binary from lists, history and xrefs moves the views.
 */

#include "Fixtures.h"
#include "TestEnv.h"
#include "ViewHelpers.h"

#include "dialogs/XrefsDialog.h"
#include "widgets/CutterDockWidget.h"
#include "widgets/FunctionsWidget.h"
#include "widgets/MemoryDockWidget.h"

#include <QAction>
#include <QScopeGuard>
#include <QTest>
#include <QTreeView>

namespace {

constexpr int viewRefreshTimeoutMs = 10000;
constexpr int listRefreshTimeoutMs = 2000;
constexpr int minimumCheckedLists = 3;

QModelIndex rowAwayFrom(QAbstractItemView *view, RVA current)
{
    auto *addressable = dynamic_cast<AddressableItemModelI *>(view->model());
    for (int row = 0; row < view->model()->rowCount(); ++row) {
        const QModelIndex index = view->model()->index(row, 0);
        const RVA address = addressable->address(index);
        if (address != RVA_INVALID && address != current) {
            return index;
        }
    }
    return {};
}

} // namespace

class TestNavigation : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void activatingFunctionSyncsDisassemblyAndGraph();
    void activatingAnyAddressableListRowSeeks();
    void backAndForwardMoveTheViews();
    void activatingXrefSeeksToReference();

private:
    MainWindow *main = nullptr;
    DisassemblyWidget *disassembly = nullptr;
    GraphWidget *graph = nullptr;
};

void TestNavigation::initTestCase()
{
    main = CutterTest::mainWindow();
    QVERIFY(main);
    disassembly = CutterTest::findSynchronizedDock<DisassemblyWidget>(main);
    graph = CutterTest::findSynchronizedDock<GraphWidget>(main);
    QVERIFY(disassembly);
    QVERIFY(graph);
}

/**
 * @brief Double-clicking a function in Functions shows it in the Disassembly and the Graph.
 */
void TestNavigation::activatingFunctionSyncsDisassemblyAndGraph()
{
    auto *functions = main->findChild<FunctionsWidget *>();
    QVERIFY(functions);

    disassembly->raiseMemoryWidget();
    QTRY_VERIFY(!disassembly->getLines().isEmpty());
    const QSet<RVA> visibleBefore = CutterTest::disassemblyOffsets(disassembly);

    functions->raiseMemoryWidget();
    auto *view = functions->findChild<QTreeView *>();
    QVERIFY(view);
    QTRY_VERIFY(view->model()->rowCount() > 0);
    auto *addressable = dynamic_cast<AddressableItemModelI *>(view->model());
    QVERIFY(addressable);

    QModelIndex targetIndex;
    RVA target = RVA_INVALID;
    for (int row = 0; row < view->model()->rowCount(); ++row) {
        const QModelIndex index = view->model()->index(row, 0);
        const RVA address = addressable->address(index);
        if (address != RVA_INVALID && address != CutterTest::core()->getOffset()
            && !visibleBefore.contains(address) && !CutterTest::graphHasBlock(graph, address)) {
            targetIndex = index;
            target = address;
            break;
        }
    }
    QVERIFY2(targetIndex.isValid(), "no function outside the currently displayed code");

    emit view->activated(targetIndex);
    QCOMPARE(CutterTest::core()->getOffset(), target);

    disassembly->raiseMemoryWidget();
    QTRY_VERIFY_WITH_TIMEOUT(CutterTest::disassemblyOffsets(disassembly).contains(target),
                             viewRefreshTimeoutMs);

    graph->raiseMemoryWidget();
    QTRY_VERIFY_WITH_TIMEOUT(CutterTest::graphHasBlock(graph, target), viewRefreshTimeoutMs);
}

/**
 * @brief Activating a row in any list dock seeks to its address.
 */
void TestNavigation::activatingAnyAddressableListRowSeeks()
{
    int checked = 0;
    const QList<CutterDockWidget *> docks = main->findChildren<CutterDockWidget *>();
    for (CutterDockWidget *dock : docks) {
        if (qobject_cast<MemoryDockWidget *>(dock)) {
            continue;
        }
        QAbstractItemView *view = CutterTest::addressableView(dock);
        if (!view) {
            continue;
        }

        const bool wasOpen = dock->toggleViewAction()->isChecked();
        const auto restore = qScopeGuard([dock, wasOpen]() {
            if (!wasOpen) {
                dock->toggleDockWidget(false);
            }
        });
        dock->toggleDockWidget(true);
        dock->raiseMemoryWidget();
        if (!CutterTest::waitForRows(view->model(), listRefreshTimeoutMs)) {
            continue;
        }

        const QModelIndex index = rowAwayFrom(view, CutterTest::core()->getOffset());
        if (!index.isValid()) {
            continue;
        }
        const RVA target = dynamic_cast<AddressableItemModelI *>(view->model())->address(index);

        emit view->activated(index);
        QVERIFY2(CutterTest::core()->getOffset() == target,
                 qPrintable(QStringLiteral("activating a row in '%1' did not seek to it")
                                    .arg(dock->windowTitle())));
        ++checked;
    }
    QVERIFY2(checked >= minimumCheckedLists, "too few list docks had rows to activate");
}

/**
 * @brief Back and Forward move the core and the Disassembly.
 */
void TestNavigation::backAndForwardMoveTheViews()
{
    CutterCore *core = CutterTest::core();
    RVA first = RVA_INVALID;
    RVA second = RVA_INVALID;
    const QList<FunctionDescription> functions = core->getAllFunctions();
    for (const FunctionDescription &function : functions) {
        if (function.offset == core->getOffset()) {
            continue;
        }
        if (first == RVA_INVALID) {
            first = function.offset;
        } else if (second == RVA_INVALID) {
            second = function.offset;
        }
    }
    QVERIFY2(second != RVA_INVALID, "fixture needs at least three analyzed functions");

    auto *back = main->findChild<QAction *>("actionBackward");
    auto *forward = main->findChild<QAction *>("actionForward");
    QVERIFY(back);
    QVERIFY(forward);

    core->seek(first);
    core->seek(second);
    disassembly->raiseMemoryWidget();
    QTRY_VERIFY_WITH_TIMEOUT(CutterTest::disassemblyOffsets(disassembly).contains(second),
                             viewRefreshTimeoutMs);

    back->trigger();
    QCOMPARE(core->getOffset(), first);
    QTRY_VERIFY_WITH_TIMEOUT(CutterTest::disassemblyOffsets(disassembly).contains(first),
                             viewRefreshTimeoutMs);

    forward->trigger();
    QCOMPARE(core->getOffset(), second);
    QTRY_VERIFY_WITH_TIMEOUT(CutterTest::disassemblyOffsets(disassembly).contains(second),
                             viewRefreshTimeoutMs);
}

/**
 * @brief Activating an xref seeks to the referencing side.
 */
void TestNavigation::activatingXrefSeeksToReference()
{
    CutterCore *core = CutterTest::core();
    XrefsDialog dialog(main);
    auto *toView = dialog.findChild<QAbstractItemView *>("toTreeWidget");
    QVERIFY(toView);

    QModelIndex index;
    const QList<FunctionDescription> functions = core->getAllFunctions();
    for (const FunctionDescription &function : functions) {
        dialog.fillRefsForAddress(function.offset, function.name, false);
        index = rowAwayFrom(toView, core->getOffset());
        if (index.isValid()) {
            break;
        }
    }
    QVERIFY2(index.isValid(), "no function with a reference to it");
    const RVA source = dynamic_cast<AddressableItemModelI *>(toView->model())->address(index);

    dialog.show();
    emit toView->activated(index);
    QCOMPARE(core->getOffset(), source);

    disassembly->raiseMemoryWidget();
    QTRY_VERIFY_WITH_TIMEOUT(CutterTest::disassemblyOffsets(disassembly).contains(source),
                             viewRefreshTimeoutMs);
}

CUTTER_APP_TEST_MAIN_WITH_FILE(TestNavigation, CutterTest::Fixtures::helloX64())
#include "test_Navigation.moc"
