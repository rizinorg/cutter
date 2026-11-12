/**
 * @file test_EditPropagation.cpp
 * @brief An edit made anywhere reaches every view that shows the edited thing.
 */

#include "Fixtures.h"
#include "TestEnv.h"
#include "ViewHelpers.h"

#include "widgets/CommentsWidget.h"
#include "widgets/DecompilerWidget.h"
#include "widgets/FlagsWidget.h"
#include "widgets/FunctionsWidget.h"
#include "widgets/GlobalsWidget.h"

#include <QPlainTextEdit>
#include <QScopeGuard>
#include <QTest>

#include <functional>

namespace {

constexpr int viewRefreshTimeoutMs = 10000;
constexpr int decompileTimeoutMs = 60000;

const QString disassemblyView = QStringLiteral("Disassembly");
const QString graphView = QStringLiteral("Graph");
const QString decompilerView = QStringLiteral("Decompiler");
const QString functionsView = QStringLiteral("Functions");
const QString commentsView = QStringLiteral("Comments");
const QString flagsView = QStringLiteral("Flags");
const QString globalsView = QStringLiteral("Globals");

QString functionName(CutterCore *core, RVA address)
{
    const QList<FunctionDescription> functions = core->getAllFunctions();
    for (const FunctionDescription &function : functions) {
        if (function.offset == address) {
            return function.name;
        }
    }
    return {};
}

bool graphContains(GraphWidget *graph, const QString &text)
{
    if (graph->windowTitle().contains(text)) {
        return true;
    }
    const auto &blocks = graph->getGraphView()->disassemblyBlocks;
    for (const auto &[address, block] : blocks) {
        if (block.headerText.toQString().contains(text)) {
            return true;
        }
        for (const auto &instruction : block.instrs) {
            if (instruction.fullText.toQString().contains(text)
                || instruction.plainText.contains(text)) {
                return true;
            }
        }
    }
    return false;
}

bool listContains(QWidget *dock, const QString &text)
{
    const QList<QAbstractItemView *> lists = dock->findChildren<QAbstractItemView *>();
    for (QAbstractItemView *list : lists) {
        if (list->model() && CutterTest::modelContains(list->model(), text)) {
            return true;
        }
    }
    return false;
}

/** A view that can show edits: how to bring it up and how to look for text in it. */
struct View
{
    std::function<bool()> available;
    std::function<void()> show;
    std::function<bool(const QString &)> contains;
    int timeoutMs = viewRefreshTimeoutMs;
};

/** One kind of edit: apply() makes it and returns the text views must show. */
struct Edit
{
    std::function<QString()> apply;
    std::function<void()> undo;
    QStringList shownIn;
};

} // namespace

class TestEditPropagation : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();

    void editShowsInView_data();
    void editShowsInView();

private:
    QMap<QString, View> views();
    QMap<QString, Edit> edits();

    MainWindow *main = nullptr;
    DisassemblyWidget *disassembly = nullptr;
    GraphWidget *graph = nullptr;
    DecompilerWidget *decompiler = nullptr;
    QPlainTextEdit *decompiledText = nullptr;
    RVA target = RVA_INVALID;
    RVA instruction = RVA_INVALID;
    QString variable;
};

void TestEditPropagation::initTestCase()
{
    main = CutterTest::mainWindow();
    QVERIFY(main);
    disassembly = CutterTest::findSynchronizedDock<DisassemblyWidget>(main);
    graph = CutterTest::findSynchronizedDock<GraphWidget>(main);
    decompiler = CutterTest::findSynchronizedDock<DecompilerWidget>(main);
    QVERIFY(disassembly);
    QVERIFY(graph);
    if (decompiler) {
        decompiledText = decompiler->findChild<QPlainTextEdit *>("textEdit");
    }

    CutterCore *core = CutterTest::core();
    const QList<FunctionDescription> all = core->getAllFunctions();
    for (const FunctionDescription &function : all) {
        const QList<VariableDescription> variables = core->getVariables(function.offset);
        if (!variables.isEmpty()) {
            target = function.offset;
            variable = variables.first().name;
            break;
        }
    }
    QVERIFY2(target != RVA_INVALID, "no analyzed function with variables");

    init();
    const QList<DisassemblyLine> lines = disassembly->getLines();
    for (const DisassemblyLine &line : lines) {
        if (line.offset > target && core->getFunctionStart(line.offset) == target
            && core->getCommentAt(line.offset).isEmpty()
            && core->listFlagsAsStringAt(line.offset).isEmpty()) {
            instruction = line.offset;
            break;
        }
    }
    QVERIFY2(instruction != RVA_INVALID, "no plain instruction shown in the function");
}

void TestEditPropagation::init()
{
    CutterTest::core()->seek(target);
    disassembly->raiseMemoryWidget();
    QTRY_VERIFY_WITH_TIMEOUT(CutterTest::disassemblyOffsets(disassembly).contains(target),
                             viewRefreshTimeoutMs);
}

QMap<QString, View> TestEditPropagation::views()
{
    const auto always = [] { return true; };
    const auto listView = [always](auto *dock) {
        return View { always,
                      [dock] {
                          dock->toggleDockWidget(true);
                          dock->raiseMemoryWidget();
                      },
                      [dock](const QString &text) { return listContains(dock, text); } };
    };

    return {
        { disassemblyView,
          { always, [this] { disassembly->raiseMemoryWidget(); },
            [this](const QString &text) {
                return CutterTest::disassemblyContains(disassembly, text);
            } } },
        { graphView,
          { always, [this] { graph->raiseMemoryWidget(); },
            [this](const QString &text) { return graphContains(graph, text); } } },
        { decompilerView,
          { [this] { return decompiledText && !CutterTest::core()->getDecompilers().isEmpty(); },
            [this] { decompiler->raiseMemoryWidget(); },
            [this](const QString &text) { return decompiledText->toPlainText().contains(text); },
            decompileTimeoutMs } },
        { functionsView, listView(main->findChild<FunctionsWidget *>()) },
        { commentsView, listView(main->findChild<CommentsWidget *>()) },
        { flagsView, listView(main->findChild<FlagsWidget *>()) },
        { globalsView, listView(main->findChild<GlobalsWidget *>()) },
    };
}

QMap<QString, Edit> TestEditPropagation::edits()
{
    CutterCore *core = CutterTest::core();
    const RVA function = target;
    const RVA address = instruction;
    const QString oldVariable = variable;
    const QString oldFunction = functionName(core, function);
    const QString newFunction = QStringLiteral("cutter_test_function");
    const QString renamedVariable = QStringLiteral("cutter_test_variable");
    const QString comment = QStringLiteral("cutter_test_comment");
    const QString flag = QStringLiteral("cutter_test_flag");
    const QString global = QStringLiteral("cutter_test_global");

    return {
        { "renameFunction",
          { [=] {
               core->renameFunction(function, newFunction);
               return newFunction;
           },
            [=] { core->renameFunction(function, oldFunction); },
            { disassemblyView, graphView, decompilerView, functionsView } } },
        { "renameVariable",
          { [=] {
               core->renameFunctionVariable(renamedVariable, oldVariable, function);
               return renamedVariable;
           },
            [=] { core->renameFunctionVariable(oldVariable, renamedVariable, function); },
            { disassemblyView, graphView, decompilerView } } },
        { "comment",
          { [=] {
               core->setComment(address, comment);
               return comment;
           },
            [=] { core->delComment(address); },
            { disassemblyView, graphView, commentsView } } },
        { "flag",
          { [=] {
               core->addFlag(address, flag, 1);
               return flag;
           },
            [=] { core->delFlag(flag); },
            { disassemblyView, graphView, flagsView } } },
        { "globalVariable",
          { [=] {
               core->addGlobalVariable(address, global, "int");
               return global;
           },
            [=] { core->delGlobalVariable(address); },
            { globalsView } } },
    };
}

void TestEditPropagation::editShowsInView_data()
{
    QTest::addColumn<QString>("edit");
    QTest::addColumn<QString>("view");

    const QMap<QString, Edit> all = edits();
    for (auto it = all.cbegin(); it != all.cend(); ++it) {
        for (const QString &view : it->shownIn) {
            QTest::addRow("%s in %s", qPrintable(it.key()), qPrintable(view)) << it.key() << view;
        }
    }
}

/**
 * @brief One kind of edit shows in one view; the data rows cover every edit in every view.
 *
 * A view that misses a core signal keeps showing the old name, comment or variable. A new view or
 * kind of edit is one entry in views() or edits().
 */
void TestEditPropagation::editShowsInView()
{
    QFETCH(const QString, edit);
    QFETCH(const QString, view);

    const View shownIn = views().value(view);
    QVERIFY2(shownIn.contains, "unknown view");
    if (!shownIn.available()) {
        QSKIP("view not available in this build");
    }
    const Edit change = edits().value(edit);

    shownIn.show();
    const QString text = change.apply();
    const auto undo = qScopeGuard([&]() { change.undo(); });

    QTRY_VERIFY2_WITH_TIMEOUT(shownIn.contains(text), "the view still shows the old state",
                              shownIn.timeoutMs);
}

CUTTER_APP_TEST_MAIN_WITH_FILE(TestEditPropagation, CutterTest::Fixtures::helloX64())
#include "test_EditPropagation.moc"
