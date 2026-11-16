/**
 * @file test_LayoutManager.cpp
 * @brief Renaming the user's saved layouts.
 *
 * Covers: LayoutManager.
 */

#include "TestEnv.h"
#include "UiHelpers.h"

#include "common/CutterLayout.h"
#include "dialogs/LayoutManager.h"

#include <QComboBox>
#include <QInputDialog>
#include <QTest>

namespace {

using Layouts = QMap<QString, Cutter::CutterLayout>;

Cutter::CutterLayout layoutWithState(const QByteArray &state)
{
    Cutter::CutterLayout layout;
    layout.state = state;
    return layout;
}

/**
 * @brief Answers the next input dialogs with answers in order, then rejects any more.
 */
void answerInputDialogs(QObject *context, QStringList answers)
{
    auto *timer = new QTimer(context);
    timer->setInterval(20);
    QObject::connect(timer, &QTimer::timeout, timer, [answers]() mutable {
        auto *dialog = CutterTest::findVisibleTopLevel<QInputDialog>();
        if (!dialog) {
            return;
        }
        if (answers.isEmpty()) {
            dialog->reject();
            return;
        }
        dialog->setTextValue(answers.takeFirst());
        dialog->accept();
    });
    timer->start();
}

bool select(LayoutManager &manager, const QString &name)
{
    auto *selector = manager.findChild<QComboBox *>("layoutSelector");
    if (!selector) {
        return false;
    }
    selector->setCurrentText(name);
    return selector->currentText() == name;
}

} // namespace

class TestLayoutManager : public QObject
{
    Q_OBJECT

private slots:
    void renameToSameNameIsNotAnError();
};

/**
 * @brief Confirming the prefilled name when renaming a layout shows no error.
 *
 * Regression: OK on the unchanged name used to warn that the name is already used.
 */
void TestLayoutManager::renameToSameNameIsNotAnError()
{
    Layouts layouts { { "mine", layoutWithState("mine-state") } };
    LayoutManager manager(layouts, nullptr);
    QVERIFY(select(manager, "mine"));

    CutterTest::MessageBoxWatcher warnings;
    QObject context;
    answerInputDialogs(&context, { "mine" });
    QVERIFY(CutterTest::clickButton(&manager, "renameButton"));

    QCOMPARE(warnings.seenTexts(), QStringList());
    QCOMPARE(layouts.keys(), QStringList { "mine" });
    QCOMPARE(layouts.value("mine").state, QByteArray("mine-state"));
}

CUTTER_CORE_TEST_MAIN(TestLayoutManager)
#include "test_LayoutManager.moc"
