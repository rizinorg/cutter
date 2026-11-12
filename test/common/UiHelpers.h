#ifndef CUTTER_TEST_UI_HELPERS_H
#define CUTTER_TEST_UI_HELPERS_H

#include <QAbstractButton>
#include <QApplication>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QStringList>
#include <QTimer>
#include <QWidget>

namespace CutterTest {

template<class T>
T *findVisibleTopLevel()
{
    const QWidgetList windows = QApplication::topLevelWidgets();
    for (QWidget *window : windows) {
        auto *match = qobject_cast<T *>(window);
        if (match && match->isVisible()) {
            return match;
        }
    }
    return nullptr;
}

inline bool setLineEditText(QWidget *parent, const char *objectName, const QString &text)
{
    auto *edit = parent ? parent->findChild<QLineEdit *>(objectName) : nullptr;
    if (!edit) {
        return false;
    }
    edit->setText(text);
    return true;
}

inline bool clickButton(QWidget *parent, const char *objectName)
{
    auto *button = parent ? parent->findChild<QAbstractButton *>(objectName) : nullptr;
    if (!button) {
        return false;
    }
    button->click();
    return true;
}

inline bool clickStandardButton(QWidget *dialog, QDialogButtonBox::StandardButton which)
{
    auto *buttons = dialog ? dialog->findChild<QDialogButtonBox *>() : nullptr;
    QPushButton *button = buttons ? buttons->button(which) : nullptr;
    if (!button) {
        return false;
    }
    button->click();
    return true;
}

// TODO: this is onceVisible but it only waits for 20ms (nvm it keeps looping on 20ms)
// Can we not just hook the aboutToShow or some other signal to know when the widget becomes
// visible?
template<class T, class Action>
void onceVisible(QObject *context, Action action)
{
    auto *timer = new QTimer(context);
    timer->setInterval(20);
    QObject::connect(timer, &QTimer::timeout, timer, [timer, action]() {
        if (T *window = findVisibleTopLevel<T>()) {
            timer->stop();
            timer->deleteLater();
            action(window);
        }
    });
    timer->start();
}

class MessageBoxWatcher
{
public:
    MessageBoxWatcher()
    {
        timer.setInterval(20);
        QObject::connect(&timer, &QTimer::timeout, &timer, [this]() { dismissVisibleBoxes(); });
        timer.start();
    }

    QStringList seenTexts() const { return texts; }

private:
    void dismissVisibleBoxes()
    {
        const QWidgetList windows = QApplication::topLevelWidgets();
        for (QWidget *window : windows) {
            auto *box = qobject_cast<QMessageBox *>(window);
            if (box && box->isVisible()) {
                texts.append(box->text());
                box->done(QMessageBox::Ok);
            }
        }
    }

    QTimer timer;
    QStringList texts;
};

} // namespace CutterTest

#endif // CUTTER_TEST_UI_HELPERS_H
