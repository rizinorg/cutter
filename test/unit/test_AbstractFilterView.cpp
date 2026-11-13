/**
 * @file test_AbstractFilterView.cpp
 * @brief The list quick filter: debounce, clear, close, show and its Find/Esc keys.
 */

#include "IsolatedSettings.h"

#include "shortcuts/ShortcutManager.h"
#include "widgets/AbstractFilterView.h"
#include "widgets/ItemCountLineEdit.h"

#include <QDialog>
#include <QLineEdit>
#include <QListView>
#include <QVBoxLayout>
#include <QSignalSpy>
#include <QtTest/QtTest>

namespace {

constexpr int debounceWaitMs = 300;

} // namespace

class TestableFilterView : public AbstractFilterView
{
    Q_OBJECT

public:
    explicit TestableFilterView(QWidget *parent = nullptr, bool closable = false)
        : AbstractFilterView(parent), m_lineEdit(new ItemCountLineEdit(this)), closable(closable)
    {
        setupSharedConnections();
    }

    ItemCountLineEdit *lineEdit() const override { return m_lineEdit; }
    bool isClosable() const override { return closable; }

private:
    ItemCountLineEdit *m_lineEdit; // TODO: rename
    bool closable;
};

/** A filter bar above the list it filters, the layout every list dock uses. */
struct FilteredList
{
    explicit FilteredList(QWidget *window, bool closable)
        : filter(new TestableFilterView(window, closable)), list(new QListView(window))
    {
        auto *layout = qobject_cast<QVBoxLayout *>(window->layout());
        if (!layout) {
            layout = new QVBoxLayout(window);
        }
        layout->addWidget(filter);
        layout->addWidget(list);
    }

    TestableFilterView *filter;
    QListView *list;
};

namespace {

QKeySequence findKey()
{
    return Shortcuts()->getKeySequence("General.showFilter");
}

QKeySequence escKey()
{
    return Shortcuts()->getKeySequence("General.clearFilter");
}

/**
 * @brief Shows the window and waits until it is the active window.
 */
bool showActive(QWidget *window)
{
    window->show();
    window->activateWindow();
    return QTest::qWaitForWindowActive(window);
}

} // namespace

class TestAbstractFilterView : public QObject
{
    Q_OBJECT

private slots:
    void testFilterTextChanged_data();
    void testFilterTextChanged();
    void testRapidTypingEmitsOnce();
    void testClearFilter();
    void testCloseFilter();
    void testShowFilter();

    /**
     * @brief Find pressed in the list shows a hidden bar and focuses it.
     */
    void findInListShowsAndFocusesFilter();
    /**
     * @brief Esc clears the text, then closes a closable bar and refocuses the list.
     */
    void escClearsThenClosesAndReturnsToList();
    /**
     * @brief A bar that can't close keeps Esc free once empty, so a dialog still closes.
     */
    void escOnEmptyUnclosableBarClosesDialog();
    /**
     * @brief Two bars attached to two lists in one window each answer only their own list.
     */
    void twoBarsInOneWindowStaySeparate();
};

void TestAbstractFilterView::testFilterTextChanged_data()
{
    QTest::addColumn<QString>("inputText");
    QTest::addColumn<QString>("expectedEmittedText");

    QTest::newRow("simple_function_search") << "main" << "main";
    QTest::newRow("empty_string_search") << "" << "";
}

/**
 * @brief Typing emits filterTextChanged with the typed text once the debounce ends.
 */
void TestAbstractFilterView::testFilterTextChanged()
{
    QFETCH(QString, inputText);
    QFETCH(QString, expectedEmittedText);

    TestableFilterView filterView;
    filterView.lineEdit()->setText("seed_text");

    QSignalSpy spy(&filterView, &AbstractFilterView::filterTextChanged);

    filterView.lineEdit()->setText(inputText);

    QCOMPARE(spy.count(), 0);

    QVERIFY(spy.wait(debounceWaitMs));

    QCOMPARE(spy.count(), 1);
    QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toString(), expectedEmittedText);
}

/**
 * @brief Several keystrokes within the debounce interval emit filterTextChanged once.
 */
void TestAbstractFilterView::testRapidTypingEmitsOnce()
{
    TestableFilterView filterView;
    QSignalSpy spy(&filterView, &AbstractFilterView::filterTextChanged);

    QTest::keyClicks(filterView.lineEdit(), "main");
    QCOMPARE(filterView.lineEdit()->text(), QString("main"));
    QCOMPARE(spy.count(), 0);

    QVERIFY(spy.wait(debounceWaitMs));
    QTest::qWait(debounceWaitMs);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toString(), QString("main"));
}

/**
 * @brief clearFilter() empties the bar and emits the empty text.
 */
void TestAbstractFilterView::testClearFilter()
{
    TestableFilterView filterView;
    filterView.lineEdit()->setText("pre_existing_filter_query");

    filterView.clearFilter();

    QCOMPARE(filterView.lineEdit()->text(), QString(""));
}

/**
 * @brief closeFilter() hides the bar and clears its text.
 */
void TestAbstractFilterView::testCloseFilter()
{
    TestableFilterView filterView;
    filterView.show();
    filterView.lineEdit()->setText("active_search_string");

    QSignalSpy spy(&filterView, &AbstractFilterView::filterClosed);

    filterView.closeFilter();

    QCOMPARE(filterView.lineEdit()->text(), QString(""));
    QCOMPARE(filterView.isVisible(), false);
    QCOMPARE(spy.count(), 1);
}

/**
 * @brief showFilter() shows the bar and focuses its text field.
 */
void TestAbstractFilterView::testShowFilter()
{
    TestableFilterView filterView;
    filterView.hide();

    filterView.showFilter();

    QCOMPARE(filterView.isVisible(), true);
    QCOMPARE(filterView.focusWidget(), static_cast<QWidget *>(filterView.lineEdit()));
}

/**
 * @brief Find pressed in the attached list shows the bar and focuses it.
 *
 * attachTo() wires the keys for every view; when each view wired them by hand, the Xrefs dialog
 * shipped without them.
 */
void TestAbstractFilterView::findInListShowsAndFocusesFilter()
{
    QWidget window;
    FilteredList pair(&window, true);
    pair.filter->attachTo(pair.list);
    QVERIFY(showActive(&window));
    pair.filter->hide();

    pair.list->setFocus();
    QTest::keySequence(pair.list, findKey());

    QVERIFY(pair.filter->isVisible());
    QCOMPARE(QApplication::focusWidget(), static_cast<QWidget *>(pair.filter->lineEdit()));
}

/**
 * @brief Esc first clears the text, then closes the bar and gives focus back to the list.
 */
void TestAbstractFilterView::escClearsThenClosesAndReturnsToList()
{
    QWidget window;
    FilteredList pair(&window, true);
    pair.filter->attachTo(pair.list);
    QVERIFY(showActive(&window));

    pair.filter->showFilter();
    QTest::keyClicks(pair.filter->lineEdit(), "main");

    QTest::keySequence(pair.filter->lineEdit(), escKey());
    QVERIFY(pair.filter->lineEdit()->text().isEmpty());
    QVERIFY(pair.filter->isVisible());

    QTest::keySequence(pair.filter->lineEdit(), escKey());
    QVERIFY(!pair.filter->isVisible());
    QCOMPARE(QApplication::focusWidget(), static_cast<QWidget *>(pair.list));

    QTest::keySequence(pair.list, findKey());
    QVERIFY2(pair.filter->isVisible(), "Find did not bring the closed bar back");
}

/**
 * @brief Esc on an empty bar that can't close passes through and closes the dialog.
 */
void TestAbstractFilterView::escOnEmptyUnclosableBarClosesDialog()
{
    QDialog dialog;
    FilteredList pair(&dialog, false);
    pair.filter->attachTo(pair.list, &dialog);
    QVERIFY(showActive(&dialog));

    pair.filter->showFilter();
    QTest::keyClicks(pair.filter->lineEdit(), "main");
    QTest::keySequence(pair.filter->lineEdit(), escKey());
    QVERIFY(pair.filter->lineEdit()->text().isEmpty());
    QVERIFY(dialog.isVisible());

    QTest::keySequence(pair.filter->lineEdit(), escKey());
    QVERIFY2(!dialog.isVisible(),
             "Esc on an empty bar was swallowed instead of closing the dialog");
}

/**
 * @brief Two filtered lists in one window each get their own bar from Find and Esc.
 */
void TestAbstractFilterView::twoBarsInOneWindowStaySeparate()
{
    QWidget window;
    FilteredList to(&window, true);
    FilteredList from(&window, true);
    to.filter->attachTo(to.list);
    from.filter->attachTo(from.list);
    QVERIFY(showActive(&window));
    to.filter->hide();
    from.filter->hide();

    from.list->setFocus();
    QTest::keySequence(from.list, findKey());
    QVERIFY(from.filter->isVisible());
    QVERIFY(!to.filter->isVisible());

    to.filter->showFilter();
    QTest::keyClicks(to.filter->lineEdit(), "to");
    QTest::keyClicks(from.filter->lineEdit(), "from");
    from.filter->lineEdit()->setFocus();
    QTest::keySequence(from.filter->lineEdit(), escKey());
    QVERIFY(from.filter->lineEdit()->text().isEmpty());
    QCOMPARE(to.filter->lineEdit()->text(), QString("to"));
}

CUTTER_UNIT_TEST_MAIN(TestAbstractFilterView)
#include "test_AbstractFilterView.moc"