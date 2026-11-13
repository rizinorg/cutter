/**
 * @file test_Shortcuts.cpp
 * @brief Keyboard shortcuts reach the widget that has focus, and only that one.
 */

#include "Fixtures.h"
#include "TestEnv.h"
#include "ViewHelpers.h"

#include "dialogs/TypesVariablesDialog.h"
#include "dialogs/XrefsDialog.h"
#include "shortcuts/ShortcutManager.h"
#include "widgets/AbstractFilterView.h"
#include "widgets/CutterDockWidget.h"
#include "widgets/HexdumpWidget.h"
#include "widgets/QuickFilterView.h"

#include <QApplication>
#include <QClipboard>
#include <QHash>
#include <QPointer>
#include <QScopeGuard>
#include <QTest>

#include <memory>

namespace {

constexpr int copiedBytes = 4;

bool focusIsWithin(QWidget *widget)
{
    QWidget *focus = QApplication::focusWidget();
    return focus && (focus == widget || widget->isAncestorOf(focus));
}

/** The list a filter bar filters: the item view closest to it among its siblings. */
QAbstractItemView *pairedList(QWidget *filter)
{
    for (QWidget *child = filter; child->parentWidget(); child = child->parentWidget()) {
        const QObjectList siblings = child->parentWidget()->children();
        const qsizetype own = siblings.indexOf(child);
        QAbstractItemView *closest = nullptr;
        qsizetype distance = siblings.size();
        for (qsizetype i = 0; i < siblings.size(); ++i) {
            auto *view = qobject_cast<QAbstractItemView *>(siblings.at(i));
            if (view && qAbs(i - own) < distance) {
                closest = view;
                distance = qAbs(i - own);
            }
        }
        if (closest) {
            return closest;
        }
    }
    return nullptr;
}

} // namespace

class TestShortcuts : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();
    void hexdumpCopyShortcutCopiesSelection();
    /**
     * @brief Every filter bar: Find shows and focuses it, Esc clears it, Esc again closes it
     * (if it can close), and Find brings it back. Other filter bars stay as they were.
     */
    void filterBarKeyboardCycle_data();
    void filterBarKeyboardCycle();

private:
    QString filterRowName(AbstractFilterView *filter) const;

    MainWindow *main = nullptr;
    std::unique_ptr<XrefsDialog> xrefs;
    std::unique_ptr<TypesVariablesDialog> typeVariables;
    QMap<QString, QPointer<AbstractFilterView>> filters;
};

void TestShortcuts::initTestCase()
{
    main = CutterTest::mainWindow();
    QVERIFY(main);
    main->activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(main));

    xrefs = std::make_unique<XrefsDialog>(main);
    xrefs->fillRefsForAddress(CutterTest::core()->getOffset(), "cutter_test", true);
    typeVariables = std::make_unique<TypesVariablesDialog>(main, "int");

    const QList<AbstractFilterView *> bars = main->findChildren<AbstractFilterView *>();
    for (AbstractFilterView *bar : bars) {
        filters.insert(filterRowName(bar), bar);
    }
}

void TestShortcuts::cleanup()
{
    const QWidgetList windows = QApplication::topLevelWidgets();
    for (QWidget *window : windows) {
        if (window != main && window->isVisible()) {
            window->hide();
        }
    }
    main->activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(main));
}

/**
 * @brief The copy shortcut in the Hexdump copies the selected bytes.
 *
 * To reproduce:
 * 1. Select bytes in the Hexdump.
 * 2. Press the copy shortcut: the clipboard didn't change.
 */
void TestShortcuts::hexdumpCopyShortcutCopiesSelection()
{
    auto *hexdump = CutterTest::findSynchronizedDock<HexdumpWidget>(main);
    QVERIFY(hexdump);
    hexdump->raiseMemoryWidget();
    auto *hex = hexdump->findChild<HexWidget *>();
    QVERIFY(hex);

    const QList<QKeySequence> keys = Shortcuts()->getKeySequences("Hex.copy");
    QVERIFY(!keys.isEmpty());

    const RVA start = CutterTest::core()->getOffset();
    hex->selectRange(start, start + copiedBytes - 1);
    const QString expected =
            QString::fromLatin1(CutterTest::core()->ioRead(start, copiedBytes).toHex());

    QApplication::clipboard()->clear();
    hex->setFocus(Qt::OtherFocusReason);
    QTRY_VERIFY(focusIsWithin(hex));

    QTest::keySequence(QApplication::focusWidget(), keys.first());
    QTRY_COMPARE(QApplication::clipboard()->text(), expected);
}

QString TestShortcuts::filterRowName(AbstractFilterView *filter) const
{
    QString owner;
    if (filter->window() == main) {
        for (QWidget *parent = filter->parentWidget(); parent; parent = parent->parentWidget()) {
            if (qobject_cast<CutterDockWidget *>(parent)) {
                owner = parent->objectName();
                break;
            }
        }
    } else {
        owner = filter->window()->metaObject()->className();
    }
    return owner + '/' + filter->objectName();
}

void TestShortcuts::filterBarKeyboardCycle_data()
{
    QTest::addColumn<QString>("filter");
    for (auto it = filters.cbegin(); it != filters.cend(); ++it) {
        QTest::newRow(qPrintable(it.key())) << it.key();
    }
}

/**
 * @brief Find focuses a filter bar, and Esc clears it, then closes it and returns to the list.
 *
 * Every filter bar in a dock or in the dialogs opened in initTestCase is a data row.
 *
 * To reproduce:
 * 1. Open the X-Refs dialog and press Find: the filter bar wasn't focused.
 * 2. Press Esc: the filter was neither cleared nor closed.
 */
void TestShortcuts::filterBarKeyboardCycle()
{
    QFETCH(const QString, filter);
    AbstractFilterView *bar = filters.value(filter);
    QVERIFY(bar);

    QWidget *window = bar->window();
    if (window == main) {
        auto *dock = bar->parentWidget();
        while (dock && !qobject_cast<CutterDockWidget *>(dock)) {
            dock = dock->parentWidget();
        }
        QVERIFY(dock);
        static_cast<CutterDockWidget *>(dock)->toggleDockWidget(true);
        static_cast<CutterDockWidget *>(dock)->raiseMemoryWidget();
    } else {
        window->show();
    }
    const auto hideDialog = qScopeGuard([window, this]() {
        if (window != main) {
            window->hide();
        }
    });
    window->activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(window));

    QAbstractItemView *list = pairedList(bar);
    QVERIFY2(list, "no list next to the filter bar");
    const bool closable = qobject_cast<QuickFilterView *>(bar) != nullptr;
    if (closable) {
        bar->closeFilter();
    } else {
        bar->clearFilter();
    }

    QHash<AbstractFilterView *, bool> othersVisible;
    for (AbstractFilterView *other : std::as_const(filters)) {
        if (other && other != bar) {
            othersVisible.insert(other, other->isVisible());
        }
    }

    const QKeySequence showKey = Shortcuts()->getKeySequences("General.showFilter").value(0);
    const QKeySequence clearKey = Shortcuts()->getKeySequences("General.clearFilter").value(0);
    QVERIFY(!showKey.isEmpty() && !clearKey.isEmpty());

    list->setFocus(Qt::OtherFocusReason);
    QTRY_VERIFY(list->hasFocus());
    QTest::keySequence(list, showKey);
    QTRY_VERIFY2(bar->isVisible() && QApplication::focusWidget() == bar->lineEdit(),
                 "the find shortcut did not show and focus the filter");
    for (auto it = othersVisible.cbegin(); it != othersVisible.cend(); ++it) {
        QVERIFY2(it.key()->isVisible() == it.value(), "the find shortcut changed another filter");
    }

    QTest::keyClicks(bar->lineEdit(), "cutter");
    QTest::keySequence(bar->lineEdit(), clearKey);
    QTRY_VERIFY2(bar->lineEdit()->text().isEmpty(), "Esc did not clear the filter");
    QVERIFY2(window->isVisible(), "Esc closed the window instead of clearing the filter");

    if (!closable) {
        return;
    }
    QTest::keySequence(bar->lineEdit(), clearKey);
    QTRY_VERIFY2(!bar->isVisible(), "Esc on an empty filter did not close it");
    QVERIFY2(window->isVisible(), "Esc closed the window instead of the filter");

    list->setFocus(Qt::OtherFocusReason);
    QTRY_VERIFY(list->hasFocus());
    QTest::keySequence(list, showKey);
    QTRY_VERIFY2(bar->isVisible() && QApplication::focusWidget() == bar->lineEdit(),
                 "the find shortcut did not bring a closed filter back");
}

CUTTER_APP_TEST_MAIN_WITH_FILE(TestShortcuts, CutterTest::Fixtures::helloX64())
#include "test_Shortcuts.moc"
